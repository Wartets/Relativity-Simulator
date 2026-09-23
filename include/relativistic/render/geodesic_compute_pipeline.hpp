#pragma once

#include "relativistic/render/gpu_types.hpp"
#include "relativistic/render/vulkan_context.hpp"
#include "relativistic/render/vulkan_compute_executor.hpp"
#include "relativistic/render/software_compute_engine.hpp"
#include "relativistic/core/thread_pool.hpp"
#include "relativistic/core/engine_log.hpp"
#include <vector>
#include <span>
#include <memory>
#include <chrono>
#include <cmath>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <algorithm>
#include <limits>

namespace Relativistic::Render {

struct GeodesicPipelineConfig {
	uint32_t width{3840};
	uint32_t height{2160};
	PrecisionMode precision{PrecisionMode::NativeFloat64};
	MetricId metric{MetricId::Schwarzschild};
	double field_of_view_deg{60.0};
	uint32_t max_steps{2048};
	double initial_step{-0.05};
	bool headless{true};
	Observer::ProjectionMode projection_mode{Observer::ProjectionMode::Pinhole};
};

struct PipelineExecutionTelemetry {
	double execution_time_ms{0.0};
	double frame_rate_fps{0.0};
	uint64_t total_pixels_processed{0};
	uint64_t horizon_pixels_absorbed{0};
	uint64_t celestial_pixels_hit{0};
	uint64_t accretion_disk_pixels_hit{0};
	uint64_t saturated_ray_pixels{0};
	double average_iterations_used{0.0};
	uint32_t min_iterations_used{0};
	uint32_t max_iterations_used{0};
	bool used_gpu_path{false};
	double tile_prepass_skip_ms{0.0};
	double full_raytrace_tiles_ms{0.0};
	uint64_t tile_prepass_skip_tile_count{0};
	uint64_t full_raytrace_tile_count{0};
	double pixel_classification_ms{0.0};
};

class GeodesicComputePipeline {
private:
	GeodesicPipelineConfig config_;
	VulkanContext context_;
	std::vector<GpuPixelOutput> front_buffer_;
	std::vector<GpuPixelOutput> back_buffer_;
	PipelineExecutionTelemetry telemetry_{};

	mutable std::mutex mutex_;
	std::condition_variable cv_;
	std::atomic<bool> is_running_{true};
	std::atomic<bool> request_pending_{false};
	std::atomic<bool> new_frame_ready_{false};
	std::atomic<bool> is_rendering_{false};
	std::atomic<bool> cancel_render_{false};
	uint32_t rendered_width_{3840};
	uint32_t rendered_height_{2160};

	GpuCameraPushConstants pending_constants_{};
	std::vector<GpuBodyData> pending_bodies_{};
	std::unique_ptr<Core::ThreadPool> thread_pool_{};
	std::unique_ptr<VulkanComputeExecutor> gpu_executor_{};
	std::atomic<bool> use_gpu_compute_{false};
	std::jthread worker_thread_;

	[[nodiscard]] static bool is_metric_gpu_accelerable(const GpuCameraPushConstants& params) noexcept {
		switch (params.metric_type) {
			case 0U: case 1U: case 2U: case 4U: case 5U: case 6U:
				return true;
			default:
				return false;
		}
	}

	[[nodiscard]] bool try_gpu_dispatch(const GpuCameraPushConstants& params, std::vector<GpuPixelOutput>& output) noexcept {
		if (gpu_executor_ == nullptr || !gpu_executor_->is_ready()) {
			return false;
		}
		if (config_.precision != PrecisionMode::NativeFloat64) {
			return false;
		}
		if (!is_metric_gpu_accelerable(params)) {
			return false;
		}
		if (params.interlace_mode != 0U) {
			return false;
		}
		return gpu_executor_->dispatch_and_readback(params, output);
	}

