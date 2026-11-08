#pragma once

#include "relativistic/render/gpu_types.hpp"
#include "relativistic/render/vulkan_context.hpp"
#include "relativistic/render/vulkan_compute_executor.hpp"
#include "relativistic/render/software_compute_engine.hpp"
#include "relativistic/render/bodies/body_render_guard.hpp"
#include "relativistic/render/render_diagnostics.hpp"
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

#include "relativistic/distributed/cluster_telemetry.hpp"

#if defined(RELATIVISTIC_DISTRIBUTED_COMPUTE) && RELATIVISTIC_DISTRIBUTED_COMPUTE
#include "relativistic/distributed/cluster_orchestrator.hpp"
#include "relativistic/distributed/gpu_nbody_executor.hpp"
#if defined(RELATIVISTIC_PIPELINE_TILE_SPIRV)
#include "relativistic/shaders/cluster_tile_tracer_fp64.spv.hpp"
#endif
#endif

namespace Relativistic::Distributed {

class ClusterOrchestrator;
class GpuNBodyExecutor;

}

namespace Relativistic::Render {

class DistributedComputeExtension {
private:
	std::atomic<Distributed::ClusterOrchestrator*> cluster_{nullptr};

public:
	void bind_cluster(Distributed::ClusterOrchestrator& cluster) noexcept {
		cluster_.store(&cluster, std::memory_order_release);
	}

	void unbind_cluster() noexcept {
		cluster_.store(nullptr, std::memory_order_release);
	}

	[[nodiscard]] bool is_cluster_bound() const noexcept {
		return cluster_.load(std::memory_order_acquire) != nullptr;
	}

	[[nodiscard]] bool interactive_routing_enabled() const noexcept {
#if defined(RELATIVISTIC_DISTRIBUTED_COMPUTE) && RELATIVISTIC_DISTRIBUTED_COMPUTE
		const auto* cluster = cluster_.load(std::memory_order_acquire);
		return cluster != nullptr && cluster->interactive_routing_enabled();
#else
		return false;
#endif
	}

	void set_interactive_routing(bool enabled) noexcept {
#if defined(RELATIVISTIC_DISTRIBUTED_COMPUTE) && RELATIVISTIC_DISTRIBUTED_COMPUTE
		if (auto* cluster = cluster_.load(std::memory_order_acquire)) {
			cluster->set_interactive_routing(enabled);
		}
#else
		static_cast<void>(enabled);
#endif
	}

	[[nodiscard]] bool exact_metrics_routing_enabled() const noexcept {
#if defined(RELATIVISTIC_DISTRIBUTED_COMPUTE) && RELATIVISTIC_DISTRIBUTED_COMPUTE
		const auto* cluster = cluster_.load(std::memory_order_acquire);
		return cluster != nullptr && cluster->exact_metrics_routing_enabled();
#else
		return false;
#endif
	}

	void set_exact_metrics_routing(bool enabled) noexcept {
#if defined(RELATIVISTIC_DISTRIBUTED_COMPUTE) && RELATIVISTIC_DISTRIBUTED_COMPUTE
		if (auto* cluster = cluster_.load(std::memory_order_acquire)) {
			cluster->set_exact_metrics_routing(enabled);
		}
#else
		static_cast<void>(enabled);
#endif
	}

	[[nodiscard]] bool offline_routing_enabled() const noexcept {
#if defined(RELATIVISTIC_DISTRIBUTED_COMPUTE) && RELATIVISTIC_DISTRIBUTED_COMPUTE
		const auto* cluster = cluster_.load(std::memory_order_acquire);
		return cluster != nullptr && cluster->offline_routing_enabled();
#else
		return false;
#endif
	}

	void set_offline_routing(bool enabled) noexcept {
#if defined(RELATIVISTIC_DISTRIBUTED_COMPUTE) && RELATIVISTIC_DISTRIBUTED_COMPUTE
		if (auto* cluster = cluster_.load(std::memory_order_acquire)) {
			cluster->set_offline_routing(enabled);
		}
#else
		static_cast<void>(enabled);
#endif
	}

	[[nodiscard]] bool nbody_routing_enabled() const noexcept {
#if defined(RELATIVISTIC_DISTRIBUTED_COMPUTE) && RELATIVISTIC_DISTRIBUTED_COMPUTE
		const auto* cluster = cluster_.load(std::memory_order_acquire);
		return cluster != nullptr && cluster->nbody_routing_enabled();
#else
		return false;
#endif
	}

	void set_nbody_routing(bool enabled) noexcept {
#if defined(RELATIVISTIC_DISTRIBUTED_COMPUTE) && RELATIVISTIC_DISTRIBUTED_COMPUTE
		if (auto* cluster = cluster_.load(std::memory_order_acquire)) {
			cluster->set_nbody_routing(enabled);
		}
#else
		static_cast<void>(enabled);
#endif
	}

	[[nodiscard]] bool monte_carlo_routing_enabled() const noexcept {
#if defined(RELATIVISTIC_DISTRIBUTED_COMPUTE) && RELATIVISTIC_DISTRIBUTED_COMPUTE
		const auto* cluster = cluster_.load(std::memory_order_acquire);
		return cluster != nullptr && cluster->monte_carlo_routing_enabled();
#else
		return false;
#endif
	}

	void set_monte_carlo_routing(bool enabled) noexcept {
#if defined(RELATIVISTIC_DISTRIBUTED_COMPUTE) && RELATIVISTIC_DISTRIBUTED_COMPUTE
		if (auto* cluster = cluster_.load(std::memory_order_acquire)) {
			cluster->set_monte_carlo_routing(enabled);
		}
#else
		static_cast<void>(enabled);
#endif
	}

	[[nodiscard]] uint32_t workers_active() const {
#if defined(RELATIVISTIC_DISTRIBUTED_COMPUTE) && RELATIVISTIC_DISTRIBUTED_COMPUTE
		const auto* cluster = cluster_.load(std::memory_order_acquire);
		if (cluster != nullptr) {
			return cluster->workers_active();
		}
#endif
		return 0U;
	}

	[[nodiscard]] Distributed::ClusterFrameTelemetry last_frame_telemetry() const {
#if defined(RELATIVISTIC_DISTRIBUTED_COMPUTE) && RELATIVISTIC_DISTRIBUTED_COMPUTE
		const auto* cluster = cluster_.load(std::memory_order_acquire);
		if (cluster != nullptr) {
			return cluster->last_frame_telemetry();
		}
#endif
		return Distributed::ClusterFrameTelemetry{};
	}

	void detach_local_renderer() noexcept {
#if defined(RELATIVISTIC_DISTRIBUTED_COMPUTE) && RELATIVISTIC_DISTRIBUTED_COMPUTE
		if (auto* cluster = cluster_.load(std::memory_order_acquire)) {
			cluster->clear_local_tile_renderer();
			cluster->clear_local_nbody_evaluator();
		}
#endif
	}

