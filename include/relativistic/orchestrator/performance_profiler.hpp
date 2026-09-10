#pragma once

#include "relativistic/core/engine_signature.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Relativistic::Orchestrator {

enum class ProfilerTaskStage : uint32_t {
	FrameTotal = 0,
	RenderDispatch = 1,
	TextureUpload = 2,
	HudOverlay = 3,
	CameraUpdate = 4,
	SchematicOverlay = 5,
	Count
};

[[nodiscard]] constexpr const char* profiler_stage_name(ProfilerTaskStage stage) noexcept {
	switch (stage) {
		case ProfilerTaskStage::FrameTotal: return "Frame Total";
		case ProfilerTaskStage::RenderDispatch: return "Render Dispatch";
		case ProfilerTaskStage::TextureUpload: return "Texture Upload";
		case ProfilerTaskStage::HudOverlay: return "HUD Overlay";
		case ProfilerTaskStage::CameraUpdate: return "Camera Update";
		case ProfilerTaskStage::SchematicOverlay: return "Schematic Overlay";
		default: return "Unknown Stage";
	}
}

struct StatisticalSummary {
	double mean{0.0};
	double median{0.0};
	double min_value{0.0};
	double max_value{0.0};
	double std_deviation{0.0};
	double percentile_95{0.0};
	double percentile_99{0.0};
	size_t sample_count{0};

	[[nodiscard]] static StatisticalSummary compute(std::vector<double> values) noexcept {
		StatisticalSummary s;
		s.sample_count = values.size();
		if (values.empty()) return s;
		std::sort(values.begin(), values.end());
		s.min_value = values.front();
		s.max_value = values.back();
		const double sum = std::accumulate(values.begin(), values.end(), 0.0);
		s.mean = sum / static_cast<double>(values.size());
		const size_t mid = values.size() / 2;
		s.median = (values.size() % 2 == 0) ? 0.5 * (values[mid - 1] + values[mid]) : values[mid];

		double variance_sum = 0.0;
		for (double v : values) {
			const double d = v - s.mean;
			variance_sum += d * d;
		}
		s.std_deviation = std::sqrt(variance_sum / static_cast<double>(values.size()));

		auto percentile_at = [&](double p) noexcept -> double {
			const double idx = p * static_cast<double>(values.size() - 1);
			const size_t i0 = static_cast<size_t>(idx);
			const size_t i1 = std::min(i0 + 1, values.size() - 1);
			const double frac = idx - static_cast<double>(i0);
			return values[i0] * (1.0 - frac) + values[i1] * frac;
		};
		s.percentile_95 = percentile_at(0.95);
		s.percentile_99 = percentile_at(0.99);
		return s;
	}
};

struct FrameSampleInput {
	double timestamp_seconds{0.0};
	double frame_time_ms{0.0};
	double fps{0.0};
	bool used_gpu_path{false};
	uint32_t screen_width{0};
	uint32_t screen_height{0};
	uint64_t horizon_pixels{0};
	uint64_t celestial_pixels{0};
	uint64_t disk_pixels{0};
	double average_iterations{0.0};
	double max_iteration_ratio{0.0};
	double resolution_scale{0.0};
	uint32_t max_ray_steps{0};
	uint32_t precision_mode{0};
	uint32_t performance_preset{0};
	bool tiled_distribution{false};
	bool simd_pipeline{false};
	bool gpu_compute_enabled{false};
	uint32_t step_controller_mode{0};
	std::string metric_name{};
	std::string integrator_name{};
};

