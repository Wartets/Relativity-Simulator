#pragma once

#include "relativistic/capture/motion_script.hpp"
#include "relativistic/capture/motion_script_file.hpp"
#include "relativistic/capture/path_preview.hpp"
#include "relativistic/capture/path_preview_builder.hpp"
#include "relativistic/capture/script_events.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/ui/capture_widgets.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include <imgui.h>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace Relativistic::UI {

namespace MotionScriptEditorDetail {

inline constexpr std::array<const char*, 4> kAnchorNames{"World (Absolute)", "Continue From Previous End", "Offset From Previous End", "Track Body"};
inline constexpr std::array<const char*, 3> kBlendNames{"Replace", "Add", "Add Relative To Layer Start"};
inline constexpr std::array<const char*, 6> kOrientationNames{"Free (Keep Current)", "Fixed Angles", "Interpolated Angles", "Look At Target", "Along Travel Direction", "Expressions"};
inline constexpr std::array<const char*, 5> kWaveNames{"Sine", "Triangle", "Square", "Sawtooth", "Smooth Noise"};
inline constexpr std::array<const char*, 3> kPathEndNames{"Clamp At End", "Loop", "Ping-Pong"};
inline constexpr std::array<const char*, 4> kTriggerNames{"Script Time", "Segment Start", "Segment End", "Segment Fraction"};
inline constexpr std::array<const char*, 10> kActionNames{"Marker", "Capture Still", "Set Parameter", "Set Time Warp", "Pause Simulation", "Resume Simulation", "Step Ticks", "Set Metric", "Set Integrator", "Load Scenario"};
inline constexpr std::array<const char*, 10> kMetricNames{
	"Flat Minkowski", "Schwarzschild Black Hole", "Kerr Rotating Black Hole", "Reissner-Nordstrom Charged", "Kerr-Newman Charged Rotating",
	"Schwarzschild-de Sitter (Lambda)", "FLRW Cosmological Expansion", "Morris-Thorne Traversable Wormhole", "Alcubierre Warp Drive Bubble", "BSSN 3+1 Numerical Grid"
};
inline constexpr std::array<const char*, 6> kIntegratorNames{
	"Dormand-Prince RK45 (Adaptive)", "Cash-Karp 5(4) (Adaptive)", "Vernier 9(8) High-Order", "Symplectic Gauss-Legendre 4th", "Symplectic Gauss-Legendre 6th", "Hermite 4th-Order (Aarseth)"
};

struct ExpressionPreset {
	const char* name;
	std::array<const char*, 3> sources;
};

inline constexpr std::array<ExpressionPreset, 6> kCartesianPresets{{
	{"Circle", {"40*cos(2*pi*u)", "40*sin(2*pi*u)", "0"}},
	{"Rising Spiral", {"(20+30*u)*cos(6*pi*u)", "(20+30*u)*sin(6*pi*u)", "40*u"}},
	{"Lissajous 3D", {"40*sin(2*pi*3*u)", "40*sin(2*pi*2*u+pi/2)", "20*sin(2*pi*5*u)"}},
	{"Figure Eight", {"50*sin(2*pi*u)", "25*sin(4*pi*u)", "10*sin(2*pi*u)"}},
	{"Damped Spring", {"40*exp(-3*u)*cos(10*pi*u)", "40*exp(-3*u)*sin(10*pi*u)", "40*(1-u)"}},
	{"Parametric Sweep", {"a*u", "b*sin(2*pi*u*c)", "k*u"}}
}};

inline constexpr std::array<ExpressionPreset, 3> kCylindricalPresets{{
	{"Funnel", {"40-30*u", "8*pi*u", "30*u"}},
	{"Pulsing Ring", {"30+8*sin(8*pi*u)", "2*pi*u", "10*sin(4*pi*u)"}},
	{"Descending Helix", {"25", "6*pi*u", "40-80*u"}}
}};

inline constexpr std::array<ExpressionPreset, 3> kSphericalPresets{{
	{"Spiral Dive", {"60-45*u", "pi/2+0.5*sin(4*pi*u)", "10*pi*u"}},
	{"Pole To Pole", {"40", "pi*u", "8*pi*u"}},
	{"Breathing Orbit", {"40+10*sin(6*pi*u)", "pi/2", "4*pi*u"}}
}};

[[nodiscard]] inline std::span<const ExpressionPreset> expression_presets(Capture::ShapeKind kind) noexcept {
	switch (kind) {
		case Capture::ShapeKind::ExpressionCylindrical: return kCylindricalPresets;
		case Capture::ShapeKind::ExpressionSpherical: return kSphericalPresets;
		default: return kCartesianPresets;
	}
}

[[nodiscard]] inline bool control_is_position(Capture::ShapeKind kind, size_t index) noexcept {
	switch (kind) {
		case Capture::ShapeKind::Hold:
		case Capture::ShapeKind::Linear:
		case Capture::ShapeKind::QuadraticBezier:
		case Capture::ShapeKind::CubicBezier:
		case Capture::ShapeKind::Helix:
			return true;
		case Capture::ShapeKind::Wave:
			return index < 2;
		default:
			return index == 0;
	}
}

[[nodiscard]] inline const std::vector<const char*>& event_parameter_names() {
	static const std::vector<const char*> names = [] {
		std::vector<const char*> result;
		result.reserve(Capture::kEventParameters.size());
		for (const auto& entry : Capture::kEventParameters) {
			result.push_back(entry.name);
		}
		return result;
	}();
	return names;
}

}

class MotionScriptEditor {
public:
	using OrchestratorType = Orchestrator::SimulationOrchestrator<1024>;

private:
	struct BodyChoice {
		int32_t id{0};
		std::string label{};
	};

	static constexpr size_t kTextBufferCapacity = 1U << 19U;

	int selected_segment_{-1};
	int selected_layer_{0};
	int selected_event_{-1};
	uint32_t new_segment_kind_{static_cast<uint32_t>(Capture::ShapeKind::Linear)};
	uint32_t new_layer_kind_{static_cast<uint32_t>(Capture::ShapeKind::Wave)};
	double new_segment_duration_{5.0};
	int preset_index_{0};
	double time_scale_factor_{1.0};
	double fit_duration_{30.0};
	double cursor_seconds_{0.0};
	bool modified_{false};
	std::optional<double> cursor_request_{};
	std::string file_message_{};
	std::string file_path_{"config/motion_script.cfg"};
	std::vector<char> text_buffer_{};
	std::vector<BodyChoice> bodies_{};

