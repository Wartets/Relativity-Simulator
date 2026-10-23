#pragma once

#include "relativistic/core/diagnostics/diagnostics_registry.hpp"
#include <vulkan/vulkan.h>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace Relativistic::Render {

class VulkanDeviceReport {
private:
	[[nodiscard]] static const char* device_type_name(VkPhysicalDeviceType type) noexcept {
		switch (type) {
			case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: return "discrete GPU";
			case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return "integrated GPU";
			case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: return "virtual GPU";
			case VK_PHYSICAL_DEVICE_TYPE_CPU: return "CPU software device";
			default: return "other";
		}
	}

	[[nodiscard]] static const char* vendor_name(uint32_t id) noexcept {
		switch (id) {
			case 0x10DEU: return "NVIDIA";
			case 0x1002U: return "AMD";
			case 0x8086U: return "Intel";
			case 0x13B5U: return "ARM";
			case 0x5143U: return "Qualcomm";
			case 0x106BU: return "Apple";
			case 0x1010U: return "Imagination";
			case 0x10005U: return "Mesa";
			default: return "unknown vendor";
		}
	}

	[[nodiscard]] static std::string version_text(uint32_t version) {
		char text[32];
		std::snprintf(text, sizeof(text), "%u.%u.%u", version >> 22U, (version >> 12U) & 0x3FFU, version & 0xFFFU);
		return text;
	}

	[[nodiscard]] static std::string bytes_text(uint64_t bytes) {
		static constexpr const char* units[] = {"B", "KiB", "MiB", "GiB", "TiB"};
		double value = static_cast<double>(bytes);
		size_t unit = 0;
		while (value >= 1024.0 && unit + 1 < 5) {
			value /= 1024.0;
			++unit;
		}
		char text[48];
		std::snprintf(text, sizeof(text), "%.2f %s", value, units[unit]);
		return text;
	}

