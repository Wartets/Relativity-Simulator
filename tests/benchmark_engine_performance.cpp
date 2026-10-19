#include "support/test_harness.hpp"
#include "support/engine_scenario.hpp"
#include "relativistic/render/geodesic_compute_pipeline.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <sstream>
#include <span>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

using namespace Relativistic;
using Testing::format_number;
using Testing::TestSuite;

namespace {

struct Options {
	std::string profile{"standard"};
	std::string path{"both"};
	uint32_t repetitions{0};
	uint32_t base_width{0};
	uint32_t steps_cap{0};
	double max_case_seconds{0.0};
	uint32_t projection_mode{0};
	std::vector<std::string> metric_filter{};
	std::vector<uint32_t> body_counts{};
	std::vector<uint32_t> black_hole_counts{};
	std::vector<uint32_t> presets{};
	std::filesystem::path output_directory{};
	bool list_only{false};
	bool help{false};
};

struct Plan {
	std::vector<const Testing::MetricScenario*> metrics{};
	std::vector<uint32_t> body_counts{};
	std::vector<uint32_t> black_hole_counts{};
	std::vector<uint32_t> presets{};
	std::vector<bool> skipping_modes{};
	bool run_cpu{true};
	bool run_gpu{true};
	uint32_t repetitions{1};
	uint32_t base_width{256};
	uint32_t steps_cap{0};
	double max_case_seconds{120.0};
	uint32_t projection_mode{0};
};

struct FrameStatistics {
	uint64_t pixel_count{0};
	uint64_t horizon{0};
	uint64_t celestial{0};
	uint64_t disk{0};
	uint64_t body{0};
	uint64_t jet{0};
	uint64_t saturated{0};
	uint64_t non_finite{0};
	uint64_t alpha_violations{0};
	double iteration_sum{0.0};
	uint32_t iteration_min{std::numeric_limits<uint32_t>::max()};
	uint32_t iteration_max{0};
	double luminance_sum{0.0};
};

struct CaseResult {
	const Testing::MetricScenario* metric{nullptr};
	uint32_t body_count{0};
	uint32_t black_hole_count{0};
	uint32_t preset{0};
	bool space_skipping{false};
	bool gpu_requested{false};
	bool gpu_used{false};
	uint32_t width{0};
	uint32_t height{0};
	uint32_t max_steps{0};
	std::vector<double> samples_ms{};
	FrameStatistics statistics{};
	Render::PipelineExecutionTelemetry telemetry{};
	bool completed{false};

