#pragma once

#include "relativistic/orchestrator/performance_profiler.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Relativistic::Orchestrator {

[[nodiscard]] inline std::string benchmark_run_key(const BenchmarkRun& run) {
	return run.timestamp + "|" + run.label + "|" + run.engine_signature;
}

[[nodiscard]] constexpr const char* benchmark_preset_name(uint32_t preset) noexcept {
	switch (preset) {
		case 0: return "Potato";
		case 1: return "Perf";
		case 2: return "Balanced";
		case 3: return "High";
		case 4: return "Ultra";
		case 5: return "Extreme";
		default: return "Custom";
	}
}

[[nodiscard]] inline std::string benchmark_format_number(const char* format, double value) {
	char buffer[64];
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif
	std::snprintf(buffer, sizeof(buffer), format, value);
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
	return buffer;
}

enum class BenchmarkMetricUnit : uint32_t {
	None = 0,
	Milliseconds,
	FramesPerSecond,
	MegapixelsPerSecond,
	Percent,
	Seconds,
	Pixels
};

[[nodiscard]] constexpr const char* benchmark_metric_unit_text(BenchmarkMetricUnit unit) noexcept {
	switch (unit) {
		case BenchmarkMetricUnit::Milliseconds: return "ms";
		case BenchmarkMetricUnit::FramesPerSecond: return "FPS";
		case BenchmarkMetricUnit::MegapixelsPerSecond: return "MPx/s";
		case BenchmarkMetricUnit::Percent: return "%";
		case BenchmarkMetricUnit::Seconds: return "s";
		case BenchmarkMetricUnit::Pixels: return "px";
		default: return "";
	}
}

enum class BenchmarkMetricDirection : uint32_t {
	Neutral = 0,
	LowerIsBetter,
	HigherIsBetter
};

struct BenchmarkMetric {
	std::string label;
	const char* group;
	BenchmarkMetricUnit unit;
	BenchmarkMetricDirection direction;
	double (*extract)(const BenchmarkRun&, size_t);
	size_t argument;
	const char* format;
};

