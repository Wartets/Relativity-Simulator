#pragma once

#include "relativistic/render/vulkan_context.hpp"
#include "relativistic/render/gpu_types.hpp"
#include <vulkan/vulkan.h>
#include <vector>
#include <array>
#include <string>
#include <optional>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <cstdint>
#include <algorithm>

namespace Relativistic::Render {

class VulkanComputeExecutor {
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

	VkBuffer storage_buffer_{VK_NULL_HANDLE};
	VkDeviceMemory storage_memory_{VK_NULL_HANDLE};
	VkDeviceSize storage_capacity_bytes_{0};

	VkBuffer staging_buffer_{VK_NULL_HANDLE};
	VkDeviceMemory staging_memory_{VK_NULL_HANDLE};
	void* staging_mapped_{nullptr};

	VkCommandBuffer command_buffer_{VK_NULL_HANDLE};
	VkFence fence_{VK_NULL_HANDLE};

	bool ready_{false};

	[[nodiscard]] static std::optional<std::filesystem::path> find_spirv_path() {
		static constexpr const char* candidates[] = {
			"shaders/geodesic_tracer_fp64.comp.spv",
			"../shaders/geodesic_tracer_fp64.comp.spv",
			"./shaders/geodesic_tracer_fp64.comp.spv",
			"../../shaders/geodesic_tracer_fp64.comp.spv"
		};
		for (const char* candidate : candidates) {
			std::error_code ec;
			if (std::filesystem::exists(candidate, ec)) {
				return std::filesystem::path(candidate);
			}
		}
		return std::nullopt;
	}

	[[nodiscard]] static std::optional<std::vector<uint32_t>> load_spirv_bytecode(const std::filesystem::path& path) {
		std::ifstream file(path, std::ios::binary | std::ios::ate);
		if (!file.is_open()) {
			return std::nullopt;
		}
		const std::streamsize size = file.tellg();
		if (size <= 0 || (size % 4) != 0) {
			return std::nullopt;
		}
		std::vector<uint32_t> code(static_cast<size_t>(size) / 4);
		file.seekg(0);
		file.read(reinterpret_cast<char*>(code.data()), size);
		if (!file) {
			return std::nullopt;
		}
		return code;
	}

	[[nodiscard]] std::optional<uint32_t> find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties) const noexcept {
		VkPhysicalDeviceMemoryProperties mem_props{};
		vkGetPhysicalDeviceMemoryProperties(physical_device_, &mem_props);
		for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i) {
			if ((type_filter & (1U << i)) && (mem_props.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}
		return std::nullopt;
	}

	[[nodiscard]] bool create_buffer(
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkBuffer& out_buffer,
		VkDeviceMemory& out_memory
	) const noexcept {
		VkBufferCreateInfo buffer_info{};
		buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_info.size = size;
		buffer_info.usage = usage;
		buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device_, &buffer_info, nullptr, &out_buffer) != VK_SUCCESS) {
			return false;
		}

		VkMemoryRequirements mem_reqs{};
		vkGetBufferMemoryRequirements(device_, out_buffer, &mem_reqs);

		const auto type_index = find_memory_type(mem_reqs.memoryTypeBits, properties);
		if (!type_index.has_value()) {
			vkDestroyBuffer(device_, out_buffer, nullptr);
			out_buffer = VK_NULL_HANDLE;
			return false;
		}

		VkMemoryAllocateInfo alloc_info{};
		alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc_info.allocationSize = mem_reqs.size;
		alloc_info.memoryTypeIndex = *type_index;

		if (vkAllocateMemory(device_, &alloc_info, nullptr, &out_memory) != VK_SUCCESS) {
			vkDestroyBuffer(device_, out_buffer, nullptr);
			out_buffer = VK_NULL_HANDLE;
			return false;
		}