	[[nodiscard]] uint32_t total_bodies() const noexcept { return body_count + (metric->supports_secondary_sources ? black_hole_count : 0U); }
	[[nodiscard]] double median_ms() const {
		if (samples_ms.empty()) return 0.0;
		std::vector<double> sorted = samples_ms;
		std::sort(sorted.begin(), sorted.end());
		const size_t mid = sorted.size() / 2;
		return (sorted.size() % 2 == 1) ? sorted[mid] : 0.5 * (sorted[mid - 1] + sorted[mid]);
	}
	[[nodiscard]] double min_ms() const { return samples_ms.empty() ? 0.0 : *std::min_element(samples_ms.begin(), samples_ms.end()); }
	[[nodiscard]] double max_ms() const { return samples_ms.empty() ? 0.0 : *std::max_element(samples_ms.begin(), samples_ms.end()); }
	[[nodiscard]] double stddev_ms() const {
		if (samples_ms.size() < 2) return 0.0;
		const double mean = std::accumulate(samples_ms.begin(), samples_ms.end(), 0.0) / static_cast<double>(samples_ms.size());
		double accum = 0.0;
		for (const double value : samples_ms) accum += (value - mean) * (value - mean);
		return std::sqrt(accum / static_cast<double>(samples_ms.size() - 1));
	}
	[[nodiscard]] double megapixels_per_second() const {
		const double ms = median_ms();
		return (ms > 0.0) ? static_cast<double>(width) * static_cast<double>(height) / (ms * 1e-3) / 1e6 : 0.0;
	}
	[[nodiscard]] double average_iterations() const {
		return (statistics.pixel_count > 0) ? statistics.iteration_sum / static_cast<double>(statistics.pixel_count) : 0.0;
	}
	[[nodiscard]] double fraction(uint64_t value) const {
		return (statistics.pixel_count > 0) ? 100.0 * static_cast<double>(value) / static_cast<double>(statistics.pixel_count) : 0.0;
	}
	[[nodiscard]] std::string path_label() const { return gpu_used ? "GPU" : "CPU"; }
};

[[nodiscard]] std::vector<std::string> split_list(const std::string& text) {
	std::vector<std::string> items;
	std::string current;
	for (const char ch : text) {
		if (ch == ',') {
			if (!current.empty()) items.push_back(current);
			current.clear();
		} else {
			current += ch;
		}
	}
	if (!current.empty()) items.push_back(current);
	return items;
}

[[nodiscard]] std::vector<uint32_t> parse_unsigned_list(const std::string& text) {
	std::vector<uint32_t> values;
	for (const auto& item : split_list(text)) {
		values.push_back(static_cast<uint32_t>(std::strtoul(item.c_str(), nullptr, 10)));
	}
	return values;
}

void print_usage() {
	std::cout << "Usage: benchmark_engine_performance [options]\n"
	          << "  --profile quick|standard|full   Scenario matrix size (default: standard)\n"
	          << "  --path cpu|gpu|both             Execution paths to measure (default: both)\n"
	          << "  --repetitions N                 Timed frames per case\n"
	          << "  --base-width N                  Width at render scale 1.0\n"
	          << "  --steps-cap N                   Upper bound on ray steps per case\n"
	          << "  --max-case-seconds S            Time budget per case\n"
	          << "  --projection N                  Camera projection mode 0-7 (default: 0)\n"
	          << "  --metrics a,b,c                 Restrict to the named metrics\n"
	          << "  --bodies a,b,c                  Celestial body counts\n"
	          << "  --black-holes a,b,c             Secondary black hole counts\n"
	          << "  --presets a,b,c                 Performance presets 0-5\n"
	          << "  --output-dir PATH               Report directory\n"
	          << "  --list                          Print the case count and exit\n"
	          << "  --help                          Print this message\n";
}

[[nodiscard]] Options parse_options(int argc, char* argv[]) {
	Options options;
	for (int i = 1; i < argc; ++i) {
		const std::string argument(argv[i]);
		const auto next = [&]() -> std::string { return (i + 1 < argc) ? std::string(argv[++i]) : std::string(); };
		if (argument == "--profile") options.profile = next();
		else if (argument == "--path") options.path = next();
		else if (argument == "--repetitions") options.repetitions = static_cast<uint32_t>(std::strtoul(next().c_str(), nullptr, 10));
		else if (argument == "--base-width") options.base_width = static_cast<uint32_t>(std::strtoul(next().c_str(), nullptr, 10));
		else if (argument == "--steps-cap") options.steps_cap = static_cast<uint32_t>(std::strtoul(next().c_str(), nullptr, 10));
		else if (argument == "--max-case-seconds") options.max_case_seconds = std::strtod(next().c_str(), nullptr);
		else if (argument == "--projection") options.projection_mode = static_cast<uint32_t>(std::strtoul(next().c_str(), nullptr, 10));
		else if (argument == "--metrics") options.metric_filter = split_list(next());
		else if (argument == "--bodies") options.body_counts = parse_unsigned_list(next());
		else if (argument == "--black-holes") options.black_hole_counts = parse_unsigned_list(next());
		else if (argument == "--presets") options.presets = parse_unsigned_list(next());
		else if (argument == "--output-dir") options.output_directory = next();
		else if (argument == "--list") options.list_only = true;
		else if (argument == "--help" || argument == "-h") options.help = true;
	}
	return options;
}

[[nodiscard]] Plan build_plan(const Options& options) {
	Plan plan;
	std::vector<std::string> metric_labels;
	if (options.profile == "quick") {
		metric_labels = {"Schwarzschild", "Kerr"};
		plan.body_counts = {0, 16};
		plan.black_hole_counts = {0, 1};
		plan.presets = {1};
		plan.skipping_modes = {false};
		plan.repetitions = 1;
		plan.base_width = 160;
		plan.steps_cap = 1024;
		plan.max_case_seconds = 60.0;
	} else if (options.profile == "full") {
		for (const auto& scenario : Testing::kMetricScenarios) metric_labels.push_back(scenario.label);
		plan.body_counts = {0, 1, 10, 100, 500, 1000};
		plan.black_hole_counts = {0, 1, 2, 4, 8};
		plan.presets = {0, 1, 2, 3, 4, 5};
		plan.skipping_modes = {false, true};
		plan.repetitions = 3;
		plan.base_width = 320;
		plan.steps_cap = 0;
		plan.max_case_seconds = 900.0;
	} else {
		metric_labels = {"FlatMinkowski", "Schwarzschild", "Kerr", "ReissnerNordstrom", "KerrNewman", "SchwarzschildDeSitter", "MorrisThorne", "Alcubierre", "EddingtonFinkelstein", "TeVeS"};
		plan.body_counts = {0, 10, 100};
		plan.black_hole_counts = {0, 1, 2};
		plan.presets = {0, 1, 2, 3};
		plan.skipping_modes = {false};
		plan.repetitions = 2;
		plan.base_width = 256;
		plan.steps_cap = 4096;
		plan.max_case_seconds = 180.0;
	}
	if (!options.metric_filter.empty()) metric_labels = options.metric_filter;
	if (!options.body_counts.empty()) plan.body_counts = options.body_counts;
	if (!options.black_hole_counts.empty()) plan.black_hole_counts = options.black_hole_counts;
	if (!options.presets.empty()) plan.presets = options.presets;
	if (options.repetitions > 0) plan.repetitions = options.repetitions;
	if (options.base_width > 0) plan.base_width = options.base_width;
	if (options.steps_cap > 0) plan.steps_cap = options.steps_cap;
	if (options.max_case_seconds > 0.0) plan.max_case_seconds = options.max_case_seconds;
	plan.projection_mode = std::min(options.projection_mode, 7U);
	plan.run_cpu = (options.path != "gpu");
	plan.run_gpu = (options.path != "cpu");
	for (const auto& label : metric_labels) {
		if (const auto* scenario = Testing::find_metric_scenario(label)) {
			plan.metrics.push_back(scenario);
		}
	}
	return plan;
}

struct CaseSpec {
	const Testing::MetricScenario* metric;
	uint32_t body_count;
	uint32_t black_hole_count;
	uint32_t preset;
	bool space_skipping;
	bool use_gpu;
};

[[nodiscard]] std::vector<CaseSpec> enumerate_cases(const Plan& plan, bool gpu_available) {
	std::vector<CaseSpec> cases;
	for (const auto* metric : plan.metrics) {
		for (const uint32_t preset : plan.presets) {
			for (const bool skipping : plan.skipping_modes) {
				for (const uint32_t bodies : plan.body_counts) {
					for (const uint32_t black_holes : plan.black_hole_counts) {
						if (black_holes > 0U && !metric->supports_secondary_sources) continue;
						for (const bool gpu : {false, true}) {
							if (gpu && (!plan.run_gpu || !gpu_available)) continue;
							if (!gpu && !plan.run_cpu) continue;
							cases.push_back(CaseSpec{metric, bodies, black_holes, preset, skipping, gpu});
						}
					}
				}
			}
		}
	}
	return cases;
}

[[nodiscard]] FrameStatistics analyze_frame(std::span<const Render::GpuPixelOutput> pixels) {
	FrameStatistics stats;
	stats.pixel_count = pixels.size();
	for (const auto& pixel : pixels) {
		if (!std::isfinite(pixel.r) || !std::isfinite(pixel.g) || !std::isfinite(pixel.b) || !std::isfinite(pixel.a)) {
			++stats.non_finite;
			continue;
		}
		if (std::abs(pixel.a - 1.0f) > 1e-6f) ++stats.alpha_violations;
		if ((pixel.status_flags & Render::PixelFlags::HORIZON_ABSORBED) != 0U) ++stats.horizon;
		if ((pixel.status_flags & Render::PixelFlags::CELESTIAL_HIT) != 0U) ++stats.celestial;
		if ((pixel.status_flags & Render::PixelFlags::ACCRETION_DISK_HIT) != 0U) ++stats.disk;
		if ((pixel.status_flags & Render::PixelFlags::BODY_SURFACE_HIT) != 0U) ++stats.body;
		if ((pixel.status_flags & Render::PixelFlags::JET_EMISSION_HIT) != 0U) ++stats.jet;
		if ((pixel.status_flags & (Render::PixelFlags::HORIZON_ABSORBED | Render::PixelFlags::CELESTIAL_HIT)) == 0U) ++stats.saturated;
		stats.iteration_sum += static_cast<double>(pixel.iterations_used);
		stats.iteration_min = std::min(stats.iteration_min, pixel.iterations_used);
		stats.iteration_max = std::max(stats.iteration_max, pixel.iterations_used);
		stats.luminance_sum += static_cast<double>(0.2126f * pixel.r + 0.7152f * pixel.g + 0.0722f * pixel.b);
	}
	if (stats.pixel_count == 0) stats.iteration_min = 0;
	return stats;
}

class PipelineCache {
private:
	std::map<std::pair<uint32_t, uint32_t>, std::unique_ptr<Render::GeodesicComputePipeline>> pipelines_{};
	uint32_t projection_mode_;

public:
	explicit PipelineCache(uint32_t projection_mode) : projection_mode_(projection_mode) {}

