#pragma once

#include <imgui.h>
#include <implot.h>
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/orchestrator/performance_profiler.hpp"
#include "relativistic/orchestrator/command.hpp"
#include "relativistic/render/geodesic_compute_pipeline.hpp"
#include "relativistic/render/gpu_types.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include <vector>
#include <string>
#include <optional>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace Relativistic::UI {

class PerformanceAnalysisWindow {
private:
	bool is_open_{false};
	Orchestrator::SimulationOrchestrator<1024>& orchestrator_;
	Render::GeodesicComputePipeline* render_pipeline_{nullptr};

	int capture_mode_{0};
	float capture_duration_seconds_{10.0f};
	int capture_frame_count_{300};
	char capture_label_buffer_[96]{"Benchmark Run"};

	int selected_run_indices_[2]{-1, -1};
	bool show_only_matching_signature_{false};
	int history_capacity_input_{3600};
	int plot_window_{240};

public:
	explicit PerformanceAnalysisWindow(Orchestrator::SimulationOrchestrator<1024>& orchestrator)
		: orchestrator_(orchestrator) {
		history_capacity_input_ = static_cast<int>(orchestrator_.profiler().history_capacity());
	}

	[[nodiscard]] bool& open_state() noexcept { return is_open_; }

	void attach_render_pipeline(Render::GeodesicComputePipeline& pipeline) noexcept {
		render_pipeline_ = &pipeline;
	}