struct FrameSample {
	double timestamp_seconds{0.0};
	double frame_time_ms{0.0};
	double fps{0.0};
	bool used_gpu_path{false};
	uint32_t screen_width{0};
	uint32_t screen_height{0};
	uint64_t pixels_processed{0};
	uint64_t horizon_pixels{0};
	uint64_t celestial_pixels{0};
	uint64_t disk_pixels{0};
	double average_iterations{0.0};
	double max_iteration_ratio{0.0};
	double resolution_scale{0.0};
	uint32_t max_ray_steps{0};
	uint32_t precision_mode{0};
	uint32_t performance_preset{0};
	bool tiled_distribution{false};
	bool simd_pipeline{false};
	bool gpu_compute_enabled{false};
	uint32_t step_controller_mode{0};
	std::string metric_name{};
	std::string integrator_name{};
	std::array<double, static_cast<size_t>(ProfilerTaskStage::Count)> stage_time_ms{};
};

struct BenchmarkConfigSnapshot {
	std::string metric_name{};
	std::string integrator_name{};
	double resolution_scale{1.0};
	uint32_t max_ray_steps{0};
	uint32_t precision_mode{0};
	uint32_t performance_preset{0};
	bool use_gpu_compute{false};
	bool tiled_distribution{false};
	bool simd_pipeline{false};
	uint32_t screen_width{0};
	uint32_t screen_height{0};
	uint32_t step_controller_mode{0};
};

struct BenchmarkRun {
	std::string label{};
	std::string timestamp{};
	std::string engine_signature{};
	BenchmarkConfigSnapshot config{};
	StatisticalSummary frame_time_summary{};
	StatisticalSummary fps_summary{};
	double average_iterations{0.0};
	double gpu_path_ratio{0.0};
	double horizon_hit_ratio{0.0};
	double celestial_hit_ratio{0.0};
	double disk_hit_ratio{0.0};
	size_t sample_count{0};
	double capture_duration_seconds{0.0};
};

class PerformanceProfiler {
public:
	static constexpr size_t DEFAULT_HISTORY_CAPACITY = 3600;

	class ScopedStageTimer {
	private:
		PerformanceProfiler& profiler_;
		ProfilerTaskStage stage_;
		std::chrono::steady_clock::time_point start_;

	public:
		ScopedStageTimer(PerformanceProfiler& profiler, ProfilerTaskStage stage) noexcept
			: profiler_(profiler), stage_(stage), start_(std::chrono::steady_clock::now()) {}

		ScopedStageTimer(const ScopedStageTimer&) = delete;
		ScopedStageTimer& operator=(const ScopedStageTimer&) = delete;

		~ScopedStageTimer() noexcept {
			const auto end = std::chrono::steady_clock::now();
			const double ms = std::chrono::duration<double, std::milli>(end - start_).count();
			profiler_.record_stage_duration(stage_, ms);
		}
	};

private:
	std::deque<FrameSample> history_{};
	size_t history_capacity_{DEFAULT_HISTORY_CAPACITY};
	std::vector<BenchmarkRun> saved_runs_{};
	std::string engine_signature_{Core::EngineSignature::compute()};

	std::array<double, static_cast<size_t>(ProfilerTaskStage::Count)> pending_stage_ms_{};

	bool capture_active_{false};
	std::string capture_label_{"Benchmark"};
	double capture_start_time_{0.0};
	std::optional<double> capture_target_duration_{};
	std::optional<size_t> capture_target_frames_{};
	std::vector<FrameSample> capture_buffer_{};

	bool persistence_enabled_{true};

	[[nodiscard]] static std::string current_timestamp_string() {
		const auto now = std::chrono::system_clock::now();
		const std::time_t t = std::chrono::system_clock::to_time_t(now);
		std::tm tm_buf{};
#if defined(_WIN32)
		localtime_s(&tm_buf, &t);
#else
		localtime_r(&t, &tm_buf);
#endif
		std::ostringstream ss;
		ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
		return ss.str();
	}

	[[nodiscard]] static std::string escape_line(std::string_view s) {
		std::string result(s);
		std::replace(result.begin(), result.end(), '\n', ' ');
		std::replace(result.begin(), result.end(), '\r', ' ');
		return result;
	}