	[[nodiscard]] Render::GeodesicComputePipeline& get(uint32_t width, uint32_t height) {
		auto& slot = pipelines_[{width, height}];
		if (!slot) {
			Render::GeodesicPipelineConfig config;
			config.width = width;
			config.height = height;
			config.precision = Render::PrecisionMode::NativeFloat64;
			config.headless = true;
			config.projection_mode = static_cast<Observer::ProjectionMode>(projection_mode_);
			slot = std::make_unique<Render::GeodesicComputePipeline>(config);
		}
		return *slot;
	}
};

struct FrameSample {
	double milliseconds{0.0};
	Render::PipelineExecutionTelemetry telemetry{};
	FrameStatistics statistics{};
};

[[nodiscard]] FrameSample execute_frame(Render::GeodesicComputePipeline& pipeline, const Testing::PreparedFrame& frame, bool use_gpu) {
	pipeline.set_gpu_compute_enabled(use_gpu);
	pipeline.set_projection_mode(static_cast<Observer::ProjectionMode>(frame.constants.projection_mode));
	const auto start = std::chrono::steady_clock::now();
	pipeline.dispatch(frame.constants, frame.bodies, static_cast<uint32_t>(frame.bodies.size()), false);
	const auto stop = std::chrono::steady_clock::now();
	FrameSample sample;
	sample.milliseconds = std::chrono::duration<double, std::milli>(stop - start).count();
	sample.telemetry = pipeline.telemetry();
	sample.statistics = analyze_frame(pipeline.framebuffer());
	return sample;
}

[[nodiscard]] CaseResult run_case(PipelineCache& cache, const Plan& plan, const CaseSpec& spec) {
	Testing::ScenarioSpec scenario;
	scenario.metric = spec.metric;
	scenario.body_count = spec.body_count;
	scenario.black_hole_count = spec.black_hole_count;
	scenario.preset = spec.preset;
	scenario.space_skipping = spec.space_skipping;
	scenario.base_width = plan.base_width;
	scenario.max_steps_cap = plan.steps_cap;
	scenario.projection_mode = plan.projection_mode;
	const auto frame = Testing::prepare_frame(scenario);

	CaseResult result;
	result.metric = spec.metric;
	result.body_count = spec.body_count;
	result.black_hole_count = spec.black_hole_count;
	result.preset = spec.preset;
	result.space_skipping = spec.space_skipping;
	result.gpu_requested = spec.use_gpu;
	result.width = frame.width;
	result.height = frame.height;
	result.max_steps = frame.resolved_max_steps;

	auto& pipeline = cache.get(frame.width, frame.height);
	const auto budget_start = std::chrono::steady_clock::now();
	const auto warmup = execute_frame(pipeline, frame, spec.use_gpu);
	result.gpu_used = warmup.telemetry.used_gpu_path;
	result.telemetry = warmup.telemetry;
	result.statistics = warmup.statistics;
	result.completed = warmup.statistics.pixel_count == static_cast<uint64_t>(frame.width) * frame.height;
	if (!result.completed) {
		return result;
	}
	for (uint32_t repetition = 0; repetition < plan.repetitions; ++repetition) {
		const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - budget_start).count();
		if (repetition > 0 && elapsed > plan.max_case_seconds) break;
		const auto sample = execute_frame(pipeline, frame, spec.use_gpu);
		result.samples_ms.push_back(sample.milliseconds);
		result.telemetry = sample.telemetry;
		result.statistics = sample.statistics;
		result.gpu_used = sample.telemetry.used_gpu_path;
	}
	if (result.samples_ms.empty()) {
		result.samples_ms.push_back(warmup.milliseconds);
	}
	return result;
}

