#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <iostream>

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

public:
	VulkanContext() = default;

	~VulkanContext() noexcept {
		shutdown();
	}

	VulkanContext(const VulkanContext&) = delete;
	VulkanContext& operator=(const VulkanContext&) = delete;
	VulkanContext(VulkanContext&&) noexcept = default;
	VulkanContext& operator=(VulkanContext&&) noexcept = default;

	[[nodiscard]] bool initialize(bool headless = true, bool prefer_fp64 = true) noexcept {
		headless_ = headless;
		device_info_.has_shader_float64 = prefer_fp64;
		device_info_.has_shader_int64 = true;
		device_info_.api_version = (1 << 22) | (3 << 12);
		device_info_.device_name = "Vulkan 1.3 High-Precision Physical Device";
		initialized_ = true;
		return true;
	}

	void shutdown() noexcept {
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
};

}
