#pragma once

#include <imgui.h>
#include <implot.h>
#include "relativistic/orchestrator/benchmark_metrics.hpp"
#include "relativistic/orchestrator/performance_profiler.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <limits>
#include <numeric>
#include <string>
#include <string_view>
#include <vector>

namespace Relativistic::UI {

enum class WorkbenchChartType : int {
	GroupedBars = 0,
	Lines = 1,
	Scatter = 2,
	StackedStages = 3
};

enum class WorkbenchTransform : int {
	Raw = 0,
	Log10 = 1,
	NaturalLog = 2,
	Log2 = 3,
	RelativeToBaseline = 4,
	PercentDeltaFromBaseline = 5,
	MinMaxNormalized = 6,
	ZScore = 7
};

enum class WorkbenchAxisScale : int {
	Linear = 0,
	Logarithmic = 1
};

enum class WorkbenchRunOrder : int {
	LibraryOrder = 0,
	XValue = 1,
	FirstSeriesValue = 2,
	Label = 3
};

class BenchmarkWorkbench {
private:
	using Stage = Orchestrator::ProfilerTaskStage;
	static constexpr size_t kMaxSeries = 6;
	static constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();

	struct ResolvedRun {
		const Orchestrator::BenchmarkRun* run;
		size_t library_index;
	};

	struct SeriesData {
		size_t metric{0};
		std::string name{};
		std::vector<double> raw{};
		std::vector<double> plotted{};
		double baseline{0.0};
		double minimum{0.0};
		double maximum{0.0};
		double mean{0.0};
		double deviation{0.0};
		size_t best_index{0};
	};

	struct Dataset {
		std::vector<ResolvedRun> runs{};
		std::vector<std::string> labels{};
		std::vector<double> x_raw{};
		std::vector<double> x_plotted{};
		std::vector<SeriesData> series{};
	};

	struct StackedData {
		std::vector<Stage> stages{};
		std::vector<std::string> names{};
		std::vector<std::vector<double>> values{};
		std::vector<double> totals{};
	};

	std::vector<std::string> selected_keys_{};
	WorkbenchChartType chart_type_{WorkbenchChartType::GroupedBars};
	size_t x_metric_{0};
	std::vector<size_t> y_metrics_{};
	WorkbenchTransform transform_{WorkbenchTransform::Raw};
	WorkbenchAxisScale x_scale_{WorkbenchAxisScale::Linear};
	WorkbenchAxisScale y_scale_{WorkbenchAxisScale::Linear};
	WorkbenchRunOrder run_order_{WorkbenchRunOrder::LibraryOrder};
	bool descending_{false};
	std::string baseline_key_{};
	bool show_labels_{false};
	bool show_data_table_{true};
	bool stacked_percent_{false};
	float plot_height_{380.0f};
	Stage stacked_root_{Stage::RenderDispatch};
	char run_filter_[64]{};
	char metric_filter_[64]{};

	[[nodiscard]] static std::string lowercase(std::string_view text) {
		std::string result(text);
		std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return result;
	}

	[[nodiscard]] static std::string truncate_label(const std::string& label, size_t limit) {
		return (label.size() > limit) ? label.substr(0, limit - 2) + ".." : label;
	}

	[[nodiscard]] static std::vector<Stage> stage_children(Stage parent) {
		std::vector<Stage> result;
		for (size_t i = 0; i < static_cast<size_t>(Stage::Count); ++i) {
			const auto candidate = static_cast<Stage>(i);
			if (candidate != parent && Orchestrator::profiler_stage_parent(candidate) == parent) {
				result.push_back(candidate);
			}
		}
		return result;
	}

	[[nodiscard]] double metric_value(size_t metric, const ResolvedRun& resolved) const noexcept {
		return Orchestrator::benchmark_metric_value(Orchestrator::benchmark_metrics()[metric], *resolved.run, resolved.library_index);
	}

	[[nodiscard]] std::string format_value(size_t metric, double value) const {
		return Orchestrator::benchmark_metric_display(Orchestrator::benchmark_metrics()[metric], value);
	}

	[[nodiscard]] double transform_value(double value, const SeriesData& series) const noexcept {
		switch (transform_) {
			case WorkbenchTransform::Log10: return (value > 0.0) ? std::log10(value) : kNaN;
			case WorkbenchTransform::NaturalLog: return (value > 0.0) ? std::log(value) : kNaN;
			case WorkbenchTransform::Log2: return (value > 0.0) ? std::log2(value) : kNaN;
			case WorkbenchTransform::RelativeToBaseline: return (series.baseline != 0.0) ? value / series.baseline : kNaN;
			case WorkbenchTransform::PercentDeltaFromBaseline: return (series.baseline != 0.0) ? (value / series.baseline - 1.0) * 100.0 : kNaN;
			case WorkbenchTransform::MinMaxNormalized: {
				const double range = series.maximum - series.minimum;
				return (range > 1e-12) ? (value - series.minimum) / range : 0.0;
			}
			case WorkbenchTransform::ZScore: return (series.deviation > 1e-12) ? (value - series.mean) / series.deviation : 0.0;
			case WorkbenchTransform::Raw:
			default: return value;
		}
	}