[[nodiscard]] std::string case_label(const CaseResult& r) {
	std::ostringstream label;
	label << r.metric->label << " bodies=" << r.body_count << " black_holes=" << r.black_hole_count << " preset=" << r.preset << " skip=" << (r.space_skipping ? "on" : "off") << " path=" << (r.gpu_requested ? "gpu" : "cpu");
	return label.str();
}

[[nodiscard]] const CaseResult* find_result(const std::vector<CaseResult>& results, const Testing::MetricScenario* metric, uint32_t bodies, uint32_t black_holes, uint32_t preset, bool skipping, bool gpu_requested) {
	for (const auto& r : results) {
		if (r.metric == metric && r.body_count == bodies && r.black_hole_count == black_holes && r.preset == preset && r.space_skipping == skipping && r.gpu_requested == gpu_requested && r.completed) {
			return &r;
		}
	}
	return nullptr;
}

[[nodiscard]] double log_log_slope(const std::vector<std::pair<double, double>>& points) {
	if (points.size() < 3) return std::numeric_limits<double>::quiet_NaN();
	double sx = 0.0, sy = 0.0, sxx = 0.0, sxy = 0.0;
	const double n = static_cast<double>(points.size());
	for (const auto& [x, y] : points) {
		const double lx = std::log(1.0 + x);
		const double ly = std::log(std::max(y, 1e-9));
		sx += lx; sy += ly; sxx += lx * lx; sxy += lx * ly;
	}
	const double denominator = n * sxx - sx * sx;
	return (std::abs(denominator) < 1e-12) ? std::numeric_limits<double>::quiet_NaN() : (n * sxy - sx * sy) / denominator;
}

void write_csv(const std::filesystem::path& path, const std::vector<CaseResult>& results) {
	std::ofstream out(path, std::ios::trunc);
	out << "metric,bodies,black_holes,preset,space_skipping,requested_path,effective_path,width,height,max_steps,median_ms,min_ms,max_ms,stddev_ms,mpix_per_s,avg_iterations,min_iterations,max_iterations,horizon_pct,celestial_pct,disk_pct,body_pct,saturated_pct,bodies_sent,body_tiles,body_tiles_total,samples\n";
	for (const auto& r : results) {
		out << r.metric->label << ',' << r.body_count << ',' << r.black_hole_count << ',' << r.preset << ',' << (r.space_skipping ? 1 : 0) << ',' << (r.gpu_requested ? "gpu" : "cpu") << ',' << r.path_label() << ',' << r.width << ',' << r.height << ',' << r.max_steps << ','
		    << r.median_ms() << ',' << r.min_ms() << ',' << r.max_ms() << ',' << r.stddev_ms() << ',' << r.megapixels_per_second() << ',' << r.average_iterations() << ',' << r.statistics.iteration_min << ',' << r.statistics.iteration_max << ','
		    << r.fraction(r.statistics.horizon) << ',' << r.fraction(r.statistics.celestial) << ',' << r.fraction(r.statistics.disk) << ',' << r.fraction(r.statistics.body) << ',' << r.fraction(r.statistics.saturated) << ','
		    << r.telemetry.bodies_sent_this_frame << ',' << r.telemetry.body_tile_count << ',' << r.telemetry.body_tile_total_count << ',' << r.samples_ms.size() << '\n';
	}
}