	static void describe(VkInstance instance, VkPhysicalDevice selected) {
		using Core::Diagnostics::append_format;
		using Core::Diagnostics::set_environment;
		std::string text;

		uint32_t loader_version = VK_API_VERSION_1_0;
		using EnumerateVersion = VkResult (*)(uint32_t*);
		const auto enumerate_version = reinterpret_cast<EnumerateVersion>(reinterpret_cast<void*>(vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion")));
		if (enumerate_version != nullptr) {
			static_cast<void>(enumerate_version(&loader_version));
		}
		append_format(text, "  Loader API version   %s\n", version_text(loader_version).c_str());
		set_environment("gpu.loader.version", version_text(loader_version));

		uint32_t layer_count = 0;
		vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
		std::vector<VkLayerProperties> layers(layer_count);
		if (layer_count > 0) {
			vkEnumerateInstanceLayerProperties(&layer_count, layers.data());
		}
		std::string layer_names;
		for (const VkLayerProperties& layer : layers) {
			if (!layer_names.empty()) {
				layer_names += ", ";
			}
			layer_names += layer.layerName;
		}
		append_format(text, "  Instance layers      %u: %s\n", layer_count, layer_names.empty() ? "none" : layer_names.c_str());
		set_environment("gpu.layers", layer_names.empty() ? std::string("none") : layer_names);

		uint32_t instance_extension_count = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, nullptr);
		append_format(text, "  Instance extensions  %u available\n", instance_extension_count);

		std::vector<VkPhysicalDevice> devices;
		if (instance != VK_NULL_HANDLE) {
			uint32_t device_count = 0;
			vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
			devices.resize(device_count);
			if (device_count > 0) {
				vkEnumeratePhysicalDevices(instance, &device_count, devices.data());
			}
		}
		append_format(text, "  Physical devices     %zu enumerated\n", devices.size());
		set_environment("gpu.enumerated.count", std::to_string(devices.size()));
		for (size_t index = 0; index < devices.size(); ++index) {
			VkPhysicalDeviceProperties properties{};
			vkGetPhysicalDeviceProperties(devices[index], &properties);
			append_format(text, "    [%zu]%s %s | %s | %s | API %s | driver 0x%08X | vendor 0x%04X device 0x%04X\n", index, (devices[index] == selected) ? " (selected)" : "", properties.deviceName, device_type_name(properties.deviceType), vendor_name(properties.vendorID), version_text(properties.apiVersion).c_str(), properties.driverVersion, properties.vendorID, properties.deviceID);
		}

		if (selected == VK_NULL_HANDLE) {
			append_format(text, "  No device was selected: none satisfied the compute queue, scalar block layout and FP64 requirements.\n");
			Core::Diagnostics::publish_vulkan_snapshot(text);
			return;
		}

		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(selected, &properties);
		VkPhysicalDeviceDriverProperties driver{};
		driver.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;
		if (properties.apiVersion >= VK_API_VERSION_1_2) {
			VkPhysicalDeviceProperties2 properties2{};
			properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
			properties2.pNext = &driver;
			vkGetPhysicalDeviceProperties2(selected, &properties2);
		}
		append_format(text, "\n  Selected device      %s (%s, %s)\n", properties.deviceName, device_type_name(properties.deviceType), vendor_name(properties.vendorID));
		append_format(text, "  Device API version   %s\n", version_text(properties.apiVersion).c_str());
		append_format(text, "  Driver               %s | %s | raw version 0x%08X\n", driver.driverName, driver.driverInfo, properties.driverVersion);
		append_format(text, "  Conformance          %u.%u.%u.%u\n", static_cast<unsigned>(driver.conformanceVersion.major), static_cast<unsigned>(driver.conformanceVersion.minor), static_cast<unsigned>(driver.conformanceVersion.subminor), static_cast<unsigned>(driver.conformanceVersion.patch));
		set_environment("gpu.api.version", version_text(properties.apiVersion));
		set_environment("gpu.driver.name", driver.driverName);
		set_environment("gpu.driver.info", driver.driverInfo);

		const VkPhysicalDeviceLimits& limits = properties.limits;
		append_format(text, "  Limits               image2D %u, shared memory %u B, workgroup invocations %u, workgroup size %ux%ux%u\n", limits.maxImageDimension2D, limits.maxComputeSharedMemorySize, limits.maxComputeWorkGroupInvocations, limits.maxComputeWorkGroupSize[0], limits.maxComputeWorkGroupSize[1], limits.maxComputeWorkGroupSize[2]);
		append_format(text, "  Limits               storage buffer %u B, uniform buffer %u B, push constants %u B, allocations %u, descriptor sets %u\n", limits.maxStorageBufferRange, limits.maxUniformBufferRange, limits.maxPushConstantsSize, limits.maxMemoryAllocationCount, limits.maxBoundDescriptorSets);
		set_environment("gpu.limit.shared.memory", std::to_string(limits.maxComputeSharedMemorySize));
		set_environment("gpu.limit.workgroup.invocations", std::to_string(limits.maxComputeWorkGroupInvocations));

		VkPhysicalDeviceVulkan12Features features12{};
		features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		VkPhysicalDeviceFeatures2 features{};
		features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		features.pNext = &features12;
		vkGetPhysicalDeviceFeatures2(selected, &features);
		append_format(text, "  Features             fp64 %s, int64 %s, robust buffer access %s, scalar block layout %s\n", features.features.shaderFloat64 ? "yes" : "no", features.features.shaderInt64 ? "yes" : "no", features.features.robustBufferAccess ? "yes" : "no", features12.scalarBlockLayout ? "yes" : "no");

		VkPhysicalDeviceMemoryProperties memory{};
		vkGetPhysicalDeviceMemoryProperties(selected, &memory);
		uint64_t device_local_total = 0;
		for (uint32_t heap = 0; heap < memory.memoryHeapCount; ++heap) {
			const bool device_local = (memory.memoryHeaps[heap].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0U;
			if (device_local) {
				device_local_total += memory.memoryHeaps[heap].size;
			}
			append_format(text, "  Memory heap %u        %s%s\n", heap, bytes_text(memory.memoryHeaps[heap].size).c_str(), device_local ? " (device local)" : "");
		}
		for (uint32_t type = 0; type < memory.memoryTypeCount; ++type) {
			const VkMemoryPropertyFlags flags = memory.memoryTypes[type].propertyFlags;
			append_format(text, "  Memory type %u        heap %u:%s%s%s%s\n", type, memory.memoryTypes[type].heapIndex, (flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ? " device-local" : "", (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) ? " host-visible" : "", (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) ? " host-coherent" : "", (flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) ? " host-cached" : "");
		}
		set_environment("gpu.vram.device.local", bytes_text(device_local_total));

		uint32_t family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(selected, &family_count, nullptr);
		std::vector<VkQueueFamilyProperties> families(family_count);
		if (family_count > 0) {
			vkGetPhysicalDeviceQueueFamilyProperties(selected, &family_count, families.data());
		}
		for (uint32_t family = 0; family < family_count; ++family) {
			const VkQueueFlags flags = families[family].queueFlags;
			append_format(text, "  Queue family %u       %u queue(s):%s%s%s%s\n", family, families[family].queueCount, (flags & VK_QUEUE_GRAPHICS_BIT) ? " graphics" : "", (flags & VK_QUEUE_COMPUTE_BIT) ? " compute" : "", (flags & VK_QUEUE_TRANSFER_BIT) ? " transfer" : "", (flags & VK_QUEUE_SPARSE_BINDING_BIT) ? " sparse" : "");
		}

		uint32_t extension_count = 0;
		vkEnumerateDeviceExtensionProperties(selected, nullptr, &extension_count, nullptr);
		std::vector<VkExtensionProperties> extensions(extension_count);
		if (extension_count > 0) {
			vkEnumerateDeviceExtensionProperties(selected, nullptr, &extension_count, extensions.data());
		}
		std::string extension_names;
		for (const VkExtensionProperties& extension : extensions) {
			if (!extension_names.empty()) {
				extension_names += ' ';
			}
			extension_names += extension.extensionName;
		}
		append_format(text, "  Device extensions    %u available\n", extension_count);
		text += "    ";
		text += extension_names;
		text += '\n';
		set_environment("gpu.extension.count", std::to_string(extension_count));

		Core::Diagnostics::publish_vulkan_snapshot(text);
	}

public:
	static void publish(VkInstance instance, VkPhysicalDevice selected) noexcept {
		try {
			describe(instance, selected);
		} catch (...) {
		}
	}
};

}