	[[nodiscard]] std::string y_axis_label(const Dataset& data) const {
		switch (transform_) {
			case WorkbenchTransform::Log10: return "log10(value)";
			case WorkbenchTransform::NaturalLog: return "ln(value)";
			case WorkbenchTransform::Log2: return "log2(value)";
			case WorkbenchTransform::RelativeToBaseline: return "Ratio to baseline (x)";
			case WorkbenchTransform::PercentDeltaFromBaseline: return "Change from baseline (%)";
			case WorkbenchTransform::MinMaxNormalized: return "Min-max normalized (0 to 1)";
			case WorkbenchTransform::ZScore: return "Z-score (standard deviations)";
			case WorkbenchTransform::Raw:
			default: {
				if (data.series.empty()) return "Value";
				const auto& metrics = Orchestrator::benchmark_metrics();
				const auto unit = metrics[data.series.front().metric].unit;
				for (const auto& series : data.series) {
					if (metrics[series.metric].unit != unit) return "Value";
				}
				const char* text = Orchestrator::benchmark_metric_unit_text(unit);
				return (text[0] != '\0') ? std::string("Value (") + text + ")" : std::string("Value");
			}
		}
	}

	bool metric_selector(const char* label, size_t& metric_index) {
		const auto& metrics = Orchestrator::benchmark_metrics();
		bool changed = false;
		const std::string preview = Orchestrator::benchmark_metric_label(metrics[metric_index]);
		if (ImGui::BeginCombo(label, preview.c_str())) {
			ImGui::SetNextItemWidth(-1.0f);
			ImGui::InputTextWithHint("##MetricFilter", "Filter metrics...", metric_filter_, sizeof(metric_filter_));
			const std::string needle = lowercase(metric_filter_);
			const char* current_group = nullptr;
			for (size_t i = 0; i < metrics.size(); ++i) {
				const std::string entry = Orchestrator::benchmark_metric_label(metrics[i]);
				if (!needle.empty() && lowercase(entry).find(needle) == std::string::npos && lowercase(metrics[i].group).find(needle) == std::string::npos) continue;
				if (current_group == nullptr || std::string_view(current_group) != metrics[i].group) {
					current_group = metrics[i].group;
					ImGui::TextDisabled("%s", current_group);
				}
				ImGui::PushID(static_cast<int>(i));
				if (ImGui::Selectable(entry.c_str(), i == metric_index)) {
					metric_index = i;
					changed = true;
				}
				ImGui::PopID();
			}
			ImGui::EndCombo();
		}
		return changed;
	}

	void apply_preset(int preset) {
		const auto index = [](const char* label) { return Orchestrator::benchmark_metric_index(label); };
		y_metrics_.clear();
		transform_ = WorkbenchTransform::Raw;
		x_scale_ = WorkbenchAxisScale::Linear;
		y_scale_ = WorkbenchAxisScale::Linear;
		run_order_ = WorkbenchRunOrder::LibraryOrder;
		descending_ = false;
		show_labels_ = false;
		stacked_percent_ = false;
		x_metric_ = index("Run Order");
		switch (preset) {
			case 0:
				chart_type_ = WorkbenchChartType::GroupedBars;
				y_metrics_ = {index("Mean Frame Time"), index("P95 Frame Time"), index("P99 Frame Time")};
				break;
			case 1:
				chart_type_ = WorkbenchChartType::GroupedBars;
				y_metrics_ = {index("Mean FPS"), index("1% Low FPS"), index("Min FPS")};
				break;
			case 2:
				chart_type_ = WorkbenchChartType::Scatter;
				x_metric_ = index("Resolution Scale");
				y_metrics_ = {index("Mean Frame Time")};
				y_scale_ = WorkbenchAxisScale::Logarithmic;
				show_labels_ = true;
				break;
			case 3:
				chart_type_ = WorkbenchChartType::Scatter;
				x_metric_ = index("Max Ray Steps");
				y_metrics_ = {index("Mean Frame Time"), index("Average Ray Iterations")};
				transform_ = WorkbenchTransform::MinMaxNormalized;
				x_scale_ = WorkbenchAxisScale::Logarithmic;
				break;
			case 4:
				chart_type_ = WorkbenchChartType::StackedStages;
				stacked_root_ = Stage::RenderDispatch;
				y_metrics_ = {index("Mean Frame Time")};
				break;
			case 5:
				chart_type_ = WorkbenchChartType::GroupedBars;
				y_metrics_ = {index("Mean Frame Time"), index("P99 Frame Time")};
				transform_ = WorkbenchTransform::PercentDeltaFromBaseline;
				break;
			default:
				chart_type_ = WorkbenchChartType::Lines;
				y_metrics_ = {index("Mean Frame Time"), index("P95 Frame Time"), index("P99 Frame Time")};
				break;
		}
	}

	[[nodiscard]] std::vector<ResolvedRun> resolve_selection(const std::vector<Orchestrator::BenchmarkRun>& runs) {
		std::vector<std::string> library_keys;
		library_keys.reserve(runs.size());
		std::vector<ResolvedRun> resolved;
		for (size_t i = 0; i < runs.size(); ++i) {
			library_keys.push_back(Orchestrator::benchmark_run_key(runs[i]));
			if (is_selected(library_keys.back())) {
				resolved.push_back(ResolvedRun{&runs[i], i});
			}
		}
		std::erase_if(selected_keys_, [&](const std::string& key) {
			return std::find(library_keys.begin(), library_keys.end(), key) == library_keys.end();
		});
		return resolved;
	}

