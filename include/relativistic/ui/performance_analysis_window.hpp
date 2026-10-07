#pragma once

#include <imgui.h>
#include <implot.h>
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/orchestrator/performance_profiler.hpp"
#include "relativistic/orchestrator/benchmark_metrics.hpp"
#include "relativistic/orchestrator/command.hpp"
#include "relativistic/render/geodesic_compute_pipeline.hpp"
#include "relativistic/render/gpu_types.hpp"
#include "relativistic/ui/benchmark_workbench.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include <vector>
#include <string>
#include <string_view>
#include <optional>
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <utility>

namespace Relativistic::UI {

class PerformanceAnalysisWindow {
private:
	using Stage = Orchestrator::ProfilerTaskStage;
	static constexpr size_t kStageCount = static_cast<size_t>(Stage::Count);

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
	int stage_window_{120};
	int tree_command_{0};
	int statistics_window_choice_{0};
	int statistics_domain_filter_{0};
	bool statistics_hide_zero_stages_{true};
	std::array<bool, kStageCount> plotted_stages_{};
	char benchmark_search_filter_[64]{};
	std::vector<char> column_visible_{};
	int table_generation_{0};
	bool delete_requested_{false};
	int delete_index_{-1};
	bool rename_requested_{false};
	int rename_index_{-1};
	char rename_buffer_[96]{};
	bool show_only_differences_{true};
	double clipboard_feedback_until_{0.0};
	BenchmarkWorkbench workbench_{};

public:
	explicit PerformanceAnalysisWindow(Orchestrator::SimulationOrchestrator<1024>& orchestrator)
		: orchestrator_(orchestrator) {
		history_capacity_input_ = static_cast<int>(orchestrator_.profiler().history_capacity());
		plotted_stages_.fill(false);
		plotted_stages_[static_cast<size_t>(Stage::RenderDispatch)] = true;
		plotted_stages_[static_cast<size_t>(Stage::TextureUpload)] = true;
		plotted_stages_[static_cast<size_t>(Stage::HudOverlay)] = true;
	}

	[[nodiscard]] bool& open_state() noexcept { return is_open_; }

	void attach_render_pipeline(Render::GeodesicComputePipeline& pipeline) noexcept {
		render_pipeline_ = &pipeline;
	}

	void render() {
		if (!is_open_) return;

		ImGui::SetNextWindowPos(ImVec2(200.0f, 90.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(1040.0f, 760.0f), ImGuiCond_FirstUseEver);

		if (!ImGui::Begin("Performance Analysis & Profiling", &is_open_)) {
			ImGui::End();
			return;
		}

		auto& profiler = orchestrator_.profiler();
		render_signature_header(profiler);

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
			if (ImGui::BeginTabItem("Comparison Workbench")) {
				workbench_.render(profiler);
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
	void copy_to_clipboard(const std::string& text) {
		ImGui::SetClipboardText(text.c_str());
		clipboard_feedback_until_ = ImGui::GetTime() + 1.5;
	}

	static void show_text_tooltip(const char* text) {
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 28.0f);
		ImGui::TextUnformatted(text);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}

	[[nodiscard]] static std::string lowercase(std::string_view text) {
		std::string result(text);
		std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return result;
	}

	void render_signature_header(const Orchestrator::PerformanceProfiler& profiler) {
		const std::string& signature = profiler.engine_signature();
		ImGui::TextColored(ImVec4(0.3f, 0.9f, 1.0f, 1.0f), "Engine Signature: %s", signature.c_str());
		if (ImGui::IsItemHovered()) {
			ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
			if (ImGui::IsItemClicked()) {
				copy_to_clipboard(signature);
			}
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 28.0f);
			if (clipboard_feedback_until_ > ImGui::GetTime()) {
				ImGui::TextColored(ImVec4(0.4f, 0.95f, 0.5f, 1.0f), "Copied to clipboard.");
			} else {
				ImGui::TextUnformatted("Click to copy the engine signature to the clipboard.");
			}
			ImGui::Spacing();
			ImGui::TextUnformatted("Structural fingerprint of performance-relevant internal data layouts. Benchmark runs recorded under a different signature may not be directly comparable.");
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
	}

	[[nodiscard]] static std::vector<Stage> stage_children(Stage parent) {
		std::vector<Stage> result;
		for (size_t i = 0; i < kStageCount; ++i) {
			const auto candidate = static_cast<Stage>(i);
			if (candidate != parent && Orchestrator::profiler_stage_parent(candidate) == parent) {
				result.push_back(candidate);
			}
		}
		return result;
	}

	static void collect_stage_order(Stage stage, size_t depth, std::vector<std::pair<Stage, size_t>>& out) {
		out.emplace_back(stage, depth);
		for (const Stage child : stage_children(stage)) {
			collect_stage_order(child, depth + 1, out);
		}
	}

	[[nodiscard]] static ImVec4 stage_depth_color(size_t depth) noexcept {
		static constexpr ImVec4 kPalette[] = {
			ImVec4(0.30f, 0.62f, 0.95f, 1.0f),
			ImVec4(0.30f, 0.78f, 0.62f, 1.0f),
			ImVec4(0.90f, 0.68f, 0.28f, 1.0f),
			ImVec4(0.82f, 0.45f, 0.78f, 1.0f),
			ImVec4(0.88f, 0.45f, 0.40f, 1.0f)
		};
		return kPalette[depth % (sizeof(kPalette) / sizeof(kPalette[0]))];
	}

	bool render_stage_row(const char* name, const char* description, double average_ms, double share, bool leaf, size_t depth) {
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding;
		if (leaf) {
			flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
		} else if (tree_command_ != 0) {
			ImGui::SetNextItemOpen(tree_command_ == 1, ImGuiCond_Always);
		}
		const bool open = ImGui::TreeNodeEx("##stage_row", flags);
		const bool hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort);
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_PlotHistogram, stage_depth_color(depth));
		char overlay[192];
		std::snprintf(overlay, sizeof(overlay), "%s  |  %.1f%%  |  %.3f ms", name, share * 100.0, average_ms);
		ImGui::ProgressBar(static_cast<float>(std::clamp(share, 0.0, 1.0)), ImVec2(-1.0f, 0.0f), overlay);
		ImGui::PopStyleColor();
		if (hovered && description != nullptr && description[0] != '\0') {
			show_text_tooltip(description);
		}
		return open && !leaf;
	}

