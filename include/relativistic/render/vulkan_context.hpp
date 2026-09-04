#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <optional>
#include <algorithm>

namespace Relativistic::Render {

struct VulkanDeviceInfo {
	std::string device_name{"SoftwareComputeEmulator"};
	uint32_t api_version{0x00403000};
	uint32_t driver_version{1};
	uint32_t vendor_id{0x10DE};
	uint32_t device_id{0x2204};
	bool has_shader_float64{true};
	bool has_shader_int64{true};
	bool has_raytracing_pipeline{false};
	size_t max_workgroup_size_x{1024};
	size_t max_workgroup_size_y{1024};
	size_t max_workgroup_size_z{64};
	size_t max_compute_shared_memory_bytes{49152};
};

class VulkanContext {
private:
	bool initialized_{false};
	bool headless_{true};
	VulkanDeviceInfo device_info_{};

	VkInstance instance_{VK_NULL_HANDLE};
	VkPhysicalDevice physical_device_{VK_NULL_HANDLE};
	VkDevice device_{VK_NULL_HANDLE};
	VkQueue compute_queue_{VK_NULL_HANDLE};
	VkCommandPool command_pool_{VK_NULL_HANDLE};
	uint32_t compute_queue_family_index_{0};

	[[nodiscard]] bool bring_up_vulkan_device(bool prefer_fp64) noexcept {
		VkApplicationInfo app_info{};
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pApplicationName = "RelativisticEngine";
		app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.pEngineName = "RelativisticEngineCompute";
		app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.apiVersion = VK_API_VERSION_1_2;

		VkInstanceCreateInfo instance_info{};
		instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instance_info.pApplicationInfo = &app_info;

		if (vkCreateInstance(&instance_info, nullptr, &instance_) != VK_SUCCESS) {
			return false;
		}

		uint32_t device_count = 0;
		vkEnumeratePhysicalDevices(instance_, &device_count, nullptr);
		if (device_count == 0) {
			return false;
		}

		std::vector<VkPhysicalDevice> devices(device_count);
		vkEnumeratePhysicalDevices(instance_, &device_count, devices.data());

		for (VkPhysicalDevice candidate : devices) {
			VkPhysicalDeviceVulkan12Features supported_12{};
			supported_12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

			VkPhysicalDeviceFeatures2 supported_features{};
			supported_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
			supported_features.pNext = &supported_12;

			vkGetPhysicalDeviceFeatures2(candidate, &supported_features);

			if (prefer_fp64 && supported_features.features.shaderFloat64 == VK_FALSE) {
				continue;
			}
			if (supported_12.scalarBlockLayout == VK_FALSE) {
				continue;
			}

			uint32_t queue_family_count = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(candidate, &queue_family_count, nullptr);
			std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
			vkGetPhysicalDeviceQueueFamilyProperties(candidate, &queue_family_count, queue_families.data());

			std::optional<uint32_t> compute_family;
			for (uint32_t i = 0; i < queue_family_count; ++i) {
				if ((queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0U) {
					compute_family = i;
					break;
				}
			}
			if (!compute_family.has_value()) {
				continue;
			}

			physical_device_ = candidate;
			compute_queue_family_index_ = *compute_family;
			break;
		}

		if (physical_device_ == VK_NULL_HANDLE) {
			return false;
		}

		const float queue_priority = 1.0f;
		VkDeviceQueueCreateInfo queue_info{};
		queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_info.queueFamilyIndex = compute_queue_family_index_;
		queue_info.queueCount = 1;
		queue_info.pQueuePriorities = &queue_priority;

		VkPhysicalDeviceFeatures enabled_features{};
		enabled_features.shaderFloat64 = prefer_fp64 ? VK_TRUE : VK_FALSE;
		enabled_features.shaderInt64 = VK_TRUE;

		VkPhysicalDeviceVulkan12Features enabled_12{};
		enabled_12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		enabled_12.scalarBlockLayout = VK_TRUE;

		VkDeviceCreateInfo device_ci{};
		device_ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_ci.pNext = &enabled_12;
		device_ci.queueCreateInfoCount = 1;
		device_ci.pQueueCreateInfos = &queue_info;
		device_ci.pEnabledFeatures = &enabled_features;

		if (vkCreateDevice(physical_device_, &device_ci, nullptr, &device_) != VK_SUCCESS) {
			return false;
		}

		vkGetDeviceQueue(device_, compute_queue_family_index_, 0, &compute_queue_);

		VkCommandPoolCreateInfo pool_info{};
		pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		pool_info.queueFamilyIndex = compute_queue_family_index_;

		if (vkCreateCommandPool(device_, &pool_info, nullptr, &command_pool_) != VK_SUCCESS) {
			return false;
		}

		VkPhysicalDeviceProperties props{};
		vkGetPhysicalDeviceProperties(physical_device_, &props);

		device_info_.device_name = props.deviceName;
		device_info_.api_version = props.apiVersion;
		device_info_.driver_version = props.driverVersion;
		device_info_.vendor_id = props.vendorID;
		device_info_.device_id = props.deviceID;
		device_info_.has_shader_float64 = true;
		device_info_.has_shader_int64 = true;
		device_info_.max_workgroup_size_x = static_cast<size_t>(props.limits.maxComputeWorkGroupSize[0]);
		device_info_.max_workgroup_size_y = static_cast<size_t>(props.limits.maxComputeWorkGroupSize[1]);
		device_info_.max_workgroup_size_z = static_cast<size_t>(props.limits.maxComputeWorkGroupSize[2]);
		device_info_.max_compute_shared_memory_bytes = static_cast<size_t>(props.limits.maxComputeSharedMemorySize);

		return true;
	}

	void shutdown_vulkan_handles() noexcept {
		if (device_ != VK_NULL_HANDLE) {
			vkDeviceWaitIdle(device_);
		}
		if (command_pool_ != VK_NULL_HANDLE) {
			vkDestroyCommandPool(device_, command_pool_, nullptr);
			command_pool_ = VK_NULL_HANDLE;
		}
		if (device_ != VK_NULL_HANDLE) {
			vkDestroyDevice(device_, nullptr);
			device_ = VK_NULL_HANDLE;
		}
		if (instance_ != VK_NULL_HANDLE) {
			vkDestroyInstance(instance_, nullptr);
			instance_ = VK_NULL_HANDLE;
		}
		physical_device_ = VK_NULL_HANDLE;
		compute_queue_ = VK_NULL_HANDLE;
		compute_queue_family_index_ = 0;
	}

public:
	VulkanContext() = default;

	~VulkanContext() noexcept {
		shutdown();
	}

	VulkanContext(const VulkanContext&) = delete;
	VulkanContext& operator=(const VulkanContext&) = delete;
	VulkanContext(VulkanContext&&) = delete;
	VulkanContext& operator=(VulkanContext&&) = delete;

	[[nodiscard]] bool initialize(bool headless = true, bool prefer_fp64 = true) noexcept {
		headless_ = headless;

		shutdown_vulkan_handles();

		if (!bring_up_vulkan_device(prefer_fp64)) {
			device_info_ = VulkanDeviceInfo{};
			device_info_.has_shader_float64 = prefer_fp64;
		}

		initialized_ = true;
		return true;
	}

	void shutdown() noexcept {
		shutdown_vulkan_handles();
		initialized_ = false;
	}

	[[nodiscard]] bool is_initialized() const noexcept {
		return initialized_;
	}

	[[nodiscard]] bool is_headless() const noexcept {
		return headless_;
	}

	[[nodiscard]] const VulkanDeviceInfo& device_info() const noexcept {
		return device_info_;
	}

	[[nodiscard]] bool supports_fp64() const noexcept {
		return device_info_.has_shader_float64;
	}

	[[nodiscard]] bool has_compute_device() const noexcept {
		return device_ != VK_NULL_HANDLE;
	}

	[[nodiscard]] VkInstance instance() const noexcept {
		return instance_;
	}

	[[nodiscard]] VkPhysicalDevice physical_device() const noexcept {
		return physical_device_;
	}

	[[nodiscard]] VkDevice device() const noexcept {
		return device_;
	}

	[[nodiscard]] VkQueue compute_queue() const noexcept {
		return compute_queue_;
	}

	[[nodiscard]] VkCommandPool command_pool() const noexcept {
		return command_pool_;
	}

	[[nodiscard]] uint32_t compute_queue_family_index() const noexcept {
		return compute_queue_family_index_;
	}
};

}
