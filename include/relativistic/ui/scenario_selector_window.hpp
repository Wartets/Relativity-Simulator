#pragma once

#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/orchestrator/command.hpp"
#include "relativistic/io/scenario_serializer.hpp"
#include <imgui.h>
#include <vector>
#include <string>
#include <string_view>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace Relativistic::UI {

struct ScenarioFileItem {
	std::string filename;
	std::string filepath;
	bool is_compatible{false};
	std::string error_message{};
	IO::ScenarioDefinition definition{};
};

class ScenarioSelectorWindow {
private:
	bool is_open_{true};
	Orchestrator::SimulationOrchestrator<1024>& orchestrator_;
	InteractiveCameraController* camera_controller_{nullptr};
	std::vector<ScenarioFileItem> presets_{};
	int selected_index_{0};
	char custom_path_buffer_[256]{"scenarios/custom_scenario.yaml"};

public:
	void scan_scenario_directory() {
		presets_.clear();
		std::vector<std::string> search_paths = {"scenarios", "../scenarios", "./scenarios"};
		std::string target_dir;
		for (const auto& path_str : search_paths) {
			if (std::filesystem::exists(path_str) && std::filesystem::is_directory(path_str)) {
				target_dir = path_str;
				break;
			}
		}
		if (target_dir.empty()) {
			return;
		}

		for (const auto& entry : std::filesystem::directory_iterator(target_dir)) {
			if (!entry.is_regular_file()) continue;
			const auto ext = entry.path().extension().string();
			if (ext != ".yaml" && ext != ".yml") continue;

			ScenarioFileItem item;
			item.filename = entry.path().filename().string();
			item.filepath = entry.path().string();

			std::ifstream file(item.filepath);
			if (!file.is_open()) {
				item.is_compatible = false;
				item.error_message = "Cannot open scenario file for reading.";
				presets_.push_back(std::move(item));
				continue;
			}

			std::stringstream buffer;
			buffer << file.rdbuf();
			const std::string yaml_content = buffer.str();

			const auto parsed_opt = IO::ScenarioSerializer::from_yaml(yaml_content);
			if (!parsed_opt.has_value()) {
				item.is_compatible = false;
				item.error_message = "Failed to parse YAML format.";
				presets_.push_back(std::move(item));
				continue;
			}

			item.definition = *parsed_opt;
			const auto val_res = IO::ScenarioSerializer::validate(item.definition);
			item.is_compatible = val_res.is_valid;
			item.error_message = val_res.error_message;
			presets_.push_back(std::move(item));
		}

		std::sort(presets_.begin(), presets_.end(), [](const ScenarioFileItem& a, const ScenarioFileItem& b) {
			if (a.is_compatible != b.is_compatible) return a.is_compatible > b.is_compatible;
			return a.filename < b.filename;
		});

		if (selected_index_ >= static_cast<int>(presets_.size())) {
			selected_index_ = 0;
		}
	}

	explicit ScenarioSelectorWindow(Orchestrator::SimulationOrchestrator<1024>& orchestrator, InteractiveCameraController* cam_ctrl = nullptr)
		: orchestrator_(orchestrator), camera_controller_(cam_ctrl) {
		scan_scenario_directory();
	}

	[[nodiscard]] bool& open_state() noexcept {
		return is_open_;
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
				const auto& item = presets_[i];
				if (search_filter[0] != '\0') {
					if (item.definition.scenario_name.find(search_filter) == std::string::npos &&
					    item.filename.find(search_filter) == std::string::npos &&
					    item.definition.metric_type.find(search_filter) == std::string::npos) {
						continue;
					}
				}

				const bool is_selected = (selected_index_ == i);
				const std::string label = item.is_compatible
					? (item.definition.scenario_name.empty() ? item.filename : item.definition.scenario_name)
					: ("[Incompatible] " + item.filename);

				if (!item.is_compatible) {
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.35f, 0.35f, 1.0f));
				} else {
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 1.0f, 0.5f, 1.0f));
				}

				if (ImGui::Selectable(label.c_str(), is_selected)) {
					selected_index_ = i;
				}
				ImGui::PopStyleColor();

				if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
					ImGui::BeginTooltip();
					ImGui::Text("File: %s", item.filename.c_str());
					if (!item.is_compatible) {
						ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Validation Error: %s", item.error_message.c_str());
					} else {
						ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "Compatible (%s)", item.definition.metric_type.c_str());
					}
					ImGui::EndTooltip();
				}
			}

			ImGui::NextColumn();

			if (selected_index_ >= 0 && selected_index_ < static_cast<int>(presets_.size())) {
				const auto& item = presets_[selected_index_];
				const auto& def = item.definition;

				if (item.is_compatible) {
					ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.5f, 1.0f), "%s", def.scenario_name.c_str());
					ImGui::TextDisabled("File: %s", item.filename.c_str());
					ImGui::Spacing();
					ImGui::TextWrapped("%s", def.description.c_str());
					ImGui::Spacing();
					ImGui::Separator();

					ImGui::Text("Spacetime Metric: %s", def.metric_type.c_str());
					ImGui::Text("Central Mass:     %.2f M", def.central_mass);
					ImGui::Text("Central Spin:     %.2f a", def.central_spin);
					ImGui::Text("Electric Charge:  %.2f Q", def.central_charge);
					ImGui::Text("Integrator:       %s", def.integrator.scheme.c_str());
					ImGui::Text("Bodies:           %zu", def.bodies.size());
					ImGui::Text("Observers:        %zu", def.observers.size());

					ImGui::Spacing();
					if (ImGui::Button("Load Scenario", ImVec2(240.0f, 32.0f))) {
						static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_load_scenario(item.filepath)));
					}
					if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
						ImGui::SetTooltip("Load and activate this validated scenario from disk into the simulation core.");
					}
				} else {
					ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "Incompatible: %s", item.filename.c_str());
					ImGui::Separator();
					ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Error: %s", item.error_message.c_str());
					ImGui::Spacing();
					ImGui::TextWrapped("This scenario file violates physical invariants or specifies unsupported configuration parameters.");
					ImGui::Spacing();
					ImGui::BeginDisabled(true);
					ImGui::Button("Cannot Load (Incompatible)", ImVec2(240.0f, 32.0f));
					ImGui::EndDisabled();
				}
			}

			ImGui::Columns(1);
			ImGui::Spacing();
			ImGui::Separator();

			if (ImGui::Button("Rescan Scenarios Folder", ImVec2(200.0f, 26.0f))) {
				scan_scenario_directory();
			}
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
				ImGui::SetTooltip("Dynamically scan the scenarios directory for newly added or modified YAML scenario files.");
			}

			ImGui::Spacing();
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
};

}
