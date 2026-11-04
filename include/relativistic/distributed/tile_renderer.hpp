#pragma once

#include "relativistic/distributed/cluster_node.hpp"
#include "relativistic/distributed/task_payloads.hpp"
#include "relativistic/distributed/worker_options.hpp"
#include "relativistic/render/gpu_types.hpp"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <vector>

namespace Relativistic::Render {

class VulkanContext;
class VulkanComputeExecutor;

}

namespace Relativistic::Distributed {

struct TileRendererConfiguration {
	WorkerBackend backend{WorkerBackend::Auto};
	int vulkan_device{-1};
	std::span<const uint32_t> embedded_shader{};
};

struct TileRenderOutcome {
	bool completed{false};
	bool cancelled{false};
	bool used_gpu{false};
	uint32_t execution_time_us{0};
	size_t pixel_count{0};
};

class TileRenderer {
private:
	TileRendererConfiguration configuration_;
	std::unique_ptr<Render::VulkanContext> context_;
	std::unique_ptr<Render::VulkanComputeExecutor> executor_;
	std::atomic<bool> gpu_ready_{false};
	std::mutex gpu_mutex_;
	std::vector<Render::GpuBodyGpuLayout> gpu_layouts_;
	std::vector<Render::GpuPixelOutput> gpu_output_;

	[[nodiscard]] bool render_on_gpu(
		const Render::GpuCameraPushConstants& constants,
		std::span<const Render::GpuBodyData> bodies,
		std::span<Render::GpuPixelOutput> pixels,
		const std::atomic<bool>* cancel
	) noexcept;

public:
	explicit TileRenderer(const TileRendererConfiguration& configuration);
	~TileRenderer();

	TileRenderer(const TileRenderer&) = delete;
	TileRenderer& operator=(const TileRenderer&) = delete;
	TileRenderer(TileRenderer&&) = delete;
	TileRenderer& operator=(TileRenderer&&) = delete;

	[[nodiscard]] bool initialize();

	[[nodiscard]] bool gpu_available() const noexcept {
		return gpu_ready_.load(std::memory_order_acquire);
	}

	[[nodiscard]] LocalGpuInfo gpu_info() const noexcept;

	[[nodiscard]] std::string gpu_description() const;

	[[nodiscard]] TileRenderOutcome render(
		const RenderTileTaskPayload& task,
		std::span<const Render::GpuBodyData> bodies,
		std::span<Render::GpuPixelOutput> pixels,
		const std::atomic<bool>* cancel
	) noexcept;
};

}