	void render_presets() {
		static constexpr const char* kPresetNames[] = {"Frame Time Profile", "FPS Overview", "Resolution Scaling", "Step Budget Scaling", "Stage Breakdown", "Delta From Baseline", "Trend Over Runs"};
		ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.6f, 1.0f), "Presets");
		for (int i = 0; i < static_cast<int>(sizeof(kPresetNames) / sizeof(kPresetNames[0])); ++i) {
			if (i > 0) ImGui::SameLine();
			if (ImGui::SmallButton(kPresetNames[i])) {
				apply_preset(i);
			}
		}
		render_setting_tooltip("Each preset configures the chart type, axes, transform and ordering in one click. Every setting stays editable afterwards.");
	}

	void render_selection_panel(const std::vector<Orchestrator::BenchmarkRun>& runs) {
		const std::string header = "Runs In Workbench (" + std::to_string(selected_keys_.size()) + " / " + std::to_string(runs.size()) + ")###WorkbenchRuns";
		if (!ImGui::CollapsingHeader(header.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) return;

		ImGui::SetNextItemWidth(220.0f);
		ImGui::InputTextWithHint("##WorkbenchRunFilter", "Filter runs...", run_filter_, sizeof(run_filter_));
		const std::string needle = lowercase(run_filter_);
		const auto matches = [&](const Orchestrator::BenchmarkRun& run) {
			if (needle.empty()) return true;
			return lowercase(run.label).find(needle) != std::string::npos
				|| lowercase(run.config.metric_name).find(needle) != std::string::npos
				|| lowercase(run.config.integrator_name).find(needle) != std::string::npos;
		};
		ImGui::SameLine();
		if (ImGui::SmallButton("Select Matching")) {
			for (const auto& run : runs) {
				if (matches(run)) set_selected(Orchestrator::benchmark_run_key(run), true);
			}
		}
		ImGui::SameLine();
		if (ImGui::SmallButton("Select All")) {
			for (const auto& run : runs) set_selected(Orchestrator::benchmark_run_key(run), true);
		}
		ImGui::SameLine();
		if (ImGui::SmallButton("Latest 5")) {
			clear_selection();
			for (size_t i = (runs.size() > 5) ? runs.size() - 5 : 0; i < runs.size(); ++i) {
				set_selected(Orchestrator::benchmark_run_key(runs[i]), true);
			}
		}
		ImGui::SameLine();
		if (ImGui::SmallButton("Invert")) {
			for (const auto& run : runs) {
				const std::string key = Orchestrator::benchmark_run_key(run);
				set_selected(key, !is_selected(key));
			}
		}
		ImGui::SameLine();
		if (ImGui::SmallButton("Clear")) {
			clear_selection();
		}

		ImGui::BeginChild("##WorkbenchRunList", ImVec2(0.0f, 150.0f), true);
		if (runs.empty()) {
			ImGui::TextDisabled("No benchmark runs saved yet. Capture runs from the Benchmark Runs tab.");
		}
		for (size_t i = 0; i < runs.size(); ++i) {
			if (!matches(runs[i])) continue;
			const std::string key = Orchestrator::benchmark_run_key(runs[i]);
			bool checked = is_selected(key);
			const std::string label = runs[i].label + "  [" + runs[i].timestamp + "]";
			ImGui::PushID(static_cast<int>(i));
			if (ImGui::Checkbox(label.c_str(), &checked)) {
				set_selected(key, checked);
			}
			ImGui::PopID();
		}
		ImGui::EndChild();
	}

	void render_axis_controls(const std::vector<ResolvedRun>& resolved) {
		static constexpr const char* kChartNames[] = {"Grouped Bars", "Lines (sorted by X)", "Scatter", "Stacked Stage Breakdown"};
		int chart = static_cast<int>(chart_type_);
		ImGui::SetNextItemWidth(220.0f);
		if (ImGui::Combo("Chart Type", &chart, kChartNames, IM_ARRAYSIZE(kChartNames))) {
			chart_type_ = static_cast<WorkbenchChartType>(chart);
		}

		if (chart_type_ == WorkbenchChartType::Lines || chart_type_ == WorkbenchChartType::Scatter) {
			ImGui::SetNextItemWidth(260.0f);
			metric_selector("X Axis Metric", x_metric_);
			int x_scale = static_cast<int>(x_scale_);
			ImGui::SameLine();
			ImGui::SetNextItemWidth(110.0f);
			static constexpr const char* kScaleNames[] = {"Linear", "Log10"};
			if (ImGui::Combo("X Scale", &x_scale, kScaleNames, IM_ARRAYSIZE(kScaleNames))) {
				x_scale_ = static_cast<WorkbenchAxisScale>(x_scale);
			}
		}

		if (chart_type_ == WorkbenchChartType::StackedStages) {
			std::vector<Stage> candidates;
			for (size_t i = 0; i < static_cast<size_t>(Stage::Count); ++i) {
				const auto stage = static_cast<Stage>(i);
				if (!stage_children(stage).empty()) candidates.push_back(stage);
			}
			ImGui::SetNextItemWidth(260.0f);
			if (ImGui::BeginCombo("Stacked Stage", Orchestrator::profiler_stage_name(stacked_root_))) {
				for (const Stage stage : candidates) {
					if (ImGui::Selectable(Orchestrator::profiler_stage_name(stage), stage == stacked_root_)) {
						stacked_root_ = stage;
					}
				}
				ImGui::EndCombo();
			}
			ImGui::SameLine();
			ImGui::Checkbox("Normalize To 100%", &stacked_percent_);
			render_setting_tooltip("Stacks the direct child stages of the selected stage for every run, plus an untracked remainder, using the per-stage mean times stored with each run.");
		} else {
			ImGui::TextDisabled("Y Series");
			for (size_t s = 0; s < y_metrics_.size(); ++s) {
				ImGui::PushID(static_cast<int>(s));
				ImGui::SetNextItemWidth(300.0f);
				metric_selector("##SeriesMetric", y_metrics_[s]);
				ImGui::SameLine();
				if (y_metrics_.size() > 1 && ImGui::SmallButton("Remove")) {
					y_metrics_.erase(y_metrics_.begin() + static_cast<ptrdiff_t>(s));
					ImGui::PopID();
					break;
				}
				ImGui::PopID();
			}
			if (y_metrics_.size() < kMaxSeries && ImGui::SmallButton("Add Series")) {
				y_metrics_.push_back(Orchestrator::benchmark_metric_index("Mean Frame Time"));
			}

			static constexpr const char* kTransformNames[] = {
				"Raw Values", "Log10", "Natural Log", "Log2", "Ratio To Baseline", "Percent Change From Baseline", "Min-Max Normalized", "Z-Score"
			};
			int transform = static_cast<int>(transform_);
			ImGui::SetNextItemWidth(260.0f);
			if (ImGui::Combo("Value Normalization", &transform, kTransformNames, IM_ARRAYSIZE(kTransformNames))) {
				transform_ = static_cast<WorkbenchTransform>(transform);
			}
			render_setting_tooltip("Transforms every series before plotting. Log options compress wide ranges, baseline options express values relative to a chosen run, and min-max or z-score options let series with different units share one axis.");

			if (transform_ == WorkbenchTransform::RelativeToBaseline || transform_ == WorkbenchTransform::PercentDeltaFromBaseline) {
				std::string preview = "First selected run";
				for (const auto& entry : resolved) {
					if (Orchestrator::benchmark_run_key(*entry.run) == baseline_key_) preview = entry.run->label;
				}
				ImGui::SetNextItemWidth(260.0f);
				if (ImGui::BeginCombo("Baseline Run", preview.c_str())) {
					for (const auto& entry : resolved) {
						const std::string key = Orchestrator::benchmark_run_key(*entry.run);
						if (ImGui::Selectable((entry.run->label + "##baseline" + key).c_str(), key == baseline_key_)) {
							baseline_key_ = key;
						}
					}
					ImGui::EndCombo();
				}
			}

			int y_scale = static_cast<int>(y_scale_);
			static constexpr const char* kScaleNames[] = {"Linear", "Log10"};
			ImGui::SetNextItemWidth(110.0f);
			if (ImGui::Combo("Y Scale", &y_scale, kScaleNames, IM_ARRAYSIZE(kScaleNames))) {
				y_scale_ = static_cast<WorkbenchAxisScale>(y_scale);
			}
		}

		static constexpr const char* kOrderNames[] = {"Library Order", "X Value", "First Series Value", "Label"};
		if (chart_type_ == WorkbenchChartType::GroupedBars || chart_type_ == WorkbenchChartType::StackedStages) {
			int order = static_cast<int>(run_order_);
			ImGui::SameLine();
			ImGui::SetNextItemWidth(160.0f);
			if (ImGui::Combo("Order Runs By", &order, kOrderNames, IM_ARRAYSIZE(kOrderNames))) {
				run_order_ = static_cast<WorkbenchRunOrder>(order);
			}
			ImGui::SameLine();
			ImGui::Checkbox("Descending", &descending_);
		}
		ImGui::Checkbox("Run Labels On Points", &show_labels_);
		ImGui::SameLine();
		ImGui::Checkbox("Data Table", &show_data_table_);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(160.0f);
		ImGui::SliderFloat("Plot Height", &plot_height_, 220.0f, 800.0f, "%.0f px");
	}

	[[nodiscard]] Dataset build_dataset(std::vector<ResolvedRun> runs) const {
		const auto& metrics = Orchestrator::benchmark_metrics();
		Dataset data;
		const auto x_of = [&](const ResolvedRun& r) { return metric_value(x_metric_, r); };
		const auto y_of = [&](const ResolvedRun& r) { return y_metrics_.empty() ? 0.0 : metric_value(y_metrics_.front(), r); };

		if (chart_type_ == WorkbenchChartType::Lines || run_order_ == WorkbenchRunOrder::XValue) {
			std::stable_sort(runs.begin(), runs.end(), [&](const ResolvedRun& a, const ResolvedRun& b) { return x_of(a) < x_of(b); });
		} else if (run_order_ == WorkbenchRunOrder::FirstSeriesValue) {
			std::stable_sort(runs.begin(), runs.end(), [&](const ResolvedRun& a, const ResolvedRun& b) { return y_of(a) < y_of(b); });
		} else if (run_order_ == WorkbenchRunOrder::Label) {
			std::stable_sort(runs.begin(), runs.end(), [](const ResolvedRun& a, const ResolvedRun& b) { return a.run->label < b.run->label; });
		}
		if (descending_ && chart_type_ != WorkbenchChartType::Lines) {
			std::reverse(runs.begin(), runs.end());
		}

		data.runs = std::move(runs);
		size_t baseline_position = 0;
		for (size_t i = 0; i < data.runs.size(); ++i) {
			data.labels.push_back(data.runs[i].run->label);
			data.x_raw.push_back(x_of(data.runs[i]));
			if (Orchestrator::benchmark_run_key(*data.runs[i].run) == baseline_key_) baseline_position = i;
		}
		data.x_plotted = data.x_raw;
		if (x_scale_ == WorkbenchAxisScale::Logarithmic) {
			for (double& value : data.x_plotted) value = std::max(value, 1e-12);
		}

		for (const size_t metric : y_metrics_) {
			SeriesData series;
			series.metric = metric;
			series.name = Orchestrator::benchmark_metric_label(metrics[metric]);
			for (const auto& entry : data.runs) series.raw.push_back(metric_value(metric, entry));
			series.baseline = series.raw.empty() ? 0.0 : series.raw[std::min(baseline_position, series.raw.size() - 1)];
			size_t finite_count = 0;
			series.minimum = std::numeric_limits<double>::max();
			series.maximum = std::numeric_limits<double>::lowest();
			for (const double value : series.raw) {
				if (!std::isfinite(value)) continue;
				series.minimum = std::min(series.minimum, value);
				series.maximum = std::max(series.maximum, value);
				series.mean += value;
				++finite_count;
			}
			if (finite_count == 0) {
				series.minimum = series.maximum = series.mean = 0.0;
			} else {
				series.mean /= static_cast<double>(finite_count);
				double variance = 0.0;
				for (const double value : series.raw) {
					if (std::isfinite(value)) variance += (value - series.mean) * (value - series.mean);
				}
				series.deviation = std::sqrt(variance / static_cast<double>(finite_count));
			}
			const auto direction = metrics[metric].direction;
			for (size_t i = 0; i < series.raw.size(); ++i) {
				if (direction == Orchestrator::BenchmarkMetricDirection::HigherIsBetter ? series.raw[i] > series.raw[series.best_index] : series.raw[i] < series.raw[series.best_index]) {
					series.best_index = i;
				}
			}
			for (const double value : series.raw) {
				double plotted = transform_value(value, series);
				if (y_scale_ == WorkbenchAxisScale::Logarithmic && std::isfinite(plotted)) plotted = std::max(plotted, 1e-12);
				series.plotted.push_back(plotted);
			}
			data.series.push_back(std::move(series));
		}
		return data;
	}

	[[nodiscard]] StackedData build_stacked(const std::vector<ResolvedRun>& runs_in) const {
		StackedData stacked;
		std::vector<ResolvedRun> runs = runs_in;
		if (run_order_ == WorkbenchRunOrder::Label) {
			std::stable_sort(runs.begin(), runs.end(), [](const ResolvedRun& a, const ResolvedRun& b) { return a.run->label < b.run->label; });
		}
		if (descending_) std::reverse(runs.begin(), runs.end());

		for (const Stage child : stage_children(stacked_root_)) {
			const bool used = std::any_of(runs.begin(), runs.end(), [&](const ResolvedRun& r) { return r.run->stage_mean_ms[static_cast<size_t>(child)] > 0.0; });
			if (used) {
				stacked.stages.push_back(child);
				stacked.names.emplace_back(Orchestrator::profiler_stage_name(child));
			}
		}
		stacked.names.emplace_back("Untracked");
		stacked.values.assign(stacked.names.size(), std::vector<double>(runs.size(), 0.0));
		stacked.totals.assign(runs.size(), 0.0);
		for (size_t r = 0; r < runs.size(); ++r) {
			const auto& means = runs[r].run->stage_mean_ms;
			double children_sum = 0.0;
			for (size_t s = 0; s < stacked.stages.size(); ++s) {
				const double value = means[static_cast<size_t>(stacked.stages[s])];
				stacked.values[s][r] = value;
				children_sum += value;
			}
			const double root_value = means[static_cast<size_t>(stacked_root_)];
			stacked.values.back()[r] = std::max(root_value - children_sum, 0.0);
			stacked.totals[r] = children_sum + stacked.values.back()[r];
			if (stacked_percent_ && stacked.totals[r] > 1e-12) {
				for (auto& row : stacked.values) row[r] = row[r] / stacked.totals[r] * 100.0;
			}
		}
		runs_ordered_labels_.clear();
		for (const auto& entry : runs) runs_ordered_labels_.push_back(entry.run->label);
		return stacked;
	}

	mutable std::vector<std::string> runs_ordered_labels_{};

	void setup_category_ticks(const std::vector<std::string>& labels, std::vector<double>& positions, std::vector<std::string>& storage, std::vector<const char*>& pointers) const {
		positions.resize(labels.size());
		std::iota(positions.begin(), positions.end(), 0.0);
		storage.clear();
		pointers.clear();
		for (const auto& label : labels) storage.push_back(truncate_label(label, 18));
		for (const auto& label : storage) pointers.push_back(label.c_str());
		if (labels.size() <= 24 && !labels.empty()) {
			ImPlot::SetupAxisTicks(ImAxis_X1, positions.data(), static_cast<int>(positions.size()), pointers.data());
		}
	}

	void render_categorical_tooltip(const std::vector<std::string>& labels, const std::vector<std::pair<std::string, std::string>>& entries_unused) const {
		static_cast<void>(entries_unused);
		static_cast<void>(labels);
	}

	void render_plot(const Dataset& data) const {
		if (data.series.empty()) {
			ImGui::TextDisabled("Add at least one Y series to draw the chart.");
			return;
		}
		const size_t count = data.runs.size();
		const std::string y_label = y_axis_label(data);
		const bool categorical = chart_type_ == WorkbenchChartType::GroupedBars;
		const std::string x_label = categorical ? std::string("Run") : Orchestrator::benchmark_metric_label(Orchestrator::benchmark_metrics()[x_metric_]);

		if (!ImPlot::BeginPlot("##BenchmarkWorkbenchPlot", ImVec2(-1.0f, plot_height_))) return;
		ImPlot::SetupAxes(x_label.c_str(), y_label.c_str(), ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
		if (y_scale_ == WorkbenchAxisScale::Logarithmic) ImPlot::SetupAxisScale(ImAxis_Y1, ImPlotScale_Log10);
		if (!categorical && x_scale_ == WorkbenchAxisScale::Logarithmic) ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Log10);

		std::vector<double> positions;
		std::vector<std::string> tick_storage;
		std::vector<const char*> tick_pointers;

		if (categorical) {
			setup_category_ticks(data.labels, positions, tick_storage, tick_pointers);
			const double group_width = 0.8;
			const double bar_width = group_width / static_cast<double>(data.series.size());
			for (size_t s = 0; s < data.series.size(); ++s) {
				std::vector<double> xs(count);
				std::vector<double> ys(count);
				for (size_t i = 0; i < count; ++i) {
					xs[i] = static_cast<double>(i) - group_width * 0.5 + bar_width * (static_cast<double>(s) + 0.5);
					ys[i] = std::isfinite(data.series[s].plotted[i]) ? data.series[s].plotted[i] : 0.0;
				}
				ImPlot::PlotBars(data.series[s].name.c_str(), xs.data(), ys.data(), static_cast<int>(count), bar_width);
			}
			if (ImPlot::IsPlotHovered()) {
				const ImPlotPoint mouse = ImPlot::GetPlotMousePos();
				const long index = std::lround(mouse.x);
				if (index >= 0 && index < static_cast<long>(count)) {
					ImGui::BeginTooltip();
					ImGui::TextUnformatted(data.labels[static_cast<size_t>(index)].c_str());
					for (const auto& series : data.series) {
						const std::string raw = format_value(series.metric, series.raw[static_cast<size_t>(index)]);
						if (transform_ == WorkbenchTransform::Raw) {
							ImGui::Text("%s: %s", series.name.c_str(), raw.c_str());
						} else {
							ImGui::Text("%s: %s  (plotted %.4g)", series.name.c_str(), raw.c_str(), series.plotted[static_cast<size_t>(index)]);
						}
					}
					ImGui::EndTooltip();
				}
			}
		} else {
			for (const auto& series : data.series) {
				if (chart_type_ == WorkbenchChartType::Lines) {
					ImPlot::PlotLine(series.name.c_str(), data.x_plotted.data(), series.plotted.data(), static_cast<int>(count));
				}
				ImPlot::PlotScatter(series.name.c_str(), data.x_plotted.data(), series.plotted.data(), static_cast<int>(count));
				if (show_labels_) {
					for (size_t i = 0; i < count; ++i) {
						if (std::isfinite(series.plotted[i])) {
							ImPlot::PlotText(truncate_label(data.labels[i], 18).c_str(), data.x_plotted[i], series.plotted[i], ImVec2(0.0f, -10.0f));
						}
					}
				}
			}
			if (ImPlot::IsPlotHovered()) {
				const ImVec2 mouse = ImGui::GetMousePos();
				float best_distance = 14.0f;
				size_t best_series = 0;
				size_t best_run = 0;
				bool found = false;
				for (size_t s = 0; s < data.series.size(); ++s) {
					for (size_t i = 0; i < count; ++i) {
						if (!std::isfinite(data.series[s].plotted[i])) continue;
						const ImVec2 pixel = ImPlot::PlotToPixels(data.x_plotted[i], data.series[s].plotted[i]);
						const float distance = std::hypot(pixel.x - mouse.x, pixel.y - mouse.y);
						if (distance < best_distance) {
							best_distance = distance;
							best_series = s;
							best_run = i;
							found = true;
						}
					}
				}
				if (found) {
					const auto& series = data.series[best_series];
					ImGui::BeginTooltip();
					ImGui::TextUnformatted(data.labels[best_run].c_str());
					ImGui::Text("X: %s", format_value(x_metric_, data.x_raw[best_run]).c_str());
					ImGui::Text("%s: %s", series.name.c_str(), format_value(series.metric, series.raw[best_run]).c_str());
					if (transform_ != WorkbenchTransform::Raw) ImGui::Text("Plotted: %.4g", series.plotted[best_run]);
					ImGui::EndTooltip();
				}
			}
		}
		ImPlot::EndPlot();
	}

	void render_stacked_plot(const StackedData& stacked) const {
		const size_t count = runs_ordered_labels_.size();
		if (count == 0 || stacked.names.empty()) {
			ImGui::TextDisabled("The selected stage has no recorded child stages for the chosen runs.");
			return;
		}
		if (!ImPlot::BeginPlot("##BenchmarkWorkbenchStackedPlot", ImVec2(-1.0f, plot_height_))) return;
		ImPlot::SetupAxes("Run", stacked_percent_ ? "Share of stage (%)" : "Mean time per frame (ms)", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
		std::vector<double> positions;
		std::vector<std::string> tick_storage;
		std::vector<const char*> tick_pointers;
		setup_category_ticks(runs_ordered_labels_, positions, tick_storage, tick_pointers);

		std::vector<std::vector<double>> cumulative(stacked.values.size(), std::vector<double>(count, 0.0));
		for (size_t r = 0; r < count; ++r) {
			double running = 0.0;
			for (size_t s = 0; s < stacked.values.size(); ++s) {
				running += stacked.values[s][r];
				cumulative[s][r] = running;
			}
		}
		for (size_t s = stacked.values.size(); s-- > 0;) {
			ImPlot::PlotBars(stacked.names[s].c_str(), positions.data(), cumulative[s].data(), static_cast<int>(count), 0.7);
		}
		if (ImPlot::IsPlotHovered()) {
			const ImPlotPoint mouse = ImPlot::GetPlotMousePos();
			const long index = std::lround(mouse.x);
			if (index >= 0 && index < static_cast<long>(count)) {
				ImGui::BeginTooltip();
				ImGui::TextUnformatted(runs_ordered_labels_[static_cast<size_t>(index)].c_str());
				for (size_t s = 0; s < stacked.values.size(); ++s) {
					ImGui::Text("%s: %.3f%s", stacked.names[s].c_str(), stacked.values[s][static_cast<size_t>(index)], stacked_percent_ ? " %" : " ms");
				}
				ImGui::EndTooltip();
			}
		}
		ImPlot::EndPlot();
	}

	void render_series_summary(const Dataset& data) const {
		if (data.series.empty() || data.runs.empty()) return;
		ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.3f, 1.0f), "Series Statistics");
		if (ImGui::BeginTable("##WorkbenchSeriesSummary", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX)) {
			ImGui::TableSetupColumn("Series");
			ImGui::TableSetupColumn("Minimum");
			ImGui::TableSetupColumn("Maximum");
			ImGui::TableSetupColumn("Mean");
			ImGui::TableSetupColumn("Std Dev");
			ImGui::TableSetupColumn("Max / Min");
			ImGui::TableSetupColumn("Best Run");
			ImGui::TableHeadersRow();
			for (const auto& series : data.series) {
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted(series.name.c_str());
				ImGui::TableSetColumnIndex(1);
				ImGui::TextUnformatted(format_value(series.metric, series.minimum).c_str());
				ImGui::TableSetColumnIndex(2);
				ImGui::TextUnformatted(format_value(series.metric, series.maximum).c_str());
				ImGui::TableSetColumnIndex(3);
				ImGui::TextUnformatted(format_value(series.metric, series.mean).c_str());
				ImGui::TableSetColumnIndex(4);
				ImGui::TextUnformatted(format_value(series.metric, series.deviation).c_str());
				ImGui::TableSetColumnIndex(5);
				if (series.minimum > 0.0) ImGui::Text("%.3fx", series.maximum / series.minimum);
				else ImGui::TextDisabled("-");
				ImGui::TableSetColumnIndex(6);
				const auto direction = Orchestrator::benchmark_metrics()[series.metric].direction;
				if (direction == Orchestrator::BenchmarkMetricDirection::Neutral) ImGui::TextDisabled("-");
				else ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.5f, 1.0f), "%s", data.labels[series.best_index].c_str());
			}
			ImGui::EndTable();
		}
	}

	[[nodiscard]] std::string build_dataset_tsv(const Dataset& data) const {
		std::string text = "Run";
		if (chart_type_ == WorkbenchChartType::Lines || chart_type_ == WorkbenchChartType::Scatter) {
			text += "\t" + Orchestrator::benchmark_metric_label(Orchestrator::benchmark_metrics()[x_metric_]);
		}
		for (const auto& series : data.series) {
			text += "\t" + series.name;
			if (transform_ != WorkbenchTransform::Raw) text += " [plotted]";
		}
		text += "\n";
		for (size_t i = 0; i < data.runs.size(); ++i) {
			text += data.labels[i];
			if (chart_type_ == WorkbenchChartType::Lines || chart_type_ == WorkbenchChartType::Scatter) {
				text += "\t" + Orchestrator::benchmark_format_number("%.6g", data.x_raw[i]);
			}
			for (const auto& series : data.series) {
				text += "\t" + Orchestrator::benchmark_format_number("%.6g", series.raw[i]);
				if (transform_ != WorkbenchTransform::Raw) text += "\t" + Orchestrator::benchmark_format_number("%.6g", series.plotted[i]);
			}
			text += "\n";
		}
		return text;
	}

	void render_data_table(const Dataset& data) const {
		const bool show_x = chart_type_ == WorkbenchChartType::Lines || chart_type_ == WorkbenchChartType::Scatter;
		const bool show_plotted = transform_ != WorkbenchTransform::Raw;
		const int column_count = 1 + (show_x ? 1 : 0) + static_cast<int>(data.series.size()) * (show_plotted ? 2 : 1);
		if (ImGui::Button("Copy Data To Clipboard")) {
			ImGui::SetClipboardText(build_dataset_tsv(data).c_str());
		}
		render_setting_tooltip("Copies the displayed data as tab-separated text, ready to paste into a spreadsheet.");
		if (!ImGui::BeginTable("##WorkbenchData", column_count, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable, ImVec2(0.0f, 220.0f))) return;
		ImGui::TableSetupScrollFreeze(1, 1);
		ImGui::TableSetupColumn("Run", ImGuiTableColumnFlags_NoHide);
		if (show_x) ImGui::TableSetupColumn(Orchestrator::benchmark_metric_label(Orchestrator::benchmark_metrics()[x_metric_]).c_str());
		for (const auto& series : data.series) {
			ImGui::TableSetupColumn(series.name.c_str());
			if (show_plotted) ImGui::TableSetupColumn((series.name + " [plotted]").c_str());
		}
		ImGui::TableHeadersRow();
		for (size_t i = 0; i < data.runs.size(); ++i) {
			ImGui::TableNextRow();
			int column = 0;
			ImGui::TableSetColumnIndex(column++);
			ImGui::TextUnformatted(data.labels[i].c_str());
			if (show_x) {
				ImGui::TableSetColumnIndex(column++);
				ImGui::TextUnformatted(format_value(x_metric_, data.x_raw[i]).c_str());
			}
			for (const auto& series : data.series) {
				ImGui::TableSetColumnIndex(column++);
				const bool best = i == series.best_index && Orchestrator::benchmark_metrics()[series.metric].direction != Orchestrator::BenchmarkMetricDirection::Neutral && data.runs.size() > 1;
				const std::string raw = format_value(series.metric, series.raw[i]);
				if (best) ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.5f, 1.0f), "%s", raw.c_str());
				else ImGui::TextUnformatted(raw.c_str());
				if (show_plotted) {
					ImGui::TableSetColumnIndex(column++);
					if (std::isfinite(series.plotted[i])) ImGui::Text("%.5g", series.plotted[i]);
					else ImGui::TextDisabled("n/a");
				}
			}
		}
		ImGui::EndTable();
	}

	void render_stacked_table(const StackedData& stacked) const {
		const int column_count = static_cast<int>(stacked.names.size()) + 2;
		if (!ImGui::BeginTable("##WorkbenchStackedData", column_count, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable, ImVec2(0.0f, 220.0f))) return;
		ImGui::TableSetupScrollFreeze(1, 1);
		ImGui::TableSetupColumn("Run", ImGuiTableColumnFlags_NoHide);
		for (const auto& name : stacked.names) ImGui::TableSetupColumn(name.c_str());
		ImGui::TableSetupColumn("Total");
		ImGui::TableHeadersRow();
		for (size_t r = 0; r < runs_ordered_labels_.size(); ++r) {
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted(runs_ordered_labels_[r].c_str());
			for (size_t s = 0; s < stacked.values.size(); ++s) {
				ImGui::TableSetColumnIndex(static_cast<int>(s) + 1);
				ImGui::Text("%.3f%s", stacked.values[s][r], stacked_percent_ ? " %" : " ms");
			}
			ImGui::TableSetColumnIndex(column_count - 1);
			ImGui::Text("%.3f ms", stacked.totals[r]);
		}
		ImGui::EndTable();
	}