		vkBindBufferMemory(device_, out_buffer, out_memory, 0);
		return true;
	}

	void destroy_buffer(VkBuffer& buffer, VkDeviceMemory& memory, void** mapped = nullptr) noexcept {
		if (mapped != nullptr && *mapped != nullptr && memory != VK_NULL_HANDLE) {
			vkUnmapMemory(device_, memory);
			*mapped = nullptr;
		}
		if (buffer != VK_NULL_HANDLE) {
			vkDestroyBuffer(device_, buffer, nullptr);
			buffer = VK_NULL_HANDLE;
		}
		if (memory != VK_NULL_HANDLE) {
			vkFreeMemory(device_, memory, nullptr);
			memory = VK_NULL_HANDLE;
		}
	}

	[[nodiscard]] bool ensure_output_capacity(size_t pixel_count) {
		const VkDeviceSize required_bytes = static_cast<VkDeviceSize>(pixel_count) * sizeof(GpuPixelOutput);
		if (required_bytes <= storage_capacity_bytes_ && storage_buffer_ != VK_NULL_HANDLE) {
			return true;
		}

		destroy_buffer(storage_buffer_, storage_memory_);
		destroy_buffer(staging_buffer_, staging_memory_, &staging_mapped_);
		storage_capacity_bytes_ = 0;

		if (!create_buffer(
			required_bytes,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			storage_buffer_,
			storage_memory_
		)) {
			return false;
		}

		if (!create_buffer(
			required_bytes,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			staging_buffer_,
			staging_memory_
		)) {
			destroy_buffer(storage_buffer_, storage_memory_);
			return false;
		}

		if (vkMapMemory(device_, staging_memory_, 0, required_bytes, 0, &staging_mapped_) != VK_SUCCESS) {
			destroy_buffer(storage_buffer_, storage_memory_);
			destroy_buffer(staging_buffer_, staging_memory_);
			return false;
		}

		storage_capacity_bytes_ = required_bytes;

		VkDescriptorBufferInfo storage_info{};
		storage_info.buffer = storage_buffer_;
		storage_info.offset = 0;
		storage_info.range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = descriptor_set_;
		write.dstBinding = 1;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		write.pBufferInfo = &storage_info;

		vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
		return true;
	}

