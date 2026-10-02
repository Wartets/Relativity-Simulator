#pragma once

#include "relativistic/capture/easing.hpp"
#include "relativistic/capture/expression.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
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

[[nodiscard]] inline float ui_unit() noexcept {
	return ImGui::GetFontSize();
}

[[nodiscard]] inline float label_column_width() noexcept {
	const float unit = ui_unit();
	return std::clamp(ImGui::GetContentRegionAvail().x * 0.38f, unit * 7.0f, unit * 19.0f);
}

[[nodiscard]] inline std::string format_number(double value, int precision) {
	char buffer[48];
	std::snprintf(buffer, sizeof(buffer), "%.*f", precision, value);
	return buffer;
}

class FormScope {
public:
	FormScope() {
		const float unit = ui_unit();
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(unit * 0.55f, unit * 0.42f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(unit * 0.45f, unit * 0.22f));
		ImGui::PushItemWidth(-label_column_width());
		ImGui::PushTextWrapPos(0.0f);
	}

	~FormScope() {
		ImGui::PopTextWrapPos();
		ImGui::PopItemWidth();
		ImGui::PopStyleVar(2);
	}

	FormScope(const FormScope&) = delete;
	FormScope& operator=(const FormScope&) = delete;
};

class FlowLayout {
private:
	float right_edge_;
	bool first_{true};

public:
	FlowLayout() : right_edge_(ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x) {}

	[[nodiscard]] static float button_width(const char* label, bool compact = false) noexcept {
		const ImGuiStyle& style = ImGui::GetStyle();
		const float padding = compact ? style.FramePadding.x : style.FramePadding.x;
		return ImGui::CalcTextSize(label, nullptr, true).x + padding * 2.0f;
	}

	[[nodiscard]] static float checkbox_width(const char* label) noexcept {
		return ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x + ImGui::CalcTextSize(label, nullptr, true).x;
	}

	void next(float width) {
		if (!first_ && ImGui::GetItemRectMax().x + ImGui::GetStyle().ItemSpacing.x + width <= right_edge_) {
			ImGui::SameLine();
		}
		first_ = false;
	}

	[[nodiscard]] bool button(const char* label, float minimum_width = 0.0f) {
		const float width = std::max(button_width(label), minimum_width);
		next(width);
		return ImGui::Button(label, ImVec2(width, 0.0f));
	}

	[[nodiscard]] bool small_button(const char* label, bool highlighted = false) {
		next(button_width(label, true));
		if (highlighted) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
		}
		const bool pressed = ImGui::SmallButton(label);
		if (highlighted) {
			ImGui::PopStyleColor();
		}
		return pressed;
	}
};

inline void section_header(const char* label) {
	const float unit = ui_unit();
	ImGui::Dummy(ImVec2(0.0f, unit * 0.3f));
	ImGui::TextColored(kHeaderColor, "%s", label);
	const ImVec2 cursor = ImGui::GetCursorScreenPos();
	ImGui::GetWindowDrawList()->AddLine(
		ImVec2(cursor.x, cursor.y),
		ImVec2(cursor.x + ImGui::GetContentRegionAvail().x, cursor.y),
		IM_COL32(110, 170, 230, 120),
		1.0f
	);
	ImGui::Dummy(ImVec2(0.0f, unit * 0.2f));
}

inline bool begin_stat_table(const char* id, int max_columns) {
	const int columns = std::clamp(static_cast<int>(ImGui::GetContentRegionAvail().x / (ui_unit() * 8.5f)), 1, std::max(max_columns, 1));
	return ImGui::BeginTable(id, columns, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_BordersInnerV);
}

inline void end_stat_table() {
	ImGui::EndTable();
}

inline void stat_cell(const char* label, const std::string& value, const ImVec4& color = kAccentColor) {
	ImGui::TableNextColumn();
	ImGui::TextDisabled("%s", label);
	ImGui::TextColored(color, "%s", value.c_str());
}