	void finalize_capture() noexcept {
		if (capture_buffer_.empty()) {
			capture_active_ = false;
			return;
		}

		BenchmarkRun run;
		run.label = capture_label_;
		run.engine_signature = engine_signature_;
		run.timestamp = current_timestamp_string();

		const auto& last = capture_buffer_.back();
		run.config.metric_name = last.metric_name;
		run.config.integrator_name = last.integrator_name;
		run.config.resolution_scale = last.resolution_scale;
		run.config.max_ray_steps = last.max_ray_steps;
		run.config.precision_mode = last.precision_mode;
		run.config.performance_preset = last.performance_preset;
		run.config.use_gpu_compute = last.gpu_compute_enabled;
		run.config.tiled_distribution = last.tiled_distribution;
		run.config.simd_pipeline = last.simd_pipeline;
		run.config.screen_width = last.screen_width;
		run.config.screen_height = last.screen_height;
		run.config.step_controller_mode = last.step_controller_mode;

		std::vector<double> frame_times;
		std::vector<double> fps_values;
		frame_times.reserve(capture_buffer_.size());
		fps_values.reserve(capture_buffer_.size());

		double sum_iterations = 0.0;
		uint64_t gpu_frames = 0;
		uint64_t total_horizon = 0, total_celestial = 0, total_disk = 0, total_pixels = 0;

		for (const auto& s : capture_buffer_) {
			frame_times.push_back(s.frame_time_ms);
			fps_values.push_back(s.fps);
			sum_iterations += s.average_iterations;
			if (s.used_gpu_path) ++gpu_frames;
			total_horizon += s.horizon_pixels;
			total_celestial += s.celestial_pixels;
			total_disk += s.disk_pixels;
			total_pixels += s.pixels_processed;
		}

		run.frame_time_summary = StatisticalSummary::compute(frame_times);
		run.fps_summary = StatisticalSummary::compute(fps_values);
		run.average_iterations = sum_iterations / static_cast<double>(capture_buffer_.size());
		run.gpu_path_ratio = static_cast<double>(gpu_frames) / static_cast<double>(capture_buffer_.size());
		run.horizon_hit_ratio = (total_pixels > 0) ? (static_cast<double>(total_horizon) / static_cast<double>(total_pixels)) : 0.0;
		run.celestial_hit_ratio = (total_pixels > 0) ? (static_cast<double>(total_celestial) / static_cast<double>(total_pixels)) : 0.0;
		run.disk_hit_ratio = (total_pixels > 0) ? (static_cast<double>(total_disk) / static_cast<double>(total_pixels)) : 0.0;
		run.sample_count = capture_buffer_.size();
		run.capture_duration_seconds = capture_buffer_.back().timestamp_seconds - capture_buffer_.front().timestamp_seconds;

		saved_runs_.push_back(std::move(run));
		capture_active_ = false;
		capture_buffer_.clear();
		save_to_disk();
	}

public:
	PerformanceProfiler() noexcept = default;

	[[nodiscard]] const std::string& engine_signature() const noexcept { return engine_signature_; }

	void set_history_capacity(size_t capacity) noexcept {
		history_capacity_ = std::max<size_t>(capacity, 16);
		while (history_.size() > history_capacity_) history_.pop_front();
	}

	[[nodiscard]] size_t history_capacity() const noexcept { return history_capacity_; }

	[[nodiscard]] bool persistence_enabled() const noexcept { return persistence_enabled_; }
	void set_persistence_enabled(bool enabled) noexcept { persistence_enabled_ = enabled; }

	void record_stage_duration(ProfilerTaskStage stage, double milliseconds) noexcept {
		pending_stage_ms_[static_cast<size_t>(stage)] += milliseconds;
	}

	[[nodiscard]] ScopedStageTimer scoped_stage(ProfilerTaskStage stage) noexcept {
		return ScopedStageTimer(*this, stage);
	}

