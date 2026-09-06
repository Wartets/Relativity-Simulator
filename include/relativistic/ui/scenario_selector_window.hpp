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
#include <cfloat>

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
	char search_filter_[64]{};
	int sort_mode_{0};
	float left_pane_width_{300.0f};
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
		ImGui::SetNextWindowSize(ImVec2(360.0f, 660.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSizeConstraints(ImVec2(320.0f, 260.0f), ImVec2(FLT_MAX, FLT_MAX));

		if (ImGui::Begin("Scenario Manager & Presets", &is_open_)) {
			ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Scientific Scenario Catalog");
			ImGui::Separator();

			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.55f);
			ImGui::InputTextWithHint("##ScenarioSearch", "Filter scenarios by keyword...", search_filter_, sizeof(search_filter_));
			ImGui::SameLine();
			const char* sort_options[] = {"Sort: Compatible First", "Sort: Alphabetical", "Sort: Metric Type", "Sort: Recently Scanned"};
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			ImGui::Combo("##ScenarioSortMode", &sort_mode_, sort_options, IM_ARRAYSIZE(sort_options));

			std::vector<size_t> visible_indices;
			visible_indices.reserve(presets_.size());
			for (size_t i = 0; i < presets_.size(); ++i) {
				const auto& item = presets_[i];
				if (search_filter_[0] != '\0') {
					if (item.definition.scenario_name.find(search_filter_) == std::string::npos &&
					    item.filename.find(search_filter_) == std::string::npos &&
					    item.definition.metric_type.find(search_filter_) == std::string::npos) {
						continue;
					}
				}
				visible_indices.push_back(i);
			}

			if (sort_mode_ == 1) {
				std::sort(visible_indices.begin(), visible_indices.end(), [&](size_t a, size_t b) {
					const auto& na = presets_[a].definition.scenario_name.empty() ? presets_[a].filename : presets_[a].definition.scenario_name;
					const auto& nb = presets_[b].definition.scenario_name.empty() ? presets_[b].filename : presets_[b].definition.scenario_name;
					return na < nb;
				});
			} else if (sort_mode_ == 2) {
				std::sort(visible_indices.begin(), visible_indices.end(), [&](size_t a, size_t b) {
					if (presets_[a].definition.metric_type != presets_[b].definition.metric_type) {
						return presets_[a].definition.metric_type < presets_[b].definition.metric_type;
					}
					return presets_[a].filename < presets_[b].filename;
				});
			} else if (sort_mode_ == 3) {
				std::sort(visible_indices.begin(), visible_indices.end(), [](size_t a, size_t b) { return a < b; });
			} else {
				std::sort(visible_indices.begin(), visible_indices.end(), [&](size_t a, size_t b) {
					if (presets_[a].is_compatible != presets_[b].is_compatible) return presets_[a].is_compatible && !presets_[b].is_compatible;
					return presets_[a].filename < presets_[b].filename;
				});
			}

			const float total_width = ImGui::GetContentRegionAvail().x;
			left_pane_width_ = std::clamp(left_pane_width_, 160.0f, std::max(180.0f, total_width - 160.0f));

			ImGui::BeginChild("ScenarioListPane", ImVec2(left_pane_width_, ImGui::GetContentRegionAvail().y - 40.0f), true);
			for (const size_t idx : visible_indices) {
				const auto& item = presets_[idx];
				const bool is_selected = (selected_index_ == static_cast<int>(idx));
				const std::string label = item.is_compatible
					? (item.definition.scenario_name.empty() ? item.filename : item.definition.scenario_name)
					: ("[Incompatible] " + item.filename);

				ImGui::PushStyleColor(ImGuiCol_Text, item.is_compatible ? ImVec4(0.35f, 1.0f, 0.5f, 1.0f) : ImVec4(1.0f, 0.35f, 0.35f, 1.0f));
				if (ImGui::Selectable(label.c_str(), is_selected)) {
					selected_index_ = static_cast<int>(idx);
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
			if (visible_indices.empty()) {
				ImGui::TextDisabled("No scenarios match the current filter.");
			}
			ImGui::EndChild();

			ImGui::SameLine();
			ImGui::InvisibleButton("ScenarioSplitter", ImVec2(6.0f, ImGui::GetContentRegionAvail().y - 40.0f));
			if (ImGui::IsItemActive()) {
				left_pane_width_ += ImGui::GetIO().MouseDelta.x;
			}
			if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
				ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
			}
			ImGui::SameLine();

			ImGui::BeginChild("ScenarioDetailPane", ImVec2(0.0f, ImGui::GetContentRegionAvail().y - 40.0f), true);
			if (selected_index_ >= 0 && selected_index_ < static_cast<int>(presets_.size())) {
				const auto& item = presets_[static_cast<size_t>(selected_index_)];
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
			} else {
				ImGui::TextDisabled("Select a scenario from the list to inspect its details.");
			}
			ImGui::EndChild();

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