[[nodiscard]] inline const std::vector<BenchmarkMetric>& benchmark_metrics() {
	static const std::vector<BenchmarkMetric> metrics = [] {
		using Unit = BenchmarkMetricUnit;
		using Direction = BenchmarkMetricDirection;
		using Extractor = double (*)(const BenchmarkRun&, size_t);
		std::vector<BenchmarkMetric> list;
		const auto add = [&list](std::string label, const char* group, Unit unit, Direction direction, Extractor extract, size_t argument, const char* format) {
			list.push_back(BenchmarkMetric{std::move(label), group, unit, direction, extract, argument, format});
		};

		add("Run Order", "Capture", Unit::None, Direction::Neutral, nullptr, 0, "%.0f");
		add("Sample Count", "Capture", Unit::None, Direction::Neutral, [](const BenchmarkRun& r, size_t) { return static_cast<double>(r.sample_count); }, 0, "%.0f");
		add("Capture Duration", "Capture", Unit::Seconds, Direction::Neutral, [](const BenchmarkRun& r, size_t) { return r.capture_duration_seconds; }, 0, "%.2f");

		add("Mean Frame Time", "Frame Time", Unit::Milliseconds, Direction::LowerIsBetter, [](const BenchmarkRun& r, size_t) { return r.frame_time_summary.mean; }, 0, "%.3f");
		add("Median Frame Time", "Frame Time", Unit::Milliseconds, Direction::LowerIsBetter, [](const BenchmarkRun& r, size_t) { return r.frame_time_summary.median; }, 0, "%.3f");
		add("Min Frame Time", "Frame Time", Unit::Milliseconds, Direction::LowerIsBetter, [](const BenchmarkRun& r, size_t) { return r.frame_time_summary.min_value; }, 0, "%.3f");
		add("Max Frame Time", "Frame Time", Unit::Milliseconds, Direction::LowerIsBetter, [](const BenchmarkRun& r, size_t) { return r.frame_time_summary.max_value; }, 0, "%.3f");
		add("Frame Time Std Dev", "Frame Time", Unit::Milliseconds, Direction::LowerIsBetter, [](const BenchmarkRun& r, size_t) { return r.frame_time_summary.std_deviation; }, 0, "%.3f");
		add("P95 Frame Time", "Frame Time", Unit::Milliseconds, Direction::LowerIsBetter, [](const BenchmarkRun& r, size_t) { return r.frame_time_summary.percentile_95; }, 0, "%.3f");
		add("P99 Frame Time", "Frame Time", Unit::Milliseconds, Direction::LowerIsBetter, [](const BenchmarkRun& r, size_t) { return r.frame_time_summary.percentile_99; }, 0, "%.3f");
		add("Frame Time Jitter", "Frame Time", Unit::Percent, Direction::LowerIsBetter, [](const BenchmarkRun& r, size_t) { return (r.frame_time_summary.mean > 0.0) ? r.frame_time_summary.std_deviation / r.frame_time_summary.mean * 100.0 : 0.0; }, 0, "%.2f");
		add("P99 Spread", "Frame Time", Unit::Milliseconds, Direction::LowerIsBetter, [](const BenchmarkRun& r, size_t) { return r.frame_time_summary.percentile_99 - r.frame_time_summary.median; }, 0, "%.3f");

		add("Mean FPS", "Throughput", Unit::FramesPerSecond, Direction::HigherIsBetter, [](const BenchmarkRun& r, size_t) { return r.fps_summary.mean; }, 0, "%.1f");
		add("Median FPS", "Throughput", Unit::FramesPerSecond, Direction::HigherIsBetter, [](const BenchmarkRun& r, size_t) { return r.fps_summary.median; }, 0, "%.1f");
		add("Min FPS", "Throughput", Unit::FramesPerSecond, Direction::HigherIsBetter, [](const BenchmarkRun& r, size_t) { return r.fps_summary.min_value; }, 0, "%.1f");
		add("Max FPS", "Throughput", Unit::FramesPerSecond, Direction::HigherIsBetter, [](const BenchmarkRun& r, size_t) { return r.fps_summary.max_value; }, 0, "%.1f");
		add("1% Low FPS", "Throughput", Unit::FramesPerSecond, Direction::HigherIsBetter, [](const BenchmarkRun& r, size_t) { return (r.frame_time_summary.percentile_99 > 0.0) ? 1000.0 / r.frame_time_summary.percentile_99 : 0.0; }, 0, "%.1f");
		add("Mean Throughput", "Throughput", Unit::MegapixelsPerSecond, Direction::HigherIsBetter, [](const BenchmarkRun& r, size_t) { return r.throughput_summary.mean; }, 0, "%.2f");
		add("Median Throughput", "Throughput", Unit::MegapixelsPerSecond, Direction::HigherIsBetter, [](const BenchmarkRun& r, size_t) { return r.throughput_summary.median; }, 0, "%.2f");
		add("Min Throughput", "Throughput", Unit::MegapixelsPerSecond, Direction::HigherIsBetter, [](const BenchmarkRun& r, size_t) { return r.throughput_summary.min_value; }, 0, "%.2f");
		add("Max Throughput", "Throughput", Unit::MegapixelsPerSecond, Direction::HigherIsBetter, [](const BenchmarkRun& r, size_t) { return r.throughput_summary.max_value; }, 0, "%.2f");

		add("Average Ray Iterations", "Rays", Unit::None, Direction::LowerIsBetter, [](const BenchmarkRun& r, size_t) { return r.average_iterations; }, 0, "%.1f");
		add("Horizon Absorbed Ratio", "Rays", Unit::Percent, Direction::Neutral, [](const BenchmarkRun& r, size_t) { return r.horizon_hit_ratio * 100.0; }, 0, "%.2f");
		add("Celestial Escaped Ratio", "Rays", Unit::Percent, Direction::Neutral, [](const BenchmarkRun& r, size_t) { return r.celestial_hit_ratio * 100.0; }, 0, "%.2f");
		add("Disk Hit Ratio", "Rays", Unit::Percent, Direction::Neutral, [](const BenchmarkRun& r, size_t) { return r.disk_hit_ratio * 100.0; }, 0, "%.2f");
		add("GPU Path Ratio", "Rays", Unit::Percent, Direction::Neutral, [](const BenchmarkRun& r, size_t) { return r.gpu_path_ratio * 100.0; }, 0, "%.1f");

		add("Resolution Scale", "Configuration", Unit::None, Direction::Neutral, [](const BenchmarkRun& r, size_t) { return r.config.resolution_scale; }, 0, "%.2f");
		add("Max Ray Steps", "Configuration", Unit::None, Direction::Neutral, [](const BenchmarkRun& r, size_t) { return static_cast<double>(r.config.max_ray_steps); }, 0, "%.0f");
		add("Frame Pixels", "Configuration", Unit::Pixels, Direction::Neutral, [](const BenchmarkRun& r, size_t) { return static_cast<double>(r.config.screen_width) * static_cast<double>(r.config.screen_height); }, 0, "%.0f");
		add("Precision Mode", "Configuration", Unit::None, Direction::Neutral, [](const BenchmarkRun& r, size_t) { return static_cast<double>(r.config.precision_mode); }, 0, "%.0f");
		add("GPU Compute", "Configuration", Unit::None, Direction::Neutral, [](const BenchmarkRun& r, size_t) { return r.config.use_gpu_compute ? 1.0 : 0.0; }, 0, "%.0f");
		add("Tiled Distribution", "Configuration", Unit::None, Direction::Neutral, [](const BenchmarkRun& r, size_t) { return r.config.tiled_distribution ? 1.0 : 0.0; }, 0, "%.0f");
		add("SIMD Pipeline", "Configuration", Unit::None, Direction::Neutral, [](const BenchmarkRun& r, size_t) { return r.config.simd_pipeline ? 1.0 : 0.0; }, 0, "%.0f");
		add("Step Controller", "Configuration", Unit::None, Direction::Neutral, [](const BenchmarkRun& r, size_t) { return static_cast<double>(r.config.step_controller_mode); }, 0, "%.0f");
		add("Motion Quality Scale", "Configuration", Unit::None, Direction::Neutral, [](const BenchmarkRun& r, size_t) { return r.config.motion_quality_scale; }, 0, "%.2f");
		add("Space Skip Radius", "Configuration", Unit::None, Direction::Neutral, [](const BenchmarkRun& r, size_t) { return r.config.space_skipping_enabled ? r.config.space_skip_radius_scale : 0.0; }, 0, "%.1f");
		add("Pole Guard Precision", "Configuration", Unit::None, Direction::Neutral, [](const BenchmarkRun& r, size_t) { return r.config.pole_guard_precision_scale; }, 0, "%.2f");
		add("Far-Field Step Scale", "Configuration", Unit::None, Direction::Neutral, [](const BenchmarkRun& r, size_t) { return r.config.far_field_step_scale; }, 0, "%.2f");
		add("LOD Distance", "Configuration", Unit::None, Direction::Neutral, [](const BenchmarkRun& r, size_t) { return r.config.lod_enabled ? r.config.lod_distance_scale : 0.0; }, 0, "%.0f");
		add("LOD Reduced Steps", "Configuration", Unit::None, Direction::Neutral, [](const BenchmarkRun& r, size_t) { return static_cast<double>(r.config.lod_reduced_ray_steps); }, 0, "%.0f");
		add("Render Distance", "Configuration", Unit::None, Direction::Neutral, [](const BenchmarkRun& r, size_t) { return r.config.render_distance_scale; }, 0, "%.0f");
		add("Dynamic Resolution Target", "Configuration", Unit::FramesPerSecond, Direction::Neutral, [](const BenchmarkRun& r, size_t) { return r.config.dynamic_resolution_enabled ? r.config.dynamic_resolution_target_fps : 0.0; }, 0, "%.0f");
		add("Rolling Average Window", "Configuration", Unit::None, Direction::Neutral, [](const BenchmarkRun& r, size_t) { return static_cast<double>(r.config.rolling_average_frame_count); }, 0, "%.0f");
		add("Integration rtol", "Configuration", Unit::None, Direction::Neutral, [](const BenchmarkRun& r, size_t) { return r.config.integration_rtol; }, 0, "%.3e");
		add("Integration atol", "Configuration", Unit::None, Direction::Neutral, [](const BenchmarkRun& r, size_t) { return r.config.integration_atol; }, 0, "%.3e");
		add("Performance Preset", "Configuration", Unit::None, Direction::Neutral, [](const BenchmarkRun& r, size_t) { return static_cast<double>(r.config.performance_preset); }, 0, "%.0f");

		for (size_t stage = 0; stage < static_cast<size_t>(ProfilerTaskStage::Count); ++stage) {
			add(std::string("Stage: ") + profiler_stage_name(static_cast<ProfilerTaskStage>(stage)), "Stage Means", Unit::Milliseconds, Direction::LowerIsBetter,
				[](const BenchmarkRun& r, size_t index) { return r.stage_mean_ms[index]; }, stage, "%.3f");
		}
		return list;
	}();
	return metrics;
}

