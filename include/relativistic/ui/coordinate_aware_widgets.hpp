#pragma once

#include <imgui.h>
#include "relativistic/observer/coordinate_systems.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/ui/coordinate_display.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include "relativistic/units/unit_aware_widgets.hpp"
#include <algorithm>
#include <array>
#include <functional>
#include <string>
#include <string_view>

namespace Relativistic::UI {

[[nodiscard]] inline bool coordinate_system_combo(const char* label, Observer::CoordinateSystem& system) {
	int index = static_cast<int>(system);
	if (ImGui::Combo(label, &index, Observer::kCoordinateSystemNames.data(), static_cast<int>(Observer::kCoordinateSystemCount))) {
		system = Observer::coordinate_system_from_index(static_cast<uint32_t>(std::max(index, 0)));
		return true;
	}
	return false;
}

namespace CoordinateWidgetDetail {

[[nodiscard]] inline ImGuiID widget_key(const char* label) noexcept {
	return static_cast<ImGuiID>(std::hash<std::string_view>{}(std::string_view(label)));
}

[[nodiscard]] inline Observer::CoordinateSystem resolve_system(ImGuiStorage* storage, ImGuiID key, const Observer::CoordinatePreferences& preferences) noexcept {
	const int stored = storage->GetInt(key, -1);
	return (stored >= 0) ? Observer::coordinate_system_from_index(static_cast<uint32_t>(stored)) : preferences.widget_default;
}

inline void render_selector(ImGuiStorage* storage, ImGuiID key, Orchestrator::SimulationOrchestrator<1024>& orchestrator) {
	auto& preferences = orchestrator.coordinate_preferences();
	const int stored = storage->GetInt(key, -1);
	const Observer::CoordinateSystem active = resolve_system(storage, key, preferences);
	ImGui::PushID(static_cast<int>(key));
	if (ImGui::SmallButton(Observer::kCoordinateSystemShortNames[Observer::coordinate_system_index(active)])) {
		ImGui::OpenPopup("CoordinateSystemMenu");
	}
	render_setting_tooltip("Coordinate system of this field. Click to switch between Cartesian, spherical, cylindrical and metric-adapted coordinates, either for this field only or as the default for every field.");
	if (ImGui::BeginPopup("CoordinateSystemMenu")) {
		ImGui::TextDisabled("Coordinate System");
		ImGui::Separator();
		const std::string follow_label = std::string("Follow Global Default (") + Observer::kCoordinateSystemShortNames[Observer::coordinate_system_index(preferences.widget_default)] + ")";
		if (ImGui::Selectable(follow_label.c_str(), stored < 0)) {
			storage->SetInt(key, -1);
		}
		for (size_t index = 0; index < Observer::kCoordinateSystemCount; ++index) {
			if (ImGui::Selectable(Observer::kCoordinateSystemNames[index], stored == static_cast<int>(index))) {
				storage->SetInt(key, static_cast<int>(index));
			}
		}
		ImGui::Separator();
		if (ImGui::Selectable("Use Current As Global Default")) {
			preferences.widget_default = active;
			storage->SetInt(key, -1);
		}
		ImGui::EndPopup();
	}
	ImGui::PopID();
}

}

[[nodiscard]] inline bool coordinate_aware_input_position(const char* label, std::array<double, 3>& position, Orchestrator::SimulationOrchestrator<1024>& orchestrator) {
	ImGuiStorage* storage = ImGui::GetStateStorage();
	const ImGuiID key = CoordinateWidgetDetail::widget_key(label);
	const Observer::CoordinateSystem system = CoordinateWidgetDetail::resolve_system(storage, key, orchestrator.coordinate_preferences());
	const auto& unit_preferences = orchestrator.unit_preferences();
	const double length_scale = std::max(orchestrator.constants_engine().length_scale(), 1e-300);
	bool changed = false;

	if (system == Observer::CoordinateSystem::Cartesian) {
		double meters[3] = {position[0] * length_scale, position[1] * length_scale, position[2] * length_scale};
		if (unit_aware_input_double3(label, meters, UnitCategory::Distance, unit_preferences)) {
			position = {meters[0] / length_scale, meters[1] / length_scale, meters[2] / length_scale};
			changed = true;
		}
	} else {
		const Observer::CoordinateFrame frame = make_coordinate_frame(orchestrator);
		Observer::CoordinateVector values = Observer::CoordinateConverter::to_system(system, position, frame);
		double display[3];
		for (size_t axis = 0; axis < 3; ++axis) {
			display[axis] = coordinate_axis_to_display(system, axis, values[axis], length_scale, unit_preferences);
		}
		const float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
		const float width = std::max((ImGui::CalcItemWidth() - 2.0f * spacing) / 3.0f, 48.0f);
		bool edited = false;
		ImGui::PushID(label);
		for (size_t axis = 0; axis < 3; ++axis) {
			if (axis > 0) {
				ImGui::SameLine(0.0f, spacing);
			}
			const std::string format = std::string(Observer::coordinate_axis_label(system, axis)) + " %.6g " + coordinate_axis_suffix(system, axis, unit_preferences);
			ImGui::PushID(static_cast<int>(axis));
			ImGui::SetNextItemWidth(width);
			if (ImGui::InputDouble("##component", &display[axis], 0.0, 0.0, format.c_str())) {
				edited = true;
			}
			ImGui::PopID();
		}
		ImGui::PopID();
		ImGui::SameLine();
		ImGui::TextUnformatted(label, ImGui::FindRenderedTextEnd(label));
		if (edited) {
			for (size_t axis = 0; axis < 3; ++axis) {
				values[axis] = coordinate_axis_from_display(system, axis, display[axis], length_scale, unit_preferences);
			}
			position = Observer::CoordinateConverter::from_system(system, values, frame);
			changed = true;
		}
	}

	ImGui::SameLine();
	CoordinateWidgetDetail::render_selector(storage, key, orchestrator);
	return changed;
}

[[nodiscard]] inline bool coordinate_aware_input_position(const char* label, float (&position)[3], Orchestrator::SimulationOrchestrator<1024>& orchestrator) {
	std::array<double, 3> converted{static_cast<double>(position[0]), static_cast<double>(position[1]), static_cast<double>(position[2])};
	if (!coordinate_aware_input_position(label, converted, orchestrator)) {
		return false;
	}
	position[0] = static_cast<float>(converted[0]);
	position[1] = static_cast<float>(converted[1]);
	position[2] = static_cast<float>(converted[2]);
	return true;
}

}
