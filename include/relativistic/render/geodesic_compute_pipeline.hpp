#pragma once

#include "relativistic/render/gpu_types.hpp"
#include "relativistic/render/vulkan_context.hpp"
#include "relativistic/render/software_compute_engine.hpp"
#include <vector>
#include <span>
#include <memory>
#include <chrono>
#include <cmath>

namespace Relativistic::Render {

struct GeodesicPipelineConfig {
	uint32_t width{3840};
	uint32_t height{2160};
	PrecisionMode precision{PrecisionMode::NativeFloat64};
	MetricId metric{MetricId::Schwarzschild};
	double field_of_view_deg{60.0};
	uint32_t max_steps{2048};
	double initial_step{-0.05};
	bool headless{true};
};

struct PipelineExecutionTelemetry {
	double execution_time_ms{0.0};
	double frame_rate_fps{0.0};
	uint64_t total_pixels_processed{0};
	uint64_t horizon_pixels_absorbed{0};
	uint64_t celestial_pixels_hit{0};
};

class GeodesicComputePipeline {
private:
	GeodesicPipelineConfig config_;
	VulkanContext context_;
	std::vector<GpuPixelOutput> framebuffer_;
	PipelineExecutionTelemetry telemetry_{};

public:
	explicit GeodesicComputePipeline(const GeodesicPipelineConfig& config = {})
		: config_(config) {
		framebuffer_.resize(config_.width * config_.height);
		static_cast<void>(context_.initialize(config_.headless, config_.precision == PrecisionMode::NativeFloat64));
	}

	void resize(uint32_t width, uint32_t height) {
		config_.width = width;
		config_.height = height;
		framebuffer_.resize(width * height);
	}

	void set_precision_mode(PrecisionMode mode) noexcept {
		config_.precision = mode;
	}

	[[nodiscard]] const GeodesicPipelineConfig& config() const noexcept {
		return config_;
	}

	[[nodiscard]] const VulkanContext& context() const noexcept {
		return context_;
	}

	[[nodiscard]] std::span<const GpuPixelOutput> framebuffer() const noexcept {
		return framebuffer_;
	}

	[[nodiscard]] std::span<GpuPixelOutput> framebuffer() noexcept {
		return framebuffer_;
	}

	[[nodiscard]] const PipelineExecutionTelemetry& telemetry() const noexcept {
		return telemetry_;
	}

	void dispatch(const GpuCameraPushConstants& camera_constants) {
		const auto t_start = std::chrono::high_resolution_clock::now();

		if (config_.precision == PrecisionMode::NativeFloat64) {
			SoftwareComputeEngine::dispatch_fp64(camera_constants, framebuffer_);
		} else {
			SoftwareComputeEngine::dispatch_double_single(camera_constants, framebuffer_);
		}

		const auto t_end = std::chrono::high_resolution_clock::now();
		const double duration_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();

		telemetry_.execution_time_ms = duration_ms;
		telemetry_.frame_rate_fps = (duration_ms > 0.0) ? (1000.0 / duration_ms) : 0.0;
		telemetry_.total_pixels_processed = config_.width * config_.height;

		uint64_t absorbed = 0;
		uint64_t celestial = 0;
		for (const auto& px : framebuffer_) {
			if (px.status_flags == PixelFlags::HORIZON_ABSORBED) ++absorbed;
			else if (px.status_flags == PixelFlags::CELESTIAL_HIT) ++celestial;
		}
		telemetry_.horizon_pixels_absorbed = absorbed;
		telemetry_.celestial_pixels_hit = celestial;
	}
};

}