	void record_frame(const FrameSampleInput& input) noexcept {
		FrameSample sample;
		sample.timestamp_seconds = input.timestamp_seconds;
		sample.frame_time_ms = input.frame_time_ms;
		sample.fps = input.fps;
		sample.used_gpu_path = input.used_gpu_path;
		sample.screen_width = input.screen_width;
		sample.screen_height = input.screen_height;
		sample.pixels_processed = static_cast<uint64_t>(input.screen_width) * static_cast<uint64_t>(input.screen_height);
		sample.horizon_pixels = input.horizon_pixels;
		sample.celestial_pixels = input.celestial_pixels;
		sample.disk_pixels = input.disk_pixels;
		sample.average_iterations = input.average_iterations;
		sample.max_iteration_ratio = input.max_iteration_ratio;
		sample.resolution_scale = input.resolution_scale;
		sample.max_ray_steps = input.max_ray_steps;
		sample.precision_mode = input.precision_mode;
		sample.performance_preset = input.performance_preset;
		sample.tiled_distribution = input.tiled_distribution;
		sample.simd_pipeline = input.simd_pipeline;
		sample.gpu_compute_enabled = input.gpu_compute_enabled;
		sample.step_controller_mode = input.step_controller_mode;
		sample.metric_name = input.metric_name;
		sample.integrator_name = input.integrator_name;
		sample.stage_time_ms = pending_stage_ms_;
		pending_stage_ms_.fill(0.0);

		history_.push_back(sample);
		while (history_.size() > history_capacity_) history_.pop_front();

		if (capture_active_) {
			capture_buffer_.push_back(history_.back());
			const bool duration_reached = capture_target_duration_.has_value() && (input.timestamp_seconds - capture_start_time_) >= *capture_target_duration_;
			const bool frames_reached = capture_target_frames_.has_value() && capture_buffer_.size() >= *capture_target_frames_;
			if (duration_reached || frames_reached) {
				finalize_capture();
			}
		}
	}

	void start_capture(std::string_view label, std::optional<double> duration_seconds, std::optional<size_t> frame_count, double current_time_seconds) noexcept {
		capture_active_ = true;
		capture_label_ = std::string(label);
		capture_start_time_ = current_time_seconds;
		capture_target_duration_ = duration_seconds;
		capture_target_frames_ = frame_count;
		capture_buffer_.clear();
	}

	void cancel_capture() noexcept {
		capture_active_ = false;
		capture_buffer_.clear();
	}

	[[nodiscard]] bool is_capturing() const noexcept { return capture_active_; }

	[[nodiscard]] double capture_progress() const noexcept {
		if (!capture_active_) return 0.0;
		if (capture_target_frames_.has_value()) {
			return std::clamp(static_cast<double>(capture_buffer_.size()) / static_cast<double>(*capture_target_frames_), 0.0, 1.0);
		}
		if (!capture_buffer_.empty() && capture_target_duration_.has_value()) {
			const double elapsed = capture_buffer_.back().timestamp_seconds - capture_start_time_;
			return std::clamp(elapsed / *capture_target_duration_, 0.0, 1.0);
		}
		return 0.0;
	}

	[[nodiscard]] const std::deque<FrameSample>& history() const noexcept { return history_; }
	[[nodiscard]] const std::vector<BenchmarkRun>& saved_runs() const noexcept { return saved_runs_; }

	void remove_run(size_t index) noexcept {
		if (index < saved_runs_.size()) {
			saved_runs_.erase(saved_runs_.begin() + static_cast<ptrdiff_t>(index));
		}
	}

	void clear_history() noexcept {
		history_.clear();
	}

	void clear_all_runs() noexcept {
		saved_runs_.clear();
	}