inline bool horizontal_splitter(const char* id, float& bottom_height, float minimum_bottom, float maximum_bottom, float default_bottom) {
	const float thickness = std::max(ui_unit() * 0.5f, 6.0f);
	ImGui::InvisibleButton(id, ImVec2(-FLT_MIN, thickness));
	const bool hovered = ImGui::IsItemHovered();
	const bool active = ImGui::IsItemActive();
	if (hovered || active) {
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
	}
	bool changed = false;
	if (active) {
		const float next = std::clamp(bottom_height - ImGui::GetIO().MouseDelta.y, minimum_bottom, std::max(maximum_bottom, minimum_bottom));
		changed = next != bottom_height;
		bottom_height = next;
	}
	if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
		bottom_height = std::clamp(default_bottom, minimum_bottom, std::max(maximum_bottom, minimum_bottom));
		changed = true;
	}
	const ImVec2 low = ImGui::GetItemRectMin();
	const ImVec2 high = ImGui::GetItemRectMax();
	const float center_y = (low.y + high.y) * 0.5f;
	const float center_x = (low.x + high.x) * 0.5f;
	const ImU32 line_color = active ? IM_COL32(120, 190, 255, 230) : (hovered ? IM_COL32(120, 190, 255, 150) : IM_COL32(90, 100, 125, 110));
	ImDrawList* draw = ImGui::GetWindowDrawList();
	draw->AddLine(ImVec2(low.x, center_y), ImVec2(high.x, center_y), line_color, 1.0f);
	for (int k = -1; k <= 1; ++k) {
		draw->AddCircleFilled(ImVec2(center_x + static_cast<float>(k) * ui_unit() * 0.6f, center_y), 2.0f, line_color, 8);
	}
	return changed;
}

inline void help_marker(const char* text) {
	ImGui::TextDisabled("(?)");
	render_setting_tooltip(text);
}

inline bool begin_property_grid(const char* id, float label_share = 0.4f) {
	if (!ImGui::BeginTable(id, 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_PadOuterX)) {
		return false;
	}
	ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthStretch, label_share);
	ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 1.0f - label_share);
	return true;
}

inline void end_property_grid() {
	ImGui::EndTable();
}

template <typename Widget>
inline bool property_row(const char* label, const char* tooltip, Widget&& widget) {
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(label);
	if (tooltip != nullptr) {
		render_setting_tooltip(tooltip);
	}
	ImGui::TableNextColumn();
	ImGui::PushID(label);
	ImGui::SetNextItemWidth(-FLT_MIN);
	const bool changed = widget();
	ImGui::PopID();
	return changed;
}

inline void property_info(const char* label, const std::string& text) {
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(label);
	ImGui::TableNextColumn();
	ImGui::PushStyleColor(ImGuiCol_Text, kMutedColor);
	ImGui::TextWrapped("%s", text.c_str());
	ImGui::PopStyleColor();
}

inline bool property_drag(const char* label, double& value, double speed, double min_value, double max_value, const char* format = "%.3f", const char* tooltip = nullptr) {
	return property_row(label, tooltip, [&] { return drag_double("##value", value, speed, min_value, max_value, format); });
}

inline bool property_drag_free(const char* label, double& value, double speed, const char* format = "%.3f", const char* tooltip = nullptr) {
	return property_row(label, tooltip, [&] { return drag_double_free("##value", value, speed, format); });
}

inline bool property_check(const char* label, bool& value, const char* tooltip = nullptr) {
	return property_row(label, tooltip, [&] { return ImGui::Checkbox("##value", &value); });
}

inline bool property_text(const char* label, std::string& value, const char* tooltip = nullptr) {
	return property_row(label, tooltip, [&] { return input_text("##value", value); });
}

inline bool property_expression(const char* label, Capture::Expression& value, const char* tooltip = nullptr) {
	return property_row(label, tooltip, [&] { return input_expression("##value", value); });
}

inline bool property_u32(const char* label, uint32_t& value, uint32_t min_value, uint32_t max_value, ImGuiSliderFlags flags = 0, const char* tooltip = nullptr) {
	return property_row(label, tooltip, [&] { return slider_u32("##value", value, min_value, max_value, flags); });
}

template <typename EnumType, size_t N>
inline bool property_enum(const char* label, EnumType& value, const std::array<const char*, N>& names, const char* tooltip = nullptr) {
	return property_row(label, tooltip, [&] { return enum_combo("##value", value, names); });
}

inline bool property_vec3(const char* label, std::array<double, 3>& value, double speed = 0.1, const char* format = "%.3f", const char* tooltip = nullptr, const char* action_label = nullptr, bool* action_clicked = nullptr) {
	return property_row(label, tooltip, [&] {
		if (action_label != nullptr) {
			ImGui::SetNextItemWidth(-(ImGui::CalcTextSize(action_label).x + ImGui::GetStyle().FramePadding.x * 2.0f + ImGui::GetStyle().ItemSpacing.x));
		}
		const bool changed = drag_vec3("##value", value, speed, format);
		if (action_label != nullptr) {
			ImGui::SameLine();
			if (ImGui::Button(action_label) && action_clicked != nullptr) {
				*action_clicked = true;
			}
		}
		return changed;
	});
}