void write_json(const std::filesystem::path& path, const std::vector<CaseResult>& results, const Plan& plan, const std::string& gpu_name, bool gpu_available) {
	std::ofstream out(path, std::ios::trunc);
	out << "{\n  \"generated\": \"" << Testing::json_escape(Testing::utc_timestamp()) << "\",\n";
	out << "  \"hardware_threads\": " << std::thread::hardware_concurrency() << ",\n";
	out << "  \"gpu_available\": " << (gpu_available ? "true" : "false") << ",\n";
	out << "  \"gpu_device\": \"" << Testing::json_escape(gpu_name) << "\",\n";
	out << "  \"base_width\": " << plan.base_width << ",\n  \"repetitions\": " << plan.repetitions << ",\n  \"steps_cap\": " << plan.steps_cap << ",\n";
	out << "  \"cases\": [\n";
	for (size_t i = 0; i < results.size(); ++i) {
		const auto& r = results[i];
		out << "    {\"metric\": \"" << r.metric->label << "\", \"bodies\": " << r.body_count << ", \"black_holes\": " << r.black_hole_count << ", \"preset\": " << r.preset << ", \"space_skipping\": " << (r.space_skipping ? "true" : "false")
		    << ", \"requested_path\": \"" << (r.gpu_requested ? "gpu" : "cpu") << "\", \"effective_path\": \"" << r.path_label() << "\", \"width\": " << r.width << ", \"height\": " << r.height << ", \"max_steps\": " << r.max_steps
		    << ", \"median_ms\": " << format_number(r.median_ms(), 9) << ", \"min_ms\": " << format_number(r.min_ms(), 9) << ", \"max_ms\": " << format_number(r.max_ms(), 9) << ", \"stddev_ms\": " << format_number(r.stddev_ms(), 9)
		    << ", \"mpix_per_s\": " << format_number(r.megapixels_per_second(), 9) << ", \"avg_iterations\": " << format_number(r.average_iterations(), 9)
		    << ", \"horizon_pct\": " << format_number(r.fraction(r.statistics.horizon), 9) << ", \"disk_pct\": " << format_number(r.fraction(r.statistics.disk), 9) << ", \"body_pct\": " << format_number(r.fraction(r.statistics.body), 9)
		    << ", \"saturated_pct\": " << format_number(r.fraction(r.statistics.saturated), 9) << "}" << (i + 1 < results.size() ? "," : "") << "\n";
	}
	out << "  ]\n}\n";
}

