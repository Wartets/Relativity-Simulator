#pragma once

#include "relativistic/render/vulkan_context.hpp"
#include "relativistic/render/gpu_types.hpp"
#include "relativistic/optics/textures/sky_panorama_image.hpp"
#include "relativistic/optics/textures/earth_texture_image.hpp"
#include "relativistic/render/bodies/earth_texture_requirements.hpp"
#include <thread>
#include "relativistic/core/engine_log.hpp"
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
#include <atomic>
#include <chrono>

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

	struct EarthTextureSlot {
		VkImage image{VK_NULL_HANDLE};
		VkDeviceMemory memory{VK_NULL_HANDLE};
		VkImageView view{VK_NULL_HANDLE};
		uint32_t width{0};
		uint32_t height{0};
		Optics::EarthMapQuality quality{Optics::EarthMapQuality::Q1K};
		bool resident{false};
		bool rejected{false};
	};

	static constexpr uint32_t kEarthDescriptorBinding = 5;

	VkSampler earth_sampler_{VK_NULL_HANDLE};
	EarthTextureSlot earth_placeholder_{};
	std::array<EarthTextureSlot, Optics::kEarthMapKindCount> earth_slots_{};
	Optics::EarthTextureIdleGate earth_idle_gate_{};
	bool earth_supported_{false};

	bool ready_{false};
	bool device_lost_{false};

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
		if (size < 20 || (size % 4) != 0) {
			return std::nullopt;
		}
		std::vector<uint32_t> code(static_cast<size_t>(size) / 4);
		file.seekg(0);
		file.read(reinterpret_cast<char*>(code.data()), size);
		if (!file) {
			return std::nullopt;
		}
		if (code.front() != 0x07230203U) {
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

	[[nodiscard]] bool create_panorama_image(uint32_t width, uint32_t height, VkImage& out_image, VkDeviceMemory& out_memory, uint32_t mip_levels = 1U) const noexcept {
		VkImageCreateInfo image_info{};
		image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image_info.imageType = VK_IMAGE_TYPE_2D;
		image_info.format = VK_FORMAT_R8G8B8A8_SRGB;
		image_info.extent = VkExtent3D{width, height, 1};
		image_info.mipLevels = mip_levels;
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

	[[nodiscard]] bool submit_texture_upload(VkBuffer source, VkImage destination, std::span<const VkBufferImageCopy> regions, uint32_t mip_levels) noexcept {
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
		to_transfer.subresourceRange.levelCount = mip_levels;
		to_transfer.subresourceRange.layerCount = 1;
		to_transfer.srcAccessMask = 0;
		to_transfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		vkCmdPipelineBarrier(
			command_buffer_,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0, 0, nullptr, 0, nullptr, 1, &to_transfer
		);

		vkCmdCopyBufferToImage(command_buffer_, source, destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(regions.size()), regions.data());

		VkImageMemoryBarrier to_shader_read{};
		to_shader_read.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		to_shader_read.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		to_shader_read.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		to_shader_read.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		to_shader_read.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		to_shader_read.image = destination;
		to_shader_read.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		to_shader_read.subresourceRange.levelCount = mip_levels;
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
			Core::log_error("GPU texture upload submission failed; falling back to the CPU renderer for subsequent frames.");
			device_lost_ = true;
			ready_ = false;
			return false;
		}
		constexpr uint64_t kPanoramaUploadTimeoutNs = 3000000000ULL;
		const VkResult panorama_fence_result = vkWaitForFences(device_, 1, &fence_, VK_TRUE, kPanoramaUploadTimeoutNs);
		if (panorama_fence_result != VK_SUCCESS) {
			Core::log_error("GPU texture upload timed out or the device was lost; falling back to the CPU renderer for subsequent frames.");
			device_lost_ = true;
			ready_ = false;
			return false;
		}
		return true;
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

		VkBufferImageCopy copy_region{};
		copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copy_region.imageSubresource.layerCount = 1;
		copy_region.imageExtent = VkExtent3D{width, height, 1};
		const bool copied = submit_texture_upload(upload_buffer, new_image, std::span<const VkBufferImageCopy>(&copy_region, 1), 1U);
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

	void destroy_earth_texture(EarthTextureSlot& slot) noexcept {
		if (slot.view != VK_NULL_HANDLE) {
			vkDestroyImageView(device_, slot.view, nullptr);
			slot.view = VK_NULL_HANDLE;
		}
		if (slot.image != VK_NULL_HANDLE) {
			vkDestroyImage(device_, slot.image, nullptr);
			slot.image = VK_NULL_HANDLE;
		}
		if (slot.memory != VK_NULL_HANDLE) {
			vkFreeMemory(device_, slot.memory, nullptr);
			slot.memory = VK_NULL_HANDLE;
		}
		slot.width = 0;
		slot.height = 0;
		slot.resident = false;
	}

	void write_earth_descriptor(size_t kind_index, VkImageView view) noexcept {
		VkDescriptorImageInfo image_info{};
		image_info.sampler = earth_sampler_;
		image_info.imageView = view;
		image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = descriptor_set_;
		write.dstBinding = kEarthDescriptorBinding + static_cast<uint32_t>(kind_index);
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.pImageInfo = &image_info;

		vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
	}

	[[nodiscard]] bool upload_earth_texture(const Optics::EarthTextureImage& image, EarthTextureSlot& destination) {
		if (!image.is_valid()) {
			return false;
		}

		const auto& base = image.levels.front();
		VkPhysicalDeviceProperties device_properties{};
		vkGetPhysicalDeviceProperties(physical_device_, &device_properties);
		if (base.width > device_properties.limits.maxImageDimension2D || base.height > device_properties.limits.maxImageDimension2D) {
			return false;
		}

		const uint32_t level_count = static_cast<uint32_t>(image.levels.size());
		std::vector<VkBufferImageCopy> regions;
		regions.reserve(level_count);
		VkDeviceSize total_bytes = 0;
		for (uint32_t level = 0; level < level_count; ++level) {
			const auto& mip = image.levels[level];
			if (!mip.is_valid()) {
				return false;
			}
			VkBufferImageCopy region{};
			region.bufferOffset = total_bytes;
			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = level;
			region.imageSubresource.layerCount = 1;
			region.imageExtent = VkExtent3D{mip.width, mip.height, 1};
			regions.push_back(region);
			total_bytes += static_cast<VkDeviceSize>(mip.texels.size()) * sizeof(uint32_t);
		}

		VkImage new_image{VK_NULL_HANDLE};
		VkDeviceMemory new_memory{VK_NULL_HANDLE};
		if (!create_panorama_image(base.width, base.height, new_image, new_memory, level_count)) {
			return false;
		}

		VkImageViewCreateInfo view_info{};
		view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_info.image = new_image;
		view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view_info.format = VK_FORMAT_R8G8B8A8_SRGB;
		view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		view_info.subresourceRange.levelCount = level_count;
		view_info.subresourceRange.layerCount = 1;

		VkImageView new_view{VK_NULL_HANDLE};
		if (vkCreateImageView(device_, &view_info, nullptr, &new_view) != VK_SUCCESS) {
			vkDestroyImage(device_, new_image, nullptr);
			vkFreeMemory(device_, new_memory, nullptr);
			return false;
		}

		const auto discard_new_resources = [&]() noexcept {
			vkDestroyImageView(device_, new_view, nullptr);
			vkDestroyImage(device_, new_image, nullptr);
			vkFreeMemory(device_, new_memory, nullptr);
		};

		VkBuffer upload_buffer{VK_NULL_HANDLE};
		VkDeviceMemory upload_memory{VK_NULL_HANDLE};
		if (!create_buffer(
			total_bytes,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			upload_buffer,
			upload_memory
		)) {
			discard_new_resources();
			return false;
		}

		void* mapped = nullptr;
		if (vkMapMemory(device_, upload_memory, 0, total_bytes, 0, &mapped) != VK_SUCCESS) {
			destroy_buffer(upload_buffer, upload_memory);
			discard_new_resources();
			return false;
		}
		for (uint32_t level = 0; level < level_count; ++level) {
			const auto& mip = image.levels[level];
			std::memcpy(static_cast<uint8_t*>(mapped) + regions[level].bufferOffset, mip.texels.data(), mip.texels.size() * sizeof(uint32_t));
		}
		vkUnmapMemory(device_, upload_memory);

		const bool copied = submit_texture_upload(upload_buffer, new_image, regions, level_count);
		destroy_buffer(upload_buffer, upload_memory);
		if (!copied) {
			discard_new_resources();
			return false;
		}

		destroy_earth_texture(destination);
		destination.image = new_image;
		destination.memory = new_memory;
		destination.view = new_view;
		destination.width = base.width;
		destination.height = base.height;
		destination.resident = true;
		return true;
	}

	void release_earth_slot(size_t index) noexcept {
		EarthTextureSlot& slot = earth_slots_[index];
		slot.rejected = false;
		if (!slot.resident) {
			return;
		}
		write_earth_descriptor(index, earth_placeholder_.view);
		destroy_earth_texture(slot);
	}

	[[nodiscard]] bool synchronize_earth_textures(const EarthTextureRequirements& requirements) {
		if (!earth_supported_) {
			return false;
		}

		auto& loader = Optics::EarthTextureLoader::instance();
		if (earth_idle_gate_.should_release(requirements.any())) {
			for (size_t index = 0; index < earth_slots_.size(); ++index) {
				release_earth_slot(index);
			}
			loader.release_all();
			return false;
		}

		const std::array<bool, Optics::kEarthMapKindCount> demanded{requirements.day, requirements.night};
		bool pending = false;
		for (size_t index = 0; index < earth_slots_.size(); ++index) {
			const auto kind = static_cast<Optics::EarthMapKind>(index);
			EarthTextureSlot& slot = earth_slots_[index];

			if (!demanded[index]) {
				if (requirements.any()) {
					release_earth_slot(index);
					loader.release_kind(kind);
				}
				continue;
			}
			if (slot.resident && slot.quality == requirements.quality) {
				continue;
			}
			if (slot.rejected && slot.quality == requirements.quality) {
				continue;
			}

			const auto image = loader.try_acquire(kind, requirements.quality);
			if (image == nullptr) {
				pending = pending || loader.is_decoding(kind, requirements.quality);
				continue;
			}

			if (upload_earth_texture(*image, slot)) {
				slot.quality = requirements.quality;
				slot.rejected = false;
				write_earth_descriptor(index, slot.view);
			} else {
				slot.quality = requirements.quality;
				slot.rejected = true;
				Core::log_error("Earth texture could not be uploaded to the GPU, the body keeps its base color.");
			}
			loader.release(kind, requirements.quality);
		}
		return pending;
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

		std::array<VkDescriptorSetLayoutBinding, 7> bindings{};
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

		bindings[5].binding = 5;
		bindings[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bindings[5].descriptorCount = 1;
		bindings[5].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		bindings[6].binding = 6;
		bindings[6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bindings[6].descriptorCount = 1;
		bindings[6].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

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
		pool_sizes[2] = VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3};

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
		panorama_supported_ = (vkCreateSampler(device_, &panorama_sampler_info, nullptr, &panorama_sampler_) == VK_SUCCESS);

		if (panorama_supported_) {
			(void)ensure_panorama_placeholder();
		}

		VkSamplerCreateInfo earth_sampler_info{};
		earth_sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		earth_sampler_info.magFilter = VK_FILTER_LINEAR;
		earth_sampler_info.minFilter = VK_FILTER_LINEAR;
		earth_sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		earth_sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		earth_sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		earth_sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		earth_sampler_info.minLod = 0.0f;
		earth_sampler_info.maxLod = VK_LOD_CLAMP_NONE;
		earth_sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		earth_supported_ = (vkCreateSampler(device_, &earth_sampler_info, nullptr, &earth_sampler_) == VK_SUCCESS);
		if (earth_supported_) {
			earth_supported_ = upload_earth_texture(Optics::EarthTextureImage::make_placeholder(), earth_placeholder_);
		}
		if (!earth_supported_) {
			return false;
		}
		for (size_t index = 0; index < earth_slots_.size(); ++index) {
			write_earth_descriptor(index, earth_placeholder_.view);
		}

		ready_ = true;
		return true;
	}

	[[nodiscard]] bool is_ready() const noexcept {
		return ready_ && !device_lost_;
	}

	[[nodiscard]] bool await_earth_textures(const EarthTextureRequirements& requirements, std::chrono::milliseconds timeout) {
		const auto deadline = std::chrono::steady_clock::now() + timeout;
		while (synchronize_earth_textures(requirements)) {
			if (!is_ready() || std::chrono::steady_clock::now() >= deadline) {
				return false;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}
		return true;
	}

	void shutdown() noexcept {
		if (device_ == VK_NULL_HANDLE) {
			ready_ = false;
			return;
		}
		if (!device_lost_) {
			vkDeviceWaitIdle(device_);
		}

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
		for (auto& earth_slot : earth_slots_) {
			destroy_earth_texture(earth_slot);
		}
		destroy_earth_texture(earth_placeholder_);
		if (earth_sampler_ != VK_NULL_HANDLE) {
			vkDestroySampler(device_, earth_sampler_, nullptr);
			earth_sampler_ = VK_NULL_HANDLE;
		}
		earth_supported_ = false;
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

	static constexpr VkDeviceSize kMaxBandBytes = 64ULL * 1024ULL * 1024ULL;
	static constexpr uint32_t kBandRowAlignment = 16U;
	static constexpr double kBandTargetMs = 120.0;

	[[nodiscard]] bool dispatch_and_readback(
		const GpuCameraPushConstants& params,
		std::vector<GpuPixelOutput>& output,
		std::span<const GpuBodyGpuLayout> bodies = {},
		const std::atomic<bool>* cancel_flag = nullptr
	) {
		if (!is_ready()) {
			return false;
		}

		const uint32_t width = params.screen_width;
		const uint32_t height = params.screen_height;
		if (width == 0U || height == 0U) {
			return false;
		}

		const uint32_t window_first = std::min(params.dispatch_row_offset, height);
		const uint32_t window_rows = (params.dispatch_row_count == 0U)
			? (height - window_first)
			: std::min(params.dispatch_row_count, height - window_first);
		if (window_rows == 0U) {
			return false;
		}
		const uint32_t window_end = window_first + window_rows;
		const size_t window_pixels = static_cast<size_t>(width) * static_cast<size_t>(window_rows);
		if (output.size() != window_pixels) {
			output.resize(window_pixels);
		}

		const uint64_t rows_by_memory = std::max<uint64_t>(
			kBandRowAlignment,
			(kMaxBandBytes / sizeof(GpuPixelOutput) / width) / kBandRowAlignment * kBandRowAlignment
		);
		const uint32_t max_rows = static_cast<uint32_t>(std::min<uint64_t>(rows_by_memory, window_rows));
		const uint32_t min_rows = std::min(kBandRowAlignment, max_rows);

		uint32_t rows = min_rows;
		uint32_t row = window_first;
		while (row < window_end) {
			if (cancel_flag != nullptr && cancel_flag->load(std::memory_order_relaxed)) {
				return false;
			}

			const uint32_t band_rows = std::min(rows, window_end - row);
			GpuCameraPushConstants band_params = params;
			band_params.dispatch_row_offset = row;
			band_params.dispatch_row_count = band_rows;

			const auto band_start = std::chrono::steady_clock::now();
			if (!dispatch_band(band_params, output, bodies, window_first)) {
				return false;
			}
			const double band_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - band_start).count();

			row += band_rows;
			const double scale = std::clamp(kBandTargetMs / std::max(band_ms, 1.0), 0.5, 2.0);
			const uint32_t scaled = static_cast<uint32_t>(std::max(1.0, std::round(static_cast<double>(rows) * scale)));
			const uint32_t aligned = ((scaled + kBandRowAlignment - 1U) / kBandRowAlignment) * kBandRowAlignment;
			rows = std::clamp(aligned, min_rows, max_rows);
		}
		return true;
	}

private:
	[[nodiscard]] bool dispatch_band(const GpuCameraPushConstants& params, std::vector<GpuPixelOutput>& output, std::span<const GpuBodyGpuLayout> bodies, uint32_t row_origin) {
		if (!is_ready()) {
			return false;
		}

		const size_t pixel_count = static_cast<size_t>(params.screen_width) * static_cast<size_t>(params.dispatch_row_count);
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
		static_cast<void>(synchronize_earth_textures(EarthTextureRequirements::gather(bodies)));
		actual_params.earth_day_width = earth_slots_[0].resident ? earth_slots_[0].width : 0U;
		actual_params.earth_night_width = earth_slots_[1].resident ? earth_slots_[1].width : 0U;
		std::memcpy(uniform_mapped_, &actual_params, sizeof(GpuCameraPushConstants));
		if (!bodies.empty() && body_mapped_ != nullptr) {
			std::memcpy(body_mapped_, bodies.data(), bodies.size() * sizeof(GpuBodyGpuLayout));
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

		const uint32_t tiles_x = (params.screen_width + 15U) / 16U;
		const uint32_t tiles_y = (params.dispatch_row_count + 15U) / 16U;
		vkCmdDispatch(command_buffer_, tiles_x, tiles_y, 1);

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
			Core::log_error("GPU compute queue submission failed; falling back to the CPU renderer for subsequent frames.");
			device_lost_ = true;
			ready_ = false;
			return false;
		}

		constexpr uint64_t kComputeDispatchTimeoutNs = 10000000000ULL;
		const VkResult fence_wait_result = vkWaitForFences(device_, 1, &fence_, VK_TRUE, kComputeDispatchTimeoutNs);
		if (fence_wait_result != VK_SUCCESS) {
			Core::log_error("GPU compute dispatch timed out or the device was lost; falling back to the CPU renderer for subsequent frames.");
			device_lost_ = true;
			ready_ = false;
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

		const size_t first_pixel = static_cast<size_t>(params.dispatch_row_offset - row_origin) * static_cast<size_t>(params.screen_width);
		if (output.size() < first_pixel + pixel_count) {
			return false;
		}
		std::memcpy(output.data() + first_pixel, staging_mapped_, static_cast<size_t>(copy_region.size));
		return true;
	}

	public:
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