inline bool vertical_splitter(const char* id, float& left_width, float min_left, float max_left, float height) {
	ImGui::SameLine(0.0f, 2.0f);
	ImGui::InvisibleButton(id, ImVec2(6.0f, height));
	const bool hovered = ImGui::IsItemHovered();
	const bool active = ImGui::IsItemActive();
	if (hovered || active) {
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
	}
	bool changed = false;
	if (active) {
		left_width = std::clamp(left_width + ImGui::GetIO().MouseDelta.x, min_left, std::max(max_left, min_left));
		changed = true;
	}
	const ImVec2 minimum = ImGui::GetItemRectMin();
	const ImVec2 maximum = ImGui::GetItemRectMax();
	const ImU32 color = active ? IM_COL32(120, 190, 255, 230) : (hovered ? IM_COL32(120, 190, 255, 140) : IM_COL32(90, 100, 125, 120));
	ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(minimum.x + 2.0f, minimum.y), ImVec2(minimum.x + 4.0f, maximum.y), color, 1.0f);
	ImGui::SameLine(0.0f, 2.0f);
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
	render_setting_tooltip("0 gives a straight line, 1 uses the selected curve as is, values above 1 exaggerate its deviation from linear.");
	modified |= drag_double("Repeat Cycles", spec.repeat, 0.05, 1.0, 64.0, "%.2f");
	render_setting_tooltip("Plays the curve several times across the progress range.");
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
	if (ImGui::TreeNode("Input And Output Shaping")) {
		modified |= drag_double("Input Window Start", spec.input_start, 0.005, 0.0, 1.0, "%.3f");
		render_setting_tooltip("Progress value below which the curve stays at its first value.");
		modified |= drag_double("Input Window End", spec.input_end, 0.005, 0.0, 1.0, "%.3f");
		render_setting_tooltip("Progress value above which the curve stays at its last value. The curve is stretched between the two window bounds.");
		modified |= drag_double("Bias", spec.bias, 0.005, 0.01, 0.99, "%.3f");
		render_setting_tooltip("Pushes the curve toward its start (below 0.5) or toward its end (above 0.5). 0.5 leaves the curve unchanged.");
		modified |= drag_double("Gain", spec.gain, 0.005, 0.01, 0.99, "%.3f");
		render_setting_tooltip("Sharpens (above 0.5) or flattens (below 0.5) the middle of the curve. 0.5 leaves the curve unchanged.");
		modified |= drag_double("Quantize Steps", spec.quantize_steps, 0.1, 0.0, 256.0, "%.0f");
		render_setting_tooltip("Snaps the result to this many evenly spaced levels. 0 or 1 disables quantization.");
		modified |= drag_double("Output Start", spec.output_start, 0.005, -10.0, 10.0, "%.3f");
		render_setting_tooltip("Value produced when the shaped curve is at 0.");
		modified |= drag_double("Output End", spec.output_end, 0.005, -10.0, 10.0, "%.3f");
		render_setting_tooltip("Value produced when the shaped curve is at 1. Use values beyond the range to over-travel or invert the animation.");
		modified |= ImGui::Checkbox("Clamp Output To Range", &spec.clamp_output);
		render_setting_tooltip("Prevents overshoot, bounce and elastic curves from leaving the output range.");
		if (ImGui::SmallButton("Reset Shaping")) {
			spec.input_start = 0.0;
			spec.input_end = 1.0;
			spec.output_start = 0.0;
			spec.output_end = 1.0;
			spec.bias = 0.5;
			spec.gain = 0.5;
			spec.quantize_steps = 0.0;
			spec.clamp_output = false;
			modified = true;
		}
		ImGui::TreePop();
	}

	ImGui::PlotLines("##EasingPlot", samples.data(), static_cast<int>(samples.size()), 0, nullptr, FLT_MAX, FLT_MAX, ImVec2(ImGui::GetContentRegionAvail().x, 84.0f));
	ImGui::TextDisabled("f(0.25) = %.3f   f(0.50) = %.3f   f(0.75) = %.3f", spec.evaluate(0.25), spec.evaluate(0.5), spec.evaluate(0.75));

	ImGui::PopID();
	return modified;
}

}