	[[nodiscard]] StatisticalSummary frame_time_summary(size_t last_n_samples = 0) const noexcept {
		const size_t count = (last_n_samples == 0 || last_n_samples > history_.size()) ? history_.size() : last_n_samples;
		std::vector<double> values;
		values.reserve(count);
		for (size_t i = history_.size() - count; i < history_.size(); ++i) {
			values.push_back(history_[i].frame_time_ms);
		}
		return StatisticalSummary::compute(std::move(values));
	}

	[[nodiscard]] StatisticalSummary stage_summary(ProfilerTaskStage stage, size_t last_n_samples = 0) const noexcept {
		const size_t count = (last_n_samples == 0 || last_n_samples > history_.size()) ? history_.size() : last_n_samples;
		const size_t stage_idx = static_cast<size_t>(stage);
		std::vector<double> values;
		values.reserve(count);
		for (size_t i = history_.size() - count; i < history_.size(); ++i) {
			values.push_back(history_[i].stage_time_ms[stage_idx]);
		}
		return StatisticalSummary::compute(std::move(values));
	}

	[[nodiscard]] StatisticalSummary iteration_summary(size_t last_n_samples = 0) const noexcept {
		const size_t count = (last_n_samples == 0 || last_n_samples > history_.size()) ? history_.size() : last_n_samples;
		std::vector<double> values;
		values.reserve(count);
		for (size_t i = history_.size() - count; i < history_.size(); ++i) {
			values.push_back(history_[i].average_iterations);
		}
		return StatisticalSummary::compute(std::move(values));
	}

	struct BottleneckReport {
		ProfilerTaskStage dominant_stage{ProfilerTaskStage::FrameTotal};
		double dominant_share{0.0};
		bool ray_step_saturated{false};
		double saturation_ratio{0.0};
		bool gpu_underutilized{false};
		std::string summary{};
	};

	[[nodiscard]] BottleneckReport analyze_bottleneck(size_t last_n_samples = 120) const noexcept {
		BottleneckReport report;
		if (history_.empty()) {
			report.summary = "No data collected yet.";
			return report;
		}

		const size_t count = std::min(last_n_samples, history_.size());
		std::array<double, static_cast<size_t>(ProfilerTaskStage::Count)> stage_totals{};
		double frame_total = 0.0;
		double iteration_ratio_sum = 0.0;
		size_t gpu_count = 0;

		for (size_t i = history_.size() - count; i < history_.size(); ++i) {
			const auto& s = history_[i];
			frame_total += s.frame_time_ms;
			iteration_ratio_sum += s.max_iteration_ratio;
			if (s.used_gpu_path) ++gpu_count;
			for (size_t st = 0; st < static_cast<size_t>(ProfilerTaskStage::Count); ++st) {
				if (st == static_cast<size_t>(ProfilerTaskStage::FrameTotal)) continue;
				stage_totals[st] += s.stage_time_ms[st];
			}
		}

		size_t dominant_idx = static_cast<size_t>(ProfilerTaskStage::RenderDispatch);
		double dominant_value = 0.0;
		for (size_t st = 0; st < static_cast<size_t>(ProfilerTaskStage::Count); ++st) {
			if (st == static_cast<size_t>(ProfilerTaskStage::FrameTotal)) continue;
			if (stage_totals[st] > dominant_value) {
				dominant_value = stage_totals[st];
				dominant_idx = st;
			}
		}

		report.dominant_stage = static_cast<ProfilerTaskStage>(dominant_idx);
		report.dominant_share = (frame_total > 1e-9) ? std::clamp(dominant_value / frame_total, 0.0, 1.0) : 0.0;
		report.saturation_ratio = iteration_ratio_sum / static_cast<double>(count);
		report.ray_step_saturated = report.saturation_ratio > 0.35;
		report.gpu_underutilized = (gpu_count == 0) && (count > 0);

		std::ostringstream ss;
		ss << profiler_stage_name(report.dominant_stage) << " accounts for "
		   << static_cast<int>(report.dominant_share * 100.0) << "% of measured stage time.";
		if (report.ray_step_saturated) {
			ss << " A significant share of rays reach the integration step cap, suggesting the max ray steps or step tolerance is the limiting factor.";
		}
		if (report.gpu_underutilized) {
			ss << " GPU compute offload was not used during this window.";
		}
		report.summary = ss.str();
		return report;
	}

