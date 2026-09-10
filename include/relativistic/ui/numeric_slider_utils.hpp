#pragma once

#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace Relativistic::UI {

namespace Detail {

[[nodiscard]] inline std::pair<float, float> default_log_slider_bounds(float min_val, float max_val) noexcept {
	constexpr float kMinPositive = 1e-12f;
	if (!(max_val > 0.0f)) {
		return {min_val, max_val};
	}
	const float safe_min = std::max(min_val, kMinPositive);
	const float safe_max = std::max(max_val, safe_min * 1.0001f);
	const float lower = std::pow(10.0f, std::floor(std::log10(safe_min)) - 2.0f);
	const float upper = std::pow(10.0f, std::ceil(std::log10(safe_max)) + 2.0f);
	return {std::max(lower, kMinPositive), std::max(upper, safe_max * 1.0001f)};
}

[[nodiscard]] inline bool use_log_slider(const bool* log_mode, float min_val, float max_val) noexcept {
	return log_mode != nullptr && *log_mode && min_val > 0.0f && max_val > min_val;
}

} // namespace Detail

inline bool slider_float_with_input(
	const char* label,
	float* value,
	float min_val,
	float max_val,
	const char* format = "%.3f",
	bool* log_mode = nullptr,
	float log_min_override = std::numeric_limits<float>::quiet_NaN(),
	float log_max_override = std::numeric_limits<float>::quiet_NaN()
) noexcept {
	bool changed = false;
	ImGui::PushID(label);

	constexpr float input_width = 92.0f;
	const float avail = ImGui::CalcItemWidth();
	ImGui::SetNextItemWidth(std::max(avail - input_width - 8.0f, 60.0f));

	const bool log_slider_enabled = Detail::use_log_slider(log_mode, min_val, max_val);
	float visible_min = min_val;
	float visible_max = max_val;
	if (log_slider_enabled) {
		const auto [resolved_log_min, resolved_log_max] = std::isfinite(log_min_override) && std::isfinite(log_max_override) && log_max_override > log_min_override
			? std::pair<float, float>{log_min_override, log_max_override}
			: Detail::default_log_slider_bounds(min_val, max_val);
		visible_min = resolved_log_min;
		visible_max = resolved_log_max;
		float log_val = std::log10(std::clamp(*value, visible_min, visible_max));
		const float log_min = std::log10(visible_min);
		const float log_max = std::log10(visible_max);
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
		*value = std::clamp(input_copy, log_slider_enabled ? visible_min : min_val, log_slider_enabled ? visible_max : max_val);
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
	bool* log_mode = nullptr,
	float log_min_override = std::numeric_limits<float>::quiet_NaN(),
	float log_max_override = std::numeric_limits<float>::quiet_NaN()
) noexcept {
	float f_val = static_cast<float>(*value);
	const bool changed = slider_float_with_input(label, &f_val, static_cast<float>(min_val), static_cast<float>(max_val), format, log_mode, log_min_override, log_max_override);
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
	bool* log_mode = nullptr,
	int log_min_override = std::numeric_limits<int>::min(),
	int log_max_override = std::numeric_limits<int>::max()
) noexcept {
	bool changed = false;
	ImGui::PushID(label);

	constexpr float input_width = 92.0f;
	const float avail = ImGui::CalcItemWidth();
	ImGui::SetNextItemWidth(std::max(avail - input_width - 8.0f, 60.0f));

	const bool log_slider_enabled = Detail::use_log_slider(log_mode, static_cast<float>(min_val), static_cast<float>(max_val));
	int visible_min = min_val;
	int visible_max = max_val;
	if (log_slider_enabled) {
		const int resolved_min = std::max(log_min_override, 1);
		const int resolved_max = std::max(log_max_override, resolved_min + 1);
		visible_min = std::max(min_val, resolved_min);
		visible_max = std::min(max_val, resolved_max);
		if (visible_min < visible_max) {
			float log_val = std::log10(std::clamp(static_cast<float>(*value), static_cast<float>(visible_min), static_cast<float>(visible_max)));
			const float log_min = std::log10(static_cast<float>(visible_min));
			const float log_max = std::log10(static_cast<float>(visible_max));
			if (ImGui::SliderFloat("##slider", &log_val, log_min, log_max, "10^%.2f")) {
				*value = static_cast<int>(std::round(std::pow(10.0f, log_val)));
				changed = true;
			}
		} else {
			if (ImGui::SliderInt("##slider", value, min_val, max_val)) {
				changed = true;
			}
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
		*value = std::clamp(input_copy, log_slider_enabled ? visible_min : min_val, log_slider_enabled ? visible_max : max_val);
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