[[nodiscard]] inline size_t benchmark_metric_index(std::string_view label) noexcept {
	const auto& metrics = benchmark_metrics();
	for (size_t i = 0; i < metrics.size(); ++i) {
		if (metrics[i].label == label) return i;
	}
	return 0;
}

[[nodiscard]] inline std::string benchmark_metric_label(const BenchmarkMetric& metric) {
	const char* unit = benchmark_metric_unit_text(metric.unit);
	return (unit[0] != '\0') ? metric.label + " (" + unit + ")" : metric.label;
}

[[nodiscard]] inline double benchmark_metric_value(const BenchmarkMetric& metric, const BenchmarkRun& run, size_t run_order) noexcept {
	return (metric.extract != nullptr) ? metric.extract(run, metric.argument) : static_cast<double>(run_order + 1);
}

[[nodiscard]] inline std::string benchmark_metric_display(const BenchmarkMetric& metric, double value) {
	const char* unit = benchmark_metric_unit_text(metric.unit);
	std::string text = benchmark_format_number(metric.format, value);
	if (unit[0] != '\0') {
		text += " ";
		text += unit;
	}
	return text;
}

struct BenchmarkSignificance {
	bool valid{false};
	double mean_difference{0.0};
	double relative_percent{0.0};
	double standard_error{0.0};
	double z_score{0.0};
	double p_value{1.0};
	double cohens_d{0.0};
	double interval_low{0.0};
	double interval_high{0.0};

