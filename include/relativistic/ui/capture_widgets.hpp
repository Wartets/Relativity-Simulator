#pragma once

#include "relativistic/capture/easing.hpp"
#include "relativistic/capture/expression.hpp"
#include <imgui.h>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace Relativistic::UI::CaptureWidgets {

inline const ImVec4 kHeaderColor(0.45f, 0.85f, 1.0f, 1.0f);
inline const ImVec4 kWarningColor(1.0f, 0.55f, 0.35f, 1.0f);
inline const ImVec4 kAccentColor(0.55f, 1.0f, 0.65f, 1.0f);

template <size_t N>
inline bool index_combo(const char* label, uint32_t& value, const std::array<const char*, N>& names) {
	int index = std::min(static_cast<int>(value), static_cast<int>(N) - 1);
	if (ImGui::Combo(label, &index, names.data(), static_cast<int>(N))) {
		value = static_cast<uint32_t>(index);
		return true;
	}
	return false;
}

template <typename EnumType, size_t N>
inline bool enum_combo(const char* label, EnumType& value, const std::array<const char*, N>& names) {
	uint32_t raw = static_cast<uint32_t>(value);
	if (index_combo(label, raw, names)) {
		value = static_cast<EnumType>(raw);
		return true;
	}
	return false;
}

inline bool drag_double_clamped(const char* label, double* value, float speed, double min_val, double max_val, const char* format = "%.3f") {
	float temp = static_cast<float>(*value);
	if (ImGui::DragFloat(label, &temp, speed, static_cast<float>(min_val), static_cast<float>(max_val), format)) {
		*value = std::clamp(static_cast<double>(temp), min_val, max_val);
		return true;
	}
	return false;
}

inline bool drag_vec3(const char* label, std::array<double, 3>& values, float speed = 0.1f, const char* format = "%.3f") {
	float temp[3] = {static_cast<float>(values[0]), static_cast<float>(values[1]), static_cast<float>(values[2])};
	if (ImGui::DragFloat3(label, temp, speed, 0.0f, 0.0f, format)) {
		values[0] = static_cast<double>(temp[0]);
		values[1] = static_cast<double>(temp[1]);
		values[2] = static_cast<double>(temp[2]);
		return true;
	}
	return false;
}

inline bool input_expression(const char* label, std::string& expression, char* buffer, size_t buffer_size, std::string& error_out) {
	if (std::strcmp(buffer, expression.c_str()) != 0) {
		std::snprintf(buffer, buffer_size, "%s", expression.c_str());
	}

	bool changed = false;
	if (ImGui::InputText(label, buffer, buffer_size)) {
		expression = buffer;
		changed = true;
	}

	Capture::ExpressionParser parser;
	auto parsed = parser.parse(expression);
	if (!parsed.has_value()) {
		error_out = parser.error();
		ImGui::SameLine();
		ImGui::TextColored(kWarningColor, "(!)");
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("%s", error_out.c_str());
		}
	} else {
		error_out.clear();
	}

	return changed;
}

inline bool edit_easing(const char* label, Capture::EasingSpec& spec) {
	bool modified = false;
	ImGui::PushID(label);

	static constexpr std::array<const char*, 34> kEasingNames{{
		"Linear", "QuadIn", "QuadOut", "QuadInOut", "CubicIn", "CubicOut", "CubicInOut",
		"QuartIn", "QuartOut", "QuartInOut", "QuintIn", "QuintOut", "QuintInOut",
		"SineIn", "SineOut", "SineInOut", "ExpIn", "ExpOut", "ExpInOut",
		"CircIn", "CircOut", "CircInOut", "Smoothstep", "Smootherstep",
		"BackIn", "BackOut", "BackInOut", "BounceIn", "BounceOut", "BounceInOut",
		"ElasticIn", "ElasticOut", "ElasticInOut", "CustomCurve"
	}};

	uint32_t kind_idx = static_cast<uint32_t>(spec.kind);
	if (index_combo("Kind", kind_idx, kEasingNames)) {
		spec.kind = static_cast<Capture::EasingKind>(kind_idx);
		modified = true;
	}

	std::array<float, 64> plot_values{};
	for (size_t i = 0; i < plot_values.size(); ++i) {
		const double t = static_cast<double>(i) / static_cast<double>(plot_values.size() - 1);
		plot_values[i] = static_cast<float>(spec.evaluate(t));
	}
	ImGui::PlotLines("##Curve", plot_values.data(), static_cast<int>(plot_values.size()), 0, nullptr, 0.0f, 1.0f, ImVec2(ImGui::GetContentRegionAvail().x, 50.0f));

	if (spec.kind == Capture::EasingKind::CustomCurve) {
		ImGui::TextColored(kHeaderColor, "Custom Points");
		for (size_t i = 0; i < spec.curve_points.size(); ++i) {
			ImGui::PushID(static_cast<int>(i));
			float p[2] = {static_cast<float>(spec.curve_points[i][0]), static_cast<float>(spec.curve_points[i][1])};
			if (ImGui::DragFloat2("##Point", p, 0.01f, 0.0f, 1.0f, "%.3f")) {
				spec.curve_points[i][0] = std::clamp(static_cast<double>(p[0]), 0.0, 1.0);
				spec.curve_points[i][1] = static_cast<double>(p[1]);
				modified = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("X") && spec.curve_points.size() > 2) {
				spec.curve_points.erase(spec.curve_points.begin() + static_cast<ptrdiff_t>(i));
				modified = true;
				ImGui::PopID();
				break;
			}
			ImGui::PopID();
		}

		if (ImGui::Button("+ Add Point")) {
			spec.curve_points.push_back({0.5, 0.5});
			std::stable_sort(spec.curve_points.begin(), spec.curve_points.end(), [](const auto& a, const auto& b) noexcept {
				return a[0] < b[0];
			});
			modified = true;
		}
	}

	ImGui::PopID();
	return modified;
}

}
