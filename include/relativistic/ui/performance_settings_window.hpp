#pragma once

#include <imgui.h>
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/orchestrator/command.hpp"
#include "relativistic/render/gpu_types.hpp"
#include "relativistic/render/geodesic_compute_pipeline.hpp"
#include <algorithm>
#include <array>

namespace Relativistic::UI {

class PerformanceSettingsWindow {
private:
	bool is_open_{true};
	Orchestrator::SimulationOrchestrator<1024>& orchestrator_;
	Render::GeodesicComputePipeline* render_pipeline_{nullptr};

	int preset_idx_{2};
	float res_scale_{1.0f};
	int ray_steps_{2048};
	int precision_mode_{0};
	bool enable_dynamic_resolution_{false};
	float target_framerate_{60.0f};
	uint64_t last_synced_version_{0};

public:
	explicit PerformanceSettingsWindow(Orchestrator::SimulationOrchestrator<1024>& orchestrator)
		: orchestrator_(orchestrator) {
		sync_from_orchestrator();
	}

	[[nodiscard]] bool& open_state() noexcept {
		return is_open_;
	}

	void attach_render_pipeline(Render::GeodesicComputePipeline& pipeline) noexcept {
		render_pipeline_ = &pipeline;
	}

	void sync_from_orchestrator() noexcept {
		const auto& p = orchestrator_.parameters();
		preset_idx_ = static_cast<int>(p.performance_preset);
		res_scale_ = static_cast<float>(p.resolution_scale);
		ray_steps_ = static_cast<int>(p.max_ray_steps);
		precision_mode_ = static_cast<int>(orchestrator_.get_custom_param("precision_mode", 0.0));
	}