	void worker_loop(std::stop_token st) noexcept {
		while (!st.stop_requested() && is_running_.load(std::memory_order_relaxed)) {
			GpuCameraPushConstants current_job;
			std::vector<GpuBodyData> current_bodies;
			{
				std::unique_lock<std::mutex> lock(mutex_);
				cv_.wait(lock, [&]() {
					return st.stop_requested() || !is_running_.load(std::memory_order_relaxed) || request_pending_.load(std::memory_order_relaxed);
				});

				if (st.stop_requested() || !is_running_.load(std::memory_order_relaxed)) {
					break;
				}

				current_job = pending_constants_;
				current_bodies = pending_bodies_;
				request_pending_.store(false, std::memory_order_relaxed);
				is_rendering_.store(true, std::memory_order_relaxed);
			}

			const size_t req_pixels = static_cast<size_t>(current_job.screen_width) * static_cast<size_t>(current_job.screen_height);
			if (back_buffer_.size() != req_pixels) {
				back_buffer_.assign(req_pixels, GpuPixelOutput{});
			}

			cancel_render_.store(false, std::memory_order_relaxed);
			const auto t_start = std::chrono::high_resolution_clock::now();

			bool rendered_on_gpu = false;
			SoftwareComputeEngine::RenderStageStats stage_stats{};
			if (use_gpu_compute_.load(std::memory_order_relaxed) && current_bodies.empty()) {
				std::vector<GpuPixelOutput> gpu_output;
				if (try_gpu_dispatch(current_job, gpu_output) && gpu_output.size() == req_pixels) {
					back_buffer_ = std::move(gpu_output);
					rendered_on_gpu = true;
				}
			}

			if (!rendered_on_gpu) {
				if (config_.precision == PrecisionMode::NativeFloat64) {
					SoftwareComputeEngine::dispatch_fp64(current_job, back_buffer_, current_bodies, thread_pool_.get(), &cancel_render_, &stage_stats);
				} else {
					SoftwareComputeEngine::dispatch_double_single(current_job, back_buffer_, current_bodies, thread_pool_.get(), &cancel_render_, &stage_stats);
				}
			}
			const auto t_end = std::chrono::high_resolution_clock::now();
			if (!rendered_on_gpu && cancel_render_.load(std::memory_order_relaxed)) {
				is_rendering_.store(false, std::memory_order_relaxed);
				continue;
			}
			const double duration_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();

			const auto classify_t0 = std::chrono::high_resolution_clock::now();
			uint64_t absorbed = 0;
			uint64_t celestial = 0;
			uint64_t disk_hits = 0;
			uint64_t saturated = 0;
			double iteration_sum = 0.0;
			uint32_t iter_min = std::numeric_limits<uint32_t>::max();
			uint32_t iter_max = 0;
			for (const auto& px : back_buffer_) {
				if (px.status_flags == PixelFlags::HORIZON_ABSORBED) ++absorbed;
				else if (px.status_flags == PixelFlags::CELESTIAL_HIT) ++celestial;
				if ((px.status_flags & PixelFlags::ACCRETION_DISK_HIT) != 0U) ++disk_hits;
				if ((px.status_flags & (PixelFlags::HORIZON_ABSORBED | PixelFlags::CELESTIAL_HIT)) == 0U) ++saturated;
				iteration_sum += static_cast<double>(px.iterations_used);
				iter_min = std::min(iter_min, px.iterations_used);
				iter_max = std::max(iter_max, px.iterations_used);
			}
			if (back_buffer_.empty()) {
				iter_min = 0;
			}
			const auto classify_t1 = std::chrono::high_resolution_clock::now();
			const double classification_ms = std::chrono::duration<double, std::milli>(classify_t1 - classify_t0).count();

			{
				std::lock_guard<std::mutex> lock(mutex_);
				front_buffer_ = back_buffer_;
				rendered_width_ = current_job.screen_width;
				rendered_height_ = current_job.screen_height;
				telemetry_.execution_time_ms = duration_ms;
				telemetry_.frame_rate_fps = (duration_ms > 0.0) ? (1000.0 / duration_ms) : 0.0;
				telemetry_.total_pixels_processed = req_pixels;
				telemetry_.horizon_pixels_absorbed = absorbed;
				telemetry_.celestial_pixels_hit = celestial;
				telemetry_.used_gpu_path = rendered_on_gpu;
				telemetry_.accretion_disk_pixels_hit = disk_hits;
				telemetry_.saturated_ray_pixels = saturated;
				telemetry_.average_iterations_used = (req_pixels > 0) ? (iteration_sum / static_cast<double>(req_pixels)) : 0.0;
				telemetry_.min_iterations_used = iter_min;
				telemetry_.max_iterations_used = iter_max;
				telemetry_.tile_prepass_skip_ms = static_cast<double>(stage_stats.tile_prepass_skip_ns.load(std::memory_order_relaxed)) / 1.0e6;
				telemetry_.full_raytrace_tiles_ms = static_cast<double>(stage_stats.full_trace_ns.load(std::memory_order_relaxed)) / 1.0e6;
				telemetry_.tile_prepass_skip_tile_count = stage_stats.tile_prepass_skip_count.load(std::memory_order_relaxed);
				telemetry_.full_raytrace_tile_count = stage_stats.full_trace_tile_count.load(std::memory_order_relaxed);
				telemetry_.pixel_classification_ms = classification_ms;
				new_frame_ready_.store(true, std::memory_order_release);
				is_rendering_.store(false, std::memory_order_relaxed);
			}
		}
	}

public:
	explicit GeodesicComputePipeline(const GeodesicPipelineConfig& config = {})
		: config_(config),
		  thread_pool_(std::make_unique<Core::ThreadPool>()) {
		front_buffer_.resize(config_.width * config_.height);
		back_buffer_.resize(config_.width * config_.height);
		static_cast<void>(context_.initialize(config_.headless, config_.precision == PrecisionMode::NativeFloat64));

		if (context_.has_compute_device()) {
			auto candidate_executor = std::make_unique<VulkanComputeExecutor>();
			if (candidate_executor->initialize(context_)) {
				gpu_executor_ = std::move(candidate_executor);
			} else {
				Core::log_warning("Vulkan compute device detected but pipeline initialization failed; falling back to CPU rendering.");
			}
		} else {
			Core::log_warning("No Vulkan compute-capable device detected; GPU offload is unavailable for this session.");
		}

		if (!config_.headless) {
			worker_thread_ = std::jthread([this](std::stop_token st) {
				worker_loop(st);
			});
		}
	}

