#include "relativistic/distributed/tile_renderer.hpp"
#include "relativistic/core/engine_log.hpp"
#include "relativistic/distributed/session_protocol.hpp"
#include "relativistic/render/geodesic_compute_pipeline.hpp"
#include <algorithm>
#include <chrono>
#include <exception>

namespace Relativistic::Distributed {

TileRenderer::TileRenderer(const TileRendererConfiguration& configuration)
	: configuration_(configuration) {}

TileRenderer::~TileRenderer() {
	std::lock_guard<std::mutex> lock(gpu_mutex_);
	executor_.reset();
	context_.reset();
}

bool TileRenderer::initialize() {
	if (configuration_.backend == WorkerBackend::Cpu) {
		return true;
	}
	try {
		auto context = std::make_unique<Render::VulkanContext>();
		context->set_preferred_device_index(configuration_.vulkan_device);
		static_cast<void>(context->initialize(true, true));
		if (!context->has_compute_device()) {
			Core::log_warning("No Vulkan compute device with FP64 support is available for the tile renderer; tiles will run on the CPU.");
			return true;
		}
		auto executor = std::make_unique<Render::VulkanComputeExecutor>();
		Render::VulkanComputeExecutor::ShaderSource source{};
		source.file_name = "cluster_tile_tracer_fp64.comp.spv";
		source.embedded = configuration_.embedded_shader;
		source.tile_dispatch = true;
		if (!executor->initialize(*context, source)) {
			Core::log_warning("The tile compute pipeline could not be initialized; tiles will run on the CPU.");
			return true;
		}
		context_ = std::move(context);
		executor_ = std::move(executor);
		gpu_ready_.store(true, std::memory_order_release);
	} catch (const std::exception& ex) {
		Core::log_error(std::string("Tile renderer GPU initialization failed, continuing on the CPU: ") + ex.what());
		executor_.reset();
		context_.reset();
		gpu_ready_.store(false, std::memory_order_release);
	}
	return true;
}

LocalGpuInfo TileRenderer::gpu_info() const noexcept {
	LocalGpuInfo info{};
	if (!gpu_available() || context_ == nullptr) {
		return info;
	}
	const Render::VulkanDeviceInfo& device = context_->device_info();
	info.has_vulkan_compute = true;
	info.has_fp64_support = context_->supports_fp64();
	info.vulkan_api_version = device.api_version;
	info.vendor_id = device.vendor_id;
	info.device_id = device.device_id;
	return info;
}

std::string TileRenderer::gpu_description() const {
	if (!gpu_available() || context_ == nullptr) {
		return "none";
	}
	return context_->device_info().device_name;
}

bool TileRenderer::render_on_gpu(
	const Render::GpuCameraPushConstants& constants,
	std::span<const Render::GpuBodyData> bodies,
	std::span<Render::GpuPixelOutput> pixels,
	const std::atomic<bool>* cancel
) noexcept {
	if (!gpu_available()) {
		return false;
	}
	if (!Render::GeodesicComputePipeline::gpu_path_supported(constants, bodies.size())) {
		return false;
	}
	std::unique_lock<std::mutex> lock(gpu_mutex_, std::defer_lock);
	if (configuration_.backend == WorkerBackend::Auto) {
		if (!lock.try_lock()) {
			return false;
		}
	} else {
		lock.lock();
	}
	if (cancel != nullptr && cancel->load(std::memory_order_relaxed)) {
		return false;
	}
	try {
		if (executor_ == nullptr || !executor_->is_ready()) {
			gpu_ready_.store(false, std::memory_order_release);
			return false;
		}
		gpu_layouts_.clear();
		gpu_layouts_.reserve(bodies.size());
		for (const Render::GpuBodyData& body : bodies) {
			gpu_layouts_.push_back(Render::GpuBodyGpuLayout::from(body));
		}
		if (!executor_->dispatch_tile_and_readback(constants, gpu_output_, gpu_layouts_)) {
			if (!executor_->is_ready()) {
				gpu_ready_.store(false, std::memory_order_release);
				Core::log_error("The GPU device was lost while rendering a tile; the worker continues on the CPU.");
			}
			return false;
		}
		if (gpu_output_.size() < pixels.size()) {
			return false;
		}
		std::copy_n(gpu_output_.begin(), pixels.size(), pixels.begin());
		return true;
	} catch (const std::exception& ex) {
		Core::log_error(std::string("GPU tile dispatch threw an exception: ") + ex.what());
		return false;
	} catch (...) {
		return false;
	}
}

TileRenderOutcome TileRenderer::render(
	const RenderTileTaskPayload& task,
	std::span<const Render::GpuBodyData> bodies,
	std::span<Render::GpuPixelOutput> pixels,
	const std::atomic<bool>* cancel
) noexcept {
	TileRenderOutcome outcome{};
	try {
		const auto started = std::chrono::steady_clock::now();
		const Render::GpuCameraPushConstants& source = task.camera_constants;
		const auto geometry = resolve_tile_geometry(task.header.tile_index, source.screen_width, source.screen_height);
		if (!geometry.has_value() || pixels.size() < geometry->pixel_count()) {
			return outcome;
		}
		const std::span<Render::GpuPixelOutput> output = pixels.first(geometry->pixel_count());
		outcome.pixel_count = output.size();

		Render::GpuCameraPushConstants constants = source;
		constants.interlace_mode = 0U;
		constants.interlace_phase = 0U;
		constants.dispatch_row_offset = 0U;
		constants.dispatch_row_count = 0U;
		if (task.header.lod_reduced_steps != 0U) {
			constants.lod_reduced_steps = task.header.lod_reduced_steps;
		}
		constants.tile_origin_x = geometry->origin_x;
		constants.tile_origin_y = geometry->origin_y;
		constants.tile_extent_x = geometry->width;
		constants.tile_extent_y = geometry->height;

		const bool force_cpu = configuration_.backend == WorkerBackend::Cpu || (task.header.flags & kTileTaskFlagForceCpu) != 0U;
		bool rendered = false;
		if (!force_cpu) {
			rendered = render_on_gpu(constants, bodies, output, cancel);
			outcome.used_gpu = rendered;
		}
		if (!rendered && !(cancel != nullptr && cancel->load(std::memory_order_relaxed))) {
			const Render::SoftwareComputeEngine::TileRegion region{geometry->origin_x, geometry->origin_y, geometry->width, geometry->height};
			rendered = Render::SoftwareComputeEngine::dispatch_tile(constants, region, output, bodies, cancel);
		}

		outcome.cancelled = cancel != nullptr && cancel->load(std::memory_order_relaxed);
		outcome.completed = rendered && !outcome.cancelled;
		const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - started).count();
		outcome.execution_time_us = static_cast<uint32_t>(std::clamp<int64_t>(elapsed, 1, 0xFFFFFFFFLL));
	} catch (...) {
		outcome.completed = false;
	}
	return outcome;
}

}