	[[nodiscard]] bool is_cluster_active() const noexcept {
#if defined(RELATIVISTIC_DISTRIBUTED_COMPUTE) && RELATIVISTIC_DISTRIBUTED_COMPUTE
		const auto* cluster = cluster_.load(std::memory_order_acquire);
		return cluster != nullptr && cluster->is_cluster_operational();
#else
		return false;
#endif
	}

	bool dispatch_distributed(
		const GpuCameraPushConstants& constants,
		std::span<GpuPixelOutput> framebuffer,
		std::span<const GpuBodyData> bodies,
		const std::atomic<bool>* cancel_flag
	) {
#if defined(RELATIVISTIC_DISTRIBUTED_COMPUTE) && RELATIVISTIC_DISTRIBUTED_COMPUTE
		auto* cluster = cluster_.load(std::memory_order_acquire);
		if (cluster == nullptr) {
			return false;
		}
		return cluster->dispatch_distributed(constants, framebuffer, bodies, cancel_flag);
#else
		static_cast<void>(constants);
		static_cast<void>(framebuffer);
		static_cast<void>(bodies);
		static_cast<void>(cancel_flag);
		return false;
#endif
	}

	#if defined(RELATIVISTIC_DISTRIBUTED_COMPUTE) && RELATIVISTIC_DISTRIBUTED_COMPUTE
	[[nodiscard]] bool evaluate_nbody_accelerations(
		std::span<const Distributed::NBodyBodyState> bodies,
		const Distributed::NBodyEvaluationOptions& options,
		std::span<Distributed::NBodyRowResult> output,
		const std::atomic<bool>* cancel_flag = nullptr
	) {
		auto* cluster = cluster_.load(std::memory_order_acquire);
		if (cluster != nullptr && cluster->workers_active() > 0U) {
			return cluster->evaluate_nbody_accelerations(bodies, options, output, cancel_flag);
		}
		return Distributed::evaluate_nbody_accelerations_local(bodies, options, output);
	}

	[[nodiscard]] bool run_geodesic_ensemble(
		const Distributed::GeodesicEnsembleSpec& spec,
		uint32_t samples_per_unit,
		Distributed::GeodesicEnsemblePartial& result,
		const std::atomic<bool>* cancel_flag = nullptr
	) {
		auto* cluster = cluster_.load(std::memory_order_acquire);
		if (cluster != nullptr && cluster->workers_active() > 0U) {
			return cluster->run_geodesic_ensemble(spec, samples_per_unit, result, cancel_flag);
		}
		return Distributed::run_geodesic_ensemble_local(spec, samples_per_unit, result);
	}

	[[nodiscard]] bool evaluate_stochastic_pce_quadrature(
		const Distributed::StochasticEvaluationSpec& spec,
		uint32_t points_per_unit,
		std::span<Distributed::StochasticQuadratureResult> results,
		const std::atomic<bool>* cancel_flag = nullptr
	) {
		auto* cluster = cluster_.load(std::memory_order_acquire);
		if (cluster != nullptr && cluster->workers_active() > 0U) {
			return cluster->evaluate_stochastic_pce_quadrature(spec, points_per_unit, results, cancel_flag);
		}
		return Distributed::run_stochastic_pce_quadrature_local(spec, points_per_unit, results);
	}
#else
	[[nodiscard]] bool evaluate_nbody_accelerations(
		std::span<const void*> bodies,
		const void* options,
		std::span<void*> output,
		const std::atomic<bool>* cancel_flag = nullptr
	) noexcept {
		static_cast<void>(bodies);
		static_cast<void>(options);
		static_cast<void>(output);
		static_cast<void>(cancel_flag);
		return false;
	}

	[[nodiscard]] bool run_geodesic_ensemble(
		const void* spec,
		uint32_t samples_per_unit,
		void* result,
		const std::atomic<bool>* cancel_flag = nullptr
	) noexcept {
		static_cast<void>(spec);
		static_cast<void>(samples_per_unit);
		static_cast<void>(result);
		static_cast<void>(cancel_flag);
		return false;
	}

	[[nodiscard]] bool evaluate_stochastic_pce_quadrature(
		const void* spec,
		uint32_t points_per_unit,
		std::span<void*> results,
		const std::atomic<bool>* cancel_flag = nullptr
	) noexcept {
		static_cast<void>(spec);
		static_cast<void>(points_per_unit);
		static_cast<void>(results);
		static_cast<void>(cancel_flag);
		return false;
	}
#endif

	bool dispatch_capture(
		const GpuCameraPushConstants& constants,
		std::span<GpuPixelOutput> pixels,
		uint32_t first_row,
		uint32_t row_count,
		std::span<const GpuBodyData> bodies,
		const std::atomic<bool>* cancel_flag
	) {
#if defined(RELATIVISTIC_DISTRIBUTED_COMPUTE) && RELATIVISTIC_DISTRIBUTED_COMPUTE
		auto* cluster = cluster_.load(std::memory_order_acquire);
		if (cluster == nullptr) {
			return false;
		}
		const Distributed::FrameWindow window{pixels, constants.screen_width, constants.screen_height, first_row, row_count};
		return cluster->dispatch_frame(constants, window, bodies, Distributed::DispatchProfile::Offline, cancel_flag);
#else
		static_cast<void>(constants);
		static_cast<void>(pixels);
		static_cast<void>(first_row);
		static_cast<void>(row_count);
		static_cast<void>(bodies);
		static_cast<void>(cancel_flag);
		return false;
#endif
	}
};

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
	bool bodies_patched_over_gpu_background{false};
	bool bodies_rendered_on_gpu{false};
	uint32_t bodies_sent_this_frame{0};
	uint32_t bodies_total_enabled_this_frame{0};
	double tile_prepass_skip_ms{0.0};
	double full_raytrace_tiles_ms{0.0};
	uint64_t tile_prepass_skip_tile_count{0};
	uint64_t full_raytrace_tile_count{0};
	double pixel_classification_ms{0.0};
	uint64_t body_tile_count{0};
	uint64_t body_tile_total_count{0};
	bool used_cluster_path{false};
	Distributed::ClusterFrameTelemetry cluster{};
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
	std::unique_ptr<Core::ThreadPool, void(*)(Core::ThreadPool*)> thread_pool_{nullptr, [](Core::ThreadPool*) noexcept {}};
	std::unique_ptr<VulkanComputeExecutor> gpu_executor_{};
	std::unique_ptr<VulkanComputeExecutor> tile_executor_{};
#if defined(RELATIVISTIC_DISTRIBUTED_COMPUTE) && RELATIVISTIC_DISTRIBUTED_COMPUTE
	std::unique_ptr<Distributed::GpuNBodyExecutor> nbody_executor_{};
#endif
	std::atomic<bool> use_gpu_compute_{false};
	std::mutex gpu_mutex_;
	std::atomic<PrecisionMode> precision_mode_{PrecisionMode::NativeFloat64};
	std::atomic<uint32_t> pending_total_enabled_bodies_{0};
	std::atomic<uint64_t> last_guard_signature_{0};
	std::atomic<int> last_render_route_{-1};
	std::jthread worker_thread_;
	static constexpr size_t kMaxGpuBackgroundBodies = 512;
	DistributedComputeExtension cluster_extension_{};