	void render_stage_branch(const Orchestrator::PerformanceProfiler::StageAccumulation& accumulation, Stage stage, double basis_ms, size_t depth) {
		const double average = accumulation.average(stage);
		const double share = (basis_ms > 1e-9) ? std::clamp(average / basis_ms, 0.0, 1.0) : 0.0;
		std::vector<Stage> children = stage_children(stage);
		children.erase(std::remove_if(children.begin(), children.end(), [&](Stage child) { return accumulation.average(child) <= 0.0; }), children.end());
		std::sort(children.begin(), children.end(), [&](Stage a, Stage b) { return accumulation.average(a) > accumulation.average(b); });
		double children_sum = 0.0;
		for (const Stage child : children) children_sum += accumulation.average(child);

		ImGui::PushID(static_cast<int>(stage));
		const bool open = render_stage_row(Orchestrator::profiler_stage_name(stage), Orchestrator::profiler_stage_description(stage), average, share, children.empty(), depth);
		if (open) {
			const double child_basis = std::max(average, children_sum);
			for (const Stage child : children) {
				render_stage_branch(accumulation, child, child_basis, depth + 1);
			}
			const double untracked = average - children_sum;
			if (untracked > std::max(average * 0.02, 1e-4)) {
				ImGui::PushID(-1);
				render_stage_row("Untracked", "Time inside this stage that is not covered by any instrumented child stage.", untracked, (child_basis > 1e-9) ? untracked / child_basis : 0.0, true, depth + 1);
				ImGui::PopID();
			}
			ImGui::TreePop();
		}
		ImGui::PopID();
	}

	void render_stage_hierarchy(const Orchestrator::PerformanceProfiler& profiler) {
		ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.3f, 1.0f), "Stage Time Breakdown (average ms per frame)");
		ImGui::SetNextItemWidth(220.0f);
		ImGui::SliderInt("Window (frames)", &stage_window_, 10, static_cast<int>(std::max<size_t>(std::min<size_t>(profiler.history_capacity(), 3600), 10)));
		ImGui::SameLine();
		tree_command_ = 0;
		if (ImGui::SmallButton("Expand All")) tree_command_ = 1;
		ImGui::SameLine();
		if (ImGui::SmallButton("Collapse All")) tree_command_ = 2;
		render_setting_tooltip("Every bar can be expanded to reveal the stages it is made of. Percentages of child stages are relative to their parent; top-level percentages are relative to the UI frame time. Render worker and simulation thread stages run concurrently with the UI thread.");

		const auto accumulation = profiler.accumulate_stages(static_cast<size_t>(std::max(stage_window_, 1)));
		std::vector<Stage> primary;
		double primary_sum = 0.0;
		for (size_t i = 0; i < kStageCount; ++i) {
			const auto stage = static_cast<Stage>(i);
			if (Orchestrator::profiler_stage_is_primary(stage) && accumulation.average(stage) > 0.0) {
				primary.push_back(stage);
				primary_sum += accumulation.average(stage);
			}
		}
		std::sort(primary.begin(), primary.end(), [&](Stage a, Stage b) { return accumulation.average(a) > accumulation.average(b); });
		const double frame_basis = (accumulation.average(Stage::FrameTotal) > 1e-9) ? accumulation.average(Stage::FrameTotal) : std::max(primary_sum, 1e-9);
		for (const Stage stage : primary) {
			render_stage_branch(accumulation, stage, frame_basis, 0);
		}

