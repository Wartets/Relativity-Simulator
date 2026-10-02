#pragma once

#include "relativistic/capture/easing.hpp"
#include "relativistic/capture/expression.hpp"
#include <imgui.h>
#include <algorithm>
#include <array>
#include <cfloat>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

namespace Relativistic::UI::CaptureWidgets {

inline const ImVec4 kHeaderColor(0.45f, 0.85f, 1.0f, 1.0f);
inline const ImVec4 kWarningColor(1.0f, 0.55f, 0.35f, 1.0f);
inline const ImVec4 kAccentColor(0.55f, 1.0f, 0.65f, 1.0f);
inline const ImVec4 kMutedColor(0.65f, 0.70f, 0.78f, 1.0f);

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

inline bool slider_u32(const char* label, uint32_t& value, uint32_t min_value, uint32_t max_value, ImGuiSliderFlags flags = 0) {
	int scratch = static_cast<int>(value);
	if (ImGui::SliderInt(label, &scratch, static_cast<int>(min_value), static_cast<int>(max_value), "%d", flags)) {
		value = static_cast<uint32_t>(scratch);
		return true;
	}
	return false;
}

inline bool drag_double(const char* label, double& value, double speed, double min_value, double max_value, const char* format = "%.3f") {
	return ImGui::DragScalar(label, ImGuiDataType_Double, &value, static_cast<float>(speed), &min_value, &max_value, format);
}

inline bool drag_double_free(const char* label, double& value, double speed, const char* format = "%.3f") {
	return ImGui::DragScalar(label, ImGuiDataType_Double, &value, static_cast<float>(speed), nullptr, nullptr, format);
}

inline bool drag_vec3(const char* label, std::array<double, 3>& value, double speed = 0.1, const char* format = "%.3f") {
	return ImGui::DragScalarN(label, ImGuiDataType_Double, value.data(), 3, static_cast<float>(speed), nullptr, nullptr, format);
}

inline bool input_text(const char* label, std::string& value) {
	char buffer[512];
	std::snprintf(buffer, sizeof(buffer), "%s", value.c_str());
	if (ImGui::InputText(label, buffer, sizeof(buffer))) {
		value = buffer;
		return true;
	}
	return false;
}

inline bool input_expression(const char* label, Capture::Expression& expression) {
	std::string source = expression.source();
	const bool changed = input_text(label, source);
	if (changed) {
		expression.assign(std::move(source));
	}
	if (!expression.valid()) {
		ImGui::TextColored(kWarningColor, "%s", expression.error().c_str());
	}
	return changed;
}

inline void wrapped_text(const ImVec4& color, const std::string& text) {
	ImGui::PushStyleColor(ImGuiCol_Text, color);
	ImGui::TextWrapped("%s", text.c_str());
	ImGui::PopStyleColor();
}

[[nodiscard]] inline std::string format_bytes(uint64_t bytes) {
	static constexpr std::array<const char*, 5> units{"B", "KiB", "MiB", "GiB", "TiB"};
	double value = static_cast<double>(bytes);
	size_t unit = 0;
	while (value >= 1024.0 && unit + 1 < units.size()) {
		value /= 1024.0;
		++unit;
	}
	char buffer[48];
	std::snprintf(buffer, sizeof(buffer), "%.2f %s", value, units[unit]);
	return buffer;
}

[[nodiscard]] inline std::string format_duration(double seconds) {
	if (!(seconds >= 0.0) || seconds > 1.0e9) {
		return "--";
	}
	const uint64_t total = static_cast<uint64_t>(seconds + 0.5);
	char buffer[48];
	std::snprintf(buffer, sizeof(buffer), "%lluh %02llum %02llus", static_cast<unsigned long long>(total / 3600ULL), static_cast<unsigned long long>((total / 60ULL) % 60ULL), static_cast<unsigned long long>(total % 60ULL));
	return buffer;
}

inline bool edit_easing(const char* label, Capture::EasingSpec& spec) {
	bool modified = false;
	ImGui::PushID(label);

	const auto& names = Capture::easing_kind_names();
	int kind_index = static_cast<int>(spec.kind);
	if (ImGui::Combo("Curve", &kind_index, names.data(), static_cast<int>(names.size()))) {
		spec.kind = static_cast<Capture::EasingKind>(kind_index);
		spec.reset_parameters();
		modified = true;
	}

	const auto labels = Capture::easing_parameter_labels(spec.kind);
	for (size_t i = 0; i < labels.size(); ++i) {
		if (labels[i] == nullptr) {
			continue;
		}
		ImGui::PushID(static_cast<int>(i));
		modified |= drag_double_free(labels[i], spec.parameters[i], 0.01, "%.4f");
		ImGui::PopID();
	}

	if (spec.kind == Capture::EasingKind::CustomExpression) {
		modified |= input_expression("Expression f(u)", spec.expression);
		ImGui::TextDisabled("Variables: u progress, a b c k parameters above.");
	}

	if (spec.kind == Capture::EasingKind::CustomCurve) {
		modified |= ImGui::Checkbox("Smooth Interpolation", &spec.smooth_curve);
		int remove_index = -1;
		bool needs_sort = false;
		for (size_t i = 0; i < spec.points.size(); ++i) {
			ImGui::PushID(static_cast<int>(i));
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.34f);
			modified |= drag_double("##position", spec.points[i].position, 0.005, 0.0, 1.0, "%.3f");
			needs_sort |= ImGui::IsItemDeactivatedAfterEdit();
			ImGui::SameLine();
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.55f);
			modified |= drag_double("##value", spec.points[i].value, 0.005, -4.0, 5.0, "%.3f");
			ImGui::SameLine();
			if (ImGui::SmallButton("X") && spec.points.size() > 2) {
				remove_index = static_cast<int>(i);
			}
			ImGui::PopID();
		}
		if (remove_index >= 0) {
			spec.points.erase(spec.points.begin() + remove_index);
			modified = true;
		}
		if (ImGui::SmallButton("Add Point")) {
			spec.points.push_back(Capture::EasingPoint{0.5, spec.evaluate(0.5)});
			needs_sort = true;
			modified = true;
		}
		if (needs_sort) {
			spec.sort_points();
		}
	}

	modified |= drag_double("Blend With Linear", spec.blend, 0.01, 0.0, 2.0, "%.3f");
	modified |= drag_double("Repeat Cycles", spec.repeat, 0.05, 1.0, 64.0, "%.2f");
	if (spec.repeat > 1.0) {
		modified |= ImGui::Checkbox("Ping-Pong Cycles", &spec.ping_pong);
	}
	modified |= ImGui::Checkbox("Reverse", &spec.reverse);
	ImGui::SameLine();
	if (ImGui::SmallButton("Reset Parameters")) {
		spec.reset_parameters();
		modified = true;
	}

	std::array<float, 96> samples{};
	for (size_t i = 0; i < samples.size(); ++i) {
		samples[i] = static_cast<float>(spec.evaluate(static_cast<double>(i) / static_cast<double>(samples.size() - 1)));
	}
	ImGui::PlotLines("##EasingPlot", samples.data(), static_cast<int>(samples.size()), 0, nullptr, FLT_MAX, FLT_MAX, ImVec2(ImGui::GetContentRegionAvail().x, 56.0f));

	ImGui::PopID();
	return modified;
}

}