	bool mark(bool value) noexcept {
		modified_ = modified_ || value;
		return value;
	}

	void refresh_bodies(OrchestratorType& orchestrator) {
		bodies_.clear();
		std::lock_guard<std::recursive_mutex> lock(orchestrator.nbody_system().bodies_mutex());
		for (const auto& body : orchestrator.nbody_system().bodies()) {
			std::string label = "Body #" + std::to_string(body.id);
			if (body.has_name()) {
				label += " (" + std::string(body.name_view()) + ")";
			}
			bodies_.push_back(BodyChoice{static_cast<int32_t>(body.id), std::move(label)});
		}
	}

	bool edit_body_reference(const char* label, int32_t& id) const {
		std::vector<const char*> items;
		items.reserve(bodies_.size() + 1);
		items.push_back("World Origin / Fixed Point");
		int current = 0;
		for (size_t i = 0; i < bodies_.size(); ++i) {
			items.push_back(bodies_[i].label.c_str());
			if (bodies_[i].id == id) {
				current = static_cast<int>(i + 1);
			}
		}
		if (ImGui::Combo(label, &current, items.data(), static_cast<int>(items.size()))) {
			id = (current == 0) ? Capture::kOriginReference : bodies_[static_cast<size_t>(current - 1)].id;
			return true;
		}
		return false;
	}

	void select_segment(int index) noexcept {
		selected_segment_ = index;
		selected_layer_ = 0;
	}

	void insert_segment(Capture::MotionScript& script, Capture::ScriptSegment segment) {
		const size_t position = (selected_segment_ >= 0 && selected_segment_ < static_cast<int>(script.segments.size()))
			? static_cast<size_t>(selected_segment_ + 1)
			: script.segments.size();
		script.segments.insert(script.segments.begin() + static_cast<ptrdiff_t>(position), std::move(segment));
		select_segment(static_cast<int>(position));
		mark(true);
	}

	[[nodiscard]] static int segment_index_at(const Capture::MotionScript& script, double seconds) {
		double accumulated = 0.0;
		int last_active = -1;
		for (size_t i = 0; i < script.segments.size(); ++i) {
			const auto& segment = script.segments[i];
			if (!segment.enabled || segment.duration <= 1.0e-9) {
				continue;
			}
			last_active = static_cast<int>(i);
			if (seconds <= accumulated + segment.duration) {
				return last_active;
			}
			accumulated += segment.duration;
		}
		return last_active;
	}

	void render_timeline(Capture::MotionScript& script) {
		using namespace CaptureWidgets;
		const float width = std::max(ImGui::GetContentRegionAvail().x, 120.0f);
		constexpr float height = 52.0f;
		const ImVec2 origin = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton("##ScriptTimeline", ImVec2(width, height));
		const bool hovered = ImGui::IsItemHovered();
		const bool active = ImGui::IsItemActive();
		ImDrawList* draw = ImGui::GetWindowDrawList();
		const double total = script.total_duration();
		const auto time_to_x = [&](double seconds) noexcept {
			return origin.x + static_cast<float>(std::clamp(seconds / total, 0.0, 1.0)) * width;
		};

		draw->AddRectFilled(origin, ImVec2(origin.x + width, origin.y + height), IM_COL32(18, 20, 30, 255), 4.0f);

		size_t slot = 0;
		double accumulated = 0.0;
		for (size_t i = 0; i < script.segments.size(); ++i) {
			const auto& segment = script.segments[i];
			if (!segment.enabled || segment.duration <= 1.0e-9) {
				continue;
			}
			const float x0 = time_to_x(accumulated);
			const float x1 = time_to_x(accumulated + segment.duration);
			const auto& rgb = Capture::kPathPreviewPalette[slot % Capture::kPathPreviewPalette.size()];
			const bool selected = selected_segment_ == static_cast<int>(i);
			const ImVec2 top_left(x0 + 1.0f, origin.y + 14.0f);
			const ImVec2 bottom_right(std::max(x1 - 1.0f, x0 + 2.0f), origin.y + height - 6.0f);
			draw->AddRectFilled(top_left, bottom_right, IM_COL32(rgb[0], rgb[1], rgb[2], selected ? 235 : 150), 3.0f);
			if (selected) {
				draw->AddRect(top_left, bottom_right, IM_COL32(255, 255, 255, 255), 3.0f, 0, 2.0f);
			}
			if (bottom_right.x - top_left.x > 36.0f) {
				draw->PushClipRect(top_left, bottom_right, true);
				draw->AddText(ImVec2(top_left.x + 4.0f, top_left.y + 8.0f), IM_COL32(10, 12, 18, 255), segment.name.c_str());
				draw->PopClipRect();
			}
			accumulated += segment.duration;
			++slot;
		}

		for (size_t i = 0; i < script.events.size(); ++i) {
			if (!script.events[i].enabled) {
				continue;
			}
			const float x = time_to_x(script.resolved_event_time(script.events[i]));
			const float y = origin.y + 7.0f;
			const ImU32 color = (selected_event_ == static_cast<int>(i)) ? IM_COL32(255, 255, 255, 255) : IM_COL32(255, 220, 70, 235);
			draw->AddQuadFilled(ImVec2(x, y - 5.0f), ImVec2(x + 5.0f, y), ImVec2(x, y + 5.0f), ImVec2(x - 5.0f, y), color);
		}

		const float cursor_x = time_to_x(cursor_seconds_);
		draw->AddLine(ImVec2(cursor_x, origin.y), ImVec2(cursor_x, origin.y + height), IM_COL32(255, 245, 140, 255), 2.0f);

		const ImVec2 mouse = ImGui::GetIO().MousePos;
		const double hover_time = std::clamp(static_cast<double>((mouse.x - origin.x) / width), 0.0, 1.0) * total;
		if (hovered) {
			const int hovered_segment = segment_index_at(script, hover_time);
			if (hovered_segment >= 0) {
				ImGui::SetTooltip("%s | %.3f s", script.segments[static_cast<size_t>(hovered_segment)].name.c_str(), hover_time);
			}
		}
		if (ImGui::IsItemClicked()) {
			int event_hit = -1;
			float best_distance = 7.0f;
			if (mouse.y < origin.y + 15.0f) {
				for (size_t i = 0; i < script.events.size(); ++i) {
					const float distance = std::abs(time_to_x(script.resolved_event_time(script.events[i])) - mouse.x);
					if (distance < best_distance) {
						best_distance = distance;
						event_hit = static_cast<int>(i);
					}
				}
			}
			if (event_hit >= 0) {
				selected_event_ = event_hit;
			} else {
				const int segment_hit = segment_index_at(script, hover_time);
				if (segment_hit >= 0) {
					select_segment(segment_hit);
				}
			}
		}
		if (active) {
			cursor_request_ = hover_time;
		}
	}

