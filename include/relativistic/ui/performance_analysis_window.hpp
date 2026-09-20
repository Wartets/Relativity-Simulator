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
#include <limits>

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
	bool capture_label_auto_{true};
	std::string last_suggested_label_{};

	int selected_run_indices_[2]{-1, -1};
	bool show_only_matching_signature_{false};
	int history_capacity_input_{3600};
	int plot_window_{240};
	int benchmark_sort_mode_{0};
	bool benchmark_sort_descending_{true};
	char benchmark_search_filter_[64]{};

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

		const double megapixels_per_second = (latest.frame_time_ms > 0.0)
			? (static_cast<double>(latest.pixels_processed) / 1.0e6) / (latest.frame_time_ms / 1000.0)
			: 0.0;
		ImGui::Text("Throughput: %.2f Megapixels/s at %ux%u", megapixels_per_second, latest.screen_width, latest.screen_height);
		render_setting_tooltip("Pixels fully ray-traced per second at the current internal render resolution, derived from the most recent frame time. Useful for comparing configurations independently of window size.");

		if (render_pipeline_ != nullptr) {
			const auto& live_tel = render_pipeline_->telemetry();
			ImGui::Text("Ray Iteration Range: %u - %u (avg %.1f)", live_tel.min_iterations_used, live_tel.max_iterations_used, live_tel.average_iterations_used);
			render_setting_tooltip("Minimum and maximum geodesic integration steps consumed by any single ray in the most recently completed frame, alongside the mean across all rays.");
		}

		{
			const size_t window_count = std::min<size_t>(history.size(), 120);
			size_t gpu_frames = 0;
			double stage_totals[static_cast<size_t>(Orchestrator::ProfilerTaskStage::Count)] = {};
			for (size_t i = history.size() - window_count; i < history.size(); ++i) {
				if (history[i].used_gpu_path) ++gpu_frames;
				for (size_t st = 0; st < static_cast<size_t>(Orchestrator::ProfilerTaskStage::Count); ++st) {
					stage_totals[st] += history[i].stage_time_ms[st];
				}
			}
			const double gpu_ratio = static_cast<double>(gpu_frames) / static_cast<double>(window_count);
			ImGui::Text("Render Path Split (last %zu frames): GPU %.0f%% | CPU %.0f%%", window_count, gpu_ratio * 100.0, (1.0 - gpu_ratio) * 100.0);
			render_setting_tooltip("Fraction of recently completed frames dispatched through the Vulkan compute path versus the CPU SIMD/scalar fallback. Also available as a standalone HUD readout.");

			ImGui::Spacing();
			ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.3f, 1.0f), "Stage Time Share (last %zu frames, sorted by cost)", window_count);
			double frame_total = stage_totals[static_cast<size_t>(Orchestrator::ProfilerTaskStage::FrameTotal)];
			if (frame_total <= 1e-9) frame_total = 1.0;
			std::vector<size_t> sorted_stage_indices;
			for (size_t st = 0; st < static_cast<size_t>(Orchestrator::ProfilerTaskStage::Count); ++st) {
				if (st == static_cast<size_t>(Orchestrator::ProfilerTaskStage::FrameTotal)) continue;
				if (st == static_cast<size_t>(Orchestrator::ProfilerTaskStage::GpuDispatchExecution) || st == static_cast<size_t>(Orchestrator::ProfilerTaskStage::CpuDispatchExecution)
					|| st == static_cast<size_t>(Orchestrator::ProfilerTaskStage::AdaptiveTilePrepassSky) || st == static_cast<size_t>(Orchestrator::ProfilerTaskStage::PixelClassification)
					|| st == static_cast<size_t>(Orchestrator::ProfilerTaskStage::CameraConstantsBuild)) continue;
				sorted_stage_indices.push_back(st);
			}
			std::sort(sorted_stage_indices.begin(), sorted_stage_indices.end(), [&](size_t a, size_t b) { return stage_totals[a] > stage_totals[b]; });
			for (const size_t st : sorted_stage_indices) {
				const auto stage = static_cast<Orchestrator::ProfilerTaskStage>(st);
				const float share = static_cast<float>(std::clamp(stage_totals[st] / frame_total, 0.0, 1.0));
				ImGui::ProgressBar(share, ImVec2(-1.0f, 0.0f), (std::string(Orchestrator::profiler_stage_name(stage)) + " " + std::to_string(static_cast<int>(share * 100.0f)) + "%").c_str());
			}
			render_setting_tooltip("Share of accumulated frame time spent in each instrumented pipeline stage, sorted from most to least expensive. The largest bar identifies where optimization effort is likely to pay off first; see the Bottleneck Analysis tab for recommendations.");

			ImGui::Spacing();
			if (ImGui::CollapsingHeader("Detailed Render Sub-Task Breakdown")) {
				double render_dispatch_total = stage_totals[static_cast<size_t>(Orchestrator::ProfilerTaskStage::RenderDispatch)];
				if (render_dispatch_total <= 1e-9) render_dispatch_total = 1.0;
				std::vector<size_t> render_sub_stages{
					static_cast<size_t>(Orchestrator::ProfilerTaskStage::GpuDispatchExecution),
					static_cast<size_t>(Orchestrator::ProfilerTaskStage::CpuDispatchExecution),
					static_cast<size_t>(Orchestrator::ProfilerTaskStage::AdaptiveTilePrepassSky),
					static_cast<size_t>(Orchestrator::ProfilerTaskStage::PixelClassification),
					static_cast<size_t>(Orchestrator::ProfilerTaskStage::CameraConstantsBuild)
				};
				std::sort(render_sub_stages.begin(), render_sub_stages.end(), [&](size_t a, size_t b) { return stage_totals[a] > stage_totals[b]; });
				for (const size_t st : render_sub_stages) {
					const auto stage = static_cast<Orchestrator::ProfilerTaskStage>(st);
					const float share = static_cast<float>(std::clamp(stage_totals[st] / render_dispatch_total, 0.0, 1.0));
					const double avg_ms = stage_totals[st] / static_cast<double>(window_count);
					ImGui::ProgressBar(share, ImVec2(-1.0f, 0.0f), (std::string(Orchestrator::profiler_stage_name(stage)) + " " + std::to_string(static_cast<int>(share * 100.0f)) + "% (avg " + std::to_string(avg_ms).substr(0, 5) + " ms)").c_str());
				}
				if (render_pipeline_ != nullptr) {
					const auto& live_tel = render_pipeline_->telemetry();
					ImGui::Spacing();
					ImGui::Text("Adaptive Tile Prepass Skip: %llu tiles (%.3f ms) | Full Ray-Traced Tiles: %llu tiles (%.3f ms)", static_cast<unsigned long long>(live_tel.tile_prepass_skip_tile_count), live_tel.tile_prepass_skip_ms, static_cast<unsigned long long>(live_tel.full_raytrace_tile_count), live_tel.full_raytrace_tiles_ms);
					render_setting_tooltip("Tile counts and cumulative elapsed time from the most recently completed CPU-tiled render, split between tiles the Adaptive Tile Sky Prepass analytically filled without ray-tracing and tiles that were fully integrated. Only populated when Tiled Work Distribution and Adaptive Tile Sky Prepass are both enabled and the CPU path is active.");
				}
				render_setting_tooltip("Breaks the coarse Render Dispatch stage above down into the individual sub-tasks measured inside the ray-tracing pipeline itself, expressed as a share of total render dispatch time rather than total frame time.");
			}
		}

		ImGui::Separator();
		ImGui::SliderInt("Chart Window (frames)", &plot_window_, 30, static_cast<int>(std::min<size_t>(profiler.history_capacity(), 3600)));
		render_setting_tooltip("Number of most recent frame samples displayed in the charts below. Independent from the History Capacity setting on the Settings tab, which controls how many samples are retained in memory.");

		const size_t count = std::min(static_cast<size_t>(std::max(plot_window_, 1)), history.size());
		std::vector<double> frame_times(count), fps_values(count), x_axis(count);
		std::vector<double> dispatch_stage(count), texture_stage(count), hud_stage(count);

		double frame_time_min = std::numeric_limits<double>::max();
		double frame_time_max = std::numeric_limits<double>::lowest();
		double fps_min = std::numeric_limits<double>::max();
		double fps_max = std::numeric_limits<double>::lowest();

		for (size_t i = 0; i < count; ++i) {
			const auto& s = history[history.size() - count + i];
			frame_times[i] = s.frame_time_ms;
			fps_values[i] = s.fps;
			x_axis[i] = static_cast<double>(i);
			dispatch_stage[i] = s.stage_time_ms[static_cast<size_t>(Orchestrator::ProfilerTaskStage::RenderDispatch)];
			texture_stage[i] = s.stage_time_ms[static_cast<size_t>(Orchestrator::ProfilerTaskStage::TextureUpload)];
			hud_stage[i] = s.stage_time_ms[static_cast<size_t>(Orchestrator::ProfilerTaskStage::HudOverlay)];
			frame_time_min = std::min(frame_time_min, s.frame_time_ms);
			frame_time_max = std::max(frame_time_max, s.frame_time_ms);
			fps_min = std::min(fps_min, s.fps);
			fps_max = std::max(fps_max, s.fps);
		}

		if (frame_time_max <= frame_time_min) frame_time_max = frame_time_min + 1.0;
		if (fps_max <= fps_min) fps_max = fps_min + 1.0;
		const double ft_pad = std::max((frame_time_max - frame_time_min) * 0.12, 0.05);
		const double fps_pad = std::max((fps_max - fps_min) * 0.12, 0.5);

		if (ImPlot::BeginPlot("Frame Time History", ImVec2(-1, 220))) {
			ImPlot::SetupAxes("Sample", "Milliseconds");
			ImPlot::SetupAxesLimits(0.0, static_cast<double>(count > 0 ? count - 1 : 0), std::max(frame_time_min - ft_pad, 0.0), frame_time_max + ft_pad, ImPlotCond_Always);
			ImPlot::PlotLine("Frame Time (Total UI)", x_axis.data(), frame_times.data(), static_cast<int>(count));
			ImPlot::PlotLine("Render Dispatch", x_axis.data(), dispatch_stage.data(), static_cast<int>(count));
			ImPlot::PlotLine("Texture Upload", x_axis.data(), texture_stage.data(), static_cast<int>(count));
			ImPlot::PlotLine("HUD Overlay", x_axis.data(), hud_stage.data(), static_cast<int>(count));
			ImPlot::EndPlot();
		}

		if (ImPlot::BeginPlot("FPS History", ImVec2(-1, 180))) {
			ImPlot::SetupAxes("Sample", "FPS");
			ImPlot::SetupAxesLimits(0.0, static_cast<double>(count > 0 ? count - 1 : 0), std::max(fps_min - fps_pad, 0.0), fps_max + fps_pad, ImPlotCond_Always);
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
		render_setting_tooltip("Classification of every pixel in the most recently completed frame by how its geodesic terminated. High celestial-escape share with a bright accretion disk configured may indicate the render distance or step budget is cutting rays short.");
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

	[[nodiscard]] static size_t history_start_for_window(const Orchestrator::PerformanceProfiler& profiler, size_t n) noexcept {
		const auto& history = profiler.history();
		const size_t count = (n == 0 || n > history.size()) ? history.size() : n;
		return history.size() - count;
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
		draw_summary_table("GPU Dispatch Execution (ms)", profiler.stage_summary(Orchestrator::ProfilerTaskStage::GpuDispatchExecution, n));
		draw_summary_table("CPU Dispatch Execution (ms)", profiler.stage_summary(Orchestrator::ProfilerTaskStage::CpuDispatchExecution, n));
		draw_summary_table("Adaptive Tile Prepass Sky (ms)", profiler.stage_summary(Orchestrator::ProfilerTaskStage::AdaptiveTilePrepassSky, n));
		draw_summary_table("Pixel Classification (ms)", profiler.stage_summary(Orchestrator::ProfilerTaskStage::PixelClassification, n));
		draw_summary_table("Camera Constants Build (ms)", profiler.stage_summary(Orchestrator::ProfilerTaskStage::CameraConstantsBuild, n));
		draw_summary_table("Post-Processing (ms)", profiler.stage_summary(Orchestrator::ProfilerTaskStage::PostProcessing, n));
		draw_summary_table("Framebuffer Readback (ms)", profiler.stage_summary(Orchestrator::ProfilerTaskStage::FramebufferReadback, n));
		draw_summary_table("Texture Upload (ms)", profiler.stage_summary(Orchestrator::ProfilerTaskStage::TextureUpload, n));
		draw_summary_table("HUD Overlay (ms)", profiler.stage_summary(Orchestrator::ProfilerTaskStage::HudOverlay, n));
		draw_summary_table("Camera Update (ms)", profiler.stage_summary(Orchestrator::ProfilerTaskStage::CameraUpdate, n));
		draw_summary_table("Schematic Overlay (ms)", profiler.stage_summary(Orchestrator::ProfilerTaskStage::SchematicOverlay, n));
		draw_summary_table("Average Ray Iterations Per Frame", profiler.iteration_summary(n));

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.6f, 0.9f, 1.0f, 1.0f), "Render Path Distribution");
		const size_t window_start = history_start_for_window(profiler, n);
		size_t gpu_count = 0;
		for (size_t i = window_start; i < profiler.history().size(); ++i) {
			if (profiler.history()[i].used_gpu_path) ++gpu_count;
		}
		const size_t sample_window = profiler.history().size() - window_start;
		if (sample_window > 0) {
			ImGui::Text("GPU Path Frames: %zu / %zu (%.1f%%)", gpu_count, sample_window, static_cast<double>(gpu_count) / static_cast<double>(sample_window) * 100.0);
		}
		render_setting_tooltip("Count of frames rendered via GPU compute dispatch versus the CPU pipeline across the selected sample window, useful for confirming whether the GPU path is actually engaging for the current metric and precision mode.");
	}

	void render_bottleneck_tab(Orchestrator::PerformanceProfiler& profiler) {
		static int sample_span = 120;
		ImGui::SliderInt("Analysis Window (frames)", &sample_span, 10, 1000);
		render_setting_tooltip("Number of most recent frames analyzed to determine which pipeline stage dominates render time and whether the geodesic step budget is being exhausted before rays terminate naturally.");

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

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.3f, 1.0f), "Render Dispatch Cost Breakdown");
		render_setting_tooltip("Heuristic estimate of how Render Dispatch time is distributed across ray outcome categories, derived from ray classification counts and average integration steps over the analysis window. Disk-hit rays carry extra shading and Doppler evaluation cost, so their share is weighted higher; this is not per-pixel instrumentation, since timing every geodesic step individually would itself slow down the render loop.");

		const auto& history = profiler.history();
		if (history.empty()) {
			ImGui::TextDisabled("No frame samples available for this breakdown.");
			return;
		}

		const size_t breakdown_window = std::min(static_cast<size_t>(sample_span), history.size());
		uint64_t horizon_total = 0, celestial_total = 0, disk_total = 0, other_total = 0;
		double dispatch_time_total = 0.0;
		double iteration_total = 0.0;
		for (size_t i = history.size() - breakdown_window; i < history.size(); ++i) {
			const auto& s = history[i];
			const uint64_t classified = s.horizon_pixels + s.celestial_pixels + s.disk_pixels;
			const uint64_t total_px = std::max<uint64_t>(s.pixels_processed, 1);
			horizon_total += s.horizon_pixels;
			celestial_total += s.celestial_pixels;
			disk_total += s.disk_pixels;
			other_total += (total_px > classified) ? (total_px - classified) : 0;
			dispatch_time_total += s.stage_time_ms[static_cast<size_t>(Orchestrator::ProfilerTaskStage::RenderDispatch)];
			iteration_total += s.average_iterations;
		}

		constexpr double kDiskShadingWeight = 1.6;
		constexpr double kCelestialSkyWeight = 1.0;
		constexpr double kHorizonWeight = 0.7;
		constexpr double kSaturatedWeight = 1.3;

		const double horizon_cost = static_cast<double>(horizon_total) * kHorizonWeight;
		const double celestial_cost = static_cast<double>(celestial_total) * kCelestialSkyWeight;
		const double disk_cost = static_cast<double>(disk_total) * kDiskShadingWeight;
		const double saturated_cost = static_cast<double>(other_total) * kSaturatedWeight;
		const double weighted_total = std::max(horizon_cost + celestial_cost + disk_cost + saturated_cost, 1e-9);
		const double avg_dispatch_ms = dispatch_time_total / static_cast<double>(breakdown_window);
		const double avg_iterations = iteration_total / static_cast<double>(breakdown_window);

		auto draw_share_bar = [&](const char* label, double weighted_cost, ImVec4 color) {
			const double share = std::clamp(weighted_cost / weighted_total, 0.0, 1.0);
			const double est_ms = avg_dispatch_ms * share;
			char overlay[96];
			std::snprintf(overlay, sizeof(overlay), "%s: %.0f%% (~%.2f ms/frame)", label, share * 100.0, est_ms);
			ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
			ImGui::ProgressBar(static_cast<float>(share), ImVec2(-1.0f, 0.0f), overlay);
			ImGui::PopStyleColor();
		};

		draw_share_bar("Absorbed at Horizon", horizon_cost, ImVec4(0.85f, 0.35f, 0.35f, 1.0f));
		draw_share_bar("Escaped to Sky", celestial_cost, ImVec4(0.35f, 0.55f, 0.9f, 1.0f));
		draw_share_bar("Accretion Disk Shading", disk_cost, ImVec4(0.95f, 0.7f, 0.2f, 1.0f));
		draw_share_bar("Step-Saturated / Undetermined", saturated_cost, ImVec4(0.6f, 0.6f, 0.65f, 1.0f));

		ImGui::Spacing();
		ImGui::Text("Average Integration Steps Per Pixel: %.1f", avg_iterations);
		ImGui::Text("Average Render Dispatch Time: %.3f ms/frame over last %zu frames", avg_dispatch_ms, breakdown_window);
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
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::MotionQualityMode, static_cast<double>(run.config.motion_quality_mode))));
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::MotionQualityScale, run.config.motion_quality_scale)));
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SpaceSkippingEnabled, run.config.space_skipping_enabled ? 1.0 : 0.0)));
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SpaceSkipRadiusScale, run.config.space_skip_radius_scale)));
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::PoleGuardPrecisionScale, run.config.pole_guard_precision_scale)));
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::FarFieldStepScale, run.config.far_field_step_scale)));
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::LodEnabled, run.config.lod_enabled ? 1.0 : 0.0)));
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::LodDistanceThreshold, run.config.lod_distance_scale)));
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::LodReducedSteps, static_cast<double>(run.config.lod_reduced_ray_steps))));
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::RenderDistanceScale, run.config.render_distance_scale)));
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::InterlaceRenderingEnabled, run.config.interlace_rendering_enabled ? 1.0 : 0.0)));
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::DynamicResolutionEnabled, run.config.dynamic_resolution_enabled ? 1.0 : 0.0)));
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::DynamicResolutionTargetFps, run.config.dynamic_resolution_target_fps)));
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::AdaptiveTilePrepassEnabled, run.config.adaptive_tile_prepass_enabled ? 1.0 : 0.0)));
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::RollingAverageFrameCount, static_cast<double>(run.config.rolling_average_frame_count))));
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::IntegrationRtol, run.config.integration_rtol)));
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::IntegrationAtol, run.config.integration_atol)));
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

		if (ImGui::BeginTable("ComparisonTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
			ImGui::TableSetupColumn("Metric");
			ImGui::TableSetupColumn(a.label.c_str());
			ImGui::TableSetupColumn(b.label.c_str());
			ImGui::TableSetupColumn("Delta (B - A)");
			ImGui::TableHeadersRow();

			auto format_metric_value = [](const char* fmt, double v) -> std::string {
				char buf[64];
				if (std::strcmp(fmt, "%.3f") == 0) {
					std::snprintf(buf, sizeof(buf), "%.3f", v);
				} else if (std::strcmp(fmt, "%.2f") == 0) {
					std::snprintf(buf, sizeof(buf), "%.2f", v);
				} else if (std::strcmp(fmt, "%.1f") == 0) {
					std::snprintf(buf, sizeof(buf), "%.1f", v);
				} else if (std::strcmp(fmt, "%.1f%%") == 0) {
					std::snprintf(buf, sizeof(buf), "%.1f%%", v);
				} else if (std::strcmp(fmt, "%.3e") == 0) {
					std::snprintf(buf, sizeof(buf), "%.3e", v);
				} else {
					std::snprintf(buf, sizeof(buf), "%.0f", v);
				}
				return std::string(buf);
			};

			auto row = [&](const char* metric_label, double va, double vb, const char* fmt) {
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted(metric_label);

				ImGui::TableSetColumnIndex(1);
				ImGui::TextUnformatted(format_metric_value(fmt, va).c_str());

				ImGui::TableSetColumnIndex(2);
				ImGui::TextUnformatted(format_metric_value(fmt, vb).c_str());

				ImGui::TableSetColumnIndex(3);
				const double delta = vb - va;
				const ImVec4 color = (delta <= 0.0) ? ImVec4(0.4f, 0.9f, 0.5f, 1.0f) : ImVec4(1.0f, 0.55f, 0.35f, 1.0f);
				ImGui::TextColored(color, "%s", format_metric_value(fmt, delta).c_str());
			};

			row("Mean Frame Time (ms)", a.frame_time_summary.mean, b.frame_time_summary.mean, "%.3f");
			row("P95 Frame Time (ms)", a.frame_time_summary.percentile_95, b.frame_time_summary.percentile_95, "%.3f");
			row("P99 Frame Time (ms)", a.frame_time_summary.percentile_99, b.frame_time_summary.percentile_99, "%.3f");
			row("Mean FPS", a.fps_summary.mean, b.fps_summary.mean, "%.1f");
			row("Min FPS", a.fps_summary.min_value, b.fps_summary.min_value, "%.1f");
			row("Avg Ray Iterations", a.average_iterations, b.average_iterations, "%.1f");
			row("GPU Path Ratio (%%)", a.gpu_path_ratio * 100.0, b.gpu_path_ratio * 100.0, "%.1f%%");
			row("Resolution Scale", a.config.resolution_scale, b.config.resolution_scale, "%.2f");
			row("Max Ray Steps", static_cast<double>(a.config.max_ray_steps), static_cast<double>(b.config.max_ray_steps), "%.0f");
			row("Motion Quality Scale", a.config.motion_quality_scale, b.config.motion_quality_scale, "%.2f");
			row("Space Skip Radius (M)", a.config.space_skip_radius_scale, b.config.space_skip_radius_scale, "%.1f");
			row("Pole Guard Precision", a.config.pole_guard_precision_scale, b.config.pole_guard_precision_scale, "%.2f");
			row("Far-Field Step Multiplier", a.config.far_field_step_scale, b.config.far_field_step_scale, "%.2f");
			row("LOD Distance Threshold (M)", a.config.lod_distance_scale, b.config.lod_distance_scale, "%.0f");
			row("LOD Reduced Steps", static_cast<double>(a.config.lod_reduced_ray_steps), static_cast<double>(b.config.lod_reduced_ray_steps), "%.0f");
			row("Render Distance (M)", a.config.render_distance_scale, b.config.render_distance_scale, "%.0f");
			row("Dynamic Res Target FPS", a.config.dynamic_resolution_target_fps, b.config.dynamic_resolution_target_fps, "%.0f");
			row("Rolling Average Window (N)", static_cast<double>(a.config.rolling_average_frame_count), static_cast<double>(b.config.rolling_average_frame_count), "%.0f");
			row("Integration rtol", a.config.integration_rtol, b.config.integration_rtol, "%.3e");
			row("Integration atol", a.config.integration_atol, b.config.integration_atol, "%.3e");

			ImGui::EndTable();
		}
	}

	[[nodiscard]] std::string build_suggested_label() const {
		const auto& p = orchestrator_.parameters();
		static constexpr const char* kPresetNames[] = {"Potato", "Perf", "Balanced", "High", "Ultra", "Extreme", "Custom"};
		const uint32_t preset_idx = std::min<uint32_t>(p.performance_preset, 6U);
		std::string metric_short = orchestrator_.active_metric_name();
		const size_t space_pos = metric_short.find(' ');
		if (space_pos != std::string::npos) metric_short = metric_short.substr(0, space_pos);
		std::string integrator_short = orchestrator_.active_integrator_name();
		const size_t int_space = integrator_short.find(' ');
		if (int_space != std::string::npos) integrator_short = integrator_short.substr(0, int_space);
		return metric_short + "_" + integrator_short + "_" + kPresetNames[preset_idx];
	}

	void render_benchmark_runs_tab(Orchestrator::PerformanceProfiler& profiler) {
		ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.6f, 1.0f), "Capture a Benchmark Run");

		if (capture_label_auto_ && !profiler.is_capturing()) {
			const std::string suggested = build_suggested_label();
			if (suggested != last_suggested_label_) {
				std::strncpy(capture_label_buffer_, suggested.c_str(), sizeof(capture_label_buffer_) - 1);
				capture_label_buffer_[sizeof(capture_label_buffer_) - 1] = '\0';
				last_suggested_label_ = suggested;
			}
		}
		if (ImGui::InputText("Run Label", capture_label_buffer_, sizeof(capture_label_buffer_))) {
			capture_label_auto_ = false;
		}
		ImGui::SameLine();
		if (ImGui::Checkbox("Auto-Name", &capture_label_auto_)) {
			if (capture_label_auto_) {
				last_suggested_label_.clear();
			}
		}
		render_setting_tooltip("When enabled, the run label is regenerated automatically from the active metric, integrator, and performance preset. Typing in the field disables auto-naming; the checkbox re-enables it.");

		const char* modes[] = {"By Duration", "By Frame Count"};
		ImGui::Combo("Capture Mode", &capture_mode_, modes, IM_ARRAYSIZE(modes));
		render_setting_tooltip("Chooses whether the benchmark run stops after a fixed wall-clock duration or after a fixed number of rendered frames.");
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
		render_setting_tooltip("Hides benchmark runs captured under a different internal data-layout signature, since their timings are not directly comparable to runs captured with the current build.");

		ImGui::SetNextItemWidth(220.0f);
		ImGui::InputTextWithHint("##BenchmarkSearch", "Filter by label, metric, or integrator...", benchmark_search_filter_, sizeof(benchmark_search_filter_));
		ImGui::SameLine();
		const char* benchmark_sort_options[] = {
			"Sort: Timestamp", "Sort: Label", "Sort: Mean Frame Time", "Sort: P95 Frame Time", "Sort: Mean FPS",
			"Sort: Resolution Scale", "Sort: Ray Steps", "Sort: GPU Ratio", "Sort: Metric", "Sort: Integrator"
		};
		ImGui::SetNextItemWidth(200.0f);
		ImGui::Combo("##BenchmarkSortMode", &benchmark_sort_mode_, benchmark_sort_options, IM_ARRAYSIZE(benchmark_sort_options));
		ImGui::SameLine();
		if (ImGui::ArrowButton("##BenchmarkSortDirection", benchmark_sort_descending_ ? ImGuiDir_Down : ImGuiDir_Up)) {
			benchmark_sort_descending_ = !benchmark_sort_descending_;
		}
		render_setting_tooltip("Chooses how the saved runs table below is ordered, and toggles between ascending and descending order.");

		const auto& runs = profiler.saved_runs();
		if (runs.empty()) {
			ImGui::TextDisabled("No benchmark runs saved yet.");
		}

		std::vector<size_t> visible_run_indices;
		visible_run_indices.reserve(runs.size());
		const std::string benchmark_filter_str(benchmark_search_filter_);
		for (size_t i = 0; i < runs.size(); ++i) {
			const auto& run = runs[i];
			if (show_only_matching_signature_ && run.engine_signature != profiler.engine_signature()) continue;
			if (!benchmark_filter_str.empty()) {
				if (run.label.find(benchmark_filter_str) == std::string::npos &&
				    run.config.metric_name.find(benchmark_filter_str) == std::string::npos &&
				    run.config.integrator_name.find(benchmark_filter_str) == std::string::npos) {
					continue;
				}
			}
			visible_run_indices.push_back(i);
		}

		std::sort(visible_run_indices.begin(), visible_run_indices.end(), [&](size_t a, size_t b) {
			const auto& ra = runs[a];
			const auto& rb = runs[b];
			bool less;
			switch (benchmark_sort_mode_) {
				case 1: less = ra.label < rb.label; break;
				case 2: less = ra.frame_time_summary.mean < rb.frame_time_summary.mean; break;
				case 3: less = ra.frame_time_summary.percentile_95 < rb.frame_time_summary.percentile_95; break;
				case 4: less = ra.fps_summary.mean < rb.fps_summary.mean; break;
				case 5: less = ra.config.resolution_scale < rb.config.resolution_scale; break;
				case 6: less = ra.config.max_ray_steps < rb.config.max_ray_steps; break;
				case 7: less = ra.gpu_path_ratio < rb.gpu_path_ratio; break;
				case 8: less = ra.config.metric_name < rb.config.metric_name; break;
				case 9: less = ra.config.integrator_name < rb.config.integrator_name; break;
				case 0:
				default: less = ra.timestamp < rb.timestamp; break;
			}
			return benchmark_sort_descending_ ? !less : less;
		});

		ImGui::BeginChild("BenchmarkRunsTableRegion", ImVec2(0.0f, 340.0f), false, ImGuiWindowFlags_HorizontalScrollbar);
		if (ImGui::BeginTable("BenchmarkRunsTable", 29, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollX)) {
			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 160.0f);
			ImGui::TableSetupColumn("Timestamp", ImGuiTableColumnFlags_WidthFixed, 130.0f);
			ImGui::TableSetupColumn("Metric", ImGuiTableColumnFlags_WidthFixed, 140.0f);
			ImGui::TableSetupColumn("Integrator", ImGuiTableColumnFlags_WidthFixed, 130.0f);
			ImGui::TableSetupColumn("Mean FT (ms)", ImGuiTableColumnFlags_WidthFixed, 90.0f);
			ImGui::TableSetupColumn("P95 FT (ms)", ImGuiTableColumnFlags_WidthFixed, 90.0f);
			ImGui::TableSetupColumn("P99 FT (ms)", ImGuiTableColumnFlags_WidthFixed, 90.0f);
			ImGui::TableSetupColumn("Mean FPS", ImGuiTableColumnFlags_WidthFixed, 80.0f);
			ImGui::TableSetupColumn("Min FPS", ImGuiTableColumnFlags_WidthFixed, 80.0f);
			ImGui::TableSetupColumn("Res Scale", ImGuiTableColumnFlags_WidthFixed, 80.0f);
			ImGui::TableSetupColumn("Ray Steps", ImGuiTableColumnFlags_WidthFixed, 80.0f);
			ImGui::TableSetupColumn("Precision", ImGuiTableColumnFlags_WidthFixed, 100.0f);
			ImGui::TableSetupColumn("GPU Ratio", ImGuiTableColumnFlags_WidthFixed, 80.0f);
			ImGui::TableSetupColumn("Tiled", ImGuiTableColumnFlags_WidthFixed, 60.0f);
			ImGui::TableSetupColumn("SIMD", ImGuiTableColumnFlags_WidthFixed, 60.0f);
			ImGui::TableSetupColumn("Step Ctrl", ImGuiTableColumnFlags_WidthFixed, 110.0f);
			ImGui::TableSetupColumn("Motion Quality", ImGuiTableColumnFlags_WidthFixed, 130.0f);
			ImGui::TableSetupColumn("Space Skip", ImGuiTableColumnFlags_WidthFixed, 110.0f);
			ImGui::TableSetupColumn("Pole Guard", ImGuiTableColumnFlags_WidthFixed, 90.0f);
			ImGui::TableSetupColumn("Far-Field Step", ImGuiTableColumnFlags_WidthFixed, 110.0f);
			ImGui::TableSetupColumn("LOD", ImGuiTableColumnFlags_WidthFixed, 150.0f);
			ImGui::TableSetupColumn("Render Distance", ImGuiTableColumnFlags_WidthFixed, 120.0f);
			ImGui::TableSetupColumn("Interlace", ImGuiTableColumnFlags_WidthFixed, 70.0f);
			ImGui::TableSetupColumn("Dynamic Res", ImGuiTableColumnFlags_WidthFixed, 130.0f);
			ImGui::TableSetupColumn("Tile Prepass", ImGuiTableColumnFlags_WidthFixed, 90.0f);
			ImGui::TableSetupColumn("Rolling Avg N", ImGuiTableColumnFlags_WidthFixed, 100.0f);
			ImGui::TableSetupColumn("rtol / atol", ImGuiTableColumnFlags_WidthFixed, 140.0f);
			ImGui::TableSetupColumn("Resolution", ImGuiTableColumnFlags_WidthFixed, 100.0f);
			ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 260.0f);
			ImGui::TableHeadersRow();

			for (const size_t i : visible_run_indices) {
				const auto& run = runs[i];

				ImGui::TableNextRow();
				ImGui::PushID(static_cast<int>(i));

				int col = 0;
				ImGui::TableSetColumnIndex(col++);
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

				ImGui::TableSetColumnIndex(col++); ImGui::TextUnformatted(run.timestamp.c_str());
				ImGui::TableSetColumnIndex(col++); ImGui::TextUnformatted(run.config.metric_name.c_str());
				ImGui::TableSetColumnIndex(col++); ImGui::TextUnformatted(run.config.integrator_name.c_str());
				ImGui::TableSetColumnIndex(col++); ImGui::Text("%.3f", run.frame_time_summary.mean);
				ImGui::TableSetColumnIndex(col++); ImGui::Text("%.3f", run.frame_time_summary.percentile_95);
				ImGui::TableSetColumnIndex(col++); ImGui::Text("%.3f", run.frame_time_summary.percentile_99);
				ImGui::TableSetColumnIndex(col++); ImGui::Text("%.1f", run.fps_summary.mean);
				ImGui::TableSetColumnIndex(col++); ImGui::Text("%.1f", run.fps_summary.min_value);
				ImGui::TableSetColumnIndex(col++); ImGui::Text("%.2fx", run.config.resolution_scale);
				ImGui::TableSetColumnIndex(col++); ImGui::Text("%u", run.config.max_ray_steps);
				ImGui::TableSetColumnIndex(col++); ImGui::TextUnformatted(run.config.precision_mode == 0 ? "FP64" : "Double-Single");
				ImGui::TableSetColumnIndex(col++); ImGui::Text("%.0f%%", run.gpu_path_ratio * 100.0);
				ImGui::TableSetColumnIndex(col++); ImGui::TextUnformatted(run.config.tiled_distribution ? "Yes" : "No");
				ImGui::TableSetColumnIndex(col++); ImGui::TextUnformatted(run.config.simd_pipeline ? "Yes" : "No");
				ImGui::TableSetColumnIndex(col++);
				{
					static constexpr const char* kStepCtrlNames[] = {"Standard", "PI-30", "PID-42"};
					ImGui::TextUnformatted(kStepCtrlNames[std::min<uint32_t>(run.config.step_controller_mode, 2U)]);
				}
				ImGui::TableSetColumnIndex(col++);
				{
					static constexpr const char* kMotionModes[] = {"Disabled", "Automatic", "Fixed"};
					ImGui::Text("%s (%.2fx)", kMotionModes[std::min<uint32_t>(run.config.motion_quality_mode, 2U)], run.config.motion_quality_scale);
				}
				ImGui::TableSetColumnIndex(col++);
				ImGui::TextUnformatted(run.config.space_skipping_enabled ? (std::to_string(run.config.space_skip_radius_scale).substr(0, 5) + " M").c_str() : "Off");
				ImGui::TableSetColumnIndex(col++); ImGui::Text("%.2f", run.config.pole_guard_precision_scale);
				ImGui::TableSetColumnIndex(col++); ImGui::Text("%.2fx", run.config.far_field_step_scale);
				ImGui::TableSetColumnIndex(col++);
				ImGui::TextUnformatted(run.config.lod_enabled ? (std::to_string(static_cast<int>(run.config.lod_distance_scale)) + " M / " + std::to_string(run.config.lod_reduced_ray_steps) + " steps").c_str() : "Off");
				ImGui::TableSetColumnIndex(col++);
				if (run.config.render_distance_scale > 0.0) {
					ImGui::Text("%.0f M", run.config.render_distance_scale);
				} else {
					ImGui::TextUnformatted("Unbounded");
				}
				ImGui::TableSetColumnIndex(col++); ImGui::TextUnformatted(run.config.interlace_rendering_enabled ? "Yes" : "No");
				ImGui::TableSetColumnIndex(col++);
				ImGui::TextUnformatted(run.config.dynamic_resolution_enabled ? (std::to_string(static_cast<int>(run.config.dynamic_resolution_target_fps)) + " fps target").c_str() : "Off");
				ImGui::TableSetColumnIndex(col++); ImGui::TextUnformatted(run.config.adaptive_tile_prepass_enabled ? "Yes" : "No");
				ImGui::TableSetColumnIndex(col++); ImGui::Text("%u", run.config.rolling_average_frame_count);
				ImGui::TableSetColumnIndex(col++); ImGui::Text("%.1e / %.1e", run.config.integration_rtol, run.config.integration_atol);
				ImGui::TableSetColumnIndex(col++); ImGui::Text("%ux%u", run.config.screen_width, run.config.screen_height);

				ImGui::TableSetColumnIndex(col++);
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
		ImGui::EndChild();

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
		if (ImGui::SliderInt("History Capacity (frames)", &history_capacity_input_, 120, 20000)) {
			profiler.set_history_capacity(static_cast<size_t>(history_capacity_input_));
		}
		render_setting_tooltip("Number of recent frame samples retained in memory for the Live Monitor and Statistics tabs. Does not affect saved benchmark runs. Very large values increase memory use and the cost of statistics computation without improving accuracy beyond a few thousand samples.");

		ImGui::TextDisabled("Quick presets:");
		ImGui::SameLine();
		if (ImGui::SmallButton("600")) { history_capacity_input_ = 600; profiler.set_history_capacity(600); }
		ImGui::SameLine();
		if (ImGui::SmallButton("3600")) { history_capacity_input_ = 3600; profiler.set_history_capacity(3600); }
		ImGui::SameLine();
		if (ImGui::SmallButton("10000")) { history_capacity_input_ = 10000; profiler.set_history_capacity(10000); }
		ImGui::SameLine();
		if (ImGui::SmallButton("20000")) { history_capacity_input_ = 20000; profiler.set_history_capacity(20000); }
		render_setting_tooltip("Common capacity presets. 3600 frames covers roughly one minute at 60 FPS.");

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
		ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.6f, 1.0f), "Linked HUD Readouts");
		ImGui::TextDisabled("Every profiler statistic on this window can also be pinned directly to the viewport via HUD Manager:");
		ImGui::BulletText("Profiler: Frame Time Detail mirrors dispatch and upload timings shown in the Live Monitor tab.");
		ImGui::BulletText("Profiler: Bottleneck Summary mirrors the dominant stage identified in the Bottleneck Analysis tab.");
		ImGui::BulletText("Profiler: Stage Breakdown mirrors HUD Overlay, Camera Update, and Schematic Overlay timings.");
		ImGui::BulletText("Profiler: GPU/CPU Split mirrors the render path distribution shown above.");
		ImGui::BulletText("Profiler: Iteration Range mirrors the ray step statistics from the Live Monitor tab.");

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.6f, 1.0f), "Proposed Keybinds");
		ImGui::TextDisabled("No default keys are bound. Assign these in Keybind Settings if desired:");
		ImGui::BulletText("Toggle Performance Analysis Window");
		ImGui::BulletText("Start/Stop Quick Benchmark Capture");
		ImGui::BulletText("Quick Save Live Window As Benchmark Run");
	}
};

}