	[[nodiscard]] std::vector<GpuBodyData> guarded_bodies(std::span<const GpuBodyData> bodies) {
		BodyGuardReport report;
		std::vector<GpuBodyData> safe = BodyRenderGuard::sanitized_copy(bodies, &report);
		if (report.clean()) {
			last_guard_signature_.store(0, std::memory_order_release);
			return safe;
		}
		const uint64_t signature = (static_cast<uint64_t>(report.repaired) << 32U) ^ static_cast<uint64_t>(report.discarded) ^ 0x8000000000000000ULL;
		if (last_guard_signature_.exchange(signature, std::memory_order_acq_rel) != signature) {
			try {
				Core::log_warning("Body render guard: " + std::to_string(report.repaired) + " of " + std::to_string(report.inspected) + " bodies had invalid values repaired, " + std::to_string(report.discarded) + " were discarded.");
			} catch (...) {
			}
		}
		return safe;
	}

	void note_render_route(bool gpu) noexcept {
		const int route = gpu ? 1 : 0;
		if (last_render_route_.exchange(route, std::memory_order_acq_rel) == route) {
			return;
		}
		try {
			Core::log_info(gpu ? "Render path changed to the Vulkan compute dispatch." : "Render path changed to the CPU software renderer.");
		} catch (...) {
		}
	}

	[[nodiscard]] static bool is_metric_gpu_accelerable(const GpuCameraPushConstants& params) noexcept {
		switch (params.metric_type) {
			case 6U:
				if (std::abs(params.cosmological_lambda) > 1e-18) {
					return false;
				}
				[[fallthrough]];
			case 0U: case 1U: case 2U: case 4U: case 5U: case 7U: case 8U: case 9U:
			case 10U: case 11U: case 12U: case 13U: case 14U: case 15U: case 16U:
				return true;
			default:
				return false;
		}
	}

	void dispatch_software(
		const GpuCameraPushConstants& constants,
		std::vector<GpuPixelOutput>& output,
		std::span<const GpuBodyData> bodies,
		Core::ThreadPool* pool,
		const std::atomic<bool>* cancel,
		SoftwareComputeEngine::RenderStageStats* stats
	) const {
		if (precision_mode_.load(std::memory_order_relaxed) == PrecisionMode::NativeFloat64) {
			SoftwareComputeEngine::dispatch_fp64(constants, output, bodies, pool, cancel, stats);
		} else {
			SoftwareComputeEngine::dispatch_double_single(constants, output, bodies, pool, cancel, stats);
		}
	}

	void await_earth_textures(std::span<const GpuBodyData> bodies) noexcept {
		const auto requirements = EarthTextureRequirements::gather(bodies);
		if (!requirements.any()) {
			return;
		}
		constexpr std::chrono::seconds timeout{30};
		try {
			if (use_gpu_compute_.load(std::memory_order_relaxed) && gpu_executor_ != nullptr && gpu_executor_->is_ready()) {
				std::lock_guard<std::mutex> gpu_lock(gpu_mutex_);
				static_cast<void>(gpu_executor_->await_earth_textures(requirements, timeout));
				return;
			}
			auto& loader = Optics::EarthTextureLoader::instance();
			if (requirements.day) {
				static_cast<void>(loader.await(Optics::EarthMapKind::Day, requirements.quality, timeout));
			}
			if (requirements.night) {
				static_cast<void>(loader.await(Optics::EarthMapKind::Night, requirements.quality, timeout));
			}
		} catch (...) {
			Core::log_error("Waiting for the Earth textures failed; the capture continues with the fallback surface color.");
		}
	}

	[[nodiscard]] bool try_gpu_dispatch(const GpuCameraPushConstants& params, std::vector<GpuPixelOutput>& output, std::span<const GpuBodyData> bodies, const std::atomic<bool>* cancel = nullptr) noexcept {
		if (gpu_executor_ == nullptr || !gpu_executor_->is_ready()) {
			return false;
		}
		if (!bodies.empty() && SoftwareComputeEngine::requires_exact_metric_path(params)) {
			return false;
		}
		if (precision_mode_.load(std::memory_order_relaxed) != PrecisionMode::NativeFloat64) {
			return false;
		}
		if (!is_metric_gpu_accelerable(params)) {
			return false;
		}
		if (params.interlace_mode != 0U) {
			return false;
		}
		if (bodies.size() > kMaxGpuBackgroundBodies) {
			return false;
		}
		std::lock_guard<std::mutex> gpu_lock(gpu_mutex_);
		if (params.jet.enabled > 0.5f && params.jet.emission_model > 0.5f) {
			return false;
		}
		GpuCameraPushConstants gpu_params = params;
		bool dispatched = false;
		try {
			std::vector<GpuBodyGpuLayout> gpu_bodies_layout;
			gpu_bodies_layout.reserve(bodies.size());
			for (const auto& body : bodies) {
				if (!std::isfinite(body.position[0]) || !std::isfinite(body.position[1]) || !std::isfinite(body.position[2]) || !std::isfinite(body.radius) || !std::isfinite(body.mass)) {
					continue;
				}
				gpu_bodies_layout.push_back(GpuBodyGpuLayout::from(body));
			}
			dispatched = gpu_executor_->dispatch_and_readback(gpu_params, output, gpu_bodies_layout, cancel);
		} catch (const std::exception& ex) {
			Core::log_error(std::string("GPU dispatch threw an exception and was aborted: ") + ex.what());
		} catch (...) {
			Core::log_error("GPU dispatch threw an unknown exception and was aborted.");
		}
		if (!dispatched && !gpu_executor_->is_ready()) {
			Core::log_error("GPU compute dispatch timed out or the device was lost; falling back to the CPU renderer for subsequent frames.");
			use_gpu_compute_.store(false, std::memory_order_relaxed);
		}
		return dispatched;
	}