public:
	BenchmarkWorkbench() {
		apply_preset(0);
	}

	void set_selected(const std::string& key, bool selected) {
		const auto it = std::find(selected_keys_.begin(), selected_keys_.end(), key);
		if (selected && it == selected_keys_.end()) {
			selected_keys_.push_back(key);
		} else if (!selected && it != selected_keys_.end()) {
			selected_keys_.erase(it);
		}
	}

	[[nodiscard]] bool is_selected(const std::string& key) const noexcept {
		return std::find(selected_keys_.begin(), selected_keys_.end(), key) != selected_keys_.end();
	}

	void clear_selection() noexcept {
		selected_keys_.clear();
	}

	[[nodiscard]] size_t selection_size() const noexcept {
		return selected_keys_.size();
	}

	void render(const Orchestrator::PerformanceProfiler& profiler) {
		const auto& runs = profiler.saved_runs();
		const std::vector<ResolvedRun> resolved = resolve_selection(runs);

		render_presets();
		ImGui::Separator();
		render_selection_panel(runs);
		ImGui::Separator();
		render_axis_controls(resolved);
		ImGui::Separator();

		if (resolved.empty()) {
			ImGui::TextDisabled("Select one or more benchmark runs above, or use the checkbox in the run library, to build a comparison chart.");
			return;
		}

		if (chart_type_ == WorkbenchChartType::StackedStages) {
			const StackedData stacked = build_stacked(resolved);
			render_stacked_plot(stacked);
			if (show_data_table_) {
				render_stacked_table(stacked);
			}
			return;
		}

		if (y_metrics_.empty()) {
			y_metrics_.push_back(Orchestrator::benchmark_metric_index("Mean Frame Time"));
		}
		const Dataset data = build_dataset(resolved);
		render_plot(data);
		render_series_summary(data);
		if (show_data_table_) {
			ImGui::Spacing();
			render_data_table(data);
		}
	}
};

}