	[[nodiscard]] bool significant() const noexcept {
		return valid && p_value < 0.05;
	}
};

[[nodiscard]] inline BenchmarkSignificance compare_summaries(const StatisticalSummary& baseline, const StatisticalSummary& candidate) noexcept {
	BenchmarkSignificance result;
	if (baseline.sample_count < 2 || candidate.sample_count < 2) {
		return result;
	}
	const double n_a = static_cast<double>(baseline.sample_count);
	const double n_b = static_cast<double>(candidate.sample_count);
	const double var_a = baseline.std_deviation * baseline.std_deviation;
	const double var_b = candidate.std_deviation * candidate.std_deviation;
	result.mean_difference = candidate.mean - baseline.mean;
	result.relative_percent = (baseline.mean != 0.0) ? result.mean_difference / baseline.mean * 100.0 : 0.0;
	result.standard_error = std::sqrt(var_a / n_a + var_b / n_b);
	if (result.standard_error > 1e-15) {
		result.z_score = result.mean_difference / result.standard_error;
		result.p_value = std::erfc(std::abs(result.z_score) / std::sqrt(2.0));
	} else {
		result.p_value = (std::abs(result.mean_difference) > 0.0) ? 0.0 : 1.0;
	}
	const double pooled_variance = ((n_a - 1.0) * var_a + (n_b - 1.0) * var_b) / (n_a + n_b - 2.0);
	result.cohens_d = (pooled_variance > 1e-30) ? result.mean_difference / std::sqrt(pooled_variance) : 0.0;
	result.interval_low = result.mean_difference - 1.96 * result.standard_error;
	result.interval_high = result.mean_difference + 1.96 * result.standard_error;
	result.valid = true;
	return result;
}