	void worker_loop(std::stop_token st) noexcept {
		Core::Diagnostics::set_thread_name("render-worker");
		while (!st.stop_requested() && is_running_.load(std::memory_order_relaxed)) {
			GpuCameraPushConstants current_job;
			std::vector<GpuBodyData> current_bodies;
			PrecisionMode current_precision = PrecisionMode::NativeFloat64;
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
			current_precision = precision_mode_.load(std::memory_order_relaxed);

			const size_t req_pixels = static_cast<size_t>(current_job.screen_width) * static_cast<size_t>(current_job.screen_height);
			bool frame_allocation_ok = true;
			try {
				if (back_buffer_.size() != req_pixels) {
					back_buffer_.assign(req_pixels, GpuPixelOutput{});
				}
			} catch (const std::exception& ex) {
				Core::log_error(std::string("Render frame buffer allocation failed, skipping frame: ") + ex.what());
				frame_allocation_ok = false;
			} catch (...) {
				Core::log_error("Render frame buffer allocation failed with an unknown error, skipping frame.");
				frame_allocation_ok = false;
			}

			if (!frame_allocation_ok) {
				is_rendering_.store(false, std::memory_order_relaxed);
				continue;
			}

			cancel_render_.store(false, std::memory_order_relaxed);
			const auto t_start = std::chrono::high_resolution_clock::now();

			bool rendered_on_gpu = false;
			bool bodies_patched_on_top = false;
			SoftwareComputeEngine::RenderStageStats stage_stats{};
			const bool gpu_background_eligible = use_gpu_compute_.load(std::memory_order_relaxed) && current_bodies.size() <= kMaxGpuBackgroundBodies;
			bool dispatch_failed = false;
			bool rendered_on_cluster_path = false;
			Core::Diagnostics::set_activity("render worker: dispatching frame");
			RenderDiagnostics::publish(current_job, current_bodies, gpu_background_eligible ? "vulkan compute preferred" : "cpu software", DispatchState::InFlight, 0.0);
			try {
				bool rendered_on_cluster = false;
				if (cluster_extension_.is_cluster_active() && cluster_extension_.workers_active() > 0U) {
					rendered_on_cluster = cluster_extension_.dispatch_distributed(current_job, back_buffer_, current_bodies, &cancel_render_);
					rendered_on_cluster_path = rendered_on_cluster;
				}

				if (!rendered_on_cluster) {
					if (gpu_background_eligible) {
						if (try_gpu_dispatch(current_job, back_buffer_, current_bodies, &cancel_render_) && back_buffer_.size() == req_pixels) {
							rendered_on_gpu = true;
						}
					}

					if (!rendered_on_gpu) {
						if (current_precision == PrecisionMode::NativeFloat64) {
							SoftwareComputeEngine::dispatch_fp64(current_job, back_buffer_, current_bodies, thread_pool_.get(), &cancel_render_, &stage_stats);
						} else {
							SoftwareComputeEngine::dispatch_double_single(current_job, back_buffer_, current_bodies, thread_pool_.get(), &cancel_render_, &stage_stats);
						}
					}
				}
			} catch (const std::exception& ex) {
				Core::log_error(std::string("Render dispatch threw an exception, frame skipped: ") + ex.what());
				dispatch_failed = true;
			} catch (...) {
				Core::log_error("Render dispatch threw an unknown exception, frame skipped.");
				dispatch_failed = true;
			}

			if (dispatch_failed) {
				RenderDiagnostics::publish(current_job, current_bodies, "dispatch threw an exception", DispatchState::Failed, 0.0);
				is_rendering_.store(false, std::memory_order_relaxed);
				continue;
			}
			const auto t_end = std::chrono::high_resolution_clock::now();
			if (!rendered_on_gpu && cancel_render_.load(std::memory_order_relaxed)) {
				is_rendering_.store(false, std::memory_order_relaxed);
				continue;
			}
			const double duration_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
			note_render_route(rendered_on_gpu);
			RenderDiagnostics::publish(current_job, current_bodies, rendered_on_gpu ? "vulkan compute" : (current_precision == PrecisionMode::NativeFloat64 ? "cpu fp64" : "cpu double-single"), DispatchState::Completed, duration_ms);
			Core::Diagnostics::set_activity("render worker: finishing frame");

			const auto classify_t0 = std::chrono::high_resolution_clock::now();
			struct TelemetryAccumulator {
				uint64_t absorbed{0};
				uint64_t celestial{0};
				uint64_t disk_hits{0};
				uint64_t saturated{0};
				double iteration_sum{0.0};
				uint32_t iter_min{std::numeric_limits<uint32_t>::max()};
				uint32_t iter_max{0};
			};

			uint64_t absorbed = 0;
			uint64_t celestial = 0;
			uint64_t disk_hits = 0;
			uint64_t saturated = 0;
			double iteration_sum = 0.0;
			uint32_t iter_min = std::numeric_limits<uint32_t>::max();
			uint32_t iter_max = 0;

			const size_t total_px = back_buffer_.size();
			if (thread_pool_ && total_px >= 32768) {
				const size_t num_workers = thread_pool_->thread_count();
				std::vector<TelemetryAccumulator> partials(num_workers);
				thread_pool_->parallel_for(num_workers, [&](size_t w_start, size_t w_end) noexcept {
					for (size_t w = w_start; w < w_end; ++w) {
						const size_t chunk_size = (total_px + num_workers - 1) / num_workers;
						const size_t start = w * chunk_size;
						const size_t end = std::min(start + chunk_size, total_px);
						auto& local = partials[w];
						for (size_t i = start; i < end; ++i) {
							const auto& px = back_buffer_[i];
							if (px.status_flags == PixelFlags::HORIZON_ABSORBED) ++local.absorbed;
							else if (px.status_flags == PixelFlags::CELESTIAL_HIT) ++local.celestial;
							if ((px.status_flags & PixelFlags::ACCRETION_DISK_HIT) != 0U) ++local.disk_hits;
							if ((px.status_flags & (PixelFlags::HORIZON_ABSORBED | PixelFlags::CELESTIAL_HIT)) == 0U) ++local.saturated;
							local.iteration_sum += static_cast<double>(px.iterations_used);
							local.iter_min = std::min(local.iter_min, px.iterations_used);
							local.iter_max = std::max(local.iter_max, px.iterations_used);
						}
					}
				}, 1);
				for (const auto& part : partials) {
					absorbed += part.absorbed;
					celestial += part.celestial;
					disk_hits += part.disk_hits;
					saturated += part.saturated;
					iteration_sum += part.iteration_sum;
					iter_min = std::min(iter_min, part.iter_min);
					iter_max = std::max(iter_max, part.iter_max);
				}
			} else {
				for (const auto& px : back_buffer_) {
					if (px.status_flags == PixelFlags::HORIZON_ABSORBED) ++absorbed;
					else if (px.status_flags == PixelFlags::CELESTIAL_HIT) ++celestial;
					if ((px.status_flags & PixelFlags::ACCRETION_DISK_HIT) != 0U) ++disk_hits;
					if ((px.status_flags & (PixelFlags::HORIZON_ABSORBED | PixelFlags::CELESTIAL_HIT)) == 0U) ++saturated;
					iteration_sum += static_cast<double>(px.iterations_used);
					iter_min = std::min(iter_min, px.iterations_used);
					iter_max = std::max(iter_max, px.iterations_used);
				}
			}
			if (back_buffer_.empty()) {
				iter_min = 0;
			}
			const auto classify_t1 = std::chrono::high_resolution_clock::now();
			const double classification_ms = std::chrono::duration<double, std::milli>(classify_t1 - classify_t0).count();

			{
				std::lock_guard<std::mutex> lock(mutex_);
				std::swap(front_buffer_, back_buffer_);
				rendered_width_ = current_job.screen_width;
				rendered_height_ = current_job.screen_height;
				telemetry_.execution_time_ms = duration_ms;
				telemetry_.frame_rate_fps = (duration_ms > 0.0) ? (1000.0 / duration_ms) : 0.0;
				telemetry_.total_pixels_processed = req_pixels;
				telemetry_.horizon_pixels_absorbed = absorbed;
				telemetry_.celestial_pixels_hit = celestial;
				telemetry_.used_gpu_path = rendered_on_gpu;
				telemetry_.bodies_patched_over_gpu_background = bodies_patched_on_top;
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
				telemetry_.body_tile_count = stage_stats.body_tile_count.load(std::memory_order_relaxed);
				telemetry_.body_tile_total_count = stage_stats.total_tile_count.load(std::memory_order_relaxed);
				telemetry_.bodies_rendered_on_gpu = rendered_on_gpu && !current_bodies.empty();
				telemetry_.used_cluster_path = rendered_on_cluster_path;
				telemetry_.cluster = cluster_extension_.last_frame_telemetry();
				telemetry_.bodies_sent_this_frame = static_cast<uint32_t>(current_bodies.size());
				telemetry_.bodies_total_enabled_this_frame = pending_total_enabled_bodies_.load(std::memory_order_relaxed);
				Core::Diagnostics::count_render_frame();
				new_frame_ready_.store(true, std::memory_order_release);
				is_rendering_.store(false, std::memory_order_relaxed);
			}
		}
	}

public:
	explicit GeodesicComputePipeline(const GeodesicPipelineConfig& config = {})
		: config_(config),
		  thread_pool_(&Core::global_render_thread_pool(), [](Core::ThreadPool*) noexcept {}) {
		precision_mode_.store(config_.precision, std::memory_order_relaxed);
		try {
			Startup::note("Allocating framebuffers (" + std::to_string(config_.width) + "x" + std::to_string(config_.height) + ")...");
			front_buffer_.resize(static_cast<size_t>(config_.width) * config_.height);
			back_buffer_.resize(static_cast<size_t>(config_.width) * config_.height);
		} catch (const std::exception& ex) {
			Core::log_error(std::string("Failed to allocate initial render framebuffers, retrying at reduced resolution: ") + ex.what());
			config_.width = std::min(config_.width, 1280U);
			config_.height = std::min(config_.height, 720U);
			front_buffer_.assign(static_cast<size_t>(config_.width) * config_.height, GpuPixelOutput{});
			back_buffer_.assign(static_cast<size_t>(config_.width) * config_.height, GpuPixelOutput{});
		}

		try {
			Startup::note("Initializing the Vulkan compute context...");
			static_cast<void>(context_.initialize(config_.headless, config_.precision == PrecisionMode::NativeFloat64));

			if (context_.has_compute_device()) {
				Startup::note("Initializing the GPU compute executor...");
				auto candidate_executor = std::make_unique<VulkanComputeExecutor>();
				if (candidate_executor->initialize(context_)) {
					gpu_executor_ = std::move(candidate_executor);
					use_gpu_compute_.store(true, std::memory_order_relaxed);
				} else {
					Core::log_warning("Vulkan compute device detected but pipeline initialization failed; falling back to CPU rendering.");
				}
			} else {
				Core::log_warning("No Vulkan compute-capable device detected; GPU offload is unavailable for this session.");
			}
		} catch (const std::exception& ex) {
			Core::log_error(std::string("GPU compute initialization failed, continuing with CPU rendering only: ") + ex.what());
			gpu_executor_.reset();
		} catch (...) {
			Core::log_error("GPU compute initialization failed with an unknown error, continuing with CPU rendering only.");
			gpu_executor_.reset();
		}

		if (!config_.headless) {
			Startup::note("Starting the asynchronous render worker...");
			worker_thread_ = std::jthread([this](std::stop_token st) {
				worker_loop(st);
			});
		}
	}

