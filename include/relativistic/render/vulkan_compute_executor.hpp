#pragma once

#include "relativistic/render/vulkan_context.hpp"
#include "relativistic/render/gpu_types.hpp"
#include "relativistic/optics/sky_panorama_image.hpp"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <array>
#include <string>
#include <optional>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <cstdint>
#include <algorithm>
#include <span>

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

	VkBuffer body_buffer_{VK_NULL_HANDLE};
	VkDeviceMemory body_memory_{VK_NULL_HANDLE};
	void* body_mapped_{nullptr};
	VkDeviceSize body_capacity_bytes_{0};

	VkBuffer persistent_counter_buffer_{VK_NULL_HANDLE};
	VkDeviceMemory persistent_counter_memory_{VK_NULL_HANDLE};
	void* persistent_counter_mapped_{nullptr};

	VkBuffer staging_buffer_{VK_NULL_HANDLE};
	VkDeviceMemory staging_memory_{VK_NULL_HANDLE};
	void* staging_mapped_{nullptr};

	VkImage panorama_texture_image_{VK_NULL_HANDLE};
	VkDeviceMemory panorama_texture_memory_{VK_NULL_HANDLE};
	VkImageView panorama_texture_view_{VK_NULL_HANDLE};
	VkSampler panorama_sampler_{VK_NULL_HANDLE};
	VkDeviceSize panorama_image_capacity_bytes_{0};
	bool panorama_supported_{false};
	Optics::SkyPanoramaLoader::ImageHandle panorama_image_{};
	uint32_t panorama_width_{0};
	uint32_t panorama_height_{0};
	uint32_t last_panorama_key_{0xFFFFFFFFU};

	VkCommandBuffer command_buffer_{VK_NULL_HANDLE};
	VkFence fence_{VK_NULL_HANDLE};
	VkPhysicalDeviceMemoryProperties memory_properties_{};
	bool staging_is_coherent_{false};

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
		for (uint32_t i = 0; i < memory_properties_.memoryTypeCount; ++i) {
			if ((type_filter & (1U << i)) && (memory_properties_.memoryTypes[i].propertyFlags & properties) == properties) {
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

		VkBufferCreateInfo staging_buffer_info{};
		staging_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		staging_buffer_info.size = required_bytes;
		staging_buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		staging_buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		if (vkCreateBuffer(device_, &staging_buffer_info, nullptr, &staging_buffer_) != VK_SUCCESS) {
			destroy_buffer(storage_buffer_, storage_memory_);
			return false;
		}

		VkMemoryRequirements staging_mem_reqs{};
		vkGetBufferMemoryRequirements(device_, staging_buffer_, &staging_mem_reqs);

		auto staging_type_index = find_memory_type(
			staging_mem_reqs.memoryTypeBits,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT
		);
		staging_is_coherent_ = false;
		if (!staging_type_index.has_value()) {
			staging_type_index = find_memory_type(
				staging_mem_reqs.memoryTypeBits,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			);
			staging_is_coherent_ = true;
		} else {
			const auto flags = memory_properties_.memoryTypes[*staging_type_index].propertyFlags;
			staging_is_coherent_ = (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0;
		}

		if (!staging_type_index.has_value()) {
			destroy_buffer(storage_buffer_, storage_memory_);
			vkDestroyBuffer(device_, staging_buffer_, nullptr);
			staging_buffer_ = VK_NULL_HANDLE;
			return false;
		}

		VkMemoryAllocateInfo staging_alloc_info{};
		staging_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		staging_alloc_info.allocationSize = staging_mem_reqs.size;
		staging_alloc_info.memoryTypeIndex = *staging_type_index;

		if (vkAllocateMemory(device_, &staging_alloc_info, nullptr, &staging_memory_) != VK_SUCCESS) {
			destroy_buffer(storage_buffer_, storage_memory_);
			vkDestroyBuffer(device_, staging_buffer_, nullptr);
			staging_buffer_ = VK_NULL_HANDLE;
			return false;
		}

		vkBindBufferMemory(device_, staging_buffer_, staging_memory_, 0);

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

	[[nodiscard]] bool create_panorama_image(uint32_t width, uint32_t height, VkImage& out_image, VkDeviceMemory& out_memory) const noexcept {
		VkImageCreateInfo image_info{};
		image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image_info.imageType = VK_IMAGE_TYPE_2D;
		image_info.format = VK_FORMAT_R8G8B8A8_SRGB;
		image_info.extent = VkExtent3D{width, height, 1};
		image_info.mipLevels = 1;
		image_info.arrayLayers = 1;
		image_info.samples = VK_SAMPLE_COUNT_1_BIT;
		image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		if (vkCreateImage(device_, &image_info, nullptr, &out_image) != VK_SUCCESS) {
			return false;
		}

		VkMemoryRequirements mem_reqs{};
		vkGetImageMemoryRequirements(device_, out_image, &mem_reqs);

		const auto type_index = find_memory_type(mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		if (!type_index.has_value()) {
			vkDestroyImage(device_, out_image, nullptr);
			out_image = VK_NULL_HANDLE;
			return false;
		}

		VkMemoryAllocateInfo alloc_info{};
		alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc_info.allocationSize = mem_reqs.size;
		alloc_info.memoryTypeIndex = *type_index;

		if (vkAllocateMemory(device_, &alloc_info, nullptr, &out_memory) != VK_SUCCESS) {
			vkDestroyImage(device_, out_image, nullptr);
			out_image = VK_NULL_HANDLE;
			return false;
		}

		vkBindImageMemory(device_, out_image, out_memory, 0);
		return true;
	}

	void destroy_panorama_texture() noexcept {
		if (panorama_texture_view_ != VK_NULL_HANDLE) {
			vkDestroyImageView(device_, panorama_texture_view_, nullptr);
			panorama_texture_view_ = VK_NULL_HANDLE;
		}
		if (panorama_texture_image_ != VK_NULL_HANDLE) {
			vkDestroyImage(device_, panorama_texture_image_, nullptr);
			panorama_texture_image_ = VK_NULL_HANDLE;
		}
		if (panorama_texture_memory_ != VK_NULL_HANDLE) {
			vkFreeMemory(device_, panorama_texture_memory_, nullptr);
			panorama_texture_memory_ = VK_NULL_HANDLE;
		}
		panorama_image_capacity_bytes_ = 0;
	}

	void write_panorama_descriptor() noexcept {
		VkDescriptorImageInfo image_info{};
		image_info.sampler = panorama_sampler_;
		image_info.imageView = panorama_texture_view_;
		image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = descriptor_set_;
		write.dstBinding = 3;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.pImageInfo = &image_info;

		vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
	}

	[[nodiscard]] bool submit_panorama_upload(VkBuffer source, VkImage destination, uint32_t width, uint32_t height) noexcept {
		if (vkResetCommandBuffer(command_buffer_, 0) != VK_SUCCESS) {
			return false;
		}

		VkCommandBufferBeginInfo begin_info{};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		if (vkBeginCommandBuffer(command_buffer_, &begin_info) != VK_SUCCESS) {
			return false;
		}

		VkImageMemoryBarrier to_transfer{};
		to_transfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		to_transfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		to_transfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		to_transfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		to_transfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		to_transfer.image = destination;
		to_transfer.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		to_transfer.subresourceRange.levelCount = 1;
		to_transfer.subresourceRange.layerCount = 1;
		to_transfer.srcAccessMask = 0;
		to_transfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		vkCmdPipelineBarrier(
			command_buffer_,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0, 0, nullptr, 0, nullptr, 1, &to_transfer
		);

		VkBufferImageCopy region{};
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.layerCount = 1;
		region.imageExtent = VkExtent3D{width, height, 1};
		vkCmdCopyBufferToImage(command_buffer_, source, destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		VkImageMemoryBarrier to_shader_read{};
		to_shader_read.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		to_shader_read.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		to_shader_read.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		to_shader_read.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		to_shader_read.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		to_shader_read.image = destination;
		to_shader_read.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		to_shader_read.subresourceRange.levelCount = 1;
		to_shader_read.subresourceRange.layerCount = 1;
		to_shader_read.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		to_shader_read.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		vkCmdPipelineBarrier(
			command_buffer_,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0, 0, nullptr, 0, nullptr, 1, &to_shader_read
		);

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
		constexpr uint64_t kPanoramaUploadTimeoutNs = 4000000000ULL;
		return vkWaitForFences(device_, 1, &fence_, VK_TRUE, kPanoramaUploadTimeoutNs) == VK_SUCCESS;
	}

	[[nodiscard]] bool upload_panorama_pixels(uint32_t width, uint32_t height, const uint32_t* pixels) {
		const VkDeviceSize required_bytes = static_cast<VkDeviceSize>(width) * static_cast<VkDeviceSize>(height) * sizeof(uint32_t);

		VkImage new_image{VK_NULL_HANDLE};
		VkDeviceMemory new_memory{VK_NULL_HANDLE};
		if (!create_panorama_image(width, height, new_image, new_memory)) {
			return false;
		}

		VkImageViewCreateInfo view_info{};
		view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_info.image = new_image;
		view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view_info.format = VK_FORMAT_R8G8B8A8_SRGB;
		view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		view_info.subresourceRange.levelCount = 1;
		view_info.subresourceRange.layerCount = 1;

		VkImageView new_view{VK_NULL_HANDLE};
		if (vkCreateImageView(device_, &view_info, nullptr, &new_view) != VK_SUCCESS) {
			vkDestroyImage(device_, new_image, nullptr);
			vkFreeMemory(device_, new_memory, nullptr);
			return false;
		}

		VkBuffer upload_buffer{VK_NULL_HANDLE};
		VkDeviceMemory upload_memory{VK_NULL_HANDLE};
		if (!create_buffer(
			required_bytes,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			upload_buffer,
			upload_memory
		)) {
			vkDestroyImageView(device_, new_view, nullptr);
			vkDestroyImage(device_, new_image, nullptr);
			vkFreeMemory(device_, new_memory, nullptr);
			return false;
		}

		void* mapped = nullptr;
		if (vkMapMemory(device_, upload_memory, 0, required_bytes, 0, &mapped) != VK_SUCCESS) {
			destroy_buffer(upload_buffer, upload_memory);
			vkDestroyImageView(device_, new_view, nullptr);
			vkDestroyImage(device_, new_image, nullptr);
			vkFreeMemory(device_, new_memory, nullptr);
			return false;
		}
		std::memcpy(mapped, pixels, static_cast<size_t>(required_bytes));
		vkUnmapMemory(device_, upload_memory);

		const bool copied = submit_panorama_upload(upload_buffer, new_image, width, height);
		destroy_buffer(upload_buffer, upload_memory);

		if (!copied) {
			vkDestroyImageView(device_, new_view, nullptr);
			vkDestroyImage(device_, new_image, nullptr);
			vkFreeMemory(device_, new_memory, nullptr);
			return false;
		}

		destroy_panorama_texture();
		panorama_texture_image_ = new_image;
		panorama_texture_memory_ = new_memory;
		panorama_texture_view_ = new_view;
		panorama_image_capacity_bytes_ = required_bytes;

		write_panorama_descriptor();
		return true;
	}

	[[nodiscard]] bool ensure_panorama_placeholder() {
		const std::array<uint32_t, 1> placeholder_pixel{0xFF000000U};
		return upload_panorama_pixels(1, 1, placeholder_pixel.data());
	}

	[[nodiscard]] bool upload_panorama(const Optics::SkyPanoramaLoader::ImageHandle& image) {
		if (image == panorama_image_) {
			return true;
		}

		VkPhysicalDeviceProperties device_properties{};
		vkGetPhysicalDeviceProperties(physical_device_, &device_properties);
		if (image->width > device_properties.limits.maxImageDimension2D || image->height > device_properties.limits.maxImageDimension2D) {
			return false;
		}

		if (!upload_panorama_pixels(image->width, image->height, image->texels.data())) {
			return false;
		}

		panorama_image_ = image;
		panorama_width_ = image->width;
		panorama_height_ = image->height;
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
		vkGetPhysicalDeviceMemoryProperties(physical_device_, &memory_properties_);

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

		std::array<VkDescriptorSetLayoutBinding, 5> bindings{};
		bindings[0].binding = 0;
		bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[0].descriptorCount = 1;
		bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		bindings[1].binding = 1;
		bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		bindings[1].descriptorCount = 1;
		bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		bindings[2].binding = 2;
		bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		bindings[2].descriptorCount = 1;
		bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		bindings[3].binding = 3;
		bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bindings[3].descriptorCount = 1;
		bindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		bindings[4].binding = 4;
		bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		bindings[4].descriptorCount = 1;
		bindings[4].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

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

		std::array<VkDescriptorPoolSize, 3> pool_sizes{};
		pool_sizes[0] = VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1};
		pool_sizes[1] = VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3};
		pool_sizes[2] = VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1};

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

		if (!create_buffer(
			sizeof(uint32_t),
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			persistent_counter_buffer_,
			persistent_counter_memory_
		)) {
			return false;
		}
		if (vkMapMemory(device_, persistent_counter_memory_, 0, sizeof(uint32_t), 0, &persistent_counter_mapped_) != VK_SUCCESS) {
			return false;
		}
		*static_cast<uint32_t*>(persistent_counter_mapped_) = 0U;

		VkDescriptorBufferInfo persistent_counter_info{};
		persistent_counter_info.buffer = persistent_counter_buffer_;
		persistent_counter_info.offset = 0;
		persistent_counter_info.range = sizeof(uint32_t);

		VkWriteDescriptorSet persistent_counter_write{};
		persistent_counter_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		persistent_counter_write.dstSet = descriptor_set_;
		persistent_counter_write.dstBinding = 4;
		persistent_counter_write.descriptorCount = 1;
		persistent_counter_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		persistent_counter_write.pBufferInfo = &persistent_counter_info;

		vkUpdateDescriptorSets(device_, 1, &persistent_counter_write, 0, nullptr);

		if (!ensure_output_capacity(64 * 64)) {
			return false;
		}

		if (!ensure_body_capacity(1)) {
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

		VkSamplerCreateInfo panorama_sampler_info{};
		panorama_sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		panorama_sampler_info.magFilter = VK_FILTER_LINEAR;
		panorama_sampler_info.minFilter = VK_FILTER_LINEAR;
		panorama_sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		panorama_sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		panorama_sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		panorama_sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		panorama_sampler_info.maxLod = 0.0f;
		panorama_sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		panorama_supported_ = (vkCreateSampler(device_, &panorama_sampler_info, nullptr, &panorama_sampler_) == VK_SUCCESS)
			&& ensure_panorama_placeholder();

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
		destroy_buffer(body_buffer_, body_memory_, &body_mapped_);
		destroy_buffer(persistent_counter_buffer_, persistent_counter_memory_, &persistent_counter_mapped_);
		destroy_panorama_texture();
		if (panorama_sampler_ != VK_NULL_HANDLE) {
			vkDestroySampler(device_, panorama_sampler_, nullptr);
			panorama_sampler_ = VK_NULL_HANDLE;
		}
		panorama_image_.reset();
		panorama_width_ = 0;
		panorama_height_ = 0;
		last_panorama_key_ = 0xFFFFFFFFU;
		panorama_supported_ = false;
		storage_capacity_bytes_ = 0;
		body_capacity_bytes_ = 0;

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

	[[nodiscard]] bool ensure_body_capacity(size_t body_count) {
		const VkDeviceSize required_bytes = static_cast<VkDeviceSize>(std::max<size_t>(body_count, 1)) * sizeof(GpuBodyGpuLayout);
		if (required_bytes <= body_capacity_bytes_ && body_buffer_ != VK_NULL_HANDLE) {
			return true;
		}

		destroy_buffer(body_buffer_, body_memory_, &body_mapped_);
		body_capacity_bytes_ = 0;

		if (!create_buffer(
			required_bytes,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			body_buffer_,
			body_memory_
		)) {
			return false;
		}

		if (vkMapMemory(device_, body_memory_, 0, required_bytes, 0, &body_mapped_) != VK_SUCCESS) {
			destroy_buffer(body_buffer_, body_memory_);
			return false;
		}

		body_capacity_bytes_ = required_bytes;

		VkDescriptorBufferInfo body_info{};
		body_info.buffer = body_buffer_;
		body_info.offset = 0;
		body_info.range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = descriptor_set_;
		write.dstBinding = 2;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		write.pBufferInfo = &body_info;

		vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
		return true;
	}

	[[nodiscard]] bool dispatch_and_readback(const GpuCameraPushConstants& params, std::vector<GpuPixelOutput>& output, std::span<const GpuBodyGpuLayout> bodies = {}) {
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

		if (!ensure_body_capacity(bodies.size())) {
			return false;
		}

		GpuCameraPushConstants actual_params = params;
		actual_params.body_count = static_cast<uint32_t>(bodies.size());
		actual_params.sky_panorama_width = 0U;
		actual_params.sky_panorama_height = 0U;
		if (params.sky_background_source != 0U && panorama_supported_) {
			const uint32_t requested_panorama_key = (static_cast<uint32_t>(params.sky_panorama_id) << 4) | static_cast<uint32_t>(params.sky_panorama_quality);
			if (requested_panorama_key == last_panorama_key_ && panorama_width_ > 0U && panorama_height_ > 0U) {
				actual_params.sky_panorama_width = panorama_width_;
				actual_params.sky_panorama_height = panorama_height_;
			} else {
				const auto panorama = Optics::SkyPanoramaLoader::instance().try_acquire(
					static_cast<Optics::SkyPanoramaId>(params.sky_panorama_id),
					static_cast<Optics::SkyPanoramaQuality>(params.sky_panorama_quality)
				);
				if (panorama != nullptr && upload_panorama(panorama)) {
					last_panorama_key_ = requested_panorama_key;
					actual_params.sky_panorama_width = panorama_width_;
					actual_params.sky_panorama_height = panorama_height_;
				}
			}
		} else {
			last_panorama_key_ = 0xFFFFFFFFU;
		}
		std::memcpy(uniform_mapped_, &actual_params, sizeof(GpuCameraPushConstants));
		if (!bodies.empty() && body_mapped_ != nullptr) {
			std::memcpy(body_mapped_, bodies.data(), bodies.size() * sizeof(GpuBodyGpuLayout));
		}
		if (persistent_counter_mapped_ != nullptr) {
			*static_cast<uint32_t*>(persistent_counter_mapped_) = 0U;
		}

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

		const uint32_t total_pixel_count = params.screen_width * params.screen_height;
		constexpr uint32_t persistent_local_size = 256U;
		constexpr uint32_t persistent_batch_size = 8U;
		const uint32_t work_per_group = persistent_local_size * persistent_batch_size;
		const uint32_t ideal_group_count = (total_pixel_count + work_per_group - 1U) / std::max(work_per_group, 1U);
		const uint32_t persistent_group_count = std::clamp(ideal_group_count, 64U, 4096U);
		vkCmdDispatch(command_buffer_, persistent_group_count, 1, 1);

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

		constexpr uint64_t kComputeDispatchTimeoutNs = 4000000000ULL;
		if (vkWaitForFences(device_, 1, &fence_, VK_TRUE, kComputeDispatchTimeoutNs) != VK_SUCCESS) {
			return false;
		}

		if (!staging_is_coherent_) {
			VkMappedMemoryRange range{};
			range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			range.memory = staging_memory_;
			range.offset = 0;
			range.size = copy_region.size;
			vkInvalidateMappedMemoryRanges(device_, 1, &range);
		}

		if (output.size() != pixel_count) {
			output.resize(pixel_count);
		}
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