	[[nodiscard]] static std::filesystem::path storage_file_path() {
		return std::filesystem::path("config") / "performance_profiler.cfg";
	}

	void save_to_disk() {
		if (!persistence_enabled_) return;
		std::error_code ec;
		std::filesystem::create_directories(storage_file_path().parent_path(), ec);
		std::ofstream out(storage_file_path(), std::ios::trunc);
		if (!out.is_open()) return;

		out << "format_version=1\n";
		out << "history_capacity=" << history_capacity_ << "\n";

		for (const auto& run : saved_runs_) {
			out << "[run]\n";
			out << "label=" << escape_line(run.label) << "\n";
			out << "timestamp=" << escape_line(run.timestamp) << "\n";
			out << "engine_signature=" << run.engine_signature << "\n";
			out << "cfg_metric=" << escape_line(run.config.metric_name) << "\n";
			out << "cfg_integrator=" << escape_line(run.config.integrator_name) << "\n";
			out << "cfg_res_scale=" << run.config.resolution_scale << "\n";
			out << "cfg_ray_steps=" << run.config.max_ray_steps << "\n";
			out << "cfg_precision=" << run.config.precision_mode << "\n";
			out << "cfg_preset=" << run.config.performance_preset << "\n";
			out << "cfg_gpu=" << (run.config.use_gpu_compute ? 1 : 0) << "\n";
			out << "cfg_tiled=" << (run.config.tiled_distribution ? 1 : 0) << "\n";
			out << "cfg_simd=" << (run.config.simd_pipeline ? 1 : 0) << "\n";
			out << "cfg_width=" << run.config.screen_width << "\n";
			out << "cfg_height=" << run.config.screen_height << "\n";
			out << "cfg_step_controller=" << run.config.step_controller_mode << "\n";
			out << "ft_mean=" << run.frame_time_summary.mean << "\n";
			out << "ft_median=" << run.frame_time_summary.median << "\n";
			out << "ft_min=" << run.frame_time_summary.min_value << "\n";
			out << "ft_max=" << run.frame_time_summary.max_value << "\n";
			out << "ft_std=" << run.frame_time_summary.std_deviation << "\n";
			out << "ft_p95=" << run.frame_time_summary.percentile_95 << "\n";
			out << "ft_p99=" << run.frame_time_summary.percentile_99 << "\n";
			out << "fps_mean=" << run.fps_summary.mean << "\n";
			out << "fps_min=" << run.fps_summary.min_value << "\n";
			out << "fps_max=" << run.fps_summary.max_value << "\n";
			out << "avg_iterations=" << run.average_iterations << "\n";
			out << "gpu_ratio=" << run.gpu_path_ratio << "\n";
			out << "horizon_ratio=" << run.horizon_hit_ratio << "\n";
			out << "celestial_ratio=" << run.celestial_hit_ratio << "\n";
			out << "disk_ratio=" << run.disk_hit_ratio << "\n";
			out << "sample_count=" << run.sample_count << "\n";
			out << "duration=" << run.capture_duration_seconds << "\n";
			out << "[/run]\n";
		}
	}