	void render() {
		if (!is_open_) return;

		ImGui::SetNextWindowPos(ImVec2(200.0f, 90.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(940.0f, 720.0f), ImGuiCond_FirstUseEver);

		if (!ImGui::Begin("Performance Analysis & Profiling", &is_open_)) {
			ImGui::End();
			return;
		}

		auto& profiler = orchestrator_.profiler();

		ImGui::TextColored(ImVec4(0.3f, 0.9f, 1.0f, 1.0f), "Engine Signature: %s", profiler.engine_signature().c_str());
		render_setting_tooltip("Structural fingerprint of performance-relevant internal data layouts. Benchmark runs recorded under a different signature may not be directly comparable.");

		if (ImGui::BeginTabBar("PerformanceAnalysisTabs")) {
			if (ImGui::BeginTabItem("Live Monitor")) {
				render_live_monitor_tab(profiler);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Statistics")) {
				render_statistics_tab(profiler);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Bottleneck Analysis")) {
				render_bottleneck_tab(profiler);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Benchmark Runs")) {
				render_benchmark_runs_tab(profiler);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Settings")) {
				render_settings_tab(profiler);
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		ImGui::End();
	}

private:
	void render_live_monitor_tab(Orchestrator::PerformanceProfiler& profiler) {
		const auto& history = profiler.history();
		if (history.empty()) {
			ImGui::TextDisabled("No frame samples recorded yet. Keep the primary viewport visible to begin collecting data.");
			return;
		}

		const auto& latest = history.back();
		ImGui::Columns(4, nullptr, false);
		ImGui::Text("Frame Time"); ImGui::Text("%.3f ms", latest.frame_time_ms); ImGui::NextColumn();
		ImGui::Text("FPS"); ImGui::Text("%.1f", latest.fps); ImGui::NextColumn();
		ImGui::Text("Render Path"); ImGui::Text("%s", latest.used_gpu_path ? "GPU" : "CPU"); ImGui::NextColumn();
		ImGui::Text("Avg Iterations"); ImGui::Text("%.1f", latest.average_iterations); ImGui::NextColumn();
		ImGui::Columns(1);

		if (render_pipeline_ != nullptr) {
			const auto& live_tel = render_pipeline_->telemetry();
			ImGui::Text("Ray Iteration Range: %u - %u (avg %.1f)", live_tel.min_iterations_used, live_tel.max_iterations_used, live_tel.average_iterations_used);
			render_setting_tooltip("Minimum and maximum geodesic integration steps consumed by any single ray in the most recently completed frame, alongside the mean across all rays.");
		}

		ImGui::Separator();
		ImGui::SliderInt("Chart Window (frames)", &plot_window_, 30, static_cast<int>(profiler.history_capacity()));

		const size_t count = std::min(static_cast<size_t>(std::max(plot_window_, 1)), history.size());
		std::vector<double> frame_times(count), fps_values(count), x_axis(count);
		std::vector<double> dispatch_stage(count), texture_stage(count), hud_stage(count);

		for (size_t i = 0; i < count; ++i) {
			const auto& s = history[history.size() - count + i];
			frame_times[i] = s.frame_time_ms;
			fps_values[i] = s.fps;
			x_axis[i] = static_cast<double>(i);
			dispatch_stage[i] = s.stage_time_ms[static_cast<size_t>(Orchestrator::ProfilerTaskStage::RenderDispatch)];
			texture_stage[i] = s.stage_time_ms[static_cast<size_t>(Orchestrator::ProfilerTaskStage::TextureUpload)];
			hud_stage[i] = s.stage_time_ms[static_cast<size_t>(Orchestrator::ProfilerTaskStage::HudOverlay)];
		}

		if (ImPlot::BeginPlot("Frame Time History", ImVec2(-1, 220))) {
			ImPlot::SetupAxes("Sample", "Milliseconds");
			ImPlot::PlotLine("Frame Time (Total UI)", x_axis.data(), frame_times.data(), static_cast<int>(count));
			ImPlot::PlotLine("Render Dispatch", x_axis.data(), dispatch_stage.data(), static_cast<int>(count));
			ImPlot::PlotLine("Texture Upload", x_axis.data(), texture_stage.data(), static_cast<int>(count));
			ImPlot::PlotLine("HUD Overlay", x_axis.data(), hud_stage.data(), static_cast<int>(count));
			ImPlot::EndPlot();
		}

		if (ImPlot::BeginPlot("FPS History", ImVec2(-1, 180))) {
			ImPlot::SetupAxes("Sample", "FPS");
			ImPlot::PlotLine("FPS", x_axis.data(), fps_values.data(), static_cast<int>(count));
			ImPlot::EndPlot();
		}

		ImGui::Separator();
		const double denom = static_cast<double>(std::max<uint64_t>(latest.horizon_pixels + latest.celestial_pixels + latest.disk_pixels, 1));
		ImGui::Text(
			"Horizon Absorbed: %.1f%% | Celestial Escaped: %.1f%% | Disk Hits: %.1f%%",
			static_cast<double>(latest.horizon_pixels) / denom * 100.0,
			static_cast<double>(latest.celestial_pixels) / denom * 100.0,
			static_cast<double>(latest.disk_pixels) / denom * 100.0
		);
	}

	static void draw_summary_table(const char* label, const Orchestrator::StatisticalSummary& s) {
		ImGui::PushID(label);
		ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.3f, 1.0f), "%s", label);
		if (ImGui::BeginTable("SummaryTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
			ImGui::TableSetupColumn("Mean");
			ImGui::TableSetupColumn("Median");
			ImGui::TableSetupColumn("Min");
			ImGui::TableSetupColumn("Max");
			ImGui::TableSetupColumn("StdDev");
			ImGui::TableSetupColumn("P95 / P99");
			ImGui::TableHeadersRow();
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0); ImGui::Text("%.3f", s.mean);
			ImGui::TableSetColumnIndex(1); ImGui::Text("%.3f", s.median);
			ImGui::TableSetColumnIndex(2); ImGui::Text("%.3f", s.min_value);
			ImGui::TableSetColumnIndex(3); ImGui::Text("%.3f", s.max_value);
			ImGui::TableSetColumnIndex(4); ImGui::Text("%.3f", s.std_deviation);
			ImGui::TableSetColumnIndex(5); ImGui::Text("%.3f / %.3f", s.percentile_95, s.percentile_99);
			ImGui::EndTable();
		}
		ImGui::Spacing();
		ImGui::PopID();
	}

	void render_statistics_tab(Orchestrator::PerformanceProfiler& profiler) {
		static int window_choice = 0;
		const char* window_labels[] = {"Last 60 Frames", "Last 300 Frames", "Last 1000 Frames", "Entire History"};
		ImGui::Combo("Sample Window", &window_choice, window_labels, IM_ARRAYSIZE(window_labels));

		size_t n = 0;
		switch (window_choice) {
			case 0: n = 60; break;
			case 1: n = 300; break;
			case 2: n = 1000; break;
			default: n = 0; break;
		}

		const auto ft_summary = profiler.frame_time_summary(n);
		if (ft_summary.sample_count == 0) {
			ImGui::TextDisabled("Not enough data collected yet.");
			return;
		}

		draw_summary_table("Total Frame Time (ms)", ft_summary);
		draw_summary_table("Render Dispatch (ms)", profiler.stage_summary(Orchestrator::ProfilerTaskStage::RenderDispatch, n));
		draw_summary_table("Texture Upload (ms)", profiler.stage_summary(Orchestrator::ProfilerTaskStage::TextureUpload, n));
		draw_summary_table("HUD Overlay (ms)", profiler.stage_summary(Orchestrator::ProfilerTaskStage::HudOverlay, n));
		draw_summary_table("Average Ray Iterations Per Frame", profiler.iteration_summary(n));
	}

	void render_bottleneck_tab(Orchestrator::PerformanceProfiler& profiler) {
		static int sample_span = 120;
		ImGui::SliderInt("Analysis Window (frames)", &sample_span, 10, 1000);

		const auto report = profiler.analyze_bottleneck(static_cast<size_t>(sample_span));
		ImGui::TextWrapped("%s", report.summary.c_str());

		ImGui::Separator();
		ImGui::Text("Dominant Stage: %s (%.1f%% of measured stage time)", Orchestrator::profiler_stage_name(report.dominant_stage), report.dominant_share * 100.0);

		if (report.ray_step_saturated) {
			ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.3f, 1.0f), "Ray step saturation detected (%.1f%% of rays reach the integration cap without terminating).", report.saturation_ratio * 100.0);
		} else {
			ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.5f, 1.0f), "Ray step budget appears sufficient (%.1f%% saturation).", report.saturation_ratio * 100.0);
		}

		if (report.gpu_underutilized) {
			ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.3f, 1.0f), "GPU compute offload is not currently active for this render path.");
		}

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.6f, 0.85f, 1.0f, 1.0f), "Recommended Actions");
		if (report.ray_step_saturated) {
			ImGui::BulletText("Increase Max Geodesic Steps in Performance & Engine Optimization.");
			ImGui::BulletText("Enable Adaptive Space-Skipping if not already active.");
		}
		if (report.dominant_stage == Orchestrator::ProfilerTaskStage::TextureUpload) {
			ImGui::BulletText("Reduce Internal Render Scale or disable Force Texture Reallocation.");
		}
		if (report.gpu_underutilized && render_pipeline_ != nullptr && render_pipeline_->gpu_compute_available()) {
			ImGui::BulletText("A compatible GPU compute device is available; enabling it may reduce render dispatch cost.");
		}
	}

	void apply_run_configuration(const Orchestrator::BenchmarkRun& run) {
		if (!run.config.metric_name.empty()) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_metric(run.config.metric_name)));
			orchestrator_.set_active_metric_name(run.config.metric_name);
		}
		if (!run.config.integrator_name.empty()) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_integrator(run.config.integrator_name)));
			orchestrator_.set_active_integrator_name(run.config.integrator_name);
		}
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_resolution_scale(run.config.resolution_scale)));
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_render_steps(run.config.max_ray_steps)));
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::UseGpuCompute, run.config.use_gpu_compute ? 1.0 : 0.0)));
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::WorkDistributionMode, run.config.tiled_distribution ? 1.0 : 0.0)));
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_visual_overlay(Render::RenderFlags::USE_SCALAR_PIPELINE, !run.config.simd_pipeline)));
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::StepControllerMode, static_cast<double>(run.config.step_controller_mode))));
		orchestrator_.set_physical_param(Orchestrator::ParameterType::Custom, static_cast<double>(run.config.precision_mode), "precision_mode");
	}

	void render_run_comparison(Orchestrator::PerformanceProfiler& profiler) {
		const auto& runs = profiler.saved_runs();
		const bool valid_a = selected_run_indices_[0] >= 0 && selected_run_indices_[0] < static_cast<int>(runs.size());
		const bool valid_b = selected_run_indices_[1] >= 0 && selected_run_indices_[1] < static_cast<int>(runs.size());

		if (!valid_a || !valid_b) {
			ImGui::TextDisabled("Select two runs from the table above using the A and B buttons.");
			return;
		}

		const auto& a = runs[static_cast<size_t>(selected_run_indices_[0])];
		const auto& b = runs[static_cast<size_t>(selected_run_indices_[1])];

		if (a.engine_signature != b.engine_signature) {
			ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.3f, 1.0f), "Warning: these runs were captured under different engine signatures. Differences may reflect internal engine changes rather than configuration changes.");
		}

		if (ImGui::BeginTable("ComparisonTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
			ImGui::TableSetupColumn(a.label.c_str());
			ImGui::TableSetupColumn(b.label.c_str());
			ImGui::TableSetupColumn("Delta (B - A)");
			ImGui::TableHeadersRow();

			auto row = [&](double va, double vb, const char* fmt) {
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				if (std::strcmp(fmt, "%.3f") == 0) {
					ImGui::Text("%.3f", va);
				} else if (std::strcmp(fmt, "%.1f") == 0) {
					ImGui::Text("%.1f", va);
				} else if (std::strcmp(fmt, "%.1f%%") == 0) {
					ImGui::Text("%.1f%%", va);
				} else {
					ImGui::Text("%.0f", va);
				}

				ImGui::TableSetColumnIndex(1);
				if (std::strcmp(fmt, "%.3f") == 0) {
					ImGui::Text("%.3f", vb);
				} else if (std::strcmp(fmt, "%.1f") == 0) {
					ImGui::Text("%.1f", vb);
				} else if (std::strcmp(fmt, "%.1f%%") == 0) {
					ImGui::Text("%.1f%%", vb);
				} else {
					ImGui::Text("%.0f", vb);
				}

				ImGui::TableSetColumnIndex(2);
				const double delta = vb - va;
				const ImVec4 color = (delta <= 0.0) ? ImVec4(0.4f, 0.9f, 0.5f, 1.0f) : ImVec4(1.0f, 0.55f, 0.35f, 1.0f);
				if (std::strcmp(fmt, "%.3f") == 0) {
					ImGui::TextColored(color, "%.3f", delta);
				} else if (std::strcmp(fmt, "%.1f") == 0) {
					ImGui::TextColored(color, "%.1f", delta);
				} else if (std::strcmp(fmt, "%.1f%%") == 0) {
					ImGui::TextColored(color, "%.1f%%", delta);
				} else {
					ImGui::TextColored(color, "%.0f", delta);
				}
			};

			row(a.frame_time_summary.mean, b.frame_time_summary.mean, "%.3f");
			row(a.frame_time_summary.percentile_95, b.frame_time_summary.percentile_95, "%.3f");
			row(a.frame_time_summary.percentile_99, b.frame_time_summary.percentile_99, "%.3f");
			row(a.fps_summary.mean, b.fps_summary.mean, "%.1f");
			row(a.fps_summary.min_value, b.fps_summary.min_value, "%.1f");
			row(a.average_iterations, b.average_iterations, "%.1f");
			row(a.gpu_path_ratio * 100.0, b.gpu_path_ratio * 100.0, "%.1f%%");
			row(a.config.resolution_scale, b.config.resolution_scale, "%.2f");
			row(static_cast<double>(a.config.max_ray_steps), static_cast<double>(b.config.max_ray_steps), "%.0f");

			ImGui::EndTable();
		}
	}

	void render_benchmark_runs_tab(Orchestrator::PerformanceProfiler& profiler) {
		ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.6f, 1.0f), "Capture a Benchmark Run");
		ImGui::InputText("Run Label", capture_label_buffer_, sizeof(capture_label_buffer_));

		const char* modes[] = {"By Duration", "By Frame Count"};
		ImGui::Combo("Capture Mode", &capture_mode_, modes, IM_ARRAYSIZE(modes));
		if (capture_mode_ == 0) {
			ImGui::SliderFloat("Duration (seconds)", &capture_duration_seconds_, 1.0f, 120.0f, "%.1f s");
		} else {
			ImGui::SliderInt("Frame Count", &capture_frame_count_, 30, 6000);
		}

		if (profiler.is_capturing()) {
			ImGui::ProgressBar(static_cast<float>(profiler.capture_progress()), ImVec2(-1, 0), "Capturing...");
			if (ImGui::Button("Cancel Capture", ImVec2(160.0f, 26.0f))) {
				profiler.cancel_capture();
			}
		} else {
			if (ImGui::Button("Start Capture", ImVec2(160.0f, 26.0f))) {
				const double now = ImGui::GetTime();
				if (capture_mode_ == 0) {
					profiler.start_capture(capture_label_buffer_, static_cast<double>(capture_duration_seconds_), std::nullopt, now);
				} else {
					profiler.start_capture(capture_label_buffer_, std::nullopt, static_cast<size_t>(capture_frame_count_), now);
				}
			}
		}
		render_setting_tooltip("Records aggregated statistics over the chosen window and stores them as a labeled, persisted benchmark run for later comparison.");

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.6f, 1.0f), "Saved Runs");
		ImGui::Checkbox("Only Show Runs Matching Current Engine Signature", &show_only_matching_signature_);

		const auto& runs = profiler.saved_runs();
		if (runs.empty()) {
			ImGui::TextDisabled("No benchmark runs saved yet.");
		}

		if (ImGui::BeginTable("BenchmarkRunsTable", 9, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 220))) {
			ImGui::TableSetupColumn("Label");
			ImGui::TableSetupColumn("Timestamp");
			ImGui::TableSetupColumn("Mean FT (ms)");
			ImGui::TableSetupColumn("P95 FT (ms)");
			ImGui::TableSetupColumn("Mean FPS");
			ImGui::TableSetupColumn("Res Scale");
			ImGui::TableSetupColumn("Ray Steps");
			ImGui::TableSetupColumn("GPU Ratio");
			ImGui::TableSetupColumn("Actions");
			ImGui::TableHeadersRow();

			for (size_t i = 0; i < runs.size(); ++i) {
				const auto& run = runs[i];
				if (show_only_matching_signature_ && run.engine_signature != profiler.engine_signature()) continue;

				ImGui::TableNextRow();
				ImGui::PushID(static_cast<int>(i));

				ImGui::TableSetColumnIndex(0);
				const bool signature_mismatch = run.engine_signature != profiler.engine_signature();
				if (signature_mismatch) {
					ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f), "%s [!]", run.label.c_str());
				} else {
					ImGui::TextUnformatted(run.label.c_str());
				}
				if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
					ImGui::BeginTooltip();
					ImGui::Text("Metric: %s", run.config.metric_name.c_str());
					ImGui::Text("Integrator: %s", run.config.integrator_name.c_str());
					ImGui::Text("Precision: %s", run.config.precision_mode == 0 ? "Native FP64" : "Double-Single");
					ImGui::Text("Samples: %zu over %.1f s", run.sample_count, run.capture_duration_seconds);
					ImGui::Text("Signature: %s", run.engine_signature.c_str());
					ImGui::EndTooltip();
				}

				ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(run.timestamp.c_str());
				ImGui::TableSetColumnIndex(2); ImGui::Text("%.3f", run.frame_time_summary.mean);
				ImGui::TableSetColumnIndex(3); ImGui::Text("%.3f", run.frame_time_summary.percentile_95);
				ImGui::TableSetColumnIndex(4); ImGui::Text("%.1f", run.fps_summary.mean);
				ImGui::TableSetColumnIndex(5); ImGui::Text("%.2fx", run.config.resolution_scale);
				ImGui::TableSetColumnIndex(6); ImGui::Text("%u", run.config.max_ray_steps);
				ImGui::TableSetColumnIndex(7); ImGui::Text("%.0f%%", run.gpu_path_ratio * 100.0);

				ImGui::TableSetColumnIndex(8);
				if (ImGui::SmallButton("A")) selected_run_indices_[0] = static_cast<int>(i);
				ImGui::SameLine();
				if (ImGui::SmallButton("B")) selected_run_indices_[1] = static_cast<int>(i);
				ImGui::SameLine();
				if (ImGui::SmallButton("Apply Settings")) apply_run_configuration(run);
				ImGui::SameLine();
				if (ImGui::SmallButton("Delete")) {
					profiler.remove_run(i);
					profiler.save_to_disk();
					ImGui::PopID();
					break;
				}
				ImGui::PopID();
			}
			ImGui::EndTable();
		}

		if (ImGui::Button("Clear All Saved Runs", ImVec2(180.0f, 26.0f))) {
			profiler.clear_all_runs();
			profiler.save_to_disk();
		}

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.6f, 1.0f), "Comparison (Select A / B Above)");
		render_run_comparison(profiler);
	}

	void render_settings_tab(Orchestrator::PerformanceProfiler& profiler) {
		ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.6f, 1.0f), "Live History Buffer");
		if (ImGui::SliderInt("History Capacity (frames)", &history_capacity_input_, 120, 36000)) {
			profiler.set_history_capacity(static_cast<size_t>(history_capacity_input_));
		}
		render_setting_tooltip("Number of recent frame samples retained in memory for the Live Monitor and Statistics tabs. Does not affect saved benchmark runs.");

		if (ImGui::Button("Clear Live History", ImVec2(180.0f, 26.0f))) {
			profiler.clear_history();
		}

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.6f, 1.0f), "Persistence");
		bool persistence_enabled = profiler.persistence_enabled();
		if (ImGui::Checkbox("Persist Benchmark Runs Across Sessions", &persistence_enabled)) {
			profiler.set_persistence_enabled(persistence_enabled);
		}
		render_setting_tooltip("When enabled, saved benchmark runs are written to config/performance_profiler.cfg and automatically reloaded on the next session.");

		if (ImGui::Button("Save Now", ImVec2(120.0f, 26.0f))) {
			profiler.save_to_disk();
		}
		ImGui::SameLine();
		if (ImGui::Button("Reload From Disk", ImVec2(140.0f, 26.0f))) {
			profiler.load_from_disk();
			history_capacity_input_ = static_cast<int>(profiler.history_capacity());
		}

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.6f, 1.0f), "Proposed Keybinds");
		ImGui::TextDisabled("No default keys are bound. Assign these in Keybind Settings if desired:");
		ImGui::BulletText("Toggle Performance Analysis Window");
		ImGui::BulletText("Start/Stop Quick Benchmark Capture");
		ImGui::BulletText("Quick Save Live Window As Benchmark Run");
	}
};

}