	bool edit_shape(Capture::ShapeSpec& shape, OrchestratorType& orchestrator) {
		using namespace CaptureWidgets;
		using namespace MotionScriptEditorDetail;
		bool changed = false;

		const auto& names = Capture::shape_kind_names();
		int kind_index = static_cast<int>(shape.kind);
		if (ImGui::Combo("Shape", &kind_index, names.data(), static_cast<int>(names.size()))) {
			shape.reset(static_cast<Capture::ShapeKind>(kind_index));
			changed = true;
		}
		ImGui::SameLine();
		if (ImGui::SmallButton("Reset Shape")) {
			shape.reset(shape.kind);
			changed = true;
		}

		const auto& descriptor = Capture::shape_descriptor(shape.kind);
		for (size_t i = 0; i < descriptor.control_labels.size(); ++i) {
			if (descriptor.control_labels[i] == nullptr) {
				continue;
			}
			ImGui::PushID(static_cast<int>(i));
			changed |= drag_vec3(descriptor.control_labels[i], shape.controls[i], 0.1, "%.3f");
			if (control_is_position(shape.kind, i)) {
				ImGui::SameLine();
				if (ImGui::SmallButton("Cam")) {
					shape.controls[i] = orchestrator.camera().position;
					changed = true;
				}
				render_setting_tooltip("Copies the live camera position into this control.");
			}
			ImGui::PopID();
		}

		for (size_t i = 0; i < descriptor.value_labels.size(); ++i) {
			if (descriptor.value_labels[i] == nullptr) {
				continue;
			}
			ImGui::PushID(static_cast<int>(100 + i));
			changed |= drag_double_free(descriptor.value_labels[i], shape.values[i], 0.05, "%.4f");
			ImGui::PopID();
		}

		if (descriptor.waypoints) {
			ImGui::TextColored(kHeaderColor, "Waypoints (%zu)", shape.waypoints.size());
			changed |= ImGui::Checkbox("Closed Loop", &shape.closed);
			if (shape.kind != Capture::ShapeKind::BSpline) {
				ImGui::SameLine();
				changed |= ImGui::Checkbox("Uniform Speed", &shape.uniform_speed);
			}
			int remove_index = -1;
			int duplicate_index = -1;
			ImGui::BeginChild("##Waypoints", ImVec2(0.0f, 180.0f), true);
			ImGuiListClipper clipper;
			clipper.Begin(static_cast<int>(shape.waypoints.size()));
			while (clipper.Step()) {
				for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
					ImGui::PushID(i);
					changed |= drag_vec3("##waypoint", shape.waypoints[static_cast<size_t>(i)], 0.1, "%.2f");
					ImGui::SameLine();
					if (ImGui::SmallButton("Cam")) {
						shape.waypoints[static_cast<size_t>(i)] = orchestrator.camera().position;
						changed = true;
					}
					ImGui::SameLine();
					if (ImGui::SmallButton("+")) {
						duplicate_index = i;
					}
					ImGui::SameLine();
					if (ImGui::SmallButton("X")) {
						remove_index = i;
					}
					ImGui::PopID();
				}
			}
			ImGui::EndChild();
			if (duplicate_index >= 0) {
				const auto copy = shape.waypoints[static_cast<size_t>(duplicate_index)];
				shape.waypoints.insert(shape.waypoints.begin() + duplicate_index + 1, copy);
				changed = true;
			}
			if (remove_index >= 0) {
				shape.waypoints.erase(shape.waypoints.begin() + remove_index);
				changed = true;
			}
			if (ImGui::SmallButton("Add From Camera")) {
				shape.waypoints.push_back(orchestrator.camera().position);
				changed = true;
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Add Blank")) {
				shape.waypoints.push_back(shape.waypoints.empty() ? Capture::Vec3{0.0, 0.0, 0.0} : shape.waypoints.back());
				changed = true;
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Reverse")) {
				std::reverse(shape.waypoints.begin(), shape.waypoints.end());
				changed = true;
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Subdivide")) {
				std::vector<Capture::Vec3> refined;
				refined.reserve(shape.waypoints.size() * 2);
				for (size_t i = 0; i < shape.waypoints.size(); ++i) {
					refined.push_back(shape.waypoints[i]);
					const bool has_next = (i + 1 < shape.waypoints.size()) || (shape.closed && shape.waypoints.size() > 1);
					if (has_next) {
						refined.push_back(Capture::ScriptMath::lerp(shape.waypoints[i], shape.waypoints[(i + 1) % shape.waypoints.size()], 0.5));
					}
				}
				shape.waypoints = std::move(refined);
				changed = true;
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Clear")) {
				shape.waypoints.clear();
				changed = true;
			}
		}