enum class BenchmarkColumnCategory : uint32_t {
	Identity = 0,
	Timing,
	Configuration,
	Capture
};

struct BenchmarkColumn {
	const char* header;
	const char* tooltip;
	BenchmarkColumnCategory category;
	bool default_visible;
	bool copy_on_click;
	const char* numeric_format;
	double (*numeric)(const BenchmarkRun&);
	std::string (*text)(const BenchmarkRun&);

	[[nodiscard]] std::string cell(const BenchmarkRun& run) const {
		if (numeric != nullptr) {
			return benchmark_format_number(numeric_format, numeric(run));
		}
		return text(run);
	}
};

[[nodiscard]] inline const std::vector<BenchmarkColumn>& benchmark_columns() {
	using Category = BenchmarkColumnCategory;
	static const std::vector<BenchmarkColumn> columns = {
		BenchmarkColumn{"Timestamp", "Local time at which the run was captured.", Category::Identity, true, false, nullptr, nullptr,
			[](const BenchmarkRun& r) -> std::string { return r.timestamp; }},
		BenchmarkColumn{"Metric", "Spacetime metric active during the capture.", Category::Identity, true, false, nullptr, nullptr,
			[](const BenchmarkRun& r) -> std::string { return r.config.metric_name; }},
		BenchmarkColumn{"Integrator", "ODE integrator active during the capture.", Category::Identity, true, false, nullptr, nullptr,
			[](const BenchmarkRun& r) -> std::string { return r.config.integrator_name; }},
		BenchmarkColumn{"Preset", "Performance preset active during the capture.", Category::Identity, false, false, nullptr, nullptr,
			[](const BenchmarkRun& r) -> std::string { return benchmark_preset_name(r.config.performance_preset); }},
		BenchmarkColumn{"Mean FT (ms)", "Mean render time per frame.", Category::Timing, true, false, "%.3f",
			[](const BenchmarkRun& r) { return r.frame_time_summary.mean; }, nullptr},
		BenchmarkColumn{"Median FT (ms)", "Median render time per frame.", Category::Timing, false, false, "%.3f",
			[](const BenchmarkRun& r) { return r.frame_time_summary.median; }, nullptr},
		BenchmarkColumn{"P95 FT (ms)", "95th percentile render time per frame.", Category::Timing, true, false, "%.3f",
			[](const BenchmarkRun& r) { return r.frame_time_summary.percentile_95; }, nullptr},
		BenchmarkColumn{"P99 FT (ms)", "99th percentile render time per frame.", Category::Timing, false, false, "%.3f",
			[](const BenchmarkRun& r) { return r.frame_time_summary.percentile_99; }, nullptr},
		BenchmarkColumn{"FT Std Dev (ms)", "Standard deviation of the render time per frame.", Category::Timing, false, false, "%.3f",
			[](const BenchmarkRun& r) { return r.frame_time_summary.std_deviation; }, nullptr},
		BenchmarkColumn{"Mean FPS", "Mean frames per second.", Category::Timing, true, false, "%.1f",
			[](const BenchmarkRun& r) { return r.fps_summary.mean; }, nullptr},
		BenchmarkColumn{"Min FPS", "Lowest frames per second sample.", Category::Timing, false, false, "%.1f",
			[](const BenchmarkRun& r) { return r.fps_summary.min_value; }, nullptr},
		BenchmarkColumn{"1% Low FPS", "Frames per second equivalent of the 99th percentile frame time.", Category::Timing, true, false, "%.1f",
			[](const BenchmarkRun& r) { return (r.frame_time_summary.percentile_99 > 0.0) ? 1000.0 / r.frame_time_summary.percentile_99 : 0.0; }, nullptr},
		BenchmarkColumn{"Throughput (MPx/s)", "Mean number of fully traced megapixels per second.", Category::Timing, false, false, "%.2f",
			[](const BenchmarkRun& r) { return r.throughput_summary.mean; }, nullptr},
		BenchmarkColumn{"Avg Iterations", "Mean number of geodesic steps per ray.", Category::Timing, false, false, "%.1f",
			[](const BenchmarkRun& r) { return r.average_iterations; }, nullptr},
		BenchmarkColumn{"GPU Ratio", "Share of frames rendered through the Vulkan compute path.", Category::Timing, true, false, "%.0f%%",
			[](const BenchmarkRun& r) { return r.gpu_path_ratio * 100.0; }, nullptr},
		BenchmarkColumn{"Res Scale", "Internal render scale.", Category::Configuration, true, false, "%.2fx",
			[](const BenchmarkRun& r) { return r.config.resolution_scale; }, nullptr},
		BenchmarkColumn{"Resolution", "Internal render resolution in pixels.", Category::Configuration, true, false, nullptr, nullptr,
			[](const BenchmarkRun& r) -> std::string {
				char buffer[48];
				std::snprintf(buffer, sizeof(buffer), "%ux%u", r.config.screen_width, r.config.screen_height);
				return buffer;
			}},
		BenchmarkColumn{"Ray Steps", "Maximum geodesic integration steps per ray.", Category::Configuration, true, false, "%.0f",
			[](const BenchmarkRun& r) { return static_cast<double>(r.config.max_ray_steps); }, nullptr},
		BenchmarkColumn{"Precision", "Arithmetic precision mode.", Category::Configuration, false, false, nullptr, nullptr,
			[](const BenchmarkRun& r) -> std::string { return r.config.precision_mode == 0 ? "FP64" : "Double-Single"; }},
		BenchmarkColumn{"Tiled", "Tiled work distribution.", Category::Configuration, false, false, nullptr, nullptr,
			[](const BenchmarkRun& r) -> std::string { return r.config.tiled_distribution ? "Yes" : "No"; }},
		BenchmarkColumn{"SIMD", "SIMD geodesic bundles.", Category::Configuration, false, false, nullptr, nullptr,
			[](const BenchmarkRun& r) -> std::string { return r.config.simd_pipeline ? "Yes" : "No"; }},
		BenchmarkColumn{"Step Ctrl", "Adaptive step-size controller.", Category::Configuration, false, false, nullptr, nullptr,
			[](const BenchmarkRun& r) -> std::string {
				static constexpr const char* kNames[] = {"Standard", "PI-30", "PID-42"};
				return kNames[std::min<uint32_t>(r.config.step_controller_mode, 2U)];
			}},
		BenchmarkColumn{"Motion Quality", "Camera motion render quality mode.", Category::Configuration, false, false, nullptr, nullptr,
			[](const BenchmarkRun& r) -> std::string {
				static constexpr const char* kNames[] = {"Disabled", "Automatic", "Fixed"};
				return std::string(kNames[std::min<uint32_t>(r.config.motion_quality_mode, 2U)]) + " (" + benchmark_format_number("%.2fx", r.config.motion_quality_scale) + ")";
			}},
		BenchmarkColumn{"Space Skip", "Adaptive space-skipping radius.", Category::Configuration, false, false, nullptr, nullptr,
			[](const BenchmarkRun& r) -> std::string { return r.config.space_skipping_enabled ? benchmark_format_number("%.1f M", r.config.space_skip_radius_scale) : std::string("Off"); }},
		BenchmarkColumn{"Pole Guard", "Polar step damping strength.", Category::Configuration, false, false, "%.2f",
			[](const BenchmarkRun& r) { return r.config.pole_guard_precision_scale; }, nullptr},
		BenchmarkColumn{"Far-Field Step", "Far-field step multiplier.", Category::Configuration, false, false, "%.2fx",
			[](const BenchmarkRun& r) { return r.config.far_field_step_scale; }, nullptr},
		BenchmarkColumn{"LOD", "Distance-based level of detail.", Category::Configuration, false, false, nullptr, nullptr,
			[](const BenchmarkRun& r) -> std::string {
				return r.config.lod_enabled ? benchmark_format_number("%.0f M", r.config.lod_distance_scale) + " / " + std::to_string(r.config.lod_reduced_ray_steps) + " steps" : std::string("Off");
			}},
		BenchmarkColumn{"Render Distance", "Render distance cutoff.", Category::Configuration, false, false, nullptr, nullptr,
			[](const BenchmarkRun& r) -> std::string { return r.config.render_distance_scale > 0.0 ? benchmark_format_number("%.0f M", r.config.render_distance_scale) : std::string("Unbounded"); }},
		BenchmarkColumn{"Interlace", "Interlaced scanline rendering.", Category::Configuration, false, false, nullptr, nullptr,
			[](const BenchmarkRun& r) -> std::string { return r.config.interlace_rendering_enabled ? "Yes" : "No"; }},
		BenchmarkColumn{"Dynamic Res", "Dynamic resolution throttling.", Category::Configuration, false, false, nullptr, nullptr,
			[](const BenchmarkRun& r) -> std::string { return r.config.dynamic_resolution_enabled ? benchmark_format_number("%.0f fps target", r.config.dynamic_resolution_target_fps) : std::string("Off"); }},
		BenchmarkColumn{"Tile Prepass", "Adaptive tile sky prepass.", Category::Configuration, false, false, nullptr, nullptr,
			[](const BenchmarkRun& r) -> std::string { return r.config.adaptive_tile_prepass_enabled ? "Yes" : "No"; }},
		BenchmarkColumn{"Rolling Avg N", "Rolling average window used by the HUD.", Category::Configuration, false, false, "%.0f",
			[](const BenchmarkRun& r) { return static_cast<double>(r.config.rolling_average_frame_count); }, nullptr},
		BenchmarkColumn{"rtol / atol", "Integration tolerances.", Category::Configuration, false, false, nullptr, nullptr,
			[](const BenchmarkRun& r) -> std::string {
				char buffer[64];
				std::snprintf(buffer, sizeof(buffer), "%.1e / %.1e", r.config.integration_rtol, r.config.integration_atol);
				return buffer;
			}},
		BenchmarkColumn{"Samples", "Number of frames aggregated into the run.", Category::Capture, false, false, "%.0f",
			[](const BenchmarkRun& r) { return static_cast<double>(r.sample_count); }, nullptr},
		BenchmarkColumn{"Duration (s)", "Wall-clock duration of the capture.", Category::Capture, false, false, "%.1f",
			[](const BenchmarkRun& r) { return r.capture_duration_seconds; }, nullptr},
		BenchmarkColumn{"Engine Signature", "Structural fingerprint of the engine build. Click a value to copy it.", Category::Capture, false, true, nullptr, nullptr,
			[](const BenchmarkRun& r) -> std::string { return r.engine_signature; }}
	};
	return columns;
}

}