void write_markdown(const std::filesystem::path& path, const std::vector<CaseResult>& results, const Plan& plan, const Options& options, const std::string& gpu_name, bool gpu_available, double total_seconds) {
	std::ofstream out(path, std::ios::trunc);
	out << "# Engine performance report\n\n";
	out << "| Property | Value |\n| --- | --- |\n";
	out << "| Generated | " << Testing::utc_timestamp() << " |\n";
	out << "| Profile | " << options.profile << " |\n";
	out << "| Cases executed | " << results.size() << " |\n";
	out << "| Total duration | " << format_number(total_seconds, 5) << " s |\n";
	out << "| Hardware threads | " << std::thread::hardware_concurrency() << " |\n";
	out << "| GPU compute available | " << (gpu_available ? "yes" : "no") << " |\n";
	out << "| GPU device | " << Testing::markdown_escape(gpu_name) << " |\n";
	out << "| Base width at scale 1.0 | " << plan.base_width << " px |\n";
	out << "| Timed repetitions | " << plan.repetitions << " |\n";
	out << "| Ray step cap | " << (plan.steps_cap == 0 ? std::string("none") : std::to_string(plan.steps_cap)) << " |\n";
	out << "| Projection mode | " << plan.projection_mode << " |\n\n";

	out << "## Capability envelope on Schwarzschild\n\nLargest total body count whose median frame time stays within the target, with no secondary black hole and no space skipping.\n\n";
	out << "| Path | Preset | Resolution | >= 30 FPS | >= 10 FPS | >= 1 FPS |\n| --- | --- | --- | --- | --- | --- |\n";
	const auto* schwarzschild = Testing::find_metric_scenario("Schwarzschild");
	for (const bool gpu : {false, true}) {
		for (const uint32_t preset : plan.presets) {
			std::vector<const CaseResult*> rows;
			for (const auto& r : results) {
				if (r.metric == schwarzschild && r.preset == preset && r.gpu_requested == gpu && r.black_hole_count == 0 && !r.space_skipping && r.completed) rows.push_back(&r);
			}
			if (rows.empty()) continue;
			std::sort(rows.begin(), rows.end(), [](const CaseResult* a, const CaseResult* b) noexcept { return a->total_bodies() < b->total_bodies(); });
			const auto envelope = [&](double fps) {
				std::string best = "none";
				for (const auto* row : rows) {
					if (row->median_ms() <= 1000.0 / fps) best = std::to_string(row->total_bodies());
				}
				return best;
			};
			out << "| " << (gpu ? "GPU" : "CPU") << " | " << preset << " | " << rows.front()->width << "x" << rows.front()->height << " | " << envelope(30.0) << " | " << envelope(10.0) << " | " << envelope(1.0) << " |\n";
		}
	}

	out << "\n## Metric cost without bodies\n\nMedian frame time per metric, no bodies, no secondary black hole, no space skipping.\n\n| Metric | Preset | CPU ms | GPU ms | GPU speedup |\n| --- | --- | --- | --- | --- |\n";
	for (const auto* metric : plan.metrics) {
		for (const uint32_t preset : plan.presets) {
			const auto* cpu = find_result(results, metric, 0, 0, preset, false, false);
			const auto* gpu = find_result(results, metric, 0, 0, preset, false, true);
			if (cpu == nullptr && gpu == nullptr) continue;
			out << "| " << metric->label << " | " << preset << " | " << (cpu ? format_number(cpu->median_ms(), 5) : "n/a") << " | " << (gpu ? format_number(gpu->median_ms(), 5) : "n/a") << " | "
			    << ((cpu && gpu && gpu->median_ms() > 0.0 && gpu->gpu_used) ? format_number(cpu->median_ms() / gpu->median_ms(), 4) + "x" : "n/a") << " |\n";
		}
	}

	out << "\n## Body count scaling\n\nExponent of the power law fitted to frame time against (1 + total bodies); 1 means linear growth.\n\n| Metric | Path | Preset | Exponent | Time at lowest count (ms) | Time at highest count (ms) |\n| --- | --- | --- | --- | --- | --- |\n";
	for (const auto* metric : plan.metrics) {
		for (const bool gpu : {false, true}) {
			for (const uint32_t preset : plan.presets) {
				std::vector<const CaseResult*> rows;
				for (const auto& r : results) {
					if (r.metric == metric && r.gpu_requested == gpu && r.preset == preset && r.black_hole_count == 0 && !r.space_skipping && r.completed) rows.push_back(&r);
				}
				if (rows.size() < 3) continue;
				std::sort(rows.begin(), rows.end(), [](const CaseResult* a, const CaseResult* b) noexcept { return a->total_bodies() < b->total_bodies(); });
				std::vector<std::pair<double, double>> points;
				for (const auto* row : rows) points.emplace_back(static_cast<double>(row->total_bodies()), row->median_ms());
				out << "| " << metric->label << " | " << (gpu ? "GPU" : "CPU") << " | " << preset << " | " << format_number(log_log_slope(points), 3) << " | " << format_number(rows.front()->median_ms(), 5) << " | " << format_number(rows.back()->median_ms(), 5) << " |\n";
			}
		}
	}

	out << "\n## Secondary black hole cost\n\nAdditional median frame time per secondary black hole, with no celestial bodies.\n\n| Metric | Path | Preset | Black holes | Baseline ms | Loaded ms | Cost per black hole (ms) |\n| --- | --- | --- | --- | --- | --- | --- |\n";
	for (const auto* metric : plan.metrics) {
		if (!metric->supports_secondary_sources) continue;
		for (const bool gpu : {false, true}) {
			for (const uint32_t preset : plan.presets) {
				const auto* base = find_result(results, metric, 0, 0, preset, false, gpu);
				const CaseResult* loaded = nullptr;
				for (const auto& r : results) {
					if (r.metric == metric && r.gpu_requested == gpu && r.preset == preset && r.body_count == 0 && r.black_hole_count > 0 && !r.space_skipping && r.completed && (loaded == nullptr || r.black_hole_count > loaded->black_hole_count)) loaded = &r;
				}
				if (base == nullptr || loaded == nullptr) continue;
				out << "| " << metric->label << " | " << (gpu ? "GPU" : "CPU") << " | " << preset << " | " << loaded->black_hole_count << " | " << format_number(base->median_ms(), 5) << " | " << format_number(loaded->median_ms(), 5) << " | "
				    << format_number((loaded->median_ms() - base->median_ms()) / static_cast<double>(loaded->black_hole_count), 5) << " |\n";
			}
		}
	}

	out << "\n## Space skipping gain\n\n| Metric | Path | Preset | Bodies | Black holes | Without ms | With ms | Gain |\n| --- | --- | --- | --- | --- | --- | --- | --- |\n";
	for (const auto& r : results) {
		if (r.space_skipping || !r.completed) continue;
		const auto* skipped = find_result(results, r.metric, r.body_count, r.black_hole_count, r.preset, true, r.gpu_requested);
		if (skipped == nullptr || skipped->median_ms() <= 0.0) continue;
		out << "| " << r.metric->label << " | " << r.path_label() << " | " << r.preset << " | " << r.body_count << " | " << r.black_hole_count << " | " << format_number(r.median_ms(), 5) << " | " << format_number(skipped->median_ms(), 5) << " | " << format_number(r.median_ms() / skipped->median_ms(), 4) << "x |\n";
	}

	out << "\n## Throughput leaderboard\n\n| Rank | Case | Resolution | MPix/s | Median ms |\n| --- | --- | --- | --- | --- |\n";
	std::vector<const CaseResult*> ranked;
	for (const auto& r : results) if (r.completed) ranked.push_back(&r);
	std::sort(ranked.begin(), ranked.end(), [](const CaseResult* a, const CaseResult* b) noexcept { return a->megapixels_per_second() > b->megapixels_per_second(); });
	for (size_t i = 0; i < std::min<size_t>(ranked.size(), 10); ++i) {
		out << "| " << (i + 1) << " | " << Testing::markdown_escape(case_label(*ranked[i])) << " | " << ranked[i]->width << "x" << ranked[i]->height << " | " << format_number(ranked[i]->megapixels_per_second(), 5) << " | " << format_number(ranked[i]->median_ms(), 5) << " |\n";
	}
	out << "\n## Slowest cases\n\n| Rank | Case | Resolution | Median ms | Average iterations |\n| --- | --- | --- | --- | --- |\n";
	std::reverse(ranked.begin(), ranked.end());
	for (size_t i = 0; i < std::min<size_t>(ranked.size(), 10); ++i) {
		out << "| " << (i + 1) << " | " << Testing::markdown_escape(case_label(*ranked[i])) << " | " << ranked[i]->width << "x" << ranked[i]->height << " | " << format_number(ranked[i]->median_ms(), 5) << " | " << format_number(ranked[i]->average_iterations(), 5) << " |\n";
	}

	out << "\n## Complete results\n\n| Metric | Bodies | BH | Preset | Skip | Path | Resolution | Steps | Median ms | Min ms | Max ms | Stddev ms | MPix/s | Avg iter | Horizon % | Disk % | Body % | Saturated % |\n| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |\n";
	for (const auto& r : results) {
		out << "| " << r.metric->label << " | " << r.body_count << " | " << r.black_hole_count << " | " << r.preset << " | " << (r.space_skipping ? "on" : "off") << " | " << r.path_label() << (r.gpu_requested != r.gpu_used ? " (fallback)" : "") << " | " << r.width << "x" << r.height << " | " << r.max_steps << " | "
		    << format_number(r.median_ms(), 5) << " | " << format_number(r.min_ms(), 5) << " | " << format_number(r.max_ms(), 5) << " | " << format_number(r.stddev_ms(), 4) << " | " << format_number(r.megapixels_per_second(), 5) << " | " << format_number(r.average_iterations(), 5) << " | "
		    << format_number(r.fraction(r.statistics.horizon), 4) << " | " << format_number(r.fraction(r.statistics.disk), 4) << " | " << format_number(r.fraction(r.statistics.body), 4) << " | " << format_number(r.fraction(r.statistics.saturated), 4) << " |\n";
	}
}