	~GeodesicComputePipeline() noexcept {
		is_running_.store(false, std::memory_order_release);
		cancel_render_.store(true, std::memory_order_release);
		cv_.notify_all();
		worker_thread_.request_stop();
		if (worker_thread_.joinable()) {
			worker_thread_.join();
		}
	}

	void resize(uint32_t width, uint32_t height) {
		std::lock_guard<std::mutex> lock(mutex_);
		config_.width = std::clamp(width, 16U, 7680U);
		config_.height = std::clamp(height, 16U, 4320U);
	}

	void set_precision_mode(PrecisionMode mode) noexcept {
		precision_mode_.store(mode, std::memory_order_relaxed);
		config_.precision = mode;
	}

	void set_projection_mode(Observer::ProjectionMode mode) noexcept {
		std::lock_guard<std::mutex> lock(mutex_);
		config_.projection_mode = mode;
	}

	private:
#if defined(RELATIVISTIC_DISTRIBUTED_COMPUTE) && RELATIVISTIC_DISTRIBUTED_COMPUTE
	static constexpr size_t kMinimumGpuTileRun = 3;

	void ensure_tile_executor() noexcept {
		if (tile_executor_ != nullptr || !context_.has_compute_device()) {
			return;
		}
		try {
			auto candidate = std::make_unique<VulkanComputeExecutor>();
			VulkanComputeExecutor::ShaderSource source{};
			source.file_name = "cluster_tile_tracer_fp64.comp.spv";
#if defined(RELATIVISTIC_PIPELINE_TILE_SPIRV)
			source.embedded = std::span<const uint32_t>(::cluster_tile_tracer_fp64_spv);
#endif
			source.tile_dispatch = true;
			if (candidate->initialize(context_, source)) {
				tile_executor_ = std::move(candidate);
			} else {
				Core::log_warning("The GPU tile pipeline could not be initialized; local cluster tiles will run on the CPU.");
			}
		} catch (const std::exception& ex) {
			Core::log_error(std::string("GPU tile pipeline initialization failed: ") + ex.what());
			tile_executor_.reset();
		} catch (...) {
			tile_executor_.reset();
		}
	}