		ImGui::Spacing();
		if (ImGui::CollapsingHeader("Simulation Thread (time per displayed frame)")) {
			if (accumulation.average(Stage::SimulationTotal) <= 0.0) {
				ImGui::TextDisabled("No simulation activity was recorded in this window. The simulation clock may be paused.");
			} else {
				render_stage_branch(accumulation, Stage::SimulationTotal, accumulation.average(Stage::SimulationTotal), 0);
			}
		}
		tree_command_ = 0;
	}

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
			const auto live_tel = render_pipeline_->telemetry();
			ImGui::Text("Ray Iteration Range: %u - %u (avg %.1f)", live_tel.min_iterations_used, live_tel.max_iterations_used, live_tel.average_iterations_used);
			render_setting_tooltip("Minimum and maximum geodesic integration steps consumed by any single ray in the most recently completed frame, alongside the mean across all rays.");
			ImGui::Text("Adaptive Tile Prepass Skip: %llu tiles (%.3f ms) | Full Ray-Traced Tiles: %llu tiles (%.3f ms)", static_cast<unsigned long long>(live_tel.tile_prepass_skip_tile_count), live_tel.tile_prepass_skip_ms, static_cast<unsigned long long>(live_tel.full_raytrace_tile_count), live_tel.full_raytrace_tiles_ms);
			render_setting_tooltip("Cumulative thread time of the most recently completed CPU-tiled render, split between tiles filled analytically by the Adaptive Tile Sky Prepass and tiles that were fully integrated. Only populated when Tiled Work Distribution and Adaptive Tile Sky Prepass are both enabled and the CPU path is active.");
		}

		{
			const size_t window_count = std::min<size_t>(history.size(), 120);
			size_t gpu_frames = 0;
			for (size_t i = history.size() - window_count; i < history.size(); ++i) {
				if (history[i].used_gpu_path) ++gpu_frames;
			}
			const double gpu_ratio = static_cast<double>(gpu_frames) / static_cast<double>(window_count);
			ImGui::Text("Render Path Split (last %zu frames): GPU %.0f%% | CPU %.0f%%", window_count, gpu_ratio * 100.0, (1.0 - gpu_ratio) * 100.0);
			render_setting_tooltip("Fraction of recently completed frames dispatched through the Vulkan compute path versus the CPU SIMD/scalar fallback.");
		}

		ImGui::Spacing();
		render_stage_hierarchy(profiler);

		ImGui::Separator();
		ImGui::SliderInt("Chart Window (frames)", &plot_window_, 30, static_cast<int>(std::min<size_t>(profiler.history_capacity(), 3600)));
		render_setting_tooltip("Number of most recent frame samples displayed in the charts below. Independent from the History Capacity setting on the Settings tab.");
		ImGui::SameLine();
		if (ImGui::Button("Plotted Stages...")) {
			ImGui::OpenPopup("##PlottedStages");
		}
		if (ImGui::BeginPopup("##PlottedStages")) {
			for (size_t i = 0; i < kStageCount; ++i) {
				ImGui::Checkbox(Orchestrator::profiler_stage_name(static_cast<Stage>(i)), &plotted_stages_[i]);
			}
			ImGui::EndPopup();
		}

		const size_t count = std::min(static_cast<size_t>(std::max(plot_window_, 1)), history.size());
		std::vector<double> frame_times(count), fps_values(count), x_axis(count);
		std::vector<std::pair<Stage, std::vector<double>>> stage_series;
		for (size_t i = 0; i < kStageCount; ++i) {
			if (plotted_stages_[i]) stage_series.emplace_back(static_cast<Stage>(i), std::vector<double>(count));
		}
		for (size_t i = 0; i < count; ++i) {
			const auto& s = history[history.size() - count + i];
			frame_times[i] = s.frame_time_ms;
			fps_values[i] = s.fps;
			x_axis[i] = static_cast<double>(i);
			for (auto& series : stage_series) {
				series.second[i] = s.stage_time_ms[static_cast<size_t>(series.first)];
			}
		}

		if (ImPlot::BeginPlot("Frame Time History", ImVec2(-1, 240))) {
			ImPlot::SetupAxes("Sample", "Milliseconds", ImPlotAxisFlags_None, ImPlotAxisFlags_AutoFit);
			ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, static_cast<double>(count > 0 ? count - 1 : 0), ImPlotCond_Always);
			ImPlot::PlotLine("Frame Time (Total UI)", x_axis.data(), frame_times.data(), static_cast<int>(count));
			for (const auto& series : stage_series) {
				ImPlot::PlotLine(Orchestrator::profiler_stage_name(series.first), x_axis.data(), series.second.data(), static_cast<int>(count));
			}
			ImPlot::EndPlot();
		}

		if (ImPlot::BeginPlot("FPS History", ImVec2(-1, 180))) {
			ImPlot::SetupAxes("Sample", "FPS", ImPlotAxisFlags_None, ImPlotAxisFlags_AutoFit);
			ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, static_cast<double>(count > 0 ? count - 1 : 0), ImPlotCond_Always);
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

	static void draw_summary_table(const char* label, const char* description, const char* hint, const Orchestrator::StatisticalSummary& s, size_t depth = 0) {
		if (s.sample_count == 0) return;
		ImGui::PushID(label);
		std::string indent(depth * 2, ' ');
		if (depth > 0) indent += "- ";
		ImGui::TextColored(stage_depth_color(depth), "%s%s", indent.c_str(), label);
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 28.0f);
			if (description != nullptr && description[0] != '\0') {
				ImGui::TextUnformatted(description);
			}
			if (hint != nullptr && hint[0] != '\0') {
				ImGui::Spacing();
				ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.5f, 1.0f), "Optimization Hint: %s", hint);
			}
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}

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
		const char* window_labels[] = {"Last 60 Frames", "Last 300 Frames", "Last 1000 Frames", "Entire History"};
		ImGui::SetNextItemWidth(180.0f);
		ImGui::Combo("Sample Window", &statistics_window_choice_, window_labels, IM_ARRAYSIZE(window_labels));
		ImGui::SameLine();
		const char* domain_labels[] = {"All Domains", "UI Thread", "Render Worker", "Simulation Thread"};
		ImGui::SetNextItemWidth(180.0f);
		ImGui::Combo("Domain Filter", &statistics_domain_filter_, domain_labels, IM_ARRAYSIZE(domain_labels));
		ImGui::SameLine();
		ImGui::Checkbox("Hide Inactive Stages", &statistics_hide_zero_stages_);

		size_t n = 0;
		switch (statistics_window_choice_) {
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

		if (statistics_domain_filter_ == 0 || statistics_domain_filter_ == 1) {
			draw_summary_table("Total Frame Time (ms)", Orchestrator::profiler_stage_description(Stage::FrameTotal), "", ft_summary);
			draw_summary_table("Average Ray Iterations Per Frame", "Average number of geodesic integration steps evaluated per pixel.", "", profiler.iteration_summary(n));
		}

		std::vector<std::pair<Stage, size_t>> ordered_stages;
		collect_stage_order(Stage::FrameTotal, 0, ordered_stages);
		collect_stage_order(Stage::RenderDispatch, 0, ordered_stages);
		collect_stage_order(Stage::SimulationTotal, 0, ordered_stages);

		std::vector<std::pair<Stage, size_t>> unique_stages;
		for (const auto& entry : ordered_stages) {
			if (entry.first == Stage::FrameTotal) continue;
			bool exists = false;
			for (const auto& u : unique_stages) {
				if (u.first == entry.first) { exists = true; break; }
			}
			if (!exists) unique_stages.push_back(entry);
		}

		for (const auto& [stage, depth] : unique_stages) {
			const auto domain = Orchestrator::profiler_stage_domain(stage);
			if (statistics_domain_filter_ == 1 && domain != Orchestrator::ProfilerStageDomain::UiThread) continue;
			if (statistics_domain_filter_ == 2 && domain != Orchestrator::ProfilerStageDomain::RenderWorker) continue;
			if (statistics_domain_filter_ == 3 && domain != Orchestrator::ProfilerStageDomain::SimulationThread) continue;

			const auto summary = profiler.stage_summary(stage, n);
			if (statistics_hide_zero_stages_ && summary.mean <= 0.0 && summary.max_value <= 0.0) continue;

			draw_summary_table(
				Orchestrator::profiler_stage_name(stage),
				Orchestrator::profiler_stage_description(stage),
				Orchestrator::profiler_stage_hint(stage),
				summary,
				depth
			);
		}

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
		render_setting_tooltip("Count of frames rendered via GPU compute dispatch versus the CPU pipeline across the selected sample window.");
	}

	void render_bottleneck_tab(Orchestrator::PerformanceProfiler& profiler) {
		static int sample_span = 120;
		ImGui::SliderInt("Analysis Window (frames)", &sample_span, 10, 1000);
		render_setting_tooltip("Number of most recent frames analyzed to determine which pipeline stage dominates render time and whether the geodesic step budget is being exhausted before rays terminate naturally.");

		const auto report = profiler.analyze_bottleneck(static_cast<size_t>(sample_span));
		ImGui::TextWrapped("%s", report.summary.c_str());

		ImGui::Separator();
		ImGui::Text("Dominant Primary Stage: %s (%.1f%% of primary measured time)", Orchestrator::profiler_stage_name(report.dominant_stage), report.dominant_share * 100.0);

		if (report.dominant_chain.size() > 1) {
			ImGui::Spacing();
			ImGui::TextColored(ImVec4(0.9f, 0.75f, 0.3f, 1.0f), "Hot Path Sub-Stage Chain:");
			for (size_t i = 0; i < report.dominant_chain.size(); ++i) {
				const auto st = report.dominant_chain[i];
				std::string prefix(i * 2, ' ');
				prefix += (i > 0) ? "-> " : "";
				ImGui::Text("%s%s", prefix.c_str(), Orchestrator::profiler_stage_name(st));
				const char* desc = Orchestrator::profiler_stage_description(st);
				if (desc != nullptr && desc[0] != '\0') {
					ImGui::SameLine();
					ImGui::TextDisabled("(%s)", desc);
				}
			}
		}

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
		if (report.dominant_stage == Stage::TextureUpload) {
			ImGui::BulletText("Reduce Internal Render Scale or disable Force Texture Reallocation.");
		}
		if (report.gpu_underutilized && render_pipeline_ != nullptr && render_pipeline_->gpu_compute_available()) {
			ImGui::BulletText("A compatible GPU compute device is available; enabling it may reduce render dispatch cost.");
		}
		for (const auto st : report.dominant_chain) {
			const char* hint = Orchestrator::profiler_stage_hint(st);
			if (hint != nullptr && hint[0] != '\0') {
				ImGui::BulletText("[%s]: %s", Orchestrator::profiler_stage_name(st), hint);
			}
		}

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.3f, 1.0f), "Render Dispatch Cost Breakdown");
		render_setting_tooltip("Heuristic estimate of how Render Dispatch time is distributed across ray outcome categories, derived from ray classification counts and average integration steps over the analysis window.");

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
			dispatch_time_total += s.stage_time_ms[static_cast<size_t>(Stage::RenderDispatch)];
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
			ImGui::TextDisabled("Select two runs from the library above using the [A] and [B] buttons to compare them.");
			return;
		}

		const auto& a = runs[static_cast<size_t>(selected_run_indices_[0])];
		const auto& b = runs[static_cast<size_t>(selected_run_indices_[1])];

		if (a.engine_signature != b.engine_signature) {
			ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.3f, 1.0f), "Warning: these runs were captured under different engine signatures. Timings may reflect internal code or struct layout changes.");
		}

		ImGui::Checkbox("Show Only Differences", &show_only_differences_);
		ImGui::SameLine();
		if (ImGui::SmallButton("Swap A <-> B")) {
			std::swap(selected_run_indices_[0], selected_run_indices_[1]);
		}

		const auto ft_sig = Orchestrator::compare_summaries(a.frame_time_summary, b.frame_time_summary);
		const auto fps_sig = Orchestrator::compare_summaries(a.fps_summary, b.fps_summary);
		const auto tp_sig = Orchestrator::compare_summaries(a.throughput_summary, b.throughput_summary);

		if (ft_sig.valid) {
			ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.3f, 1.0f), "Statistical Significance Summary (B vs A baseline)");
			const auto cohen_label = [](double d) {
				const double ad = std::abs(d);
				if (ad < 0.2) return "Negligible";
				if (ad < 0.5) return "Small";
				if (ad < 0.8) return "Medium";
				return "Large";
			};
			ImGui::BulletText("Frame Time: %+.3f ms (%+.1f%%) | p = %.4f (%s) | Cohen's d = %.2f (%s) | 95%% CI [%+.3f, %+.3f] ms",
				ft_sig.mean_difference, ft_sig.relative_percent, ft_sig.p_value, ft_sig.significant() ? "Significant" : "Not Significant",
				ft_sig.cohens_d, cohen_label(ft_sig.cohens_d), ft_sig.interval_low, ft_sig.interval_high);
			if (fps_sig.valid) {
				ImGui::BulletText("FPS: %+.1f FPS (%+.1f%%) | p = %.4f (%s) | Cohen's d = %.2f (%s)",
					fps_sig.mean_difference, fps_sig.relative_percent, fps_sig.p_value, fps_sig.significant() ? "Significant" : "Not Significant",
					fps_sig.cohens_d, cohen_label(fps_sig.cohens_d));
			}
			if (tp_sig.valid && (a.throughput_summary.mean > 0.0 || b.throughput_summary.mean > 0.0)) {
				ImGui::BulletText("Throughput: %+.2f MPx/s (%+.1f%%) | p = %.4f (%s)",
					tp_sig.mean_difference, tp_sig.relative_percent, tp_sig.p_value, tp_sig.significant() ? "Significant" : "Not Significant");
			}
			ImGui::Spacing();
		}

		if (ImGui::BeginTable("ComparisonTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
			ImGui::TableSetupColumn("Metric / Parameter", ImGuiTableColumnFlags_WidthFixed, 220.0f);
			ImGui::TableSetupColumn((std::string("[A] ") + a.label).c_str(), ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn((std::string("[B] ") + b.label).c_str(), ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Delta (B - A)", ImGuiTableColumnFlags_WidthFixed, 140.0f);
			ImGui::TableHeadersRow();

			auto row_numeric = [&](const char* metric_label, double va, double vb, const char* fmt, Orchestrator::BenchmarkMetricDirection dir) {
				const double delta = vb - va;
				if (show_only_differences_ && std::abs(delta) < 1e-9) return;
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted(metric_label);

				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%s", Orchestrator::benchmark_format_number(fmt, va).c_str());

				ImGui::TableSetColumnIndex(2);
				ImGui::Text("%s", Orchestrator::benchmark_format_number(fmt, vb).c_str());

				ImGui::TableSetColumnIndex(3);
				ImVec4 color(0.7f, 0.7f, 0.7f, 1.0f);
				if (dir == Orchestrator::BenchmarkMetricDirection::LowerIsBetter) {
					color = (delta < -1e-6) ? ImVec4(0.4f, 0.9f, 0.5f, 1.0f) : ((delta > 1e-6) ? ImVec4(1.0f, 0.5f, 0.4f, 1.0f) : color);
				} else if (dir == Orchestrator::BenchmarkMetricDirection::HigherIsBetter) {
					color = (delta > 1e-6) ? ImVec4(0.4f, 0.9f, 0.5f, 1.0f) : ((delta < -1e-6) ? ImVec4(1.0f, 0.5f, 0.4f, 1.0f) : color);
				}
				ImGui::TextColored(color, "%s", Orchestrator::benchmark_format_number(fmt, delta).c_str());
			};

			auto row_text = [&](const char* param_label, const std::string& ta, const std::string& tb) {
				if (show_only_differences_ && ta == tb) return;
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted(param_label);

				ImGui::TableSetColumnIndex(1);
				ImGui::TextUnformatted(ta.c_str());

				ImGui::TableSetColumnIndex(2);
				ImGui::TextUnformatted(tb.c_str());

				ImGui::TableSetColumnIndex(3);
				if (ta == tb) {
					ImGui::TextDisabled("Identical");
				} else {
					ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Changed");
				}
			};

			row_numeric("Mean Frame Time (ms)", a.frame_time_summary.mean, b.frame_time_summary.mean, "%.3f", Orchestrator::BenchmarkMetricDirection::LowerIsBetter);
			row_numeric("Median Frame Time (ms)", a.frame_time_summary.median, b.frame_time_summary.median, "%.3f", Orchestrator::BenchmarkMetricDirection::LowerIsBetter);
			row_numeric("P95 Frame Time (ms)", a.frame_time_summary.percentile_95, b.frame_time_summary.percentile_95, "%.3f", Orchestrator::BenchmarkMetricDirection::LowerIsBetter);
			row_numeric("P99 Frame Time (ms)", a.frame_time_summary.percentile_99, b.frame_time_summary.percentile_99, "%.3f", Orchestrator::BenchmarkMetricDirection::LowerIsBetter);
			row_numeric("Frame Time Std Dev (ms)", a.frame_time_summary.std_deviation, b.frame_time_summary.std_deviation, "%.3f", Orchestrator::BenchmarkMetricDirection::LowerIsBetter);
			row_numeric("Mean FPS", a.fps_summary.mean, b.fps_summary.mean, "%.1f", Orchestrator::BenchmarkMetricDirection::HigherIsBetter);
			row_numeric("Min FPS", a.fps_summary.min_value, b.fps_summary.min_value, "%.1f", Orchestrator::BenchmarkMetricDirection::HigherIsBetter);
			row_numeric("1% Low FPS", (a.frame_time_summary.percentile_99 > 0.0) ? 1000.0 / a.frame_time_summary.percentile_99 : 0.0,
			                          (b.frame_time_summary.percentile_99 > 0.0) ? 1000.0 / b.frame_time_summary.percentile_99 : 0.0, "%.1f", Orchestrator::BenchmarkMetricDirection::HigherIsBetter);
			row_numeric("Mean Throughput (MPx/s)", a.throughput_summary.mean, b.throughput_summary.mean, "%.2f", Orchestrator::BenchmarkMetricDirection::HigherIsBetter);
			row_numeric("Avg Ray Iterations", a.average_iterations, b.average_iterations, "%.1f", Orchestrator::BenchmarkMetricDirection::LowerIsBetter);
			row_numeric("GPU Path Ratio (%)", a.gpu_path_ratio * 100.0, b.gpu_path_ratio * 100.0, "%.1f%%", Orchestrator::BenchmarkMetricDirection::Neutral);
			row_numeric("Horizon Hit Ratio (%)", a.horizon_hit_ratio * 100.0, b.horizon_hit_ratio * 100.0, "%.2f%%", Orchestrator::BenchmarkMetricDirection::Neutral);
			row_numeric("Celestial Hit Ratio (%)", a.celestial_hit_ratio * 100.0, b.celestial_hit_ratio * 100.0, "%.2f%%", Orchestrator::BenchmarkMetricDirection::Neutral);
			row_numeric("Disk Hit Ratio (%)", a.disk_hit_ratio * 100.0, b.disk_hit_ratio * 100.0, "%.2f%%", Orchestrator::BenchmarkMetricDirection::Neutral);

			row_text("Metric", a.config.metric_name, b.config.metric_name);
			row_text("Integrator", a.config.integrator_name, b.config.integrator_name);
			row_text("Performance Preset", Orchestrator::benchmark_preset_name(a.config.performance_preset), Orchestrator::benchmark_preset_name(b.config.performance_preset));
			row_numeric("Resolution Scale", a.config.resolution_scale, b.config.resolution_scale, "%.2f", Orchestrator::BenchmarkMetricDirection::Neutral);
			row_text("Resolution (px)", std::to_string(a.config.screen_width) + "x" + std::to_string(a.config.screen_height), std::to_string(b.config.screen_width) + "x" + std::to_string(b.config.screen_height));
			row_numeric("Max Ray Steps", static_cast<double>(a.config.max_ray_steps), static_cast<double>(b.config.max_ray_steps), "%.0f", Orchestrator::BenchmarkMetricDirection::Neutral);
			row_text("Precision Mode", a.config.precision_mode == 0 ? "FP64" : "Double-Single", b.config.precision_mode == 0 ? "FP64" : "Double-Single");
			row_text("GPU Compute", a.config.use_gpu_compute ? "Enabled" : "Disabled", b.config.use_gpu_compute ? "Enabled" : "Disabled");
			row_text("Tiled Distribution", a.config.tiled_distribution ? "Enabled" : "Disabled", b.config.tiled_distribution ? "Enabled" : "Disabled");
			row_text("SIMD Pipeline", a.config.simd_pipeline ? "Enabled" : "Disabled", b.config.simd_pipeline ? "Enabled" : "Disabled");
			{
				static constexpr const char* kStepCtrl[] = {"Standard", "PI-30", "PID-42"};
				row_text("Step Controller", kStepCtrl[std::min<uint32_t>(a.config.step_controller_mode, 2U)], kStepCtrl[std::min<uint32_t>(b.config.step_controller_mode, 2U)]);
			}
			row_numeric("Motion Quality Scale", a.config.motion_quality_scale, b.config.motion_quality_scale, "%.2f", Orchestrator::BenchmarkMetricDirection::Neutral);
			row_numeric("Space Skip Radius (M)", a.config.space_skipping_enabled ? a.config.space_skip_radius_scale : 0.0, b.config.space_skipping_enabled ? b.config.space_skip_radius_scale : 0.0, "%.1f", Orchestrator::BenchmarkMetricDirection::Neutral);
			row_numeric("Pole Guard Precision", a.config.pole_guard_precision_scale, b.config.pole_guard_precision_scale, "%.2f", Orchestrator::BenchmarkMetricDirection::Neutral);
			row_numeric("Far-Field Step Multiplier", a.config.far_field_step_scale, b.config.far_field_step_scale, "%.2f", Orchestrator::BenchmarkMetricDirection::Neutral);
			row_numeric("LOD Distance Threshold (M)", a.config.lod_enabled ? a.config.lod_distance_scale : 0.0, b.config.lod_enabled ? b.config.lod_distance_scale : 0.0, "%.0f", Orchestrator::BenchmarkMetricDirection::Neutral);
			row_numeric("LOD Reduced Steps", static_cast<double>(a.config.lod_reduced_ray_steps), static_cast<double>(b.config.lod_reduced_ray_steps), "%.0f", Orchestrator::BenchmarkMetricDirection::Neutral);
			row_numeric("Render Distance (M)", a.config.render_distance_scale, b.config.render_distance_scale, "%.0f", Orchestrator::BenchmarkMetricDirection::Neutral);
			row_text("Interlace Rendering", a.config.interlace_rendering_enabled ? "Enabled" : "Disabled", b.config.interlace_rendering_enabled ? "Enabled" : "Disabled");
			row_numeric("Dynamic Res Target FPS", a.config.dynamic_resolution_enabled ? a.config.dynamic_resolution_target_fps : 0.0, b.config.dynamic_resolution_enabled ? b.config.dynamic_resolution_target_fps : 0.0, "%.0f", Orchestrator::BenchmarkMetricDirection::Neutral);
			row_text("Adaptive Tile Prepass", a.config.adaptive_tile_prepass_enabled ? "Enabled" : "Disabled", b.config.adaptive_tile_prepass_enabled ? "Enabled" : "Disabled");
			row_numeric("Rolling Average Window (N)", static_cast<double>(a.config.rolling_average_frame_count), static_cast<double>(b.config.rolling_average_frame_count), "%.0f", Orchestrator::BenchmarkMetricDirection::Neutral);
			row_numeric("Integration rtol", a.config.integration_rtol, b.config.integration_rtol, "%.3e", Orchestrator::BenchmarkMetricDirection::Neutral);
			row_numeric("Integration atol", a.config.integration_atol, b.config.integration_atol, "%.3e", Orchestrator::BenchmarkMetricDirection::Neutral);

			for (size_t st = 0; st < kStageCount; ++st) {
				const double ma = a.stage_mean_ms[st];
				const double mb = b.stage_mean_ms[st];
				if (ma > 0.0 || mb > 0.0) {
					std::string stage_label = std::string("Stage: ") + Orchestrator::profiler_stage_name(static_cast<Stage>(st)) + " (ms)";
					row_numeric(stage_label.c_str(), ma, mb, "%.3f", Orchestrator::BenchmarkMetricDirection::LowerIsBetter);
				}
			}

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
		const auto& all_columns = Orchestrator::benchmark_columns();
		if (column_visible_.size() != all_columns.size()) {
			column_visible_.resize(all_columns.size());
			for (size_t i = 0; i < all_columns.size(); ++i) {
				column_visible_[i] = all_columns[i].default_visible ? 1 : 0;
			}
		}

		ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.6f, 1.0f), "Capture a Benchmark Run");

		if (capture_label_auto_ && !profiler.is_capturing()) {
			const std::string suggested = build_suggested_label();
			if (suggested != last_suggested_label_) {
				std::strncpy(capture_label_buffer_, suggested.c_str(), sizeof(capture_label_buffer_) - 1);
				capture_label_buffer_[sizeof(capture_label_buffer_) - 1] = '\0';
				last_suggested_label_ = suggested;
			}
		}
		ImGui::SetNextItemWidth(260.0f);
		if (ImGui::InputText("Run Label", capture_label_buffer_, sizeof(capture_label_buffer_))) {
			capture_label_auto_ = false;
		}
		ImGui::SameLine();
		if (ImGui::Checkbox("Auto-Name", &capture_label_auto_)) {
			if (capture_label_auto_) {
				last_suggested_label_.clear();
			}
		}
		render_setting_tooltip("When enabled, the run label is regenerated automatically from the active metric, integrator, and performance preset.");

		const char* modes[] = {"By Duration", "By Frame Count"};
		ImGui::SetNextItemWidth(180.0f);
		ImGui::Combo("Capture Mode", &capture_mode_, modes, IM_ARRAYSIZE(modes));
		render_setting_tooltip("Chooses whether the benchmark run stops after a fixed wall-clock duration or after a fixed number of rendered frames.");
		ImGui::SameLine();
		if (capture_mode_ == 0) {
			ImGui::SetNextItemWidth(160.0f);
			ImGui::SliderFloat("Duration (s)", &capture_duration_seconds_, 1.0f, 120.0f, "%.1f s");
		} else {
			ImGui::SetNextItemWidth(160.0f);
			ImGui::SliderInt("Frames", &capture_frame_count_, 30, 6000);
		}

		if (profiler.is_capturing()) {
			ImGui::ProgressBar(static_cast<float>(profiler.capture_progress()), ImVec2(240.0f, 0.0f), "Capturing...");
			ImGui::SameLine();
			if (ImGui::Button("Cancel Capture", ImVec2(140.0f, 24.0f))) {
				profiler.cancel_capture();
			}
		} else {
			if (ImGui::Button("Start Capture", ImVec2(140.0f, 24.0f))) {
				const double now = ImGui::GetTime();
				if (capture_mode_ == 0) {
					profiler.start_capture(capture_label_buffer_, static_cast<double>(capture_duration_seconds_), std::nullopt, now);
				} else {
					profiler.start_capture(capture_label_buffer_, std::nullopt, static_cast<size_t>(capture_frame_count_), now);
				}
			}
		}

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.6f, 1.0f), "Run Library");

		ImGui::SetNextItemWidth(220.0f);
		ImGui::InputTextWithHint("##BenchmarkSearch", "Filter by label, metric...", benchmark_search_filter_, sizeof(benchmark_search_filter_));
		ImGui::SameLine();
		ImGui::Checkbox("Matching Signature Only", &show_only_matching_signature_);
		render_setting_tooltip("Hides benchmark runs captured under a different internal engine signature.");
		ImGui::SameLine();

		if (ImGui::Button("Columns...")) {
			ImGui::OpenPopup("##ColumnVisibilityPopup");
		}
		if (ImGui::BeginPopup("##ColumnVisibilityPopup")) {
			if (ImGui::SmallButton("All")) {
				std::fill(column_visible_.begin(), column_visible_.end(), 1);
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Default")) {
				for (size_t i = 0; i < all_columns.size(); ++i) column_visible_[i] = all_columns[i].default_visible ? 1 : 0;
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Minimal")) {
				std::fill(column_visible_.begin(), column_visible_.end(), 0);
				column_visible_[0] = column_visible_[1] = column_visible_[4] = column_visible_[9] = 1;
			}
			ImGui::Separator();

			std::optional<Orchestrator::BenchmarkColumnCategory> current_cat = std::nullopt;
			for (size_t i = 0; i < all_columns.size(); ++i) {
				if (!current_cat.has_value() || all_columns[i].category != *current_cat) {
					current_cat = all_columns[i].category;
					const char* cat_name = "Identity";
					if (*current_cat == Orchestrator::BenchmarkColumnCategory::Timing) cat_name = "Timing & Throughput";
					else if (*current_cat == Orchestrator::BenchmarkColumnCategory::Configuration) cat_name = "Engine Configuration";
					else if (*current_cat == Orchestrator::BenchmarkColumnCategory::Capture) cat_name = "Capture Metadata";
					ImGui::TextDisabled("%s", cat_name);
				}
				bool visible = column_visible_[i] != 0;
				if (ImGui::Checkbox(all_columns[i].header, &visible)) {
					column_visible_[i] = visible ? 1 : 0;
				}
			}
			ImGui::EndPopup();
		}

		const auto& runs = profiler.saved_runs();
		if (runs.empty()) {
			ImGui::TextDisabled("No benchmark runs saved yet.");
		}

		std::vector<size_t> visible_run_indices;
		visible_run_indices.reserve(runs.size());
		const std::string filter_str = lowercase(benchmark_search_filter_);
		for (size_t i = 0; i < runs.size(); ++i) {
			const auto& run = runs[i];
			if (show_only_matching_signature_ && run.engine_signature != profiler.engine_signature()) continue;
			if (!filter_str.empty()) {
				if (lowercase(run.label).find(filter_str) == std::string::npos &&
				    lowercase(run.config.metric_name).find(filter_str) == std::string::npos &&
				    lowercase(run.config.integrator_name).find(filter_str) == std::string::npos) {
					continue;
				}
			}
			visible_run_indices.push_back(i);
		}

		int active_columns_count = 3;
		for (size_t i = 0; i < all_columns.size(); ++i) {
			if (column_visible_[i]) ++active_columns_count;
		}

		ImGui::BeginChild("BenchmarkRunsTableRegion", ImVec2(0.0f, 320.0f), false, ImGuiWindowFlags_HorizontalScrollbar);
		if (ImGui::BeginTable("BenchmarkRunsTable", active_columns_count, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollX | ImGuiTableFlags_Sortable)) {
			ImGui::TableSetupColumn("WB", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 32.0f);
			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 150.0f, 0);

			int col_idx = 1;
			for (size_t i = 0; i < all_columns.size(); ++i) {
				if (!column_visible_[i]) continue;
				ImGui::TableSetupColumn(all_columns[i].header, ImGuiTableColumnFlags_WidthFixed, 0.0f, static_cast<ImGuiID>(col_idx++));
			}
			ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 280.0f);
			ImGui::TableHeadersRow();

			if (ImGuiTableSortSpecs* sorts_specs = ImGui::TableGetSortSpecs()) {
				if (sorts_specs->SpecsCount > 0 && sorts_specs->SpecsDirty) {
					const auto& spec = sorts_specs->Specs[0];
					const bool desc = spec.SortDirection == ImGuiSortDirection_Descending;
					const int target_col = spec.ColumnIndex;

					if (target_col == 1) {
						std::sort(visible_run_indices.begin(), visible_run_indices.end(), [&](size_t a, size_t b) {
							return desc ? runs[a].label > runs[b].label : runs[a].label < runs[b].label;
						});
					} else if (target_col > 1 && target_col < active_columns_count - 1) {
						int cur = 2;
						size_t matched_col_idx = 0;
						for (size_t i = 0; i < all_columns.size(); ++i) {
							if (!column_visible_[i]) continue;
							if (cur == target_col) {
								matched_col_idx = i;
								break;
							}
							++cur;
						}
						const auto& col_def = all_columns[matched_col_idx];
						if (col_def.numeric != nullptr) {
							std::sort(visible_run_indices.begin(), visible_run_indices.end(), [&](size_t a, size_t b) {
								const double va = col_def.numeric(runs[a]);
								const double vb = col_def.numeric(runs[b]);
								return desc ? va > vb : va < vb;
							});
						} else {
							std::sort(visible_run_indices.begin(), visible_run_indices.end(), [&](size_t a, size_t b) {
								const std::string ta = col_def.cell(runs[a]);
								const std::string tb = col_def.cell(runs[b]);
								return desc ? ta > tb : ta < tb;
							});
						}
					}
					sorts_specs->SpecsDirty = false;
				}
			}

			for (const size_t i : visible_run_indices) {
				const auto& run = runs[i];
				const std::string key = Orchestrator::benchmark_run_key(run);

				ImGui::TableNextRow();
				ImGui::PushID(static_cast<int>(i));

				int col = 0;
				ImGui::TableSetColumnIndex(col++);
				bool wb_selected = workbench_.is_selected(key);
				if (ImGui::Checkbox("##wb", &wb_selected)) {
					workbench_.set_selected(key, wb_selected);
				}

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

				for (size_t c = 0; c < all_columns.size(); ++c) {
					if (!column_visible_[c]) continue;
					ImGui::TableSetColumnIndex(col++);
					const auto& col_def = all_columns[c];
					const std::string cell_text = col_def.cell(run);
					ImGui::TextUnformatted(cell_text.c_str());
					if (col_def.copy_on_click && ImGui::IsItemHovered()) {
						ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
						if (ImGui::IsItemClicked()) {
							copy_to_clipboard(cell_text);
						}
						ImGui::BeginTooltip();
						ImGui::TextUnformatted("Click to copy value");
						ImGui::EndTooltip();
					} else if (col_def.tooltip != nullptr && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
						ImGui::BeginTooltip();
						ImGui::TextUnformatted(col_def.tooltip);
						ImGui::EndTooltip();
					}
				}

				ImGui::TableSetColumnIndex(col++);
				const bool is_a = selected_run_indices_[0] == static_cast<int>(i);
				const bool is_b = selected_run_indices_[1] == static_cast<int>(i);

				if (is_a) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.3f, 1.0f));
				if (ImGui::SmallButton("A")) selected_run_indices_[0] = static_cast<int>(i);
				if (is_a) ImGui::PopStyleColor();
				ImGui::SameLine();

				if (is_b) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.7f, 1.0f));
				if (ImGui::SmallButton("B")) selected_run_indices_[1] = static_cast<int>(i);
				if (is_b) ImGui::PopStyleColor();
				ImGui::SameLine();

				if (ImGui::SmallButton("Apply")) apply_run_configuration(run);
				ImGui::SameLine();

				if (ImGui::SmallButton("Rename")) {
					rename_index_ = static_cast<int>(i);
					std::strncpy(rename_buffer_, run.label.c_str(), sizeof(rename_buffer_) - 1);
					rename_buffer_[sizeof(rename_buffer_) - 1] = '\0';
					rename_requested_ = true;
				}
				ImGui::SameLine();

				if (ImGui::SmallButton("Delete")) {
					delete_index_ = static_cast<int>(i);
					delete_requested_ = true;
				}

				ImGui::PopID();
			}
			ImGui::EndTable();
		}
		ImGui::EndChild();

		if (rename_requested_) {
			ImGui::OpenPopup("Rename Benchmark Run##Modal");
			rename_requested_ = false;
		}
		if (ImGui::BeginPopupModal("Rename Benchmark Run##Modal", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("Enter new label for this benchmark run:");
			ImGui::InputText("##NewRunLabel", rename_buffer_, sizeof(rename_buffer_));
			ImGui::Spacing();
			if (ImGui::Button("Save", ImVec2(100.0f, 24.0f))) {
				if (rename_index_ >= 0 && rename_index_ < static_cast<int>(profiler.saved_runs().size())) {
					profiler.rename_run(static_cast<size_t>(rename_index_), rename_buffer_);
					profiler.save_to_disk();
				}
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(100.0f, 24.0f))) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		if (delete_requested_) {
			ImGui::OpenPopup("Delete Benchmark Run##Modal");
			delete_requested_ = false;
		}
		if (ImGui::BeginPopupModal("Delete Benchmark Run##Modal", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			if (delete_index_ >= 0 && delete_index_ < static_cast<int>(profiler.saved_runs().size())) {
				ImGui::Text("Are you sure you want to delete '%s'?", profiler.saved_runs()[static_cast<size_t>(delete_index_)].label.c_str());
			}
			ImGui::Spacing();
			if (ImGui::Button("Delete", ImVec2(100.0f, 24.0f))) {
				if (delete_index_ >= 0 && delete_index_ < static_cast<int>(profiler.saved_runs().size())) {
					profiler.remove_run(static_cast<size_t>(delete_index_));
					profiler.save_to_disk();
				}
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(100.0f, 24.0f))) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		if (ImGui::Button("Clear All Saved Runs", ImVec2(160.0f, 24.0f))) {
			profiler.clear_all_runs();
			profiler.save_to_disk();
		}

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.6f, 1.0f), "A / B Comparison");
		render_run_comparison(profiler);
	}

	void render_settings_tab(Orchestrator::PerformanceProfiler& profiler) {
		ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.6f, 1.0f), "Live History Buffer");
		if (ImGui::SliderInt("History Capacity (frames)", &history_capacity_input_, 120, 20000)) {
			profiler.set_history_capacity(static_cast<size_t>(history_capacity_input_));
		}
		render_setting_tooltip("Number of recent frame samples retained in memory for the Live Monitor and Statistics tabs. Does not affect saved benchmark runs.");

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

		if (ImGui::Button("Clear Live History", ImVec2(160.0f, 24.0f))) {
			profiler.clear_history();
		}

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.6f, 1.0f), "Persistence");
		bool persistence_enabled = profiler.persistence_enabled();
		if (ImGui::Checkbox("Persist Benchmark Runs Across Sessions", &persistence_enabled)) {
			profiler.set_persistence_enabled(persistence_enabled);
		}
		render_setting_tooltip("When enabled, saved benchmark runs are written to config/performance_profiler.cfg and automatically reloaded on the next session.");

		if (ImGui::Button("Save Now", ImVec2(120.0f, 24.0f))) {
			profiler.save_to_disk();
		}
		ImGui::SameLine();
		if (ImGui::Button("Reload From Disk", ImVec2(140.0f, 24.0f))) {
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