[[nodiscard]] std::vector<float> capture_rgb(std::span<const Render::GpuPixelOutput> pixels) {
	std::vector<float> rgb;
	rgb.reserve(pixels.size() * 3);
	for (const auto& pixel : pixels) {
		rgb.push_back(pixel.r);
		rgb.push_back(pixel.g);
		rgb.push_back(pixel.b);
	}
	return rgb;
}

void run_path_parity(TestSuite& suite, PipelineCache& cache, const Plan& plan) {
	suite.section("CPU and GPU image parity");
	Testing::ScenarioSpec scenario;
	scenario.metric = Testing::find_metric_scenario("Schwarzschild");
	scenario.preset = 1;
	scenario.base_width = plan.base_width;
	scenario.max_steps_cap = plan.steps_cap;
	scenario.projection_mode = plan.projection_mode;
	const auto frame = Testing::prepare_frame(scenario);
	auto& pipeline = cache.get(frame.width, frame.height);

	const auto cpu = execute_frame(pipeline, frame, false);
	const auto cpu_image = capture_rgb(pipeline.framebuffer());
	const auto gpu = execute_frame(pipeline, frame, true);
	const auto gpu_image = capture_rgb(pipeline.framebuffer());
	if (!gpu.telemetry.used_gpu_path || cpu_image.size() != gpu_image.size() || cpu_image.empty()) {
		suite.note("The GPU path did not execute the parity frame; the comparison was skipped.");
		return;
	}
	double difference = 0.0;
	for (size_t i = 0; i < cpu_image.size(); ++i) {
		difference += std::abs(static_cast<double>(cpu_image[i]) - static_cast<double>(gpu_image[i]));
	}
	difference /= static_cast<double>(cpu_image.size());
	suite.measure("mean_absolute_rgb_difference_cpu_vs_gpu", difference, "linear sRGB units");
	suite.measure("parity_frame_cpu_time", cpu.milliseconds, "ms");
	suite.measure("parity_frame_gpu_time", gpu.milliseconds, "ms");
	suite.check_at_most(difference, 0.25, "CPU and GPU renderings of the reference scene do not diverge grossly");
}