	void load_from_disk() {
		std::ifstream file(storage_file_path());
		if (!file.is_open()) return;

		saved_runs_.clear();
		std::string line;
		bool in_run = false;
		BenchmarkRun current;

		while (std::getline(file, line)) {
			if (line.empty()) continue;
			if (line == "[run]") {
				in_run = true;
				current = BenchmarkRun{};
				continue;
			}
			if (line == "[/run]") {
				if (in_run) saved_runs_.push_back(current);
				in_run = false;
				continue;
			}

			const size_t eq = line.find('=');
			if (eq == std::string::npos) continue;
			const std::string key = line.substr(0, eq);
			const std::string val = line.substr(eq + 1);

			if (!in_run) {
				if (key == "history_capacity") {
					history_capacity_ = std::max<size_t>(static_cast<size_t>(std::strtoull(val.c_str(), nullptr, 10)), 16);
				}
				continue;
			}

			if (key == "label") current.label = val;
			else if (key == "timestamp") current.timestamp = val;
			else if (key == "engine_signature") current.engine_signature = val;
			else if (key == "cfg_metric") current.config.metric_name = val;
			else if (key == "cfg_integrator") current.config.integrator_name = val;
			else if (key == "cfg_res_scale") current.config.resolution_scale = std::strtod(val.c_str(), nullptr);
			else if (key == "cfg_ray_steps") current.config.max_ray_steps = static_cast<uint32_t>(std::strtoul(val.c_str(), nullptr, 10));
			else if (key == "cfg_precision") current.config.precision_mode = static_cast<uint32_t>(std::strtoul(val.c_str(), nullptr, 10));
			else if (key == "cfg_preset") current.config.performance_preset = static_cast<uint32_t>(std::strtoul(val.c_str(), nullptr, 10));
			else if (key == "cfg_gpu") current.config.use_gpu_compute = (val == "1");
			else if (key == "cfg_tiled") current.config.tiled_distribution = (val == "1");
			else if (key == "cfg_simd") current.config.simd_pipeline = (val == "1");
			else if (key == "cfg_width") current.config.screen_width = static_cast<uint32_t>(std::strtoul(val.c_str(), nullptr, 10));
			else if (key == "cfg_height") current.config.screen_height = static_cast<uint32_t>(std::strtoul(val.c_str(), nullptr, 10));
			else if (key == "cfg_step_controller") current.config.step_controller_mode = static_cast<uint32_t>(std::strtoul(val.c_str(), nullptr, 10));
			else if (key == "ft_mean") current.frame_time_summary.mean = std::strtod(val.c_str(), nullptr);
			else if (key == "ft_median") current.frame_time_summary.median = std::strtod(val.c_str(), nullptr);
			else if (key == "ft_min") current.frame_time_summary.min_value = std::strtod(val.c_str(), nullptr);
			else if (key == "ft_max") current.frame_time_summary.max_value = std::strtod(val.c_str(), nullptr);
			else if (key == "ft_std") current.frame_time_summary.std_deviation = std::strtod(val.c_str(), nullptr);
			else if (key == "ft_p95") current.frame_time_summary.percentile_95 = std::strtod(val.c_str(), nullptr);
			else if (key == "ft_p99") current.frame_time_summary.percentile_99 = std::strtod(val.c_str(), nullptr);
			else if (key == "fps_mean") current.fps_summary.mean = std::strtod(val.c_str(), nullptr);
			else if (key == "fps_min") current.fps_summary.min_value = std::strtod(val.c_str(), nullptr);
			else if (key == "fps_max") current.fps_summary.max_value = std::strtod(val.c_str(), nullptr);
			else if (key == "avg_iterations") current.average_iterations = std::strtod(val.c_str(), nullptr);
			else if (key == "gpu_ratio") current.gpu_path_ratio = std::strtod(val.c_str(), nullptr);
			else if (key == "horizon_ratio") current.horizon_hit_ratio = std::strtod(val.c_str(), nullptr);
			else if (key == "celestial_ratio") current.celestial_hit_ratio = std::strtod(val.c_str(), nullptr);
			else if (key == "disk_ratio") current.disk_hit_ratio = std::strtod(val.c_str(), nullptr);
			else if (key == "sample_count") current.sample_count = static_cast<size_t>(std::strtoull(val.c_str(), nullptr, 10));
			else if (key == "duration") current.capture_duration_seconds = std::strtod(val.c_str(), nullptr);
		}
	}
};

}
