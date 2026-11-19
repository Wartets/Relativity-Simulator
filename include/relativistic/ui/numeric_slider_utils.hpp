#pragma once

#include <imgui.h>
#include "relativistic/units/expression_engine.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include <string>
#include <cstring>

namespace Relativistic::UI {

inline bool smart_quantity_input(
	const char* label,
	double* value_si,
	const Units::DimensionVector& expected_dimension,
	std::string& error_out
) noexcept {
	static thread_local char text_buffer[128];
	static thread_local const void* bound_value_ptr = nullptr;
	ImGui::PushID(label);

	if (bound_value_ptr != static_cast<const void*>(value_si) || !ImGui::IsItemActive()) {
		if (bound_value_ptr != static_cast<const void*>(value_si)) {
			std::snprintf(text_buffer, sizeof(text_buffer), "%.10g", *value_si);
			bound_value_ptr = value_si;
		}
	}

	bool committed = false;
	if (ImGui::InputText("##SmartQuantityInput", text_buffer, sizeof(text_buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
		std::string local_error;
		const auto parsed = Units::ExpressionEvaluator::evaluate_expect_dimension(text_buffer, expected_dimension, &local_error);
		if (parsed.has_value()) {
			*value_si = *parsed;
			error_out.clear();
			committed = true;
		} else {
			error_out = local_error;
		}
	}
	ImGui::SameLine();
	ImGui::TextUnformatted(label);
	if (!error_out.empty()) {
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.35f, 1.0f), "%s", error_out.c_str());
	}

	ImGui::PopID();
	return committed;
}

namespace Detail {

[[nodiscard]] inline std::pair<float, float> default_log_slider_bounds(float min_val, float max_val) noexcept {
	constexpr float kMinPositive = 1e-30f;
	constexpr float kMaxPositive = 1e35f;
	if (!(max_val > 0.0f)) {
		return {min_val, max_val};
	}
	const float safe_min = std::clamp(min_val, kMinPositive, kMaxPositive);
	const float safe_max = std::clamp(max_val, safe_min * 1.0001f, kMaxPositive);
	const float exp_min = std::clamp(std::floor(std::log10(safe_min)) - 1.0f, -30.0f, 35.0f);
	const float exp_max = std::clamp(std::ceil(std::log10(safe_max)) + 1.0f, exp_min + 0.1f, 35.0f);
	const float lower = std::pow(10.0f, exp_min);
	const float upper = std::pow(10.0f, exp_max);
	return {std::max(lower, kMinPositive), std::min(upper, kMaxPositive)};
}

[[nodiscard]] inline std::pair<double, double> default_log_slider_bounds_double(double min_val, double max_val) noexcept {
	constexpr double kMinPositive = 1e-300;
	constexpr double kMaxPositive = 1e300;
	if (!(max_val > 0.0)) {
		return {min_val, max_val};
	}
	const double safe_min = std::clamp(min_val, kMinPositive, kMaxPositive);
	const double safe_max = std::clamp(max_val, safe_min * 1.0001, kMaxPositive);
	const double exp_min = std::clamp(std::floor(std::log10(safe_min)) - 1.0, -300.0, 300.0);
	const double exp_max = std::clamp(std::ceil(std::log10(safe_max)) + 1.0, exp_min + 0.1, 300.0);
	const double lower = std::pow(10.0, exp_min);
	const double upper = std::pow(10.0, exp_max);
	return {std::max(lower, kMinPositive), std::min(upper, kMaxPositive)};
}

[[nodiscard]] inline bool use_log_slider(const bool* log_mode, float min_val, float max_val) noexcept {
	return log_mode != nullptr && *log_mode && min_val > 0.0f && max_val > min_val;
}

[[nodiscard]] inline bool use_log_slider_double(const bool* log_mode, double min_val, double max_val) noexcept {
	return log_mode != nullptr && *log_mode && min_val > 0.0 && max_val > min_val;
}

[[nodiscard]] inline bool format_has_exponent(const char* format) noexcept {
	for (const char* p = format; *p != '\0'; ++p) {
		if (*p == 'e' || *p == 'E' || *p == 'g' || *p == 'G') {
			return true;
		}
	}
	return false;
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

	if (log_mode != nullptr) {
		bool* prev_log_mode = ImGui::GetStateStorage()->GetBoolRef(ImGui::GetID("##prev_log_state"), *log_mode);
		if (*prev_log_mode && !*log_mode) {
			*value = std::clamp(*value, min_val, max_val);
		}
		*prev_log_mode = *log_mode;
	}

	constexpr float input_width = 92.0f;
	const float avail = ImGui::CalcItemWidth();
	ImGui::SetNextItemWidth(std::max(avail - input_width - 8.0f, 60.0f));

	const bool log_slider_enabled = Detail::use_log_slider(log_mode, min_val, max_val);
	const char* normal_display_format = Detail::format_has_exponent(format) ? "%.6f" : format;
	float visible_min = min_val;
	float visible_max = max_val;
	if (log_slider_enabled) {
		constexpr float kMaxSafeFloat = 1e35f;
		constexpr float kMinSafeFloat = 1e-30f;
		auto [resolved_log_min, resolved_log_max] = std::isfinite(log_min_override) && std::isfinite(log_max_override) && log_max_override > log_min_override
			? std::pair<float, float>{std::clamp(log_min_override, kMinSafeFloat, kMaxSafeFloat), std::clamp(log_max_override, kMinSafeFloat * 1.0001f, kMaxSafeFloat)}
			: Detail::default_log_slider_bounds(min_val, max_val);
		if (resolved_log_min >= resolved_log_max) {
			resolved_log_max = resolved_log_min * 10.0f;
		}
		visible_min = std::clamp(resolved_log_min, kMinSafeFloat, kMaxSafeFloat);
		visible_max = std::clamp(resolved_log_max, visible_min * 1.0001f, kMaxSafeFloat);
		float log_val = std::log10(std::clamp(*value, visible_min, visible_max));
		const float log_min = std::log10(visible_min);
		const float log_max = std::log10(visible_max);
		if (ImGui::SliderFloat("##slider", &log_val, log_min, log_max, "10^%.2f")) {
			*value = std::pow(10.0f, log_val);
			changed = true;
		}
	} else {
		if (ImGui::SliderFloat("##slider", value, min_val, max_val, normal_display_format)) {
			changed = true;
		}
	}

	ImGui::SameLine();
	ImGui::SetNextItemWidth(input_width);
	float input_copy = *value;
	const char* input_format = log_slider_enabled ? format : normal_display_format;
	if (ImGui::InputFloat("##input", &input_copy, 0.0f, 0.0f, input_format)) {
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
	double log_min_override = std::numeric_limits<double>::quiet_NaN(),
	double log_max_override = std::numeric_limits<double>::quiet_NaN()
) noexcept {
	bool changed = false;
	ImGui::PushID(label);

	if (log_mode != nullptr) {
		bool* prev_log_mode = ImGui::GetStateStorage()->GetBoolRef(ImGui::GetID("##prev_log_state"), *log_mode);
		if (*prev_log_mode && !*log_mode) {
			*value = std::clamp(*value, min_val, max_val);
		}
		*prev_log_mode = *log_mode;
	}

	constexpr float input_width = 92.0f;
	const float avail = ImGui::CalcItemWidth();
	ImGui::SetNextItemWidth(std::max(avail - input_width - 8.0f, 60.0f));

	const bool log_slider_enabled = Detail::use_log_slider_double(log_mode, min_val, max_val);
	const char* normal_display_format = Detail::format_has_exponent(format) ? "%.6g" : format;
	double visible_min = min_val;
	double visible_max = max_val;
	if (log_slider_enabled) {
		constexpr double kMaxSafeDouble = 1e300;
		constexpr double kMinSafeDouble = 1e-300;
		auto [resolved_log_min, resolved_log_max] = std::isfinite(log_min_override) && std::isfinite(log_max_override) && log_max_override > log_min_override
			? std::pair<double, double>{std::clamp(log_min_override, kMinSafeDouble, kMaxSafeDouble), std::clamp(log_max_override, kMinSafeDouble * 1.0001, kMaxSafeDouble)}
			: Detail::default_log_slider_bounds_double(min_val, max_val);
		if (resolved_log_min >= resolved_log_max) {
			resolved_log_max = resolved_log_min * 10.0;
		}
		visible_min = std::clamp(resolved_log_min, kMinSafeDouble, kMaxSafeDouble);
		visible_max = std::clamp(resolved_log_max, visible_min * 1.0001, kMaxSafeDouble);
		double log_val = std::log10(std::clamp(*value, visible_min, visible_max));
		const double log_min = std::log10(visible_min);
		const double log_max = std::log10(visible_max);
		if (ImGui::SliderScalar("##slider", ImGuiDataType_Double, &log_val, &log_min, &log_max, "10^%.2f")) {
			*value = std::pow(10.0, log_val);
			changed = true;
		}
	} else {
		if (ImGui::SliderScalar("##slider", ImGuiDataType_Double, value, &min_val, &max_val, normal_display_format)) {
			changed = true;
		}
	}

	ImGui::SameLine();
	ImGui::SetNextItemWidth(input_width);
	double input_copy = *value;
	const char* input_format = log_slider_enabled ? format : normal_display_format;
	if (ImGui::InputDouble("##input", &input_copy, 0.0, 0.0, input_format)) {
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

	if (log_mode != nullptr) {
		bool* prev_log_mode = ImGui::GetStateStorage()->GetBoolRef(ImGui::GetID("##prev_log_state"), *log_mode);
		if (*prev_log_mode && !*log_mode) {
			*value = std::clamp(*value, min_val, max_val);
		}
		*prev_log_mode = *log_mode;
	}

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