void run_sanity_checks(TestSuite& suite, const std::vector<CaseResult>& results) {
	suite.section("Per-case sanity");
	size_t checked = 0;
	std::vector<std::string> failures;
	for (const auto& r : results) {
		++checked;
		const std::string label = case_label(r);
		if (!r.completed) {
			failures.push_back(label + ": frame buffer was not fully produced");
			continue;
		}
		if (r.statistics.non_finite > 0) failures.push_back(label + ": non-finite pixels=" + std::to_string(r.statistics.non_finite));
		if (r.statistics.alpha_violations > 0) failures.push_back(label + ": pixels with alpha != 1=" + std::to_string(r.statistics.alpha_violations));
		if (r.median_ms() <= 0.0) failures.push_back(label + ": non-positive frame time");
		if (r.metric->expects_horizon_pixels && r.statistics.horizon == 0) failures.push_back(label + ": no horizon pixels were produced");
		if (std::string(r.metric->label) == "FlatMinkowski" && r.body_count == 0 && r.black_hole_count == 0 && r.statistics.horizon != 0) failures.push_back(label + ": flat spacetime produced horizon pixels");
	}
	suite.check(failures.empty(), "all " + std::to_string(checked) + " executed cases satisfy the sanity invariants", "violations=" + std::to_string(failures.size()));
	for (const auto& failure : failures) {
		suite.check(false, failure);
	}
}

}

int main(int argc, char* argv[]) {
	const Options options = parse_options(argc, argv);
	if (options.help) {
		print_usage();
		return 0;
	}

	const Plan plan = build_plan(options);
	TestSuite suite(
		"engine_performance",
		"Large-scale performance and capability benchmark of the rendering engine. It sweeps spacetime metrics, celestial body counts, secondary black hole counts, performance presets and space skipping on the CPU and GPU compute paths, and reports frame times, throughput, ray statistics, scaling exponents and capability envelopes."
	);

	PipelineCache cache(plan.projection_mode);
	bool gpu_available = false;
	std::string gpu_name = "none";
	{
		auto& probe = cache.get(160, 90);
		gpu_available = probe.gpu_compute_available();
		if (gpu_available) gpu_name = probe.context().device_info().device_name;
	}

	const auto cases = enumerate_cases(plan, gpu_available);
	std::cout << "Profile " << options.profile << ": " << cases.size() << " cases, GPU " << (gpu_available ? gpu_name : std::string("unavailable")) << std::endl;
	if (options.list_only) {
		return 0;
	}

	suite.section("Environment");
	suite.check(!plan.metrics.empty(), "at least one metric scenario is selected");
	suite.check(!cases.empty(), "the scenario matrix is not empty", "cases=" + std::to_string(cases.size()));
	suite.measure("hardware_threads", static_cast<double>(std::thread::hardware_concurrency()), "threads");
	suite.measure("gpu_available", gpu_available ? 1.0 : 0.0, "boolean");
	suite.measure("scenario_count", static_cast<double>(cases.size()), "cases");
	if (cases.empty()) {
		return suite.finish();
	}

	const auto start = std::chrono::steady_clock::now();
	std::vector<CaseResult> results;
	results.reserve(cases.size());
	size_t index = 0;
	for (const auto& spec : cases) {
		++index;
		auto result = run_case(cache, plan, spec);
		std::cout << "[" << index << "/" << cases.size() << "] " << case_label(result) << " -> " << (result.completed ? format_number(result.median_ms(), 5) + " ms (" + result.path_label() + ")" : std::string("incomplete")) << std::endl;
		results.push_back(std::move(result));
	}
	const double total_seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
	suite.measure("benchmark_wall_time", total_seconds, "s");

	run_sanity_checks(suite, results);
	if (gpu_available && plan.run_gpu && plan.run_cpu) {
		run_path_parity(suite, cache, plan);
	}

	const std::filesystem::path directory = options.output_directory.empty() ? (Testing::report_directory() / "performance") : options.output_directory;
	std::error_code error;
	std::filesystem::create_directories(directory, error);
	write_markdown(directory / "engine_performance_report.md", results, plan, options, gpu_name, gpu_available, total_seconds);
	write_json(directory / "engine_performance_report.json", results, plan, gpu_name, gpu_available);
	write_csv(directory / "engine_performance_report.csv", results);
	suite.note("Detailed reports written to " + directory.generic_string());
	return suite.finish();
}
