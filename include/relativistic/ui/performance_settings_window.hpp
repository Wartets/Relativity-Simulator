#pragma once

#include <imgui.h>
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/orchestrator/command.hpp"
#include "relativistic/render/gpu_types.hpp"
#include <algorithm>
#include <array>

namespace Relativistic::UI {

class PerformanceSettingsWindow {
private:
	bool is_open_{true};
	Orchestrator::SimulationOrchestrator<1024>& orchestrator_;

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