	void render() {
		if (!is_open_) return;

		const uint64_t current_ver = orchestrator_.state_version();
		if (current_ver != last_synced_version_) {
			sync_from_orchestrator();
			last_synced_version_ = current_ver;
		} else if (!ImGui::IsAnyItemActive()) {
			sync_from_orchestrator();
		}

		ImGui::SetNextWindowPos(ImVec2(880.0f, 750.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(560.0f, 290.0f), ImGuiCond_FirstUseEver);

		if (ImGui::Begin("Performance & Engine Optimization", &is_open_)) {
			ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Hardware & Computation Profiles");
			ImGui::Separator();

			const char* presets[] = {
				"Performance (0.5x scale, 512 steps)",
				"Balanced (0.75x scale, 1024 steps)",
				"Quality High (1.0x scale, 2048 steps)",
				"Ultra Fidelity (1.25x scale, 4096 steps)",
				"Scientific Extreme (1.5x scale, 8192 steps)",
				"Custom (User Defined)"
			};

			if (ImGui::Combo("Preset Profile", &preset_idx_, presets, IM_ARRAYSIZE(presets))) {
				if (preset_idx_ < 5) {
					static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_performance_preset(static_cast<uint32_t>(preset_idx_))));
					sync_from_orchestrator();
				}
			}
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
				ImGui::SetTooltip("Quick preset configuring internal render scale, maximum geodesic integration steps, and tolerances.");
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Rasterization & Ray Budget Configuration");

			if (ImGui::SliderFloat("Internal Render Scale", &res_scale_, 0.10f, 2.00f, "%.2fx")) {
				preset_idx_ = 5;
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_resolution_scale(static_cast<double>(res_scale_))));
			}
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
				ImGui::SetTooltip("Resolution scaling factor relative to the viewport window size. Lower values improve rendering framerates.");
			}

			if (ImGui::SliderInt("Max Geodesic Steps", &ray_steps_, 64, 8192)) {
				preset_idx_ = 5;
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_render_steps(static_cast<uint64_t>(ray_steps_))));
			}
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
				ImGui::SetTooltip("Maximum numerical integration steps allowed per ray before terminating trajectory evaluation.");
			}

			const char* precisions[] = {
				"IEEE 754 Float64 (Hardware Native)",
				"Double-Single Emulation (fp32/fp32 Pair)"
			};

			if (ImGui::Combo("Arithmetic Precision", &precision_mode_, precisions, IM_ARRAYSIZE(precisions))) {
				preset_idx_ = 5;
				orchestrator_.set_physical_param(Orchestrator::ParameterType::Custom, static_cast<double>(precision_mode_), "precision_mode");
			}
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
				ImGui::SetTooltip("Select native 64-bit IEEE 754 floating point arithmetic or compensated double-single emulation.");
			}

			bool use_simd = !(orchestrator_.parameters().visual_overlays_flags & Render::RenderFlags::USE_SCALAR_PIPELINE);
			if (ImGui::Checkbox("SIMD Vector Geodesic Bundles (4x Lanes)", &use_simd)) {
				preset_idx_ = 5;
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_visual_overlay(Render::RenderFlags::USE_SCALAR_PIPELINE, !use_simd)));
			}
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
				ImGui::SetTooltip("Evaluate bundles of 4 geodesic rays simultaneously using AVX2/AVX-512 vector registers.");
			}

			bool use_pool = !(orchestrator_.parameters().visual_overlays_flags & Render::RenderFlags::USE_PER_FRAME_THREADS);
			if (ImGui::Checkbox("Persistent Thread Pool Work Distribution", &use_pool)) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_visual_overlay(Render::RenderFlags::USE_PER_FRAME_THREADS, !use_pool)));
			}
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
				ImGui::SetTooltip("Use persistent worker threads synchronized via condition variables instead of spawning new threads per frame.");
			}

			bool use_tiling = (orchestrator_.parameters().visual_overlays_flags & Render::RenderFlags::USE_TILED_DISTRIBUTION) != 0U;
			if (ImGui::Checkbox("Tiled Work Distribution (32x32 Tiles)", &use_tiling)) {
				preset_idx_ = 5;
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::WorkDistributionMode, use_tiling ? 1.0 : 0.0)));
			}
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
				ImGui::SetTooltip("Subdivide the screen into 32x32 pixel tiles for optimal CPU cache locality and balanced thread loads.");
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.9f, 1.0f), "Render Distance & Level of Detail");

			float render_distance = static_cast<float>(orchestrator_.parameters().render_distance_scale);
			if (ImGui::SliderFloat("Render Distance (M units, 0 = unbounded)", &render_distance, 0.0f, 5000.0f, "%.0f")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::RenderDistanceScale, static_cast<double>(render_distance))));
			}
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
				ImGui::SetTooltip("Maximum radial distance from the origin, in units of central mass M, beyond which geodesics are treated as escaped to the sky. Set to 0 to disable the cutoff entirely and render arbitrarily distant structures.");
			}

			bool lod_enabled = orchestrator_.parameters().lod_enabled;
			if (ImGui::Checkbox("Enable Distance-Based Level of Detail", &lod_enabled)) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::LodEnabled, lod_enabled ? 1.0 : 0.0)));
			}
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
				ImGui::SetTooltip("Reduce geodesic integration step budget beyond the LOD threshold distance to save performance when observing very distant structures. Disabled by default.");
			}

			if (lod_enabled) {
				float lod_threshold = static_cast<float>(orchestrator_.parameters().lod_distance_scale);
				if (ImGui::SliderFloat("LOD Threshold Distance (M units)", &lod_threshold, 1.0f, 5000.0f, "%.0f")) {
					static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::LodDistanceThreshold, static_cast<double>(lod_threshold))));
				}

				int lod_steps = static_cast<int>(orchestrator_.parameters().lod_reduced_ray_steps);
				if (ImGui::SliderInt("LOD Reduced Step Budget", &lod_steps, 16, 4096)) {
					static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::LodReducedSteps, static_cast<double>(lod_steps))));
				}
			}

			bool force_tex_realloc = (orchestrator_.parameters().visual_overlays_flags & Render::RenderFlags::FORCE_TEXTURE_REALLOCATION) != 0U;
			if (ImGui::Checkbox("Force GPU Texture Storage Reallocation (glTexImage2D)", &force_tex_realloc)) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::ForceTextureReallocation, force_tex_realloc ? 1.0 : 0.0)));
			}
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
				ImGui::SetTooltip("Force full OpenGL texture memory reallocation each frame instead of in-place sub-image updates.");
			}

			int rolling_count = static_cast<int>(orchestrator_.parameters().rolling_average_frame_count);
			if (ImGui::SliderInt("HUD Rolling Frame Count (N)", &rolling_count, 2, 60)) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::RollingAverageFrameCount, static_cast<double>(rolling_count))));
			}
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
				ImGui::SetTooltip("Number of historical frame times used to calculate the smoothed rolling average FPS on the HUD overlay.");
			}

			if (ImGui::Checkbox("Enable Dynamic Resolution Throttling", &enable_dynamic_resolution_)) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_custom_param("dyn_res", enable_dynamic_resolution_ ? 1.0 : 0.0)));
			}
			if (enable_dynamic_resolution_) {
				if (ImGui::SliderFloat("Target Frame Rate", &target_framerate_, 30.0f, 144.0f, "%.0f FPS")) {
					static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_custom_param("target_fps", target_framerate_)));
				}
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::TextColored(ImVec4(0.3f, 0.9f, 1.0f, 1.0f), "GPU Compute Offload (Vulkan)");

			const bool gpu_platform_supported = Render::VulkanComputeExecutor::is_platform_supported();
			bool use_gpu = orchestrator_.parameters().use_gpu_compute;

			if (!gpu_platform_supported) {
				ImGui::BeginDisabled(true);
			}
			if (ImGui::Checkbox("Enable Native GPU Ray Tracing (Vulkan Compute Dispatch)", &use_gpu)) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::UseGpuCompute, use_gpu ? 1.0 : 0.0)));
			}
			if (!gpu_platform_supported) {
				ImGui::EndDisabled();
			}
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
				ImGui::SetTooltip("Dispatches the null-geodesic integration directly on a Vulkan compute-capable GPU instead of the CPU SIMD/scalar solver. Automatically falls back to the CPU path for wormhole, warp, and cosmological metrics, exact-Kerr high-spin geodesics, and double-single emulated precision.");
			}

			if (!gpu_platform_supported) {
				ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.4f, 1.0f), "No Vulkan compute-capable device with double-precision shader and scalar block layout support was detected on this system.");
			} else if (render_pipeline_ != nullptr) {
				const bool gpu_ready = render_pipeline_->gpu_compute_available();
				ImGui::TextColored(gpu_ready ? ImVec4(0.3f, 1.0f, 0.4f, 1.0f) : ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "Compute Device Status: %s", gpu_ready ? "Ready" : "Unavailable");
				ImGui::TextColored(ImVec4(0.8f, 0.85f, 0.9f, 0.9f), "Active Render Path (Last Frame): %s", render_pipeline_->telemetry().used_gpu_path ? "GPU Vulkan Compute" : "CPU SIMD / Scalar");
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.0f), "Memory & Task Scheduler Telemetry");

			ImGui::Text("Master Task Queue Capacity: 1024 commands");
			ImGui::Text("Commands Processed:         %llu", static_cast<unsigned long long>(orchestrator_.total_commands_processed()));
			ImGui::Text("Cache Line Alignment:       64 / 128 Bytes");
			ImGui::Text("Deterministic Seed Source:  PCG64 Explicit Engine");
		}
		ImGui::End();
	}
};

}
