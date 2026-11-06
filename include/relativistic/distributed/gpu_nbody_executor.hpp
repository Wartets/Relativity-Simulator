#pragma once

#include "relativistic/distributed/compute_payloads.hpp"
#include <vulkan/vulkan.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace Relativistic::Render {
class VulkanContext;
}

namespace Relativistic::Distributed {

struct alignas(8) GpuNBodyUniformParameters {
	uint32_t total_bodies{0};
	uint32_t first_row{0};
	uint32_t row_count{0};
	uint32_t phase{0};
	double gravitational_constant{1.0};
	double speed_of_light{1.0};
};

static_assert(sizeof(GpuNBodyUniformParameters) == 32);

class GpuNBodyExecutor {
private:
	VkDevice device_{VK_NULL_HANDLE};
	VkPhysicalDevice physical_device_{VK_NULL_HANDLE};
	VkQueue compute_queue_{VK_NULL_HANDLE};
	VkCommandPool command_pool_{VK_NULL_HANDLE};

	VkShaderModule shader_module_{VK_NULL_HANDLE};
	VkDescriptorSetLayout descriptor_set_layout_{VK_NULL_HANDLE};
	VkPipelineLayout pipeline_layout_{VK_NULL_HANDLE};
	VkPipeline compute_pipeline_{VK_NULL_HANDLE};
	VkDescriptorPool descriptor_pool_{VK_NULL_HANDLE};
	VkDescriptorSet descriptor_set_{VK_NULL_HANDLE};

	VkBuffer uniform_buffer_{VK_NULL_HANDLE};
	VkDeviceMemory uniform_memory_{VK_NULL_HANDLE};
	void* uniform_mapped_{nullptr};

	VkBuffer input_buffer_{VK_NULL_HANDLE};
	VkDeviceMemory input_memory_{VK_NULL_HANDLE};
	void* input_mapped_{nullptr};
	VkDeviceSize input_capacity_bytes_{0};

	VkBuffer output_buffer_{VK_NULL_HANDLE};
	VkDeviceMemory output_memory_{VK_NULL_HANDLE};
	VkDeviceSize output_capacity_bytes_{0};

	VkBuffer staging_buffer_{VK_NULL_HANDLE};
	VkDeviceMemory staging_memory_{VK_NULL_HANDLE};
	void* staging_mapped_{nullptr};
	VkDeviceSize staging_capacity_bytes_{0};

	VkCommandBuffer command_buffer_{VK_NULL_HANDLE};
	VkFence fence_{VK_NULL_HANDLE};
	VkPhysicalDeviceMemoryProperties memory_properties_{};
	bool staging_is_coherent_{false};
	bool ready_{false};
	bool device_lost_{false};

	[[nodiscard]] std::optional<uint32_t> find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties) const noexcept;
	[[nodiscard]] bool create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& out_buffer, VkDeviceMemory& out_memory) const noexcept;
	void destroy_buffer(VkBuffer& buffer, VkDeviceMemory& memory, void** mapped = nullptr) noexcept;
	[[nodiscard]] bool ensure_input_capacity(size_t body_count) noexcept;
	[[nodiscard]] bool ensure_output_capacity(size_t row_count) noexcept;

public:
	GpuNBodyExecutor() = default;
	~GpuNBodyExecutor() noexcept;

	GpuNBodyExecutor(const GpuNBodyExecutor&) = delete;
	GpuNBodyExecutor& operator=(const GpuNBodyExecutor&) = delete;

	[[nodiscard]] bool initialize(Render::VulkanContext& context, std::span<const uint32_t> embedded_shader = {}) noexcept;
	void shutdown() noexcept;

	[[nodiscard]] bool is_ready() const noexcept {
		return ready_ && !device_lost_;
	}

	[[nodiscard]] bool evaluate(
		std::span<const NBodyBodyState> bodies,
		uint32_t first_row,
		uint32_t row_count,
		uint32_t phase,
		double gravitational_constant,
		double speed_of_light,
		std::span<NBodyRowResult> output
	) noexcept;
};

}
