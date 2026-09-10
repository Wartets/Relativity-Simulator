#pragma once

#include <imgui.h>
#include <algorithm>
#include <cmath>

namespace Relativistic::UI {

inline bool slider_float_with_input(
	const char* label,
	float* value,
	float min_val,
	float max_val,
	const char* format = "%.3f",
	bool* log_mode = nullptr
) noexcept {
	bool changed = false;
	ImGui::PushID(label);

	constexpr float input_width = 92.0f;
	const float avail = ImGui::CalcItemWidth();
	ImGui::SetNextItemWidth(std::max(avail - input_width - 8.0f, 60.0f));

	if (log_mode != nullptr && *log_mode) {
		const float safe_min = std::max(min_val, 1e-9f);
		const float safe_max = std::max(max_val, safe_min * 1.0001f);
		float log_val = std::log10(std::clamp(*value, safe_min, safe_max));
		const float log_min = std::log10(safe_min);
		const float log_max = std::log10(safe_max);
		if (ImGui::SliderFloat("##slider", &log_val, log_min, log_max, "10^%.2f")) {
			*value = std::pow(10.0f, log_val);
			changed = true;
		}
	} else {
		if (ImGui::SliderFloat("##slider", value, min_val, max_val, format)) {
			changed = true;
		}
	}

	ImGui::SameLine();
	ImGui::SetNextItemWidth(input_width);
	float input_copy = *value;
	if (ImGui::InputFloat("##input", &input_copy, 0.0f, 0.0f, format)) {
		*value = std::clamp(input_copy, min_val, max_val);
		changed = true;
	}

	if (log_mode != nullptr) {
		ImGui::SameLine();
		ImGui::Checkbox("Log", log_mode);
	}

	ImGui::SameLine();
	ImGui::TextUnformatted(label);

	ImGui::PopID();
	return changed;
}

inline bool slider_double_with_input(
	const char* label,
	double* value,
	double min_val,
	double max_val,
	const char* format = "%.4f",
	bool* log_mode = nullptr
) noexcept {
	float f_val = static_cast<float>(*value);
	const bool changed = slider_float_with_input(label, &f_val, static_cast<float>(min_val), static_cast<float>(max_val), format, log_mode);
	if (changed) {
		*value = static_cast<double>(f_val);
	}
	return changed;
}

inline bool slider_int_with_input(
	const char* label,
	int* value,
	int min_val,
	int max_val,
	bool* log_mode = nullptr
) noexcept {
	bool changed = false;
	ImGui::PushID(label);

	constexpr float input_width = 92.0f;
	const float avail = ImGui::CalcItemWidth();
	ImGui::SetNextItemWidth(std::max(avail - input_width - 8.0f, 60.0f));

	if (log_mode != nullptr && *log_mode) {
		const float safe_min = std::max(static_cast<float>(min_val), 1.0f);
		const float safe_max = std::max(static_cast<float>(max_val), safe_min * 1.0001f);
		float log_val = std::log10(std::clamp(static_cast<float>(*value), safe_min, safe_max));
		const float log_min = std::log10(safe_min);
		const float log_max = std::log10(safe_max);
		if (ImGui::SliderFloat("##slider", &log_val, log_min, log_max, "10^%.2f")) {
			*value = static_cast<int>(std::round(std::pow(10.0f, log_val)));
			changed = true;
		}
	} else {
		if (ImGui::SliderInt("##slider", value, min_val, max_val)) {
			changed = true;
		}
	}

	ImGui::SameLine();
	ImGui::SetNextItemWidth(input_width);
	int input_copy = *value;
	if (ImGui::InputInt("##input", &input_copy, 0, 0)) {
		*value = std::clamp(input_copy, min_val, max_val);
		changed = true;
	}

	if (log_mode != nullptr) {
		ImGui::SameLine();
		ImGui::Checkbox("Log", log_mode);
	}

	ImGui::SameLine();
	ImGui::TextUnformatted(label);

	ImGui::PopID();
	return changed;
}

}