	[[nodiscard]] bool render_gpu_tile_run(
		const GpuCameraPushConstants& constants,
		std::span<const uint32_t> run,
		std::span<const GpuBodyGpuLayout> layouts,
		const Distributed::TileCompletionSink& sink,
		const std::atomic<bool>* cancel
	) noexcept {
		if (run.empty()) {
			return true;
		}
		try {
			std::lock_guard<std::mutex> gpu_lock(gpu_mutex_);
			if ((cancel != nullptr && cancel->load(std::memory_order_relaxed)) || tile_executor_ == nullptr || !tile_executor_->is_ready()) {
				return false;
			}
			const uint32_t columns = Distributed::tile_grid_columns(constants.screen_width);
			const uint32_t edge = Distributed::kTileEdgePixels;
			GpuCameraPushConstants params = constants;
			params.interlace_mode = 0U;
			params.interlace_phase = 0U;
			params.dispatch_row_offset = 0U;
			params.dispatch_row_count = 0U;
			params.tile_origin_x = (run.front() % columns) * edge;
			params.tile_origin_y = (run.front() / columns) * edge;
			params.tile_extent_x = static_cast<uint32_t>(std::min<size_t>(run.size() * static_cast<size_t>(edge), static_cast<size_t>(constants.screen_width - params.tile_origin_x)));
			params.tile_extent_y = std::min(edge, constants.screen_height - params.tile_origin_y);

			thread_local std::vector<GpuPixelOutput> rectangle;
			if (!tile_executor_->dispatch_tile_and_readback(params, rectangle, layouts)) {
				return false;
			}
			thread_local std::array<GpuPixelOutput, Distributed::kTilePixelCapacity> tile_pixels;
			for (const uint32_t tile : run) {
				const auto geometry = Distributed::resolve_tile_geometry(tile, constants.screen_width, constants.screen_height);
				if (!geometry.has_value()) {
					return false;
				}
				const uint32_t column_offset = geometry->origin_x - params.tile_origin_x;
				for (uint32_t row = 0; row < geometry->height; ++row) {
					std::copy_n(rectangle.data() + static_cast<size_t>(row) * static_cast<size_t>(params.tile_extent_x) + column_offset, geometry->width, tile_pixels.data() + static_cast<size_t>(row) * static_cast<size_t>(geometry->width));
				}
				sink(tile, std::span<const GpuPixelOutput>(tile_pixels.data(), geometry->pixel_count()));
			}
			return true;
		} catch (...) {
			return false;
		}
	}

	[[nodiscard]] bool render_cluster_tiles(
		const GpuCameraPushConstants& constants,
		std::span<const uint32_t> tiles,
		std::span<const GpuBodyData> bodies,
		const Distributed::TileCompletionSink& sink,
		const std::atomic<bool>* cancel
	) {
		std::vector<uint32_t> sorted(tiles.begin(), tiles.end());
		std::sort(sorted.begin(), sorted.end());
		std::vector<uint32_t> software;
		const bool gpu_ready = use_gpu_compute_.load(std::memory_order_relaxed)
			&& tile_executor_ != nullptr
			&& tile_executor_->is_ready()
			&& gpu_path_supported(constants, bodies.size());
		if (!gpu_ready) {
			return Distributed::ClusterOrchestrator::render_tiles_software(constants, sorted, bodies, sink, cancel);
		}
		std::vector<GpuBodyGpuLayout> layouts;
		layouts.reserve(bodies.size());
		for (const auto& body : bodies) {
			if (std::isfinite(body.position[0]) && std::isfinite(body.position[1]) && std::isfinite(body.position[2]) && std::isfinite(body.radius) && std::isfinite(body.mass)) {
				layouts.push_back(GpuBodyGpuLayout::from(body));
			}
		}
		const uint32_t columns = Distributed::tile_grid_columns(constants.screen_width);
		size_t index = 0;
		while (index < sorted.size()) {
			size_t end = index + 1;
			while (end < sorted.size() && sorted[end] == sorted[end - 1] + 1U && (sorted[end] % columns) != 0U) {
				++end;
			}
			const std::span<const uint32_t> run(sorted.data() + index, end - index);
			if (run.size() < kMinimumGpuTileRun || !render_gpu_tile_run(constants, run, layouts, sink, cancel)) {
				software.insert(software.end(), run.begin(), run.end());
			}
			index = end;
		}
		return Distributed::ClusterOrchestrator::render_tiles_software(constants, software, bodies, sink, cancel);
	}
#endif

public:
	void bind_cluster(Distributed::ClusterOrchestrator& cluster) noexcept {
#if defined(RELATIVISTIC_DISTRIBUTED_COMPUTE) && RELATIVISTIC_DISTRIBUTED_COMPUTE
		ensure_tile_executor();
		if (nbody_executor_ == nullptr && context_.has_compute_device()) {
			auto nbody = std::make_unique<Distributed::GpuNBodyExecutor>();
			if (nbody->initialize(context_)) {
				nbody_executor_ = std::move(nbody);
			}
		}
		Distributed::LocalGpuInfo info{};
		if (tile_executor_ != nullptr && tile_executor_->is_ready()) {
			const VulkanDeviceInfo& device = context_.device_info();
			info.has_vulkan_compute = true;
			info.has_fp64_support = context_.supports_fp64();
			info.vulkan_api_version = device.api_version;
			info.vendor_id = device.vendor_id;
			info.device_id = device.device_id;
		}
		cluster.set_local_gpu_info(info);
		try {
			cluster.set_local_tile_renderer([this](
				const GpuCameraPushConstants& constants,
				std::span<const uint32_t> tiles,
				std::span<const GpuBodyData> bodies,
				const Distributed::TileCompletionSink& sink,
				const std::atomic<bool>* cancel
			) {
				return render_cluster_tiles(constants, tiles, bodies, sink, cancel);
			});
			cluster.set_local_nbody_evaluator([this](
				std::span<const Distributed::NBodyBodyState> bodies,
				uint32_t first_row,
				uint32_t row_count,
				uint32_t phase,
				double gravitational_constant,
				double speed_of_light,
				std::span<Distributed::NBodyRowResult> output
			) -> bool {
				if (use_gpu_compute_.load(std::memory_order_relaxed)) {
					std::lock_guard<std::mutex> gpu_lock(gpu_mutex_);
					if (nbody_executor_ != nullptr && nbody_executor_->is_ready()) {
						return nbody_executor_->evaluate(bodies, first_row, row_count, phase, gravitational_constant, speed_of_light, output);
					}
				}
				return false;
			});
		} catch (...) {
		}
#endif
		cluster_extension_.bind_cluster(cluster);
	}

	void unbind_cluster() noexcept {
		cluster_extension_.detach_local_renderer();
		cluster_extension_.unbind_cluster();
	}

	[[nodiscard]] bool is_cluster_active() const noexcept {
		return cluster_extension_.is_cluster_active();
	}