		if (descriptor.expressions) {
			const auto labels = Capture::shape_expression_labels(shape.kind);
			for (size_t i = 0; i < shape.expressions.size(); ++i) {
				ImGui::PushID(static_cast<int>(200 + i));
				changed |= input_expression(labels[i], shape.expressions[i]);
				ImGui::PopID();
			}
			ImGui::TextDisabled("Variables: t local seconds, u progress, d duration, g global seconds, a b c k parameters.");
			const auto presets = expression_presets(shape.kind);
			int pick = -1;
			std::vector<const char*> preset_names;
			preset_names.reserve(presets.size());
			for (const auto& preset : presets) {
				preset_names.push_back(preset.name);
			}
			if (ImGui::Combo("Equation Preset", &pick, preset_names.data(), static_cast<int>(preset_names.size()))) {
				for (size_t i = 0; i < shape.expressions.size(); ++i) {
					shape.expressions[i].assign(presets[static_cast<size_t>(pick)].sources[i]);
				}
				changed = true;
			}
		}

		if (ImGui::TreeNode("Shape Transform")) {
			changed |= drag_vec3("Scale", shape.scale, 0.01, "%.3f");
			changed |= drag_vec3("Rotation (Roll, Pitch, Yaw deg)", shape.rotation_deg, 0.25, "%.2f");
			changed |= drag_vec3("Pivot", shape.pivot, 0.1, "%.3f");
			changed |= drag_vec3("Translation", shape.translation, 0.1, "%.3f");
			if (ImGui::SmallButton("Reset Transform")) {
				shape.scale = {1.0, 1.0, 1.0};
				shape.rotation_deg = {0.0, 0.0, 0.0};
				shape.pivot = {0.0, 0.0, 0.0};
				shape.translation = {0.0, 0.0, 0.0};
				changed = true;
			}
			ImGui::TreePop();
		}
		return changed;
	}

	bool edit_layer(Capture::ShapeLayer& layer, OrchestratorType& orchestrator) {
		using namespace CaptureWidgets;
		using namespace MotionScriptEditorDetail;
		bool changed = false;
		changed |= input_text("Layer Name", layer.name);
		changed |= ImGui::Checkbox("Layer Enabled", &layer.enabled);
		changed |= enum_combo("Blend Mode", layer.blend, kBlendNames);
		render_setting_tooltip("Replace overrides the stack, Add sums absolute positions, Add Relative sums only the displacement from the layer start, which is ideal for wobbles, spirals and offsets over a base path.");
		changed |= drag_double("Weight Start", layer.weight_start, 0.01, -50.0, 50.0, "%.3f");
		changed |= drag_double("Weight End", layer.weight_end, 0.01, -50.0, 50.0, "%.3f");
		changed |= drag_double("Active Window Start", layer.window_start, 0.005, 0.0, 1.0, "%.3f");
		changed |= drag_double("Active Window End", layer.window_end, 0.005, 0.0, 1.0, "%.3f");
		if (ImGui::TreeNode("Layer Time Easing")) {
			changed |= edit_easing("LayerEasing", layer.easing);
			ImGui::TreePop();
		}
		changed |= edit_shape(layer.shape, orchestrator);
		return changed;
	}

	bool edit_modulation(Capture::ModulationSpec& modulation) {
		using namespace CaptureWidgets;
		using namespace MotionScriptEditorDetail;
		bool changed = ImGui::Checkbox("Enable Modulation", &modulation.enabled);
		if (modulation.enabled) {
			changed |= enum_combo("Wave", modulation.wave, kWaveNames);
			changed |= drag_double_free("Amplitude Start", modulation.amplitude, 0.01, "%.4f");
			changed |= drag_double_free("Amplitude End", modulation.amplitude_end, 0.01, "%.4f");
			changed |= drag_double("Frequency (Hz)", modulation.frequency, 0.005, 0.0, 1000.0, "%.4f");
			changed |= drag_double("Phase (deg)", modulation.phase_deg, 0.5, -360.0, 360.0, "%.1f");
			changed |= drag_double("Edge Fade", modulation.fade, 0.005, 0.0, 0.5, "%.3f");
		}
		return changed;
	}

	bool edit_channel(const char* label, Capture::ScalarChannel& channel) {
		using namespace CaptureWidgets;
		bool changed = false;
		ImGui::PushID(label);
		if (ImGui::TreeNode(label)) {
			changed |= ImGui::Checkbox("Animate", &channel.enabled);
			if (channel.enabled) {
				changed |= drag_double_free("Start", channel.start, 0.05, "%.4f");
				changed |= drag_double_free("End", channel.end, 0.05, "%.4f");
				if (ImGui::TreeNode("Easing")) {
					changed |= edit_easing("ChannelEasing", channel.easing);
					ImGui::TreePop();
				}
				changed |= ImGui::Checkbox("Use Expression", &channel.use_expression);
				if (channel.use_expression) {
					changed |= input_expression("Value Expression", channel.expression);
					ImGui::TextDisabled("Variables: t, u, d, g, a = start, b = end.");
				}
				if (ImGui::TreeNode("Modulation")) {
					changed |= edit_modulation(channel.modulation);
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}
		ImGui::PopID();
		return changed;
	}

	bool edit_orientation(Capture::OrientationSpec& orientation) {
		using namespace CaptureWidgets;
		using namespace MotionScriptEditorDetail;
		bool changed = enum_combo("Orientation Mode", orientation.mode, kOrientationNames);
		switch (orientation.mode) {
			case Capture::OrientationMode::Fixed:
				changed |= drag_double("Pitch (deg)", orientation.start[0], 0.25, -89.0, 89.0, "%.2f");
				changed |= drag_double("Yaw (deg)", orientation.start[1], 0.25, -36000.0, 36000.0, "%.2f");
				break;
			case Capture::OrientationMode::Interpolated:
				changed |= drag_double("Pitch Start (deg)", orientation.start[0], 0.25, -89.0, 89.0, "%.2f");
				changed |= drag_double("Yaw Start (deg)", orientation.start[1], 0.25, -36000.0, 36000.0, "%.2f");
				changed |= drag_double("Pitch End (deg)", orientation.end[0], 0.25, -89.0, 89.0, "%.2f");
				changed |= drag_double("Yaw End (deg)", orientation.end[1], 0.25, -36000.0, 36000.0, "%.2f");
				if (ImGui::TreeNode("Orientation Easing")) {
					changed |= edit_easing("OrientationEasing", orientation.easing);
					ImGui::TreePop();
				}
				break;
			case Capture::OrientationMode::LookAtTarget:
				changed |= edit_body_reference("Target Body", orientation.target_body);
				changed |= drag_vec3("Target Offset", orientation.target_offset, 0.1, "%.3f");
				break;
			case Capture::OrientationMode::AlongTravel:
				changed |= drag_double("Look-Ahead (Progress)", orientation.look_ahead, 0.001, 0.001, 0.5, "%.4f");
				break;
			case Capture::OrientationMode::Expression:
				changed |= input_expression("Pitch Expression (deg)", orientation.expressions[0]);
				changed |= input_expression("Yaw Expression (deg)", orientation.expressions[1]);
				ImGui::TextDisabled("Variables: t, u, d, g.");
				break;
			case Capture::OrientationMode::Free:
			default:
				break;
		}
		changed |= drag_double("Pitch Offset (deg)", orientation.pitch_offset, 0.1, -180.0, 180.0, "%.2f");
		changed |= drag_double("Yaw Offset (deg)", orientation.yaw_offset, 0.1, -360.0, 360.0, "%.2f");
		return changed;
	}

	bool edit_shake(Capture::ShakeSpec& shake) {
		using namespace CaptureWidgets;
		bool changed = ImGui::Checkbox("Enable Camera Shake", &shake.enabled);
		if (shake.enabled) {
			changed |= drag_vec3("Position Amplitude", shake.position_amplitude, 0.01, "%.3f");
			changed |= drag_vec3("Rotation Amplitude (deg)", shake.rotation_amplitude, 0.01, "%.3f");
			changed |= drag_double("Frequency (Hz)", shake.frequency, 0.01, 0.0, 100.0, "%.3f");
			changed |= slider_u32("Seed", shake.seed, 0U, 9999U);
			changed |= drag_double("Edge Fade", shake.fade, 0.005, 0.0, 0.5, "%.3f");
		}
		return changed;
	}

	bool edit_segment(Capture::MotionScript& script, Capture::ScriptSegment& segment, OrchestratorType& orchestrator) {
		using namespace CaptureWidgets;
		using namespace MotionScriptEditorDetail;
		bool changed = false;

		ImGui::TextColored(kHeaderColor, "Segment: %s", segment.name.c_str());
		changed |= input_text("Segment Name", segment.name);
		changed |= ImGui::Checkbox("Segment Enabled", &segment.enabled);
		changed |= drag_double("Duration (s)", segment.duration, 0.05, 0.01, 86400.0, "%.3f");
		if (ImGui::TreeNode("Segment Time Easing")) {
			changed |= edit_easing("SegmentEasing", segment.time_easing);
			ImGui::TreePop();
		}

		if (ImGui::CollapsingHeader("Anchor")) {
			changed |= enum_combo("Anchor Mode", segment.anchor, kAnchorNames);
			if (segment.anchor == Capture::AnchorMode::TrackBody) {
				changed |= edit_body_reference("Anchor Body", segment.anchor_body);
			}
			changed |= drag_vec3("Anchor Offset", segment.anchor_offset, 0.1, "%.3f");
		}

		if (ImGui::CollapsingHeader("Movement Layers", ImGuiTreeNodeFlags_DefaultOpen)) {
			const auto& names = Capture::shape_kind_names();
			int add_kind = static_cast<int>(new_layer_kind_);
			if (ImGui::Combo("New Layer Shape", &add_kind, names.data(), static_cast<int>(names.size()))) {
				new_layer_kind_ = static_cast<uint32_t>(add_kind);
			}
			if (ImGui::Button("Add Layer")) {
				Capture::ShapeLayer layer;
				const auto kind = static_cast<Capture::ShapeKind>(new_layer_kind_);
				layer.name = Capture::shape_descriptor(kind).name;
				layer.blend = segment.layers.empty() ? Capture::LayerBlend::Replace : Capture::LayerBlend::AddRelative;
				layer.shape = Capture::ShapeSpec::make(kind);
				segment.layers.push_back(std::move(layer));
				selected_layer_ = static_cast<int>(segment.layers.size()) - 1;
				changed = true;
			}
			selected_layer_ = std::clamp(selected_layer_, 0, std::max(static_cast<int>(segment.layers.size()) - 1, 0));
			ImGui::SameLine();
			ImGui::BeginDisabled(segment.layers.empty());
			if (ImGui::Button("Duplicate Layer")) {
				Capture::ShapeLayer copy = segment.layers[static_cast<size_t>(selected_layer_)];
				copy.name += " Copy";
				segment.layers.insert(segment.layers.begin() + selected_layer_ + 1, std::move(copy));
				++selected_layer_;
				changed = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("Delete Layer")) {
				segment.layers.erase(segment.layers.begin() + selected_layer_);
				selected_layer_ = std::max(selected_layer_ - 1, 0);
				changed = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("Layer Up") && selected_layer_ > 0) {
				std::swap(segment.layers[static_cast<size_t>(selected_layer_)], segment.layers[static_cast<size_t>(selected_layer_ - 1)]);
				--selected_layer_;
				changed = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("Layer Down") && selected_layer_ + 1 < static_cast<int>(segment.layers.size())) {
				std::swap(segment.layers[static_cast<size_t>(selected_layer_)], segment.layers[static_cast<size_t>(selected_layer_ + 1)]);
				++selected_layer_;
				changed = true;
			}
			ImGui::EndDisabled();

			ImGui::BeginChild("##LayerList", ImVec2(0.0f, 96.0f), true);
			for (size_t i = 0; i < segment.layers.size(); ++i) {
				ImGui::PushID(static_cast<int>(i));
				bool enabled = segment.layers[i].enabled;
				if (ImGui::Checkbox("##layer_enabled", &enabled)) {
					segment.layers[i].enabled = enabled;
					changed = true;
				}
				ImGui::SameLine();
				char label[192];
				std::snprintf(label, sizeof(label), "%02zu | %s | %s | %s", i + 1, segment.layers[i].name.c_str(), Capture::shape_descriptor(segment.layers[i].shape.kind).name, kBlendNames[static_cast<size_t>(segment.layers[i].blend)]);
				if (ImGui::Selectable(label, selected_layer_ == static_cast<int>(i))) {
					selected_layer_ = static_cast<int>(i);
				}
				ImGui::PopID();
			}
			ImGui::EndChild();

			if (!segment.layers.empty()) {
				ImGui::PushID("SelectedLayer");
				changed |= edit_layer(segment.layers[static_cast<size_t>(selected_layer_)], orchestrator);
				ImGui::PopID();
			}
		}

		if (ImGui::CollapsingHeader("Orientation")) {
			changed |= edit_orientation(segment.orientation);
		}
		if (ImGui::CollapsingHeader("Camera Channels")) {
			changed |= edit_channel("Field Of View (deg)", segment.fov);
			changed |= edit_channel("Exposure (EV)", segment.exposure);
			changed |= edit_channel("Roll (deg)", segment.roll);
			changed |= edit_channel("Simulation Rate Multiplier", segment.warp);
		}
		if (ImGui::CollapsingHeader("Camera Shake")) {
			changed |= edit_shake(segment.shake);
		}
		static_cast<void>(script);
		return changed;
	}

	void render_segment_list(Capture::MotionScript& script, OrchestratorType& orchestrator) {
		using namespace CaptureWidgets;
		ImGui::TextColored(kHeaderColor, "Segments");
		ImGui::BeginChild("##SegmentList", ImVec2(0.0f, 150.0f), true);
		for (size_t i = 0; i < script.segments.size(); ++i) {
			ImGui::PushID(static_cast<int>(i));
			bool enabled = script.segments[i].enabled;
			if (ImGui::Checkbox("##segment_enabled", &enabled)) {
				script.segments[i].enabled = enabled;
				mark(true);
			}
			ImGui::SameLine();
			const auto& segment = script.segments[i];
			const char* first_shape = segment.layers.empty() ? "Empty" : Capture::shape_descriptor(segment.layers.front().shape.kind).name;
			char label[224];
			std::snprintf(label, sizeof(label), "%02zu | %s | %.2f s | %s | %zu layer(s)", i + 1, segment.name.c_str(), segment.duration, first_shape, segment.layers.size());
			if (ImGui::Selectable(label, selected_segment_ == static_cast<int>(i))) {
				select_segment(static_cast<int>(i));
			}
			ImGui::PopID();
		}
		if (script.segments.empty()) {
			ImGui::TextDisabled("No segments. Add one below or load a preset.");
		}
		ImGui::EndChild();

		const auto& names = Capture::shape_kind_names();
		int add_kind = static_cast<int>(new_segment_kind_);
		if (ImGui::Combo("New Segment Shape", &add_kind, names.data(), static_cast<int>(names.size()))) {
			new_segment_kind_ = static_cast<uint32_t>(add_kind);
		}
		drag_double("New Segment Duration (s)", new_segment_duration_, 0.05, 0.05, 86400.0, "%.2f");

		if (ImGui::Button("Add Segment")) {
			insert_segment(script, Capture::make_script_segment(static_cast<Capture::ShapeKind>(new_segment_kind_), new_segment_duration_));
		}
		render_setting_tooltip("Inserts a new segment after the selected one, or at the end when nothing is selected.");
		ImGui::SameLine();
		if (ImGui::Button("Add Stop")) {
			Capture::ScriptSegment stop = Capture::make_script_segment(Capture::ShapeKind::Hold, std::min(new_segment_duration_, 3600.0));
			stop.name = "Stop";
			stop.anchor = script.segments.empty() ? Capture::AnchorMode::World : Capture::AnchorMode::ContinuePrevious;
			insert_segment(script, std::move(stop));
		}
		render_setting_tooltip("Inserts a motionless segment that holds the camera at the end of the previous one.");
		ImGui::SameLine();
		if (ImGui::Button("Add Leg To Camera")) {
			const auto lookup = Capture::make_body_position_lookup(orchestrator);
			const Capture::Vec3 camera = orchestrator.camera().position;
			Capture::Vec3 start = camera;
			if (script.is_usable()) {
				const auto sample = script.sample(script.total_duration(), lookup);
				if (sample.valid) {
					start = sample.pose.position;
				}
			}
			Capture::ScriptSegment leg = Capture::make_script_segment(Capture::ShapeKind::Linear, new_segment_duration_);
			leg.name = "Leg To Camera";
			leg.layers[0].shape.controls[0] = start;
			leg.layers[0].shape.controls[1] = camera;
			insert_segment(script, std::move(leg));
		}
		render_setting_tooltip("Adds a straight segment from the current end of the script to the live camera position.");

		const bool has_selection = selected_segment_ >= 0 && selected_segment_ < static_cast<int>(script.segments.size());
		ImGui::BeginDisabled(!has_selection);
		if (ImGui::Button("Duplicate Segment")) {
			Capture::ScriptSegment copy = script.segments[static_cast<size_t>(selected_segment_)];
			copy.name += " Copy";
			insert_segment(script, std::move(copy));
		}
		ImGui::SameLine();
		if (ImGui::Button("Delete Segment")) {
			script.segments.erase(script.segments.begin() + selected_segment_);
			select_segment(std::min(selected_segment_, static_cast<int>(script.segments.size()) - 1));
			mark(true);
		}
		ImGui::SameLine();
		if (ImGui::Button("Move Up") && selected_segment_ > 0) {
			std::swap(script.segments[static_cast<size_t>(selected_segment_)], script.segments[static_cast<size_t>(selected_segment_ - 1)]);
			--selected_segment_;
			mark(true);
		}
		ImGui::SameLine();
		if (ImGui::Button("Move Down") && selected_segment_ + 1 < static_cast<int>(script.segments.size())) {
			std::swap(script.segments[static_cast<size_t>(selected_segment_)], script.segments[static_cast<size_t>(selected_segment_ + 1)]);
			++selected_segment_;
			mark(true);
		}
		ImGui::EndDisabled();
	}

	bool edit_event(Capture::ScriptEvent& event, size_t segment_count) {
		using namespace CaptureWidgets;
		using namespace MotionScriptEditorDetail;
		bool changed = false;
		changed |= input_text("Event Name", event.name);
		changed |= ImGui::Checkbox("Event Enabled", &event.enabled);
		changed |= enum_combo("Trigger", event.trigger, kTriggerNames);
		if (event.trigger == Capture::EventTrigger::ScriptTime) {
			changed |= drag_double("Time (s)", event.time_seconds, 0.02, 0.0, 86400.0, "%.3f");
		} else {
			changed |= slider_u32("Segment Index", event.segment, 0U, static_cast<uint32_t>(std::max<size_t>(segment_count, 1) - 1));
			if (event.trigger == Capture::EventTrigger::SegmentFraction) {
				changed |= drag_double("Segment Fraction", event.fraction, 0.005, 0.0, 1.0, "%.3f");
			}
		}
		changed |= enum_combo("Action", event.action, kActionNames);

		switch (event.action) {
			case Capture::EventAction::SetParameter: {
				int parameter_index = static_cast<int>(Capture::event_parameter_index(event.parameter));
				const auto& names = event_parameter_names();
				if (ImGui::Combo("Parameter", &parameter_index, names.data(), static_cast<int>(names.size()))) {
					event.parameter = static_cast<uint32_t>(Capture::kEventParameters[static_cast<size_t>(parameter_index)].type);
					changed = true;
				}
				break;
			}
			case Capture::EventAction::StepTicks:
				changed |= drag_double("Tick Count", event.value, 1.0, 0.0, 100000.0, "%.0f");
				break;
			case Capture::EventAction::SetMetric: {
				changed |= input_text("Metric Name", event.text);
				int pick = -1;
				if (ImGui::Combo("Quick Pick", &pick, kMetricNames.data(), static_cast<int>(kMetricNames.size()))) {
					event.text = kMetricNames[static_cast<size_t>(pick)];
					changed = true;
				}
				break;
			}
			case Capture::EventAction::SetIntegrator: {
				changed |= input_text("Integrator Name", event.text);
				int pick = -1;
				if (ImGui::Combo("Quick Pick", &pick, kIntegratorNames.data(), static_cast<int>(kIntegratorNames.size()))) {
					event.text = kIntegratorNames[static_cast<size_t>(pick)];
					changed = true;
				}
				break;
			}
			case Capture::EventAction::LoadScenario:
				changed |= input_text("Scenario Path", event.text);
				break;
			default:
				break;
		}

		if (Capture::is_ramp_action(event.action)) {
			changed |= drag_double_free("Value", event.value, 0.01, "%.5f");
			changed |= drag_double_free("Value At End Of Ramp", event.value_end, 0.01, "%.5f");
			changed |= drag_double("Ramp Duration (s)", event.duration, 0.02, 0.0, 86400.0, "%.3f");
			if (ImGui::TreeNode("Ramp Easing")) {
				changed |= edit_easing("EventEasing", event.easing);
				ImGui::TreePop();
			}
			ImGui::TextDisabled("A zero ramp duration applies the first value instantly.");
		}
		return changed;
	}

	void render_events(Capture::MotionScript& script) {
		using namespace CaptureWidgets;
		ImGui::TextColored(kHeaderColor, "Events (%zu)", script.events.size());
		ImGui::BeginChild("##EventList", ImVec2(0.0f, 110.0f), true);
		for (size_t i = 0; i < script.events.size(); ++i) {
			ImGui::PushID(static_cast<int>(i));
			bool enabled = script.events[i].enabled;
			if (ImGui::Checkbox("##event_enabled", &enabled)) {
				script.events[i].enabled = enabled;
				mark(true);
			}
			ImGui::SameLine();
			char label[224];
			std::snprintf(label, sizeof(label), "%02zu | %s | %s @ %.3f s", i + 1, script.events[i].display_label().c_str(), Capture::event_action_name(script.events[i].action), script.resolved_event_time(script.events[i]));
			if (ImGui::Selectable(label, selected_event_ == static_cast<int>(i))) {
				selected_event_ = static_cast<int>(i);
			}
			ImGui::PopID();
		}
		if (script.events.empty()) {
			ImGui::TextDisabled("No events. Events fire markers, stills, parameter ramps and simulation commands.");
		}
		ImGui::EndChild();

		if (ImGui::Button("Add Event At Cursor")) {
			Capture::ScriptEvent event;
			event.time_seconds = cursor_seconds_;
			event.name = "Event " + std::to_string(script.events.size() + 1);
			script.events.push_back(std::move(event));
			selected_event_ = static_cast<int>(script.events.size()) - 1;
			mark(true);
		}
		ImGui::SameLine();
		const bool has_event = selected_event_ >= 0 && selected_event_ < static_cast<int>(script.events.size());
		ImGui::BeginDisabled(!has_event);
		if (ImGui::Button("Duplicate Event")) {
			Capture::ScriptEvent copy = script.events[static_cast<size_t>(selected_event_)];
			copy.name += " Copy";
			script.events.push_back(std::move(copy));
			selected_event_ = static_cast<int>(script.events.size()) - 1;
			mark(true);
		}
		ImGui::SameLine();
		if (ImGui::Button("Delete Event")) {
			script.events.erase(script.events.begin() + selected_event_);
			selected_event_ = std::min(selected_event_, static_cast<int>(script.events.size()) - 1);
			mark(true);
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button("Sort By Time")) {
			std::stable_sort(script.events.begin(), script.events.end(), [&script](const Capture::ScriptEvent& a, const Capture::ScriptEvent& b) {
				return script.resolved_event_time(a) < script.resolved_event_time(b);
			});
			selected_event_ = -1;
			mark(true);
		}

		if (selected_event_ >= 0 && selected_event_ < static_cast<int>(script.events.size())) {
			ImGui::PushID("SelectedEvent");
			mark(edit_event(script.events[static_cast<size_t>(selected_event_)], script.segments.size()));
			ImGui::PopID();
		}
	}

	void render_script_header(Capture::MotionScript& script) {
		using namespace CaptureWidgets;
		using namespace MotionScriptEditorDetail;
		mark(input_text("Script Name", script.name));
		mark(enum_combo("End Behavior", script.end_behavior, kPathEndNames));
		if (ImGui::TreeNode("Global Time Easing")) {
			mark(edit_easing("GlobalEasing", script.global_easing));
			ImGui::TreePop();
		}
		ImGui::TextDisabled("Active segments: %zu | Events: %zu | Duration: %.3f s", script.active_segments().size(), script.events.size(), script.total_duration());

		drag_double("Time Scale Factor", time_scale_factor_, 0.01, 0.01, 100.0, "%.3f");
		ImGui::SameLine();
		if (ImGui::SmallButton("Scale")) {
			script.scale_time(time_scale_factor_);
			mark(true);
		}
		render_setting_tooltip("Multiplies every segment duration, ramp duration and timed event by the factor.");
		drag_double("Target Duration (s)", fit_duration_, 0.1, 0.1, 86400.0, "%.2f");
		ImGui::SameLine();
		if (ImGui::SmallButton("Fit")) {
			script.scale_time(fit_duration_ / script.total_duration());
			mark(true);
		}
		render_setting_tooltip("Rescales all timings so the whole script lasts the target duration.");

		ImGui::Combo("Preset", &preset_index_, Capture::kScriptPresetNames.data(), static_cast<int>(Capture::kScriptPresetCount));
		if (ImGui::Button("Replace With Preset")) {
			script = Capture::MotionScript::make_preset(static_cast<Capture::ScriptPreset>(preset_index_));
			select_segment(script.segments.empty() ? -1 : 0);
			selected_event_ = -1;
			mark(true);
		}
		ImGui::SameLine();
		if (ImGui::Button("Append Preset")) {
			Capture::MotionScript preset = Capture::MotionScript::make_preset(static_cast<Capture::ScriptPreset>(preset_index_));
			const bool had_segments = !script.segments.empty();
			bool first = true;
			for (auto& segment : preset.segments) {
				if (first && had_segments) {
					segment.anchor = Capture::AnchorMode::ContinuePrevious;
				}
				first = false;
				script.segments.push_back(std::move(segment));
			}
			mark(true);
		}
		ImGui::SameLine();
		if (ImGui::Button("New Empty Script")) {
			script = Capture::MotionScript{};
			select_segment(-1);
			selected_event_ = -1;
			mark(true);
		}
	}

	void render_script_files(Capture::MotionScript& script) {
		using namespace CaptureWidgets;
		if (!ImGui::CollapsingHeader("Script Files And Text")) {
			return;
		}
		input_text("Script File", file_path_);
		if (ImGui::Button("Save Script File")) {
			const std::string error = Capture::save_motion_script(file_path_, script);
			file_message_ = error.empty() ? ("Script saved to " + file_path_) : error;
		}
		ImGui::SameLine();
		if (ImGui::Button("Load Script File")) {
			const std::string error = Capture::load_motion_script(file_path_, script);
			if (error.empty()) {
				select_segment(script.segments.empty() ? -1 : 0);
				selected_event_ = -1;
				mark(true);
				file_message_ = "Script loaded from " + file_path_;
			} else {
				file_message_ = error;
			}
		}
		if (text_buffer_.empty()) {
			text_buffer_.assign(kTextBufferCapacity, '\0');
		}
		ImGui::InputTextMultiline("##ScriptText", text_buffer_.data(), text_buffer_.size(), ImVec2(-1.0f, 200.0f));
		if (ImGui::Button("Export To Text")) {
			const std::string text = Capture::motion_script_to_text(script);
			if (text.size() < text_buffer_.size()) {
				std::memcpy(text_buffer_.data(), text.c_str(), text.size() + 1U);
				file_message_ = "Script exported to the text field.";
			} else {
				file_message_ = "The script is too large for the text field, use the script file instead.";
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Apply Text")) {
			auto parsed = Capture::motion_script_from_text(text_buffer_.data());
			if (parsed.has_value()) {
				script = std::move(*parsed);
				select_segment(script.segments.empty() ? -1 : 0);
				selected_event_ = -1;
				mark(true);
				file_message_ = "Script imported from the text field.";
			} else {
				file_message_ = "The text does not contain a valid motion script.";
			}
		}
		if (!file_message_.empty()) {
			ImGui::TextDisabled("%s", file_message_.c_str());
		}
	}

public:
	void render(Capture::MotionScript& script, OrchestratorType& orchestrator, double cursor_seconds) {
		using namespace CaptureWidgets;
		cursor_seconds_ = cursor_seconds;
		refresh_bodies(orchestrator);
		selected_segment_ = std::clamp(selected_segment_, -1, static_cast<int>(script.segments.size()) - 1);
		selected_event_ = std::clamp(selected_event_, -1, static_cast<int>(script.events.size()) - 1);

		render_script_header(script);
		ImGui::Separator();
		ImGui::TextColored(kHeaderColor, "Timeline");
		render_timeline(script);
		render_segment_list(script, orchestrator);

		if (selected_segment_ >= 0 && selected_segment_ < static_cast<int>(script.segments.size())) {
			ImGui::Separator();
			ImGui::PushID("SelectedSegment");
			mark(edit_segment(script, script.segments[static_cast<size_t>(selected_segment_)], orchestrator));
			ImGui::PopID();
		}
		ImGui::Separator();
		render_events(script);
		ImGui::Separator();
		render_script_files(script);
		script.sanitize();
	}

	[[nodiscard]] bool consume_modified() noexcept {
		const bool value = modified_;
		modified_ = false;
		return value;
	}

	[[nodiscard]] std::optional<double> take_cursor_request() noexcept {
		const auto value = cursor_request_;
		cursor_request_.reset();
		return value;
	}

	[[nodiscard]] int32_t selected_slot(const Capture::MotionScript& script) const noexcept {
		if (selected_segment_ < 0 || selected_segment_ >= static_cast<int>(script.segments.size())) {
			return -1;
		}
		const auto& selected = script.segments[static_cast<size_t>(selected_segment_)];
		if (!selected.enabled || selected.duration <= 1.0e-9) {
			return -1;
		}
		int32_t slot = 0;
		for (int i = 0; i < selected_segment_; ++i) {
			const auto& segment = script.segments[static_cast<size_t>(i)];
			if (segment.enabled && segment.duration > 1.0e-9) {
				++slot;
			}
		}
		return slot;
	}
};

}