	~GeodesicComputePipeline() noexcept {
		is_running_.store(false, std::memory_order_release);
		cancel_render_.store(true, std::memory_order_release);
		cv_.notify_all();
	}

	void resize(uint32_t width, uint32_t height) {
		std::lock_guard<std::mutex> lock(mutex_);
		config_.width = width;
		config_.height = height;
	}

	void set_precision_mode(PrecisionMode mode) noexcept {
		config_.precision = mode;
	}

	void set_projection_mode(Observer::ProjectionMode mode) noexcept {
		config_.projection_mode = mode;
	}

	[[nodiscard]] const GeodesicPipelineConfig& config() const noexcept {
		return config_;
	}

	[[nodiscard]] const VulkanContext& context() const noexcept {
		return context_;
	}

	void copy_framebuffer(std::vector<GpuPixelOutput>& dst, uint32_t& out_w, uint32_t& out_h) const {
		std::lock_guard<std::mutex> lock(mutex_);
		dst = front_buffer_;
		out_w = rendered_width_;
		out_h = rendered_height_;
	}

	[[nodiscard]] std::span<const GpuPixelOutput> framebuffer() const noexcept {
		return front_buffer_;
	}

	[[nodiscard]] bool check_and_clear_new_frame() noexcept {
		return new_frame_ready_.exchange(false, std::memory_order_acq_rel);
	}

	[[nodiscard]] bool is_rendering() const noexcept {
		return is_rendering_.load(std::memory_order_relaxed);
	}

	[[nodiscard]] const PipelineExecutionTelemetry& telemetry() const noexcept {
		return telemetry_;
	}

	void set_gpu_compute_enabled(bool enabled) noexcept {
		use_gpu_compute_.store(enabled, std::memory_order_relaxed);
	}

	[[nodiscard]] bool gpu_compute_enabled() const noexcept {
		return use_gpu_compute_.load(std::memory_order_relaxed);
	}

	[[nodiscard]] bool gpu_compute_available() const noexcept {
		return gpu_executor_ != nullptr && gpu_executor_->is_ready();
	}

	void dispatch(const GpuCameraPushConstants& camera_constants, std::span<const GpuBodyData> bodies = {}) {
		if (config_.headless) {
			GpuCameraPushConstants actual_constants = camera_constants;
			actual_constants.projection_mode = static_cast<uint32_t>(config_.projection_mode);

			bool rendered_on_gpu = false;
			SoftwareComputeEngine::RenderStageStats headless_stage_stats{};
			if (use_gpu_compute_.load(std::memory_order_relaxed) && bodies.empty()) {
				const size_t total_pixels = static_cast<size_t>(actual_constants.screen_width) * static_cast<size_t>(actual_constants.screen_height);
				if (front_buffer_.size() >= total_pixels) {
					std::vector<GpuPixelOutput> gpu_output;
					if (try_gpu_dispatch(actual_constants, gpu_output) && gpu_output.size() == total_pixels) {
						std::copy(gpu_output.begin(), gpu_output.end(), front_buffer_.begin());
						rendered_on_gpu = true;
					}
				}
			}

			if (!rendered_on_gpu) {
				if (config_.precision == PrecisionMode::NativeFloat64) {
					SoftwareComputeEngine::dispatch_fp64(actual_constants, front_buffer_, bodies, thread_pool_.get(), nullptr, &headless_stage_stats);
				} else {
					SoftwareComputeEngine::dispatch_double_single(actual_constants, front_buffer_, bodies, thread_pool_.get(), nullptr, &headless_stage_stats);
				}
			}
			telemetry_.used_gpu_path = rendered_on_gpu;
			telemetry_.tile_prepass_skip_ms = static_cast<double>(headless_stage_stats.tile_prepass_skip_ns.load(std::memory_order_relaxed)) / 1.0e6;
			telemetry_.full_raytrace_tiles_ms = static_cast<double>(headless_stage_stats.full_trace_ns.load(std::memory_order_relaxed)) / 1.0e6;
			telemetry_.tile_prepass_skip_tile_count = headless_stage_stats.tile_prepass_skip_count.load(std::memory_order_relaxed);
			telemetry_.full_raytrace_tile_count = headless_stage_stats.full_trace_tile_count.load(std::memory_order_relaxed);
			new_frame_ready_.store(true, std::memory_order_release);
			return;
		}

		{
			std::lock_guard<std::mutex> lock(mutex_);
			if (is_rendering_.load(std::memory_order_relaxed)) {
				cancel_render_.store(true, std::memory_order_relaxed);
			}
			pending_constants_ = camera_constants;
			pending_constants_.projection_mode = static_cast<uint32_t>(config_.projection_mode);
			pending_bodies_.assign(bodies.begin(), bodies.end());
			request_pending_.store(true, std::memory_order_release);
		}
		cv_.notify_one();
	}
};

}
