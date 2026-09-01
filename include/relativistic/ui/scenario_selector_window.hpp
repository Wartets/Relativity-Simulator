#pragma once

#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/orchestrator/command.hpp"
#include "relativistic/io/scenario_serializer.hpp"
#include <imgui.h>
#include <vector>
#include <string>
#include <string_view>
#include <array>

namespace Relativistic::UI {

struct ScenarioPresetItem {
	std::string name;
	std::string category;
	std::string description;
	std::string metric_type;
	double mass{1.0};
	double spin{0.0};
	double charge{0.0};
	double lambda{0.0};
	double throat{1.0};
	double warp_vel{1.0};
	double cam_r{50.0};
	double cam_fov{60.0};
	std::string integrator{"RK45"};
};

class ScenarioSelectorWindow {
private:
	bool is_open_{true};
	Orchestrator::SimulationOrchestrator<1024>& orchestrator_;
	InteractiveCameraController* camera_controller_{nullptr};
	std::vector<ScenarioPresetItem> presets_;
	int selected_index_{0};
	char custom_path_buffer_[256]{"scenarios/custom_scenario.yaml"};

	void initialize_presets() {
		presets_ = {
			{
				"Schwarzschild Black Hole & Accretion Disk",
				"Accretion Physics & Extreme Raytracing",
				"Static Schwarzschild black hole with an equatorial Novikov-Thorne accretion disk and gravitational lensing.",
				"Schwarzschild",
				1.0, 0.0, 0.0, 0.0, 1.0, 0.0,
				33.0, 60.0, "RK45"
			},
			{
				"Kerr Rotating Black Hole (a=0.94)",
				"Accretion Physics & Extreme Raytracing",
				"Rapidly spinning Kerr black hole (a=0.94) surrounded by an asymmetric beaming accretion disk.",
				"Kerr",
				1.0, 0.94, 0.0, 0.0, 1.0, 0.0,
				33.0, 60.0, "RK45"
			},
			{
				"Hulse-Taylor Pulsar PSR B1913+16",
				"Astrophysical Verification",
				"High-precision binary neutron star system reproducing Peters-Mathews gravitational radiation orbital decay.",
				"Schwarzschild",
				2.828, 0.0, 0.0, 0.0, 1.0, 0.0,
				120.0, 40.0, "Hermite4"
			},
			{
				"Morris-Thorne Traversable Wormhole",
				"Exotic Spacetimes",
				"Static spherically symmetric traversable wormhole connecting two asymptotically flat spacetime sheets.",
				"MorrisThorne",
				0.0, 0.0, 0.0, 0.0, 5.0, 0.0,
				20.0, 75.0, "GaussLegendre6"
			},
			{
				"Alcubierre Superluminal Warp Bubble",
				"Exotic Spacetimes",
				"Dynamic spacetime curvature bubble contracting space in front and expanding behind at apparent speed vs=2.0c.",
				"Alcubierre",
				0.0, 0.0, 0.0, 0.0, 1.0, 2.0,
				40.0, 70.0, "RK45"
			}
		};
	}

public:
	explicit ScenarioSelectorWindow(Orchestrator::SimulationOrchestrator<1024>& orchestrator, InteractiveCameraController* cam_ctrl = nullptr)
		: orchestrator_(orchestrator), camera_controller_(cam_ctrl) {
		initialize_presets();
	}

	void render() {
		if (!is_open_) return;

		ImGui::SetNextWindowPos(ImVec2(15.0f, 35.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(315.0f, 660.0f), ImGuiCond_FirstUseEver);

		if (ImGui::Begin("Scenario Manager & Presets", &is_open_)) {
			ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Scientific Scenario Catalog");
			ImGui::Separator();

			static char search_filter[64] = "";
			ImGui::InputTextWithHint("Search", "Filter scenarios by keyword...", search_filter, sizeof(search_filter));

			ImGui::Columns(2, "ScenarioColumns", true);
			ImGui::SetColumnWidth(0, 300.0f);

			for (int i = 0; i < static_cast<int>(presets_.size()); ++i) {
				if (search_filter[0] != '\0') {
					if (presets_[i].name.find(search_filter) == std::string::npos &&
					    presets_[i].category.find(search_filter) == std::string::npos) {
						continue;
					}
				}

				const bool is_selected = (selected_index_ == i);
				if (ImGui::Selectable(presets_[i].name.c_str(), is_selected)) {
					selected_index_ = i;
				}
			}

			ImGui::NextColumn();

			if (selected_index_ >= 0 && selected_index_ < static_cast<int>(presets_.size())) {
				const auto& p = presets_[selected_index_];
				ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", p.name.c_str());
				ImGui::TextDisabled("Category: %s", p.category.c_str());
				ImGui::Spacing();
				ImGui::TextWrapped("%s", p.description.c_str());
				ImGui::Spacing();
				ImGui::Separator();

				ImGui::Text("Spacetime Metric: %s", p.metric_type.c_str());
				ImGui::Text("Central Mass:     %.2f M", p.mass);
				ImGui::Text("Central Spin:     %.2f a", p.spin);
				ImGui::Text("Electric Charge:  %.2f Q", p.charge);
				ImGui::Text("Integrator:       %s", p.integrator.c_str());
				ImGui::Text("Default FOV:      %.1f deg", p.cam_fov);
				ImGui::Text("Initial Distance: %.1f M", p.cam_r);

				ImGui::Spacing();
				if (ImGui::Button("Activate Scenario Preset", ImVec2(240.0f, 32.0f))) {
					apply_preset(p);
				}
			}

			ImGui::Columns(1);
			ImGui::Spacing();
			ImGui::Separator();

			ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.5f, 1.0f), "Custom Scenario File I/O (YAML)");
			ImGui::InputText("Scenario Path", custom_path_buffer_, sizeof(custom_path_buffer_));

			if (ImGui::Button("Load Scenario File")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_load_scenario(custom_path_buffer_)));
			}
			ImGui::SameLine();
			if (ImGui::Button("Save Current to File")) {
				static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_save_scenario(custom_path_buffer_)));
			}
		}
		ImGui::End();
	}

private:
	void apply_preset(const ScenarioPresetItem& p) noexcept {
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_reset()));
		
		orchestrator_.set_active_scenario_name(p.name);
		orchestrator_.set_active_metric_name(p.metric_type);
		orchestrator_.set_active_integrator_name(p.integrator);

		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_metric(p.metric_type)));
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::Mass, p.mass)));
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::Spin, p.spin)));
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::Charge, p.charge)));
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::CosmologicalLambda, p.lambda)));
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::WormholeThroat, p.throat)));
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::WarpVelocity, p.warp_vel)));
		static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_camera_set_fov(p.cam_fov)));

		if (camera_controller_) {
			camera_controller_->snap_to_equatorial_front(p.cam_r);
		} else {
			auto& cam = orchestrator_.camera();
			cam.position = {0.0, p.cam_r, 0.0};
			cam.pitch = 0.0;
			cam.yaw = 180.0;
			cam.roll = 0.0;
		}
		auto& cam = orchestrator_.camera();
		cam.fov_deg = p.cam_fov;
	}
};

}