public:
	VulkanComputeExecutor() = default;

	~VulkanComputeExecutor() noexcept {
		shutdown();
	}

	VulkanComputeExecutor(const VulkanComputeExecutor&) = delete;
	VulkanComputeExecutor& operator=(const VulkanComputeExecutor&) = delete;

	[[nodiscard]] bool initialize(VulkanContext& context) {
		if (!context.has_compute_device()) {
			return false;
		}

		device_ = context.device();
		physical_device_ = context.physical_device();
		compute_queue_ = context.compute_queue();
		command_pool_ = context.command_pool();

		const auto spirv_path = find_spirv_path();
		if (!spirv_path.has_value()) {
			return false;
		}
		const auto bytecode = load_spirv_bytecode(*spirv_path);
		if (!bytecode.has_value()) {
			return false;
		}

		VkShaderModuleCreateInfo shader_info{};
		shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shader_info.codeSize = bytecode->size() * sizeof(uint32_t);
		shader_info.pCode = bytecode->data();
		if (vkCreateShaderModule(device_, &shader_info, nullptr, &shader_module_) != VK_SUCCESS) {
			return false;
		}

		std::array<VkDescriptorSetLayoutBinding, 2> bindings{};
		bindings[0].binding = 0;
		bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[0].descriptorCount = 1;
		bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		bindings[1].binding = 1;
		bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		bindings[1].descriptorCount = 1;
		bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		VkDescriptorSetLayoutCreateInfo layout_info{};
		layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
		layout_info.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(device_, &layout_info, nullptr, &descriptor_set_layout_) != VK_SUCCESS) {
			return false;
		}

		VkPipelineLayoutCreateInfo pipeline_layout_info{};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 1;
		pipeline_layout_info.pSetLayouts = &descriptor_set_layout_;

		if (vkCreatePipelineLayout(device_, &pipeline_layout_info, nullptr, &pipeline_layout_) != VK_SUCCESS) {
			return false;
		}

		VkPipelineShaderStageCreateInfo stage_info{};
		stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		stage_info.module = shader_module_;
		stage_info.pName = "main";

		VkComputePipelineCreateInfo pipeline_info{};
		pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipeline_info.stage = stage_info;
		pipeline_info.layout = pipeline_layout_;

		if (vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &compute_pipeline_) != VK_SUCCESS) {
			return false;
		}

		std::array<VkDescriptorPoolSize, 2> pool_sizes{};
		pool_sizes[0] = VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1};
		pool_sizes[1] = VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1};

		VkDescriptorPoolCreateInfo pool_info{};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.maxSets = 1;
		pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
		pool_info.pPoolSizes = pool_sizes.data();

		if (vkCreateDescriptorPool(device_, &pool_info, nullptr, &descriptor_pool_) != VK_SUCCESS) {
			return false;
		}

		VkDescriptorSetAllocateInfo set_alloc_info{};
		set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		set_alloc_info.descriptorPool = descriptor_pool_;
		set_alloc_info.descriptorSetCount = 1;
		set_alloc_info.pSetLayouts = &descriptor_set_layout_;

		if (vkAllocateDescriptorSets(device_, &set_alloc_info, &descriptor_set_) != VK_SUCCESS) {
			return false;
		}

		if (!create_buffer(
			sizeof(GpuCameraPushConstants),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			uniform_buffer_,
			uniform_memory_
		)) {
			return false;
		}

		if (vkMapMemory(device_, uniform_memory_, 0, sizeof(GpuCameraPushConstants), 0, &uniform_mapped_) != VK_SUCCESS) {
			return false;
		}

		VkDescriptorBufferInfo uniform_info{};
		uniform_info.buffer = uniform_buffer_;
		uniform_info.offset = 0;
		uniform_info.range = sizeof(GpuCameraPushConstants);

		VkWriteDescriptorSet uniform_write{};
		uniform_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		uniform_write.dstSet = descriptor_set_;
		uniform_write.dstBinding = 0;
		uniform_write.descriptorCount = 1;
		uniform_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniform_write.pBufferInfo = &uniform_info;

		vkUpdateDescriptorSets(device_, 1, &uniform_write, 0, nullptr);

		if (!ensure_output_capacity(64 * 64)) {
			return false;
		}

		VkCommandBufferAllocateInfo cmd_alloc_info{};
		cmd_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmd_alloc_info.commandPool = command_pool_;
		cmd_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmd_alloc_info.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(device_, &cmd_alloc_info, &command_buffer_) != VK_SUCCESS) {
			return false;
		}

		VkFenceCreateInfo fence_info{};
		fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		if (vkCreateFence(device_, &fence_info, nullptr, &fence_) != VK_SUCCESS) {
			return false;
		}

		ready_ = true;
		return true;
	}

	[[nodiscard]] bool is_ready() const noexcept {
		return ready_;
	}

	void shutdown() noexcept {
		if (device_ == VK_NULL_HANDLE) {
			ready_ = false;
			return;
		}
		vkDeviceWaitIdle(device_);

		if (fence_ != VK_NULL_HANDLE) { vkDestroyFence(device_, fence_, nullptr); fence_ = VK_NULL_HANDLE; }
		if (command_buffer_ != VK_NULL_HANDLE && command_pool_ != VK_NULL_HANDLE) {
			vkFreeCommandBuffers(device_, command_pool_, 1, &command_buffer_);
			command_buffer_ = VK_NULL_HANDLE;
		}

		destroy_buffer(uniform_buffer_, uniform_memory_, &uniform_mapped_);
		destroy_buffer(storage_buffer_, storage_memory_);
		destroy_buffer(staging_buffer_, staging_memory_, &staging_mapped_);
		storage_capacity_bytes_ = 0;

		if (descriptor_pool_ != VK_NULL_HANDLE) { vkDestroyDescriptorPool(device_, descriptor_pool_, nullptr); descriptor_pool_ = VK_NULL_HANDLE; }
		if (compute_pipeline_ != VK_NULL_HANDLE) { vkDestroyPipeline(device_, compute_pipeline_, nullptr); compute_pipeline_ = VK_NULL_HANDLE; }
		if (pipeline_layout_ != VK_NULL_HANDLE) { vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr); pipeline_layout_ = VK_NULL_HANDLE; }
		if (descriptor_set_layout_ != VK_NULL_HANDLE) { vkDestroyDescriptorSetLayout(device_, descriptor_set_layout_, nullptr); descriptor_set_layout_ = VK_NULL_HANDLE; }
		if (shader_module_ != VK_NULL_HANDLE) { vkDestroyShaderModule(device_, shader_module_, nullptr); shader_module_ = VK_NULL_HANDLE; }

		device_ = VK_NULL_HANDLE;
		physical_device_ = VK_NULL_HANDLE;
		compute_queue_ = VK_NULL_HANDLE;
		command_pool_ = VK_NULL_HANDLE;
		ready_ = false;
	}

	[[nodiscard]] bool dispatch_and_readback(const GpuCameraPushConstants& params, std::vector<GpuPixelOutput>& output) {
		if (!ready_) {
			return false;
		}

		const size_t pixel_count = static_cast<size_t>(params.screen_width) * static_cast<size_t>(params.screen_height);
		if (pixel_count == 0) {
			return false;
		}

		if (!ensure_output_capacity(pixel_count)) {
			return false;
		}

		std::memcpy(uniform_mapped_, &params, sizeof(GpuCameraPushConstants));

		if (vkResetCommandBuffer(command_buffer_, 0) != VK_SUCCESS) {
			return false;
		}

		VkCommandBufferBeginInfo begin_info{};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		if (vkBeginCommandBuffer(command_buffer_, &begin_info) != VK_SUCCESS) {
			return false;
		}

		vkCmdBindPipeline(command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline_);
		vkCmdBindDescriptorSets(command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout_, 0, 1, &descriptor_set_, 0, nullptr);

		const uint32_t group_x = (params.screen_width + 15U) / 16U;
		const uint32_t group_y = (params.screen_height + 15U) / 16U;
		vkCmdDispatch(command_buffer_, group_x, group_y, 1);

		VkBufferMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.buffer = storage_buffer_;
		barrier.offset = 0;
		barrier.size = VK_WHOLE_SIZE;

		vkCmdPipelineBarrier(
			command_buffer_,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0, 0, nullptr, 1, &barrier, 0, nullptr
		);

		VkBufferCopy copy_region{};
		copy_region.srcOffset = 0;
		copy_region.dstOffset = 0;
		copy_region.size = static_cast<VkDeviceSize>(pixel_count) * sizeof(GpuPixelOutput);
		vkCmdCopyBuffer(command_buffer_, storage_buffer_, staging_buffer_, 1, &copy_region);

		if (vkEndCommandBuffer(command_buffer_) != VK_SUCCESS) {
			return false;
		}

		if (vkResetFences(device_, 1, &fence_) != VK_SUCCESS) {
			return false;
		}

		VkSubmitInfo submit_info{};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &command_buffer_;

		if (vkQueueSubmit(compute_queue_, 1, &submit_info, fence_) != VK_SUCCESS) {
			return false;
		}

		if (vkWaitForFences(device_, 1, &fence_, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
			return false;
		}

		output.resize(pixel_count);
		std::memcpy(output.data(), staging_mapped_, static_cast<size_t>(copy_region.size));
		return true;
	}

	[[nodiscard]] static bool is_platform_supported() {
		static const bool cached = [] {
			VulkanContext probe_context;
			if (!probe_context.initialize(true, true)) {
				return false;
			}
			if (!probe_context.has_compute_device()) {
				probe_context.shutdown();
				return false;
			}
			VulkanComputeExecutor probe_executor;
			const bool result = probe_executor.initialize(probe_context);
			probe_executor.shutdown();
			probe_context.shutdown();
			return result;
		}();
		return cached;
	}
};

}