	[[nodiscard]] DistributedComputeExtension& cluster_extension() noexcept {
		return cluster_extension_;
	}

	[[nodiscard]] const DistributedComputeExtension& cluster_extension() const noexcept {
		return cluster_extension_;
	}

	[[nodiscard]] static bool gpu_path_supported(const GpuCameraPushConstants& params, size_t body_count) noexcept {
		if (body_count > 0U && SoftwareComputeEngine::requires_exact_metric_path(params)) {
			return false;
		}
		if (params.precision_mode != static_cast<uint32_t>(PrecisionMode::NativeFloat64)) {
			return false;
		}
		if (!is_metric_gpu_accelerable(params)) {
			return false;
		}
		if (params.interlace_mode != 0U) {
			return false;
		}
		if (body_count > kMaxGpuBackgroundBodies) {
			return false;
		}
		return !(params.jet.enabled > 0.5f && params.jet.emission_model > 0.5f);
	}

	[[nodiscard]] bool render_tile(
		const GpuCameraPushConstants& camera_constants,
		const SoftwareComputeEngine::TileRegion& region,
		std::span<const GpuBodyData> requested_bodies,
		std::span<GpuPixelOutput> output,
		const std::atomic<bool>* cancel = nullptr
	) {
		const std::vector<GpuBodyData> guarded_storage = guarded_bodies(requested_bodies);
		const std::span<const GpuBodyData> bodies(guarded_storage);
#if defined(RELATIVISTIC_DISTRIBUTED_COMPUTE) && RELATIVISTIC_DISTRIBUTED_COMPUTE
		if (use_gpu_compute_.load(std::memory_order_relaxed)) {
			ensure_tile_executor();
		}
#endif
		if (use_gpu_compute_.load(std::memory_order_relaxed) && tile_executor_ != nullptr && tile_executor_->is_ready()) {
			if (gpu_path_supported(camera_constants, bodies.size())) {
				std::lock_guard<std::mutex> gpu_lock(gpu_mutex_);
				GpuCameraPushConstants tile_params = camera_constants;
				tile_params.interlace_mode = 0U;
				tile_params.interlace_phase = 0U;
				tile_params.dispatch_row_offset = 0U;
				tile_params.dispatch_row_count = 0U;
				tile_params.tile_origin_x = region.origin_x;
				tile_params.tile_origin_y = region.origin_y;
				tile_params.tile_extent_x = region.width;
				tile_params.tile_extent_y = region.height;
				std::vector<GpuBodyGpuLayout> layouts;
				layouts.reserve(bodies.size());
				for (const auto& b : bodies) {
					layouts.push_back(GpuBodyGpuLayout::from(b));
				}
				std::vector<GpuPixelOutput> gpu_pixels;
				if (tile_executor_->dispatch_tile_and_readback(tile_params, gpu_pixels, layouts)) {
					const size_t copy_count = std::min(gpu_pixels.size(), output.size());
					std::copy_n(gpu_pixels.begin(), copy_count, output.begin());
					return true;
				}
			}
		}
		return SoftwareComputeEngine::dispatch_tile(camera_constants, region, output, bodies, cancel);
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

	bool swap_display_framebuffer(std::vector<GpuPixelOutput>& consumer, uint32_t& out_w, uint32_t& out_h) noexcept {
		std::lock_guard<std::mutex> lock(mutex_);
		if (!new_frame_ready_.load(std::memory_order_relaxed)) {
			return false;
		}
		std::swap(consumer, front_buffer_);
		out_w = rendered_width_;
		out_h = rendered_height_;
		new_frame_ready_.store(false, std::memory_order_release);
		return true;
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

	[[nodiscard]] PipelineExecutionTelemetry telemetry() const noexcept {
		std::lock_guard<std::mutex> lock(mutex_);
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

	[[nodiscard]] bool render_capture(const GpuCameraPushConstants& camera_constants, std::span<const GpuBodyData> requested_bodies, std::vector<GpuPixelOutput>& output) {
		const std::vector<GpuBodyData> guarded_storage = guarded_bodies(requested_bodies);
		const std::span<const GpuBodyData> bodies(guarded_storage);
		const size_t pixel_count = static_cast<size_t>(camera_constants.screen_width) * static_cast<size_t>(camera_constants.screen_height);
		if (pixel_count == 0) {
			return false;
		}

		try {
			output.assign(pixel_count, GpuPixelOutput{});
		} catch (const std::exception& ex) {
			Core::log_error(std::string("Capture frame buffer allocation failed: ") + ex.what());
			return false;
		}

		try {
			await_earth_textures(bodies);
			if (cluster_extension_.is_cluster_active() && cluster_extension_.workers_active() > 0U) {
				if (cluster_extension_.dispatch_capture(camera_constants, output, 0U, camera_constants.screen_height, bodies, nullptr)) {
					return true;
				}
			}
			if (use_gpu_compute_.load(std::memory_order_relaxed) && bodies.size() <= kMaxGpuBackgroundBodies) {
				if (try_gpu_dispatch(camera_constants, output, bodies) && output.size() == pixel_count) {
					return true;
				}
			}
			dispatch_software(camera_constants, output, bodies, nullptr, nullptr, nullptr);
			return true;
		} catch (const std::exception& ex) {
			Core::log_error(std::string("Capture render failed: ") + ex.what());
		} catch (...) {
			Core::log_error("Capture render failed with an unknown error.");
		}
		return false;
	}

	[[nodiscard]] bool render_capture_band(
		const GpuCameraPushConstants& camera_constants,
		std::span<const GpuBodyData> requested_bodies,
		uint32_t first_row,
		uint32_t row_count,
		std::vector<GpuPixelOutput>& output,
		const std::atomic<bool>* cancel
	) {
		const std::vector<GpuBodyData> guarded_storage = guarded_bodies(requested_bodies);
		const std::span<const GpuBodyData> bodies(guarded_storage);
		const uint32_t width = camera_constants.screen_width;
		const uint32_t height = camera_constants.screen_height;
		if (width == 0U || height == 0U || row_count == 0U || first_row >= height) {
			return false;
		}
		const uint32_t rows = std::min(row_count, height - first_row);
		const size_t pixel_count = static_cast<size_t>(width) * static_cast<size_t>(rows);

		try {
			output.resize(pixel_count);
		} catch (const std::exception& ex) {
			Core::log_error(std::string("Capture band allocation failed: ") + ex.what());
			return false;
		}

		GpuCameraPushConstants band_constants = camera_constants;
		band_constants.dispatch_row_offset = first_row;
		band_constants.dispatch_row_count = rows;

		try {
			await_earth_textures(bodies);
			if (cluster_extension_.is_cluster_active() && cluster_extension_.workers_active() > 0U && cluster_extension_.dispatch_capture(band_constants, output, first_row, rows, bodies, cancel)) {
				return true;
			}
			if (cancel != nullptr && cancel->load(std::memory_order_relaxed)) {
				return false;
			}
			if (use_gpu_compute_.load(std::memory_order_relaxed) && bodies.size() <= kMaxGpuBackgroundBodies) {
				if (try_gpu_dispatch(band_constants, output, bodies, cancel) && output.size() == pixel_count) {
					return true;
				}
				if (cancel != nullptr && cancel->load(std::memory_order_relaxed)) {
					return false;
				}
			}
			dispatch_software(band_constants, output, bodies, nullptr, cancel, nullptr);
			return cancel == nullptr || !cancel->load(std::memory_order_relaxed);
		} catch (const std::exception& ex) {
			Core::log_error(std::string("Capture band render failed: ") + ex.what());
		} catch (...) {
			Core::log_error("Capture band render failed with an unknown error.");
		}
		return false;
	}

	void dispatch(const GpuCameraPushConstants& camera_constants, std::span<const GpuBodyData> requested_bodies = {}, uint32_t total_enabled_bodies = 0, bool preserve_in_flight_render = false) {
		const std::vector<GpuBodyData> guarded_storage = guarded_bodies(requested_bodies);
		const std::span<const GpuBodyData> bodies(guarded_storage);
		if (config_.headless) {
			GpuCameraPushConstants actual_constants = camera_constants;
			{
				std::lock_guard<std::mutex> lock(mutex_);
				actual_constants.projection_mode = static_cast<uint32_t>(config_.projection_mode);
			}

			bool rendered_on_gpu = false;
			bool rendered_on_cluster = false;
			bool bodies_patched_on_top = false;
			SoftwareComputeEngine::RenderStageStats headless_stage_stats{};
			const bool gpu_background_eligible = use_gpu_compute_.load(std::memory_order_relaxed) && bodies.size() <= kMaxGpuBackgroundBodies;
			const PrecisionMode active_precision_mode = precision_mode_.load(std::memory_order_relaxed);
			try {
				if (cluster_extension_.is_cluster_active() && cluster_extension_.workers_active() > 0U) {
					rendered_on_cluster = cluster_extension_.dispatch_distributed(actual_constants, front_buffer_, bodies, nullptr);
				}

				if (!rendered_on_cluster) {
					if (gpu_background_eligible) {
						const size_t total_pixels = static_cast<size_t>(actual_constants.screen_width) * static_cast<size_t>(actual_constants.screen_height);
						if (front_buffer_.size() >= total_pixels) {
							if (try_gpu_dispatch(actual_constants, front_buffer_, bodies) && front_buffer_.size() == total_pixels) {
								rendered_on_gpu = true;
							}
						}
					}

					if (!rendered_on_gpu) {
						if (active_precision_mode == PrecisionMode::NativeFloat64) {
							SoftwareComputeEngine::dispatch_fp64(actual_constants, front_buffer_, bodies, thread_pool_.get(), nullptr, &headless_stage_stats);
						} else {
							SoftwareComputeEngine::dispatch_double_single(actual_constants, front_buffer_, bodies, thread_pool_.get(), nullptr, &headless_stage_stats);
						}
					}
				}
			} catch (const std::exception& ex) {
				Core::log_error(std::string("Headless render dispatch threw an exception, frame skipped: ") + ex.what());
				new_frame_ready_.store(true, std::memory_order_release);
				return;
			} catch (...) {
				Core::log_error("Headless render dispatch threw an unknown exception, frame skipped.");
				new_frame_ready_.store(true, std::memory_order_release);
				return;
			}
			uint64_t absorbed = 0;
			uint64_t celestial = 0;
			uint64_t disk_hits = 0;
			uint64_t saturated = 0;
			for (const auto& px : front_buffer_) {
				if ((px.status_flags & PixelFlags::HORIZON_ABSORBED) != 0U) ++absorbed;
				else if ((px.status_flags & PixelFlags::CELESTIAL_HIT) != 0U) ++celestial;
				if ((px.status_flags & PixelFlags::ACCRETION_DISK_HIT) != 0U) ++disk_hits;
				if ((px.status_flags & (PixelFlags::HORIZON_ABSORBED | PixelFlags::CELESTIAL_HIT)) == 0U) ++saturated;
			}

			telemetry_.total_pixels_processed = front_buffer_.size();
			telemetry_.horizon_pixels_absorbed = absorbed;
			telemetry_.celestial_pixels_hit = celestial;
			telemetry_.accretion_disk_pixels_hit = disk_hits;
			telemetry_.saturated_ray_pixels = saturated;
			telemetry_.used_gpu_path = rendered_on_gpu;
			telemetry_.bodies_patched_over_gpu_background = bodies_patched_on_top;
			telemetry_.bodies_rendered_on_gpu = rendered_on_gpu && !bodies.empty();
			telemetry_.used_cluster_path = rendered_on_cluster;
			telemetry_.cluster = cluster_extension_.last_frame_telemetry();
			telemetry_.bodies_sent_this_frame = static_cast<uint32_t>(bodies.size());
			telemetry_.bodies_total_enabled_this_frame = total_enabled_bodies;
			telemetry_.tile_prepass_skip_ms = static_cast<double>(headless_stage_stats.tile_prepass_skip_ns.load(std::memory_order_relaxed)) / 1.0e6;
			telemetry_.full_raytrace_tiles_ms = static_cast<double>(headless_stage_stats.full_trace_ns.load(std::memory_order_relaxed)) / 1.0e6;
			telemetry_.tile_prepass_skip_tile_count = headless_stage_stats.tile_prepass_skip_count.load(std::memory_order_relaxed);
			telemetry_.full_raytrace_tile_count = headless_stage_stats.full_trace_tile_count.load(std::memory_order_relaxed);
			telemetry_.body_tile_count = headless_stage_stats.body_tile_count.load(std::memory_order_relaxed);
			telemetry_.body_tile_total_count = headless_stage_stats.total_tile_count.load(std::memory_order_relaxed);
			new_frame_ready_.store(true, std::memory_order_release);
			return;
		}

		{
			std::lock_guard<std::mutex> lock(mutex_);
			if (is_rendering_.load(std::memory_order_relaxed) && !preserve_in_flight_render) {
				cancel_render_.store(true, std::memory_order_relaxed);
			}
			pending_constants_ = camera_constants;
			pending_constants_.projection_mode = static_cast<uint32_t>(config_.projection_mode);
			pending_bodies_.assign(bodies.begin(), bodies.end());
			pending_total_enabled_bodies_.store(total_enabled_bodies, std::memory_order_relaxed);
			request_pending_.store(true, std::memory_order_release);
		}
		cv_.notify_one();
	}
};

}
