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
#include <implot.h>
#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace Relativistic::UI {

namespace MotionScriptEditorDetail {

inline constexpr std::array<const char*, 4> kAnchorNames{"World (Absolute)", "Continue From Previous End", "Offset From Previous End", "Track Body"};
inline constexpr std::array<const char*, 3> kBlendNames{"Replace", "Add", "Add Relative To Layer Start"};
inline constexpr std::array<const char*, 8> kOrientationNames{
	"Free (Keep Current View)", "Fixed Angles", "Interpolated Angles", "Look At Target", "Along Travel Direction", "Pitch And Yaw Expressions", "Look At Moving Target", "Surface Walker (Upright On Body)"
};
inline constexpr std::array<const char*, 5> kWaveNames{"Sine", "Triangle", "Square", "Sawtooth", "Smooth Noise"};
inline constexpr std::array<const char*, 3> kPathEndNames{"Clamp At End", "Loop", "Ping-Pong"};
inline constexpr std::array<const char*, 4> kTriggerNames{"Script Time", "Segment Start", "Segment End", "Segment Fraction"};
inline constexpr std::array<const char*, 14> kActionNames{"Marker", "Capture Still", "Set Parameter", "Set Time Warp", "Pause Simulation", "Resume Simulation", "Step Ticks", "Set Metric", "Set Integrator", "Load Scenario", "Set Tick Rate", "Apply Performance Preset", "Set Rendering Overlay", "Set Resolution Scale"};
inline constexpr std::array<const char*, 6> kPerformancePresetNames{"Potato", "Performance", "Balanced", "Quality High", "Ultra Fidelity", "Scientific Extreme"};
inline constexpr std::array<const char*, 2> kTransitionModeNames{"Shortest Arc", "Direct Interpolation"};
inline constexpr std::array<const char*, 12> kDriverSourceNames{
	"Distance To Target", "Distance To Origin", "Position X", "Position Y", "Position Z", "Camera Speed",
	"Segment Progress", "Script Progress", "Segment Time", "Script Time", "Time Relative To Event", "Custom Expression"
};
inline constexpr std::array<const char*, 3> kDriverBlendNames{"Replace Channel Value", "Add To Channel Value", "Multiply Channel Value"};
inline constexpr std::array<const char*, 10> kMetricNames{
	"Flat Minkowski", "Schwarzschild Black Hole", "Kerr Rotating Black Hole", "Reissner-Nordstrom Charged", "Kerr-Newman Charged Rotating",
	"Schwarzschild-de Sitter (Lambda)", "FLRW Cosmological Expansion", "Morris-Thorne Traversable Wormhole", "Alcubierre Warp Drive Bubble", "BSSN 3+1 Numerical Grid"
};
inline constexpr std::array<const char*, 6> kIntegratorNames{
	"Dormand-Prince RK45 (Adaptive)", "Cash-Karp 5(4) (Adaptive)", "Vernier 9(8) High-Order", "Symplectic Gauss-Legendre 4th", "Symplectic Gauss-Legendre 6th", "Hermite 4th-Order (Aarseth)"
};

inline constexpr const char* kShapeVariableHelp = "Variables: t local seconds, u progress, d duration, g global seconds, a b c k shape parameters.";
inline constexpr const char* kSignalVariableHelp = "Variables: t segment seconds, u segment progress, d segment duration, g script seconds, r distance to origin, x y z camera position, v camera speed, p script progress, s segment index, a b channel start and end, m driver measurement.";

inline constexpr std::array<Capture::ShapeKind, 4> kBasicShapes{Capture::ShapeKind::Hold, Capture::ShapeKind::Linear, Capture::ShapeKind::QuadraticBezier, Capture::ShapeKind::CubicBezier};
inline constexpr std::array<Capture::ShapeKind, 3> kWaypointShapes{Capture::ShapeKind::Spline, Capture::ShapeKind::Polyline, Capture::ShapeKind::BSpline};
inline constexpr std::array<Capture::ShapeKind, 5> kRotationShapes{Capture::ShapeKind::Arc, Capture::ShapeKind::Helix, Capture::ShapeKind::Orbit, Capture::ShapeKind::LogarithmicSpiral, Capture::ShapeKind::ArchimedeanSpiral};
inline constexpr std::array<Capture::ShapeKind, 6> kCurveShapes{Capture::ShapeKind::Lissajous, Capture::ShapeKind::TorusKnot, Capture::ShapeKind::Lemniscate, Capture::ShapeKind::Rose, Capture::ShapeKind::Epitrochoid, Capture::ShapeKind::Wave};
inline constexpr std::array<Capture::ShapeKind, 3> kEquationShapes{Capture::ShapeKind::ExpressionCartesian, Capture::ShapeKind::ExpressionCylindrical, Capture::ShapeKind::ExpressionSpherical};
inline constexpr std::array<Capture::ShapeKind, 3> kSurfaceShapes{Capture::ShapeKind::SurfaceWalk, Capture::ShapeKind::SurfaceLoop, Capture::ShapeKind::SurfaceRoute};

struct ShapeGroup {
	const char* name;
	std::span<const Capture::ShapeKind> kinds;
};

inline constexpr std::array<ShapeGroup, 6> kShapeGroups{{
	{"Static And Straight", kBasicShapes},
	{"Through Waypoints", kWaypointShapes},
	{"Rotations And Orbits", kRotationShapes},
	{"Closed And Periodic Curves", kCurveShapes},
	{"Equations", kEquationShapes},
	{"Surface Walking", kSurfaceShapes}
}};

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

[[nodiscard]] inline const std::vector<const char*>& event_overlay_names() {
	static const std::vector<const char*> names = [] {
		std::vector<const char*> result;
		result.reserve(Capture::kEventOverlays.size());
		for (const auto& entry : Capture::kEventOverlays) {
			result.push_back(entry.name);
		}
		return result;
	}();
	return names;
}

inline void apply_driver_source_defaults(Capture::DriverSpec& driver) noexcept {
	using Source = Capture::DriverSource;
	switch (driver.source) {
		case Source::DistanceToTarget:
		case Source::DistanceToOrigin: driver.input_min = 5.0; driver.input_max = 80.0; break;
		case Source::PositionX:
		case Source::PositionY:
		case Source::PositionZ: driver.input_min = -80.0; driver.input_max = 80.0; break;
		case Source::Speed: driver.input_min = 0.0; driver.input_max = 20.0; break;
		case Source::SegmentProgress:
		case Source::ScriptProgress: driver.input_min = 0.0; driver.input_max = 1.0; break;
		case Source::SegmentTime: driver.input_min = 0.0; driver.input_max = 5.0; break;
		case Source::ScriptTime: driver.input_min = 0.0; driver.input_max = 10.0; break;
		case Source::EventTimeOffset: driver.input_min = -2.0; driver.input_max = 2.0; break;
		case Source::Expression: driver.input_min = 0.0; driver.input_max = 1.0; break;
	}
}

[[nodiscard]] inline const char* driver_source_hint(Capture::DriverSource source) noexcept {
	using Source = Capture::DriverSource;
	switch (source) {
		case Source::DistanceToTarget: return "Distance between the camera and the chosen target, in world units.";
		case Source::DistanceToOrigin: return "Distance between the camera and the world origin, in world units.";
		case Source::PositionX: return "Camera X coordinate.";
		case Source::PositionY: return "Camera Y coordinate.";
		case Source::PositionZ: return "Camera Z coordinate.";
		case Source::Speed: return "Camera speed along the path, in world units per second.";
		case Source::SegmentProgress: return "Eased progress inside the current segment, from 0 to 1.";
		case Source::ScriptProgress: return "Progress through the whole script, from 0 to 1.";
		case Source::SegmentTime: return "Seconds elapsed since the current segment started.";
		case Source::ScriptTime: return "Seconds elapsed since the script started.";
		case Source::EventTimeOffset: return "Script time minus the chosen event time: negative before the event, positive after it.";
		case Source::Expression: return "Any expression built from the signal variables.";
	}
	return "";
}

[[nodiscard]] inline const char* orientation_mode_hint(Capture::OrientationMode mode) noexcept {
	using Mode = Capture::OrientationMode;
	switch (mode) {
		case Mode::Free: return "The view direction is left untouched; only the adjustments below apply.";
		case Mode::Fixed: return "The camera keeps a constant pitch and yaw during the whole segment.";
		case Mode::Interpolated: return "Pitch and yaw are interpolated from a start to an end value.";
		case Mode::LookAtTarget: return "The camera always faces a fixed point or a simulation body.";
		case Mode::AlongTravel: return "The camera looks in the direction it is moving.";
		case Mode::Expression: return "Pitch and yaw in degrees are computed by expressions every frame.";
		case Mode::TargetPath: return "The camera faces a target that itself moves along a scripted trajectory.";
		case Mode::SurfaceWalker: return "The camera stays upright relative to the chosen body and faces the walking direction, as in the live Surface Walk mode.";
	}
	return "";
}

template <typename T>
inline void move_element(std::vector<T>& values, size_t from, size_t to) {
	if (from == to || from >= values.size() || to >= values.size()) {
		return;
	}
	if (from < to) {
		std::rotate(values.begin() + static_cast<ptrdiff_t>(from), values.begin() + static_cast<ptrdiff_t>(from) + 1, values.begin() + static_cast<ptrdiff_t>(to) + 1);
	} else {
		std::rotate(values.begin() + static_cast<ptrdiff_t>(to), values.begin() + static_cast<ptrdiff_t>(from), values.begin() + static_cast<ptrdiff_t>(from) + 1);
	}
}

[[nodiscard]] inline bool shape_kind_picker(const char* label, Capture::ShapeKind& kind) {
	bool changed = false;
	if (ImGui::BeginCombo(label, Capture::shape_descriptor(kind).name)) {
		for (const auto& group : kShapeGroups) {
			ImGui::SeparatorText(group.name);
			for (const Capture::ShapeKind candidate : group.kinds) {
				const bool selected = candidate == kind;
				if (ImGui::Selectable(Capture::shape_descriptor(candidate).name, selected)) {
					kind = candidate;
					changed = true;
				}
				if (selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
		}
		ImGui::EndCombo();
	}
	return changed;
}

}

class MotionScriptEditor {
public:
	using OrchestratorType = Orchestrator::SimulationOrchestrator<1024>;

private:
	enum class Pane : int {
		None = -1,
		Path = 0,
		Events = 1,
		Curves = 2,
		Script = 3
	};

	enum class ListAction : int {
		None = 0,
		Duplicate,
		Delete,
		MoveUp,
		MoveDown,
		Toggle,
		InsertStop
	};

	struct BodyChoice {
		int32_t id{0};
		std::string label{};
		double radius{1.0};
		Capture::Vec3 position{0.0, 0.0, 0.0};
		Capture::Vec3 axes{1.0, 1.0, 1.0};
	};

	struct CurveSet {
		std::vector<double> time{};
		std::vector<double> distance{};
		std::vector<double> speed{};
		std::vector<double> x{};
		std::vector<double> y{};
		std::vector<double> z{};
		std::vector<double> pitch{};
		std::vector<double> yaw{};
		std::vector<double> roll{};
		std::vector<double> fov{};
		std::vector<double> exposure{};
		std::vector<double> rate{};
		std::vector<double> acceleration{};
		std::vector<double> angular_rate{};
		std::vector<double> path_length{};
		std::vector<double> nearest_distance{};
		std::vector<double> shake_displacement{};
		std::vector<double> shake_rotation{};
		std::vector<double> transition_weight{};
		std::vector<double> segment_marks{};
		std::vector<double> event_marks{};
	};

	struct CurveSeries {
		const char* name;
		const std::vector<double>* values;
	};

	static constexpr size_t kTextBufferCapacity = 1U << 19U;
	static constexpr size_t kCurveSamples = 512;

	int selected_segment_{-1};
	int32_t surface_walk_body_{Capture::kOriginReference};
	int selected_layer_{0};
	int selected_event_{-1};
	float list_pane_width_{300.0f};
	double new_segment_duration_{5.0};
	int preset_index_{0};
	double time_scale_factor_{1.0};
	double fit_duration_{30.0};
	double cursor_seconds_{0.0};
	bool modified_{false};
	bool curves_dirty_{true};
	Pane pane_request_{Pane::None};
	std::optional<double> cursor_request_{};
	std::string file_message_{};
	std::string file_path_{"config/motion_script.cfg"};
	std::vector<char> text_buffer_{};
	std::vector<BodyChoice> bodies_{};
	CurveSet curves_{};
	std::array<bool, 7> curve_groups_{true, true, true, true, true, true, true};

	bool mark(bool value) noexcept {
		if (value) {
			modified_ = true;
			curves_dirty_ = true;
		}
		return value;
	}

	[[nodiscard]] ImGuiTabItemFlags tab_flags(Pane pane) const noexcept {
		return (pane_request_ == pane) ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
	}

	void refresh_bodies(OrchestratorType& orchestrator) {
		bodies_.clear();
		std::lock_guard<std::recursive_mutex> lock(orchestrator.nbody_system().bodies_mutex());
		for (const auto& body : orchestrator.nbody_system().bodies()) {
			std::string label = "Body #" + std::to_string(body.id);
			if (body.has_name()) {
				label += " (" + std::string(body.name_view()) + ")";
			}
			bodies_.push_back(BodyChoice{static_cast<int32_t>(body.id), std::move(label), body.effective_radius(), body.position, Observer::SurfaceGeometry::body_semi_axes(body)});
		}
	}

	bool edit_body_reference(const char* label, int32_t& id, bool allow_nearest = false) const {
		std::vector<const char*> items;
		items.reserve(bodies_.size() + 2);
		items.push_back("World Origin / Fixed Point");
		if (allow_nearest) {
			items.push_back("Nearest Body To Camera");
		}
		const size_t first_body = items.size();
		int current = (allow_nearest && id == Capture::kNearestBodyReference) ? 1 : 0;
		for (size_t i = 0; i < bodies_.size(); ++i) {
			items.push_back(bodies_[i].label.c_str());
			if (bodies_[i].id == id) {
				current = static_cast<int>(first_body + i);
			}
		}
		if (ImGui::Combo(label, &current, items.data(), static_cast<int>(items.size()))) {
			const size_t picked = static_cast<size_t>(current);
			if (picked == 0) {
				id = Capture::kOriginReference;
			} else if (allow_nearest && picked == 1) {
				id = Capture::kNearestBodyReference;
			} else {
				id = bodies_[picked - first_body].id;
			}
			return true;
		}
		return false;
	}

	[[nodiscard]] static std::string event_label(const Capture::MotionScript& script, size_t index) {
		char buffer[192];
		std::snprintf(buffer, sizeof(buffer), "%02zu  %s  (%.2f s)", index + 1, script.events[index].display_label().c_str(), script.resolved_event_time(script.events[index]));
		return buffer;
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

	[[nodiscard]] Capture::ScriptSegment make_surface_segment(OrchestratorType& orchestrator, Capture::ShapeKind kind) const {
		namespace Geometry = Observer::SurfaceGeometry;
		Capture::Vec3 center{0.0, 0.0, 0.0};
		double radius = std::max(2.0 * orchestrator.parameters().mass, 1.0);
		Capture::Vec3 axes{radius, radius, radius};
		const bool on_body = surface_walk_body_ >= 0;
		if (on_body) {
			for (const BodyChoice& choice : bodies_) {
				if (choice.id == surface_walk_body_) {
					center = choice.position;
					axes = choice.axes;
					radius = std::max(choice.axes[0], 1e-6);
					break;
				}
			}
		}
		const auto& camera = orchestrator.camera();
		const Capture::Vec3 direction = Geometry::normalized(Geometry::subtract(camera.position, center), {1.0, 0.0, 0.0});
		const double latitude = std::asin(std::clamp(direction[2], -1.0, 1.0));
		const double longitude = std::atan2(direction[1], direction[0]);
		const Capture::Vec3 north{-std::sin(latitude) * std::cos(longitude), -std::sin(latitude) * std::sin(longitude), std::cos(latitude)};
		const Capture::Vec3 east{-std::sin(longitude), std::cos(longitude), 0.0};
		const Capture::Vec3 forward = camera.orientation_basis().forward;
		const double heading = std::atan2(Geometry::dot(forward, east), Geometry::dot(forward, north)) * Capture::PathDetail::kRadToDeg;

		Capture::ScriptSegment segment = Capture::make_script_segment(kind, std::max(new_segment_duration_, 1.0));
		const double latitude_deg = latitude * Capture::PathDetail::kRadToDeg;
		const double longitude_deg = longitude * Capture::PathDetail::kRadToDeg;
		const double eye_height = radius * 0.02;
		auto& shape = segment.layers.front().shape;
		shape.controls[0] = {0.0, 0.0, 0.0};
		shape.controls[1] = {0.0, std::max(segment.duration * 1.6, 1.0), 1.0};
		switch (kind) {
			case Capture::ShapeKind::SurfaceLoop:
				segment.name = "Surface Loop";
				shape.controls[1][1] = 0.0;
				shape.values = {latitude_deg, longitude_deg, 30.0, 1.0, 0.0, radius, eye_height, 1.0};
				break;
			case Capture::ShapeKind::SurfaceRoute:
				segment.name = "Surface Route";
				shape.controls[1][1] = 0.0;
				shape.values = {latitude_deg, longitude_deg, (latitude_deg > 0.0) ? latitude_deg - 40.0 : latitude_deg + 40.0, std::remainder(longitude_deg + 90.0, 360.0), 0.0, radius, eye_height, 1.0};
				break;
			case Capture::ShapeKind::SurfaceWalk:
			default:
				segment.name = "Surface Walk";
				shape.values = {latitude_deg, longitude_deg, heading, heading, radius * 0.5, radius, eye_height, 1.0};
				break;
		}
		shape.fit_surface_axes(axes, eye_height);
		if (on_body) {
			segment.anchor = Capture::AnchorMode::TrackBody;
			segment.anchor_body = surface_walk_body_;
		} else {
			segment.anchor = Capture::AnchorMode::World;
			segment.anchor_offset = center;
		}
		segment.orientation.mode = Capture::OrientationMode::SurfaceWalker;
		segment.orientation.target_body = surface_walk_body_;
		segment.orientation.look_ahead = 0.03;
		return segment;
	}

	void apply_segment_action(Capture::MotionScript& script, ListAction action, int index) {
		if (index < 0 || index >= static_cast<int>(script.segments.size())) {
			return;
		}
		const size_t i = static_cast<size_t>(index);
		switch (action) {
			case ListAction::Duplicate: {
				Capture::ScriptSegment copy = script.segments[i];
				copy.name += " Copy";
				script.segments.insert(script.segments.begin() + index + 1, std::move(copy));
				select_segment(index + 1);
				break;
			}
			case ListAction::Delete:
				script.segments.erase(script.segments.begin() + index);
				select_segment(std::min(index, static_cast<int>(script.segments.size()) - 1));
				break;
			case ListAction::MoveUp:
				if (index > 0) {
					MotionScriptEditorDetail::move_element(script.segments, i, i - 1);
					select_segment(index - 1);
				}
				break;
			case ListAction::MoveDown:
				if (index + 1 < static_cast<int>(script.segments.size())) {
					MotionScriptEditorDetail::move_element(script.segments, i, i + 1);
					select_segment(index + 1);
				}
				break;
			case ListAction::Toggle:
				script.segments[i].enabled = !script.segments[i].enabled;
				break;
			case ListAction::InsertStop: {
				Capture::ScriptSegment stop = Capture::make_script_segment(Capture::ShapeKind::Hold, std::min(new_segment_duration_, 3600.0));
				stop.name = "Stop";
				stop.anchor = Capture::AnchorMode::ContinuePrevious;
				script.segments.insert(script.segments.begin() + index + 1, std::move(stop));
				select_segment(index + 1);
				break;
			}
			case ListAction::None:
			default:
				return;
		}
		mark(true);
	}

	void rebuild_curves(const Capture::MotionScript& script, OrchestratorType& orchestrator) {
		curves_ = CurveSet{};
		curves_dirty_ = false;
		if (!script.is_usable()) {
			return;
		}
		const auto lookup = Capture::make_body_position_lookup(orchestrator);
		const double total = script.total_duration();
		curves_.time.reserve(kCurveSamples + 1);
		Capture::Vec3 previous{0.0, 0.0, 0.0};
		Capture::CameraPose previous_pose{};
		double previous_time = 0.0;
		double previous_speed = 0.0;
		double travelled = 0.0;
		bool has_previous = false;
		for (size_t i = 0; i <= kCurveSamples; ++i) {
			const double t = std::min(total * static_cast<double>(i) / static_cast<double>(kCurveSamples), total - 1.0e-9);
			const auto sample = script.sample(std::max(t, 0.0), lookup);
			if (!sample.valid) {
				continue;
			}
			const auto& pose = sample.pose;
			curves_.time.push_back(t);
			curves_.x.push_back(pose.position[0]);
			curves_.y.push_back(pose.position[1]);
			curves_.z.push_back(pose.position[2]);
			curves_.distance.push_back(Capture::ScriptMath::length(pose.position));
			curves_.pitch.push_back(pose.pitch_deg);
			curves_.yaw.push_back(pose.yaw_deg);
			curves_.roll.push_back(pose.roll_deg);
			curves_.fov.push_back(pose.fov_deg);
			curves_.exposure.push_back(pose.exposure_ev);
			curves_.rate.push_back(sample.simulation_rate);
			double speed = 0.0;
			double acceleration = 0.0;
			double angular_rate = 0.0;
			if (has_previous && t - previous_time > 1.0e-9) {
				const double dt = t - previous_time;
				const double step = Capture::ScriptMath::length(Capture::ScriptMath::sub(pose.position, previous));
				travelled += step;
				speed = step / dt;
				acceleration = (speed - previous_speed) / dt;
				const double delta_pitch = pose.pitch_deg - previous_pose.pitch_deg;
				const double delta_yaw = std::remainder(pose.yaw_deg - previous_pose.yaw_deg, 360.0);
				const double delta_roll = std::remainder(pose.roll_deg - previous_pose.roll_deg, 360.0);
				angular_rate = std::sqrt(delta_pitch * delta_pitch + delta_yaw * delta_yaw + delta_roll * delta_roll) / dt;
			}
			curves_.speed.push_back(speed);
			curves_.acceleration.push_back(acceleration);
			curves_.angular_rate.push_back(angular_rate);
			curves_.path_length.push_back(travelled);
			curves_.nearest_distance.push_back(sample.nearest_distance >= 0.0 ? sample.nearest_distance : 0.0);
			curves_.shake_displacement.push_back(Capture::ScriptMath::length(sample.shake_position));
			curves_.shake_rotation.push_back(Capture::ScriptMath::length(sample.shake_rotation));
			curves_.transition_weight.push_back(sample.transition_weight);
			previous = pose.position;
			previous_pose = pose;
			previous_speed = speed;
			previous_time = t;
			has_previous = true;
		}
		if (curves_.speed.size() > 1) {
			curves_.speed[0] = curves_.speed[1];
			curves_.angular_rate[0] = curves_.angular_rate[1];
		}
		double accumulated = 0.0;
		const auto active = script.active_segments();
		for (size_t slot = 0; slot + 1 < active.size(); ++slot) {
			accumulated += script.segments[active[slot]].duration;
			curves_.segment_marks.push_back(accumulated);
		}
		for (const auto& event : script.events) {
			if (event.enabled) {
				curves_.event_marks.push_back(script.resolved_event_time(event));
			}
		}
	}

	void curve_plot(const char* title, const char* y_label, std::initializer_list<CurveSeries> series, double total) {
		if (!ImPlot::BeginPlot(title)) {
			return;
		}
		ImPlot::SetupAxes("Script Time (s)", y_label, ImPlotAxisFlags_None, ImPlotAxisFlags_AutoFit);
		ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, total, ImPlotCond_Once);
		ImPlot::SetupLegend(ImPlotLocation_NorthEast);
		for (const CurveSeries& entry : series) {
			ImPlot::PlotLine(entry.name, curves_.time.data(), entry.values->data(), static_cast<int>(std::min(curves_.time.size(), entry.values->size())));
		}
		if (!curves_.segment_marks.empty()) {
			ImPlot::PlotInfLines("Segment Boundaries", curves_.segment_marks.data(), static_cast<int>(curves_.segment_marks.size()));
		}
		if (!curves_.event_marks.empty()) {
			ImPlot::PlotInfLines("Events", curves_.event_marks.data(), static_cast<int>(curves_.event_marks.size()));
		}
		double cursor = cursor_seconds_;
		if (ImPlot::DragLineX(0, &cursor, ImVec4(1.0f, 0.96f, 0.55f, 1.0f), 1.5f)) {
			cursor_request_ = std::clamp(cursor, 0.0, total);
		}
		ImPlot::EndPlot();
	}

	void render_curves(const Capture::MotionScript& script, OrchestratorType& orchestrator) {
		using namespace CaptureWidgets;
		if (curves_dirty_) {
			rebuild_curves(script, orchestrator);
		}
		if (ImGui::Button("Refresh Curves")) {
			curves_dirty_ = true;
		}
		render_setting_tooltip("Recomputes and redraws all trajectory and parameter curves.");
		ImGui::SameLine();
		help_marker("Shows every evaluated camera quantity over the script time, including the effect of drivers, expressions and modulation. Drag the yellow line to move the preview cursor.");
		if (curves_.time.size() < 2) {
			ImGui::TextDisabled("Enable at least one segment to display the curves.");
			return;
		}
		const double total = script.total_duration();
		static constexpr std::array<const char*, 7> group_names{"Position", "Motion", "Travel", "View Direction", "Angular Rate", "Lens", "Modifiers"};
		FlowLayout group_flow;
		int visible_rows = 0;
		for (size_t i = 0; i < group_names.size(); ++i) {
			group_flow.next(FlowLayout::checkbox_width(group_names[i]));
			ImGui::PushID(static_cast<int>(i));
			ImGui::Checkbox(group_names[i], &curve_groups_[i]);
			ImGui::PopID();
			visible_rows += curve_groups_[i] ? 1 : 0;
		}
		render_setting_tooltip("Chooses which curve groups are plotted. Modifiers shows the simulation rate, the camera shake displacement and the transition blend weight.");
		if (visible_rows == 0) {
			ImGui::TextDisabled("Enable at least one curve group.");
			return;
		}
		const float height = std::max(ImGui::GetContentRegionAvail().y, 190.0f * static_cast<float>(visible_rows));
		if (ImPlot::BeginSubplots("##ScriptCurves", visible_rows, 1, ImVec2(-1.0f, height), ImPlotSubplotFlags_LinkAllX)) {
			if (curve_groups_[0]) curve_plot("Position", "World Units", {{"Distance To Origin", &curves_.distance}, {"X", &curves_.x}, {"Y", &curves_.y}, {"Z", &curves_.z}}, total);
			if (curve_groups_[1]) curve_plot("Motion", "Units Per Second / Second Squared", {{"Camera Speed", &curves_.speed}, {"Tangential Acceleration", &curves_.acceleration}}, total);
			if (curve_groups_[2]) curve_plot("Travel", "World Units (0 = No Body)", {{"Path Length", &curves_.path_length}, {"Distance To Nearest Body", &curves_.nearest_distance}}, total);
			if (curve_groups_[3]) curve_plot("View Direction", "Degrees", {{"Pitch", &curves_.pitch}, {"Yaw", &curves_.yaw}, {"Roll", &curves_.roll}}, total);
			if (curve_groups_[4]) curve_plot("Angular Rate", "Degrees Per Second / Degrees", {{"View Angular Rate", &curves_.angular_rate}, {"Shake Rotation", &curves_.shake_rotation}}, total);
			if (curve_groups_[5]) curve_plot("Lens", "Degrees / EV", {{"Field Of View", &curves_.fov}, {"Exposure", &curves_.exposure}}, total);
			if (curve_groups_[6]) curve_plot("Modifiers", "Multiplier / World Units / Weight", {{"Simulation Rate", &curves_.rate}, {"Shake Displacement", &curves_.shake_displacement}, {"Transition Weight", &curves_.transition_weight}}, total);
			ImPlot::EndSubplots();
		}
	}

	void render_summary(const Capture::MotionScript& script) {
		using namespace CaptureWidgets;
		using namespace MotionScriptEditorDetail;
		ImGui::TextColored(kHeaderColor, "%s", script.name.c_str());
		ImGui::SameLine();
		ImGui::TextDisabled("%zu active segment(s)  |  %zu event(s)  |  %.3f s  |  %s", script.active_segments().size(), script.events.size(), script.total_duration(), kPathEndNames[std::min(static_cast<size_t>(script.end_behavior), kPathEndNames.size() - 1)]);
	}

	void render_timeline(Capture::MotionScript& script) {
		using namespace CaptureWidgets;
		constexpr float ruler_height = 18.0f;
		constexpr float event_lane = 16.0f;
		constexpr float track_height = 40.0f;
		constexpr float height = ruler_height + event_lane + track_height;
		const float width = std::max(ImGui::GetContentRegionAvail().x, 160.0f);
		const ImVec2 origin = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton("##ScriptTimeline", ImVec2(width, height));
		const bool hovered = ImGui::IsItemHovered();
		const bool active = ImGui::IsItemActive();
		ImDrawList* draw = ImGui::GetWindowDrawList();
		const double total = script.total_duration();
		const auto time_to_x = [&](double seconds) noexcept {
			return origin.x + static_cast<float>(std::clamp(seconds / total, 0.0, 1.0)) * width;
		};
		const float track_top = origin.y + ruler_height + event_lane;

		draw->AddRectFilled(origin, ImVec2(origin.x + width, origin.y + height), IM_COL32(18, 20, 30, 255), 4.0f);
		draw->AddRectFilled(origin, ImVec2(origin.x + width, origin.y + ruler_height), IM_COL32(28, 32, 46, 255), 4.0f, ImDrawFlags_RoundCornersTop);

		static constexpr std::array<double, 15> steps{0.1, 0.25, 0.5, 1.0, 2.0, 5.0, 10.0, 15.0, 30.0, 60.0, 120.0, 300.0, 600.0, 1800.0, 3600.0};
		double step = steps.back();
		for (const double candidate : steps) {
			if (total / candidate <= static_cast<double>(width) / 64.0) {
				step = candidate;
				break;
			}
		}
		for (double t = 0.0; t <= total + 1.0e-9; t += step) {
			const float x = time_to_x(t);
			draw->AddLine(ImVec2(x, origin.y + ruler_height - 6.0f), ImVec2(x, origin.y + ruler_height), IM_COL32(150, 160, 185, 255));
			char label[24];
			std::snprintf(label, sizeof(label), step < 1.0 ? "%.2f s" : "%.0f s", t);
			draw->AddText(ImVec2(x + 3.0f, origin.y + 2.0f), IM_COL32(170, 180, 205, 255), label);
		}

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
			const ImVec2 top_left(x0 + 1.0f, track_top + 2.0f);
			const ImVec2 bottom_right(std::max(x1 - 1.0f, x0 + 2.0f), origin.y + height - 3.0f);
			draw->AddRectFilled(top_left, bottom_right, IM_COL32(rgb[0], rgb[1], rgb[2], selected ? 235 : 150), 3.0f);
			if (selected) {
				draw->AddRect(top_left, bottom_right, IM_COL32(255, 255, 255, 255), 3.0f, 0, 2.0f);
			}
			if (segment.transition.enabled && slot > 0) {
				const float transition_end = std::min(time_to_x(accumulated + segment.transition.reach_seconds()), bottom_right.x);
				draw->AddRectFilled(ImVec2(top_left.x, bottom_right.y - 6.0f), ImVec2(transition_end, bottom_right.y), IM_COL32(255, 255, 255, 190), 2.0f);
			}
			if (segment.shake.enabled) {
				for (float zig = top_left.x + 2.0f; zig + 3.0f < bottom_right.x; zig += 6.0f) {
					draw->AddLine(ImVec2(zig, top_left.y + 20.0f), ImVec2(zig + 3.0f, top_left.y + 26.0f), IM_COL32(10, 12, 18, 200), 1.2f);
					draw->AddLine(ImVec2(zig + 3.0f, top_left.y + 26.0f), ImVec2(zig + 6.0f, top_left.y + 20.0f), IM_COL32(10, 12, 18, 200), 1.2f);
				}
			}
			if (bottom_right.x - top_left.x > 36.0f) {
				draw->PushClipRect(top_left, bottom_right, true);
				draw->AddText(ImVec2(top_left.x + 4.0f, top_left.y + 4.0f), IM_COL32(10, 12, 18, 255), segment.name.c_str());
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
			const float y = origin.y + ruler_height + event_lane * 0.5f;
			const ImU32 color = (selected_event_ == static_cast<int>(i)) ? IM_COL32(255, 255, 255, 255) : IM_COL32(255, 220, 70, 235);
			draw->AddQuadFilled(ImVec2(x, y - 6.0f), ImVec2(x + 6.0f, y), ImVec2(x, y + 6.0f), ImVec2(x - 6.0f, y), color);
		}

		const float cursor_x = time_to_x(cursor_seconds_);
		draw->AddLine(ImVec2(cursor_x, origin.y), ImVec2(cursor_x, origin.y + height), IM_COL32(255, 245, 140, 255), 2.0f);
		draw->AddTriangleFilled(ImVec2(cursor_x - 5.0f, origin.y), ImVec2(cursor_x + 5.0f, origin.y), ImVec2(cursor_x, origin.y + 8.0f), IM_COL32(255, 245, 140, 255));

		const ImVec2 mouse = ImGui::GetIO().MousePos;
		const double hover_time = std::clamp(static_cast<double>((mouse.x - origin.x) / width), 0.0, 1.0) * total;
		if (hovered) {
			const int hovered_segment = segment_index_at(script, hover_time);
			if (hovered_segment >= 0) {
				ImGui::SetTooltip("%s | %.3f s", script.segments[static_cast<size_t>(hovered_segment)].name.c_str(), hover_time);
			}
		}
		const auto pick_event = [&]() {
			int event_hit = -1;
			float best_distance = 8.0f;
			if (mouse.y >= origin.y + ruler_height && mouse.y < track_top) {
				for (size_t i = 0; i < script.events.size(); ++i) {
					const float distance = std::abs(time_to_x(script.resolved_event_time(script.events[i])) - mouse.x);
					if (distance < best_distance) {
						best_distance = distance;
						event_hit = static_cast<int>(i);
					}
				}
			}
			return event_hit;
		};
		if (ImGui::IsItemClicked()) {
			const int event_hit = pick_event();
			if (event_hit >= 0) {
				selected_event_ = event_hit;
			} else {
				const int segment_hit = segment_index_at(script, hover_time);
				if (segment_hit >= 0) {
					select_segment(segment_hit);
				}
			}
		}
		if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
			pane_request_ = (pick_event() >= 0) ? Pane::Events : Pane::Path;
		}
		if (active) {
			cursor_request_ = hover_time;
		}
	}

	bool edit_modulation(Capture::ModulationSpec& modulation) {
		using namespace CaptureWidgets;
		using namespace MotionScriptEditorDetail;
		bool changed = ImGui::Checkbox("Enable Periodic Modulation", &modulation.enabled);
		render_setting_tooltip("Enables periodic oscillation (sine, triangle, square, sawtooth, noise) on this animated channel.");
		ImGui::BeginDisabled(!modulation.enabled);
		if (begin_property_grid("##ModulationGrid")) {
			changed |= property_enum("Waveform", modulation.wave, kWaveNames, "Shape of the periodic oscillation added to the value.");
			changed |= property_drag_free("Amplitude At Start", modulation.amplitude, 0.01, "%.4f", "Oscillation amplitude at the start of the segment.");
			changed |= property_drag_free("Amplitude At End", modulation.amplitude_end, 0.01, "%.4f", "Oscillation amplitude at the end of the segment.");
			changed |= property_drag("Frequency (Hz)", modulation.frequency, 0.005, 0.0, 1000.0, "%.4f", "Number of oscillations per second.");
			changed |= property_drag("Phase (deg)", modulation.phase_deg, 0.5, -360.0, 360.0, "%.1f", "Starting phase of the oscillation.");
			changed |= property_drag("Edge Fade", modulation.fade, 0.005, 0.0, 0.5, "%.3f", "Fraction of the segment used to fade the oscillation in and out.");
			end_property_grid();
		}
		ImGui::EndDisabled();
		return changed;
	}

	bool edit_driver(Capture::DriverSpec& driver, double channel_start, double channel_end, const char* unit, const Capture::MotionScript& script) {
		using namespace CaptureWidgets;
		using namespace MotionScriptEditorDetail;
		bool changed = false;
		bool enabled = driver.enabled;
		if (ImGui::Checkbox("Drive This Value From A Live Signal", &enabled)) {
			if (enabled && !driver.enabled && driver.output_min == 0.0 && driver.output_max == 1.0) {
				driver.output_min = channel_start;
				driver.output_max = channel_end;
			}
			driver.enabled = enabled;
			changed = true;
		}
		render_setting_tooltip("Maps a measured quantity, such as the distance to a body, onto this value in real time.");
		ImGui::BeginDisabled(!driver.enabled);
		if (begin_property_grid("##DriverGrid")) {
			if (property_enum("Signal", driver.source, kDriverSourceNames, "The measured quantity that controls the value.")) {
				apply_driver_source_defaults(driver);
				changed = true;
			}
			property_info("", driver_source_hint(driver.source));
			switch (driver.source) {
				case Capture::DriverSource::DistanceToTarget:
					changed |= property_row("Target Body", "Body or fixed point the distance is measured to. Nearest Body To Camera measures the distance to the closest enabled body.", [&] { return edit_body_reference("##value", driver.body, true); });
					changed |= property_vec3("Target Offset", driver.point, 0.1, "%.3f", "Offset from the body, or the absolute point when no body is selected.");
					break;
				case Capture::DriverSource::EventTimeOffset:
					if (script.events.empty()) {
						property_info("Event", "The script has no events. Add one in the Events tab.");
					} else {
						driver.event = std::min<uint32_t>(driver.event, static_cast<uint32_t>(script.events.size() - 1));
						changed |= property_row("Event", "Event the time offset is measured from.", [&] {
							bool picked = false;
							const std::string preview = event_label(script, driver.event);
							if (ImGui::BeginCombo("##value", preview.c_str())) {
								for (size_t i = 0; i < script.events.size(); ++i) {
									const std::string label = event_label(script, i);
									if (ImGui::Selectable(label.c_str(), driver.event == i)) {
										driver.event = static_cast<uint32_t>(i);
										picked = true;
									}
								}
								ImGui::EndCombo();
							}
							return picked;
						});
					}
					break;
				case Capture::DriverSource::Expression:
					changed |= property_expression("Signal Expression", driver.expression, "Expression evaluated every frame to produce the measured value.");
					break;
				default:
					break;
			}
			changed |= property_drag_free("Input At Minimum", driver.input_min, 0.05, "%.4f", "Measured value that produces the minimum output.");
			changed |= property_drag_free("Input At Maximum", driver.input_max, 0.05, "%.4f", "Measured value that produces the maximum output.");
			changed |= property_check("Clamp Input", driver.clamp_input, "When disabled the output keeps extrapolating outside the input range.");
			changed |= property_drag_free("Output At Minimum", driver.output_min, 0.05, "%.4f", "Value produced when the input is at its minimum.");
			changed |= property_drag_free("Output At Maximum", driver.output_max, 0.05, "%.4f", "Value produced when the input is at its maximum.");
			changed |= property_enum("Combine With Animation", driver.blend, kDriverBlendNames, "How the driven value is combined with the animated value of the channel.");
			end_property_grid();
		}
		if (ImGui::TreeNode("Response Curve")) {
			changed |= edit_easing("DriverResponse", driver.response);
			ImGui::TreePop();
		}
		ImGui::TextDisabled("Input %.2f to %.2f maps to %.2f to %.2f %s", driver.input_min, driver.input_max, driver.output_min, driver.output_max, unit);
		ImGui::EndDisabled();
		return changed;
	}

	[[nodiscard]] static std::string channel_summary(const Capture::ScalarChannel& channel, const char* unit) {
		using namespace MotionScriptEditorDetail;
		if (!channel.enabled) {
			return "Inactive";
		}
		char buffer[160];
		if (channel.driver.enabled) {
			std::snprintf(buffer, sizeof(buffer), "Driven by %s", kDriverSourceNames[std::min(static_cast<size_t>(channel.driver.source), kDriverSourceNames.size() - 1)]);
		} else {
			std::snprintf(buffer, sizeof(buffer), "%.2f to %.2f %s", channel.start, channel.end, unit);
		}
		std::string text = buffer;
		if (channel.use_expression) {
			text += " + Expression";
		}
		if (channel.modulation.enabled) {
			text += " + Modulation";
		}
		return text;
	}

	bool edit_channel(const char* label, Capture::ScalarChannel& channel, const char* unit, const Capture::MotionScript& script) {
		using namespace CaptureWidgets;
		using namespace MotionScriptEditorDetail;
		bool changed = false;
		ImGui::PushID(label);
		const std::string header = std::string(label) + "  -  " + channel_summary(channel, unit) + "###channel";
		if (ImGui::CollapsingHeader(header.c_str(), channel.enabled ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_None)) {
			changed |= ImGui::Checkbox("Animate This Value", &channel.enabled);
			render_setting_tooltip("When disabled the value is left to the live camera and simulation state.");
			ImGui::BeginDisabled(!channel.enabled);
			if (begin_property_grid("##ChannelGrid")) {
				changed |= property_drag_free("Start Value", channel.start, 0.05, "%.4f", "Value at the start of the segment.");
				changed |= property_drag_free("End Value", channel.end, 0.05, "%.4f", "Value at the end of the segment.");
				end_property_grid();
			}
			if (ImGui::TreeNode("Easing Between Start And End")) {
				changed |= edit_easing("ChannelEasing", channel.easing);
				ImGui::TreePop();
			}
			ImGui::SeparatorText("Reactive Driver");
			changed |= edit_driver(channel.driver, channel.start, channel.end, unit, script);
			ImGui::SeparatorText("Expression Override");
			changed |= ImGui::Checkbox("Use Expression", &channel.use_expression);
			render_setting_tooltip("Overrides or calculates the animated channel value using a mathematical expression over signal variables.");
			if (channel.use_expression) {
				if (begin_property_grid("##ChannelExpressionGrid")) {
					changed |= property_expression("Value Expression", channel.expression, "Computes the animated value from the signal variables.");
					end_property_grid();
				}
				ImGui::TextDisabled("%s", kSignalVariableHelp);
			}
			if (ImGui::TreeNode("Periodic Modulation")) {
				changed |= edit_modulation(channel.modulation);
				ImGui::TreePop();
			}
			ImGui::EndDisabled();
		}
		ImGui::PopID();
		return changed;
	}

	bool edit_shape(Capture::ShapeSpec& shape, OrchestratorType& orchestrator) {
		using namespace CaptureWidgets;
		using namespace MotionScriptEditorDetail;
		bool changed = false;
		const auto& descriptor = Capture::shape_descriptor(shape.kind);

		if (begin_property_grid("##ShapeGrid")) {
			Capture::ShapeKind picked = shape.kind;
			if (property_row("Shape", "Mathematical family used to generate the trajectory.", [&] { return shape_kind_picker("##value", picked); })) {
				shape.reset(picked);
				changed = true;
			}
			for (size_t i = 0; i < descriptor.control_labels.size(); ++i) {
				if (descriptor.control_labels[i] == nullptr) {
					continue;
				}
				bool copy_camera = false;
				const bool position = control_is_position(shape.kind, i);
				changed |= property_vec3(descriptor.control_labels[i], shape.controls[i], 0.1, "%.3f", nullptr, position ? "Camera" : nullptr, &copy_camera);
				if (copy_camera) {
					shape.controls[i] = orchestrator.camera().position;
					changed = true;
				}
			}
			for (size_t i = 0; i < descriptor.value_labels.size(); ++i) {
				if (descriptor.value_labels[i] == nullptr) {
					continue;
				}
				changed |= property_drag_free(descriptor.value_labels[i], shape.values[i], 0.05, "%.4f");
			}
			end_property_grid();
		}
		if (ImGui::SmallButton("Reset Shape To Defaults")) {
			shape.reset(shape.kind);
			changed = true;
		}
		render_setting_tooltip("Resets all control points and shape parameters to their default geometric configuration.");

		if (Capture::is_surface_shape(shape.kind)) {
			ImGui::SeparatorText("Surface Fit");
			const auto frame = shape.surface_frame();
			ImGui::TextDisabled("Surface axes: %.4g x %.4g x %.4g | Mean radius: %.4g", frame.axes[0], frame.axes[1], frame.axes[2], frame.mean_radius);
			if (ImGui::SmallButton("Fit To Body Selected Next To Add Surface Buttons")) {
				for (const BodyChoice& choice : bodies_) {
					if (choice.id == surface_walk_body_) {
						shape.fit_surface_axes(choice.axes, frame.eye_height);
						changed = true;
						break;
					}
				}
			}
			render_setting_tooltip("Copies the radius, polar flattening and equatorial ratio of the chosen body into this path, so it follows the deformed surface of that body.");
		}

		if (descriptor.waypoints) {
			ImGui::SeparatorText("Waypoints");
			ImGui::Text("%zu waypoint(s)", shape.waypoints.size());
			ImGui::SameLine();
			changed |= ImGui::Checkbox("Closed Loop", &shape.closed);
			render_setting_tooltip("Connects the final waypoint back to the first waypoint to form a continuous closed trajectory.");
			if (shape.kind != Capture::ShapeKind::BSpline) {
				ImGui::SameLine();
				changed |= ImGui::Checkbox("Uniform Speed", &shape.uniform_speed);
				render_setting_tooltip("Normalizes camera travel speed along the spline regardless of uneven waypoint spacing.");
			}
			int remove_index = -1;
			int duplicate_index = -1;
			if (ImGui::BeginTable("##Waypoints", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp, ImVec2(0.0f, 190.0f))) {
				ImGui::TableSetupScrollFreeze(0, 1);
				ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 30.0f);
				ImGui::TableSetupColumn("Position", ImGuiTableColumnFlags_WidthStretch, 1.0f);
				ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 118.0f);
				ImGui::TableHeadersRow();
				ImGuiListClipper clipper;
				clipper.Begin(static_cast<int>(shape.waypoints.size()));
				while (clipper.Step()) {
					for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
						ImGui::PushID(i);
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::AlignTextToFramePadding();
						ImGui::Text("%d", i + 1);
						ImGui::TableNextColumn();
						ImGui::SetNextItemWidth(-FLT_MIN);
						changed |= drag_vec3("##position", shape.waypoints[static_cast<size_t>(i)], 0.1, "%.2f");
						ImGui::TableNextColumn();
						if (ImGui::SmallButton("Camera")) {
							shape.waypoints[static_cast<size_t>(i)] = orchestrator.camera().position;
							changed = true;
						}
						render_setting_tooltip("Snaps this waypoint coordinates to the current interactive camera position.");
						ImGui::SameLine();
						if (ImGui::SmallButton("Copy")) {
							duplicate_index = i;
						}
						render_setting_tooltip("Duplicates this waypoint.");
						ImGui::SameLine();
						if (ImGui::SmallButton("Del")) {
							remove_index = i;
						}
						render_setting_tooltip("Deletes this waypoint.");
						ImGui::PopID();
					}
				}
				ImGui::EndTable();
			}
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
			render_setting_tooltip("Appends a new waypoint at the current interactive camera position.");
			ImGui::SameLine();
			if (ImGui::SmallButton("Add Blank")) {
				shape.waypoints.push_back(shape.waypoints.empty() ? Capture::Vec3{0.0, 0.0, 0.0} : shape.waypoints.back());
				changed = true;
			}
			render_setting_tooltip("Appends a duplicate of the last waypoint or world origin.");
			ImGui::SameLine();
			if (ImGui::SmallButton("Reverse")) {
				std::reverse(shape.waypoints.begin(), shape.waypoints.end());
				changed = true;
			}
			render_setting_tooltip("Reverses the order of all waypoints in the list.");
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
			render_setting_tooltip("Inserts interpolated midpoints between consecutive waypoints for higher path detail.");
			ImGui::SameLine();
			if (ImGui::SmallButton("Clear")) {
				shape.waypoints.clear();
				changed = true;
			}
			render_setting_tooltip("Removes all waypoints from this shape.");
		}

		if (descriptor.expressions) {
			ImGui::SeparatorText("Equations");
			const auto labels = Capture::shape_expression_labels(shape.kind);
			if (begin_property_grid("##EquationGrid")) {
				for (size_t i = 0; i < shape.expressions.size(); ++i) {
					changed |= property_expression(labels[i], shape.expressions[i]);
				}
				end_property_grid();
			}
			ImGui::TextDisabled("%s", kShapeVariableHelp);
			const auto presets = expression_presets(shape.kind);
			int pick = -1;
			std::vector<const char*> preset_names;
			preset_names.reserve(presets.size());
			for (const auto& preset : presets) {
				preset_names.push_back(preset.name);
			}
			if (ImGui::Combo("Load Equation Preset", &pick, preset_names.data(), static_cast<int>(preset_names.size()))) {
				for (size_t i = 0; i < shape.expressions.size(); ++i) {
					shape.expressions[i].assign(presets[static_cast<size_t>(pick)].sources[i]);
				}
				changed = true;
			}
			render_setting_tooltip("Loads a predefined set of mathematical parametric coordinate equations.");
		}

		if (ImGui::TreeNode("Shape Transform")) {
			if (begin_property_grid("##TransformGrid")) {
				changed |= property_vec3("Scale", shape.scale, 0.01, "%.3f", "Per-axis scale applied around the pivot.");
				changed |= property_vec3("Rotation (Roll, Pitch, Yaw deg)", shape.rotation_deg, 0.25, "%.2f", "Euler rotation applied around the pivot.");
				changed |= property_vec3("Pivot", shape.pivot, 0.1, "%.3f", "Point the scale and rotation are applied around.");
				changed |= property_vec3("Translation", shape.translation, 0.1, "%.3f", "Offset applied after scale and rotation.");
				end_property_grid();
			}
			if (ImGui::SmallButton("Reset Transform")) {
				shape.scale = {1.0, 1.0, 1.0};
				shape.rotation_deg = {0.0, 0.0, 0.0};
				shape.pivot = {0.0, 0.0, 0.0};
				shape.translation = {0.0, 0.0, 0.0};
				changed = true;
			}
			render_setting_tooltip("Resets scale, rotation, pivot, and translation to neutral default transforms.");
			ImGui::TreePop();
		}
		return changed;
	}

	bool edit_layer(Capture::ShapeLayer& layer, OrchestratorType& orchestrator) {
		using namespace CaptureWidgets;
		using namespace MotionScriptEditorDetail;
		bool changed = false;
		ImGui::SeparatorText("Layer Settings");
		if (begin_property_grid("##LayerGrid")) {
			changed |= property_text("Name", layer.name);
			changed |= property_check("Enabled", layer.enabled);
			changed |= property_enum("Blend Mode", layer.blend, kBlendNames, "Replace overrides the stack, Add sums absolute positions, Add Relative sums only the displacement from the layer start, which suits wobbles and offsets over a base path.");
			changed |= property_drag("Weight At Start", layer.weight_start, 0.01, -50.0, 50.0, "%.3f", "Layer strength at the start of the segment.");
			changed |= property_drag("Weight At End", layer.weight_end, 0.01, -50.0, 50.0, "%.3f", "Layer strength at the end of the segment.");
			changed |= property_drag("Active Window Start", layer.window_start, 0.005, 0.0, 1.0, "%.3f", "Segment progress where the layer starts playing.");
			changed |= property_drag("Active Window End", layer.window_end, 0.005, 0.0, 1.0, "%.3f", "Segment progress where the layer finishes playing.");
			end_property_grid();
		}
		if (ImGui::TreeNode("Layer Time Easing")) {
			changed |= edit_easing("LayerEasing", layer.easing);
			ImGui::TreePop();
		}
		ImGui::SeparatorText("Layer Shape");
		ImGui::PushID("LayerShape");
		changed |= edit_shape(layer.shape, orchestrator);
		ImGui::PopID();
		return changed;
	}

	bool render_movement_pane(Capture::ScriptSegment& segment, OrchestratorType& orchestrator) {
		using namespace CaptureWidgets;
		using namespace MotionScriptEditorDetail;
		bool changed = false;
		selected_layer_ = std::clamp(selected_layer_, 0, std::max(static_cast<int>(segment.layers.size()) - 1, 0));
		ImGui::TextColored(kHeaderColor, "Movement Layers (%zu)", segment.layers.size());
		ImGui::SameLine();
		help_marker("Layers are combined from top to bottom to build the camera position. Use Add Relative layers to put a wobble or a spiral on top of a base path.");

		if (ImGui::BeginTable("##LayerTable", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp, ImVec2(0.0f, 120.0f))) {
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableSetupColumn("On", ImGuiTableColumnFlags_WidthFixed, 26.0f);
			ImGui::TableSetupColumn("Layer", ImGuiTableColumnFlags_WidthStretch, 1.0f);
			ImGui::TableSetupColumn("Blend", ImGuiTableColumnFlags_WidthStretch, 0.8f);
			ImGui::TableSetupColumn("Shape", ImGuiTableColumnFlags_WidthStretch, 0.8f);
			ImGui::TableHeadersRow();
			for (size_t i = 0; i < segment.layers.size(); ++i) {
				ImGui::PushID(static_cast<int>(i));
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				bool enabled = segment.layers[i].enabled;
				if (ImGui::Checkbox("##layer_enabled", &enabled)) {
					segment.layers[i].enabled = enabled;
					changed = true;
				}
				render_setting_tooltip("Enables or disables this movement layer in the position blend stack.");
				ImGui::TableNextColumn();
				char label[160];
				std::snprintf(label, sizeof(label), "%02zu  %s", i + 1, segment.layers[i].name.c_str());
				if (ImGui::Selectable(label, selected_layer_ == static_cast<int>(i))) {
					selected_layer_ = static_cast<int>(i);
				}
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(kBlendNames[static_cast<size_t>(segment.layers[i].blend)]);
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(Capture::shape_descriptor(segment.layers[i].shape.kind).name);
				ImGui::PopID();
			}
			ImGui::EndTable();
		}

		if (ImGui::Button("Add Layer...")) {
			ImGui::OpenPopup("##AddLayerPopup");
		}
		render_setting_tooltip("Opens a menu to add a new positional movement layer to this segment.");
		if (ImGui::BeginPopup("##AddLayerPopup")) {
			for (const auto& group : kShapeGroups) {
				if (ImGui::BeginMenu(group.name)) {
					for (const Capture::ShapeKind kind : group.kinds) {
						if (ImGui::MenuItem(Capture::shape_descriptor(kind).name)) {
							Capture::ShapeLayer layer;
							layer.name = Capture::shape_descriptor(kind).name;
							layer.blend = segment.layers.empty() ? Capture::LayerBlend::Replace : Capture::LayerBlend::AddRelative;
							layer.shape = Capture::ShapeSpec::make(kind);
							segment.layers.push_back(std::move(layer));
							selected_layer_ = static_cast<int>(segment.layers.size()) - 1;
							changed = true;
						}
					}
					ImGui::EndMenu();
				}
			}
			ImGui::EndPopup();
		}
		ImGui::BeginDisabled(segment.layers.empty());
		ImGui::SameLine();
		if (ImGui::Button("Duplicate")) {
			Capture::ShapeLayer copy = segment.layers[static_cast<size_t>(selected_layer_)];
			copy.name += " Copy";
			segment.layers.insert(segment.layers.begin() + selected_layer_ + 1, std::move(copy));
			++selected_layer_;
			changed = true;
		}
		render_setting_tooltip("Duplicates the currently selected movement layer.");
		ImGui::SameLine();
		if (ImGui::Button("Delete")) {
			segment.layers.erase(segment.layers.begin() + selected_layer_);
			selected_layer_ = std::max(selected_layer_ - 1, 0);
			changed = true;
		}
		render_setting_tooltip("Removes the currently selected movement layer.");
		ImGui::SameLine();
		if (ImGui::Button("Up") && selected_layer_ > 0) {
			std::swap(segment.layers[static_cast<size_t>(selected_layer_)], segment.layers[static_cast<size_t>(selected_layer_ - 1)]);
			--selected_layer_;
			changed = true;
		}
		render_setting_tooltip("Moves the selected layer earlier in the layer evaluation stack.");
		ImGui::SameLine();
		if (ImGui::Button("Down") && selected_layer_ + 1 < static_cast<int>(segment.layers.size())) {
			std::swap(segment.layers[static_cast<size_t>(selected_layer_)], segment.layers[static_cast<size_t>(selected_layer_ + 1)]);
			++selected_layer_;
			changed = true;
		}
		render_setting_tooltip("Moves the selected layer later in the layer evaluation stack.");
		ImGui::EndDisabled();

		if (!segment.layers.empty()) {
			ImGui::PushID("SelectedLayer");
			changed |= edit_layer(segment.layers[static_cast<size_t>(selected_layer_)], orchestrator);
			ImGui::PopID();
		}
		return changed;
	}

	bool render_look_pane(Capture::ScriptSegment& segment, OrchestratorType& orchestrator, const Capture::MotionScript& script) {
		using namespace CaptureWidgets;
		using namespace MotionScriptEditorDetail;
		bool changed = false;
		auto& orientation = segment.orientation;

		ImGui::SeparatorText("View Direction Source");
		if (begin_property_grid("##LookModeGrid")) {
			changed |= property_enum("Mode", orientation.mode, kOrientationNames, "Chooses how the viewing direction is produced during this segment.");
			end_property_grid();
		}
		ImGui::TextDisabled("%s", orientation_mode_hint(orientation.mode));

		switch (orientation.mode) {
			case Capture::OrientationMode::Fixed:
				if (begin_property_grid("##LookFixedGrid")) {
					changed |= property_drag("Pitch (deg)", orientation.start[0], 0.25, -89.0, 89.0, "%.2f");
					changed |= property_drag("Yaw (deg)", orientation.start[1], 0.25, -36000.0, 36000.0, "%.2f");
					end_property_grid();
				}
				break;
			case Capture::OrientationMode::Interpolated:
				if (begin_property_grid("##LookInterpolatedGrid")) {
					changed |= property_drag("Pitch At Start (deg)", orientation.start[0], 0.25, -89.0, 89.0, "%.2f");
					changed |= property_drag("Yaw At Start (deg)", orientation.start[1], 0.25, -36000.0, 36000.0, "%.2f");
					changed |= property_drag("Pitch At End (deg)", orientation.end[0], 0.25, -89.0, 89.0, "%.2f");
					changed |= property_drag("Yaw At End (deg)", orientation.end[1], 0.25, -36000.0, 36000.0, "%.2f");
					end_property_grid();
				}
				if (ImGui::TreeNode("Orientation Easing")) {
					changed |= edit_easing("OrientationEasing", orientation.easing);
					ImGui::TreePop();
				}
				break;
			case Capture::OrientationMode::LookAtTarget:
				if (begin_property_grid("##LookTargetGrid")) {
					changed |= property_row("Target Body", "Body or fixed point the camera faces. Nearest Body To Camera re-evaluates the closest enabled body every frame and switches instantly when another body becomes closer; use a view blend duration in the segment transition to soften segment changes.", [&] { return edit_body_reference("##value", orientation.target_body, true); });
					changed |= property_vec3("Target Offset", orientation.target_offset, 0.1, "%.3f", "Offset from the body, or the absolute point when no body is selected.");
					end_property_grid();
				}
				break;
			case Capture::OrientationMode::AlongTravel:
				if (begin_property_grid("##LookTravelGrid")) {
					changed |= property_drag("Look-Ahead (Progress)", orientation.look_ahead, 0.001, 0.001, 0.5, "%.4f", "How far ahead along the path the camera looks, as a fraction of the segment.");
					end_property_grid();
				}
				break;
			case Capture::OrientationMode::Expression:
				if (begin_property_grid("##LookExpressionGrid")) {
					changed |= property_expression("Pitch Expression (deg)", orientation.expressions[0], "Pitch angle in degrees.");
					changed |= property_expression("Yaw Expression (deg)", orientation.expressions[1], "Yaw angle in degrees.");
					end_property_grid();
				}
				ImGui::TextDisabled("%s", kSignalVariableHelp);
				break;
			case Capture::OrientationMode::TargetPath:
				if (begin_property_grid("##LookMovingTargetGrid")) {
					changed |= property_row("Anchor Body", "Body the target trajectory is relative to, the world origin, or the body nearest to the camera.", [&] { return edit_body_reference("##value", orientation.target_body, true); });
					changed |= property_vec3("Extra Offset", orientation.target_offset, 0.1, "%.3f", "Constant offset added to the target trajectory.");
					end_property_grid();
				}
				if (ImGui::TreeNode("Target Time Easing")) {
					changed |= edit_easing("TargetEasing", orientation.easing);
					ImGui::TreePop();
				}
				ImGui::SeparatorText("Target Trajectory");
				ImGui::PushID("TargetShape");
				changed |= edit_shape(orientation.target_path, orchestrator);
				ImGui::PopID();
				break;
			case Capture::OrientationMode::SurfaceWalker:
				if (begin_property_grid("##LookSurfaceWalkerGrid")) {
					changed |= property_row("Walked Body", "Body whose surface defines the local vertical. The world origin uses the primary source.", [&] { return edit_body_reference("##value", orientation.target_body); });
					changed |= property_vec3("Center Offset", orientation.target_offset, 0.1, "%.3f", "Offset added to the body center when it matches a shape center offset.");
					changed |= property_drag("Look-Ahead (Progress)", orientation.look_ahead, 0.001, 0.001, 0.5, "%.4f", "How far ahead along the path the facing direction is sampled, as a fraction of the segment.");
					end_property_grid();
				}
				break;
			case Capture::OrientationMode::Free:
			default:
				break;
		}

		ImGui::SeparatorText("Fine Adjustment");
		if (begin_property_grid("##LookOffsetGrid")) {
			changed |= property_drag("Constant Pitch Offset (deg)", orientation.pitch_offset, 0.1, -180.0, 180.0, "%.2f", "Added to the computed pitch.");
			changed |= property_drag("Constant Yaw Offset (deg)", orientation.yaw_offset, 0.1, -360.0, 360.0, "%.2f", "Added to the computed yaw.");
			end_property_grid();
		}
		changed |= edit_channel("Pitch Modifier (deg)", orientation.pitch_modifier, "deg", script);
		changed |= edit_channel("Yaw Modifier (deg)", orientation.yaw_modifier, "deg", script);
		return changed;
	}

	bool render_lens_pane(Capture::ScriptSegment& segment, const Capture::MotionScript& script) {
		bool changed = false;
		ImGui::TextDisabled("Each value can be animated, driven by a live signal, computed by an expression and modulated.");
		changed |= edit_channel("Field Of View", segment.fov, "deg", script);
		changed |= edit_channel("Exposure", segment.exposure, "EV", script);
		changed |= edit_channel("Roll", segment.roll, "deg", script);
		changed |= edit_channel("Simulation Rate Multiplier", segment.warp, "x", script);
		return changed;
	}

	bool render_shake_pane(Capture::ShakeSpec& shake) {
		using namespace CaptureWidgets;
		bool changed = ImGui::Checkbox("Enable Camera Shake", &shake.enabled);
		render_setting_tooltip("Applies procedural pseudo-random camera shake perturbations to position and rotation.");
		ImGui::BeginDisabled(!shake.enabled);
		if (begin_property_grid("##ShakeGrid")) {
			changed |= property_vec3("Position Amplitude", shake.position_amplitude, 0.01, "%.3f", "Maximum positional displacement per axis.");
			changed |= property_vec3("Rotation Amplitude (deg)", shake.rotation_amplitude, 0.01, "%.3f", "Maximum angular displacement per axis.");
			changed |= property_drag("Frequency (Hz)", shake.frequency, 0.01, 0.0, 100.0, "%.3f", "Speed of the shake.");
			changed |= property_u32("Seed", shake.seed, 0U, 9999U, 0, "Different seeds produce different shake patterns.");
			changed |= property_drag("Edge Fade", shake.fade, 0.005, 0.0, 0.5, "%.3f", "Fraction of the segment used to fade the shake in and out.");
			end_property_grid();
		}
		ImGui::EndDisabled();
		return changed;
	}

	bool render_timing_pane(Capture::MotionScript& script, Capture::ScriptSegment& segment) {
		using namespace CaptureWidgets;
		using namespace MotionScriptEditorDetail;
		bool changed = false;
		ImGui::SeparatorText("Segment");
		if (begin_property_grid("##TimingGrid")) {
			changed |= property_text("Name", segment.name);
			changed |= property_check("Enabled", segment.enabled, "Disabled segments are skipped and take no time.");
			changed |= property_drag("Duration (s)", segment.duration, 0.05, 0.01, 86400.0, "%.3f", "Length of this segment in seconds.");
			end_property_grid();
		}
		if (ImGui::TreeNode("Segment Time Easing")) {
			changed |= edit_easing("SegmentEasing", segment.time_easing);
			ImGui::TreePop();
		}
		ImGui::SeparatorText("Anchor");
		if (begin_property_grid("##AnchorGrid")) {
			changed |= property_enum("Anchor Mode", segment.anchor, kAnchorNames, "Defines what the segment positions are relative to.");
			if (segment.anchor == Capture::AnchorMode::TrackBody) {
				changed |= property_row("Anchor Body", "Body the segment follows.", [&] { return edit_body_reference("##value", segment.anchor_body); });
			}
			changed |= property_vec3("Anchor Offset", segment.anchor_offset, 0.1, "%.3f", "Offset added to the anchor position.");
			end_property_grid();
		}
		ImGui::SeparatorText("Transition From Previous Segment");
		const auto active_segments = script.active_segments();
		const bool is_first_active = !active_segments.empty() && &script.segments[active_segments.front()] == &segment;
		if (is_first_active) {
			ImGui::TextDisabled("This is the first active segment, so there is no previous segment to blend from.");
		}
		auto& transition = segment.transition;
		changed |= ImGui::Checkbox("Blend Into This Segment", &transition.enabled);
		render_setting_tooltip("Smoothly blends the camera pose from the end of the previous segment into this one instead of cutting. Position, view direction and lens values can use separate durations and easing curves.");
		ImGui::BeginDisabled(!transition.enabled);
		if (begin_property_grid("##TransitionGrid")) {
			changed |= property_drag("View Blend Duration (s)", transition.orientation_seconds, 0.02, 0.0, segment.duration, "%.3f", "Time during which pitch, yaw and roll glide from the previous segment's final view. 0 cuts immediately.");
			changed |= property_enum("View Blend Path", transition.orientation_mode, kTransitionModeNames, "Shortest Arc turns the short way around; Direct Interpolation follows the raw angle difference, which allows deliberate multi-turn spins.");
			changed |= property_drag("Position Blend Duration (s)", transition.position_seconds, 0.02, 0.0, segment.duration, "%.3f", "Time during which the camera position glides from the previous segment's end to this segment's path. Useful when segments are not anchored to each other.");
			changed |= property_drag("Lens Blend Duration (s)", transition.lens_seconds, 0.02, 0.0, segment.duration, "%.3f", "Time during which field of view, exposure and the simulation rate multiplier glide from the previous values.");
			changed |= property_check("Blend Simulation Rate", transition.blend_simulation_rate, "Includes the simulation rate multiplier in the lens blend.");
			end_property_grid();
		}
		if (ImGui::SmallButton("Match All To View Duration")) {
			transition.position_seconds = transition.orientation_seconds;
			transition.lens_seconds = transition.orientation_seconds;
			changed = true;
		}
		render_setting_tooltip("Sets position and lens blend durations to match the view blend duration.");
		ImGui::SameLine();
		if (ImGui::SmallButton("Quarter Of Segment")) {
			transition.orientation_seconds = segment.duration * 0.25;
			transition.position_seconds = transition.orientation_seconds;
			transition.lens_seconds = transition.orientation_seconds;
			changed = true;
		}
		render_setting_tooltip("Sets all transition blend durations to exactly 25% of this segment's duration.");
		if (ImGui::TreeNode("View Blend Easing")) {
			changed |= edit_easing("TransitionViewEasing", transition.orientation_easing);
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Position Blend Easing")) {
			changed |= edit_easing("TransitionPositionEasing", transition.position_easing);
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Lens Blend Easing")) {
			changed |= edit_easing("TransitionLensEasing", transition.lens_easing);
			ImGui::TreePop();
		}
		ImGui::EndDisabled();
		static_cast<void>(script);
		return changed;
	}

	void render_segment_inspector(Capture::MotionScript& script, size_t index, OrchestratorType& orchestrator) {
		using namespace CaptureWidgets;
		Capture::ScriptSegment& segment = script.segments[index];
		const double start = script.segment_start_time(index);
		ImGui::TextColored(kHeaderColor, "%02zu  %s", index + 1, segment.name.c_str());
		ImGui::SameLine();
		if (segment.enabled) {
			ImGui::TextDisabled("%.3f s to %.3f s", start, start + segment.duration);
		} else {
			ImGui::TextColored(kWarningColor, "Disabled");
		}
		bool changed = false;
		if (ImGui::BeginTabBar("##SegmentInspectorTabs")) {
			if (ImGui::BeginTabItem("Timing")) {
				changed |= render_timing_pane(script, segment);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Movement")) {
				changed |= render_movement_pane(segment, orchestrator);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Look Direction")) {
				changed |= render_look_pane(segment, orchestrator, script);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Lens")) {
				changed |= render_lens_pane(segment, script);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Shake")) {
				changed |= render_shake_pane(segment.shake);
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
		mark(changed);
	}

	void render_path_tab(Capture::MotionScript& script, OrchestratorType& orchestrator) {
		using namespace CaptureWidgets;
		using namespace MotionScriptEditorDetail;
		const float available_height = std::max(ImGui::GetContentRegionAvail().y, 350.0f);
		float left_w = std::clamp(list_pane_width_, 180.0f, std::max(ImGui::GetContentRegionAvail().x - 250.0f, 180.0f));

		ImGui::BeginChild("##SegmentListPane", ImVec2(left_w, available_height), true);
		ImGui::TextColored(kHeaderColor, "Segments (%zu)", script.segments.size());

		const float table_height = std::max(available_height - 110.0f, 100.0f);
		if (ImGui::BeginTable("##SegmentTable", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp, ImVec2(0.0f, table_height))) {
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableSetupColumn("On", ImGuiTableColumnFlags_WidthFixed, 26.0f);
			ImGui::TableSetupColumn("Segment", ImGuiTableColumnFlags_WidthStretch, 1.0f);
			ImGui::TableSetupColumn("Dur", ImGuiTableColumnFlags_WidthFixed, 54.0f);
			ImGui::TableHeadersRow();

			for (size_t i = 0; i < script.segments.size(); ++i) {
				ImGui::PushID(static_cast<int>(i));
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				bool enabled = script.segments[i].enabled;
				if (ImGui::Checkbox("##segment_enabled", &enabled)) {
					script.segments[i].enabled = enabled;
					mark(true);
				}
				render_setting_tooltip("Enables or disables this segment in the script playback.");
				ImGui::TableNextColumn();
				char label[192];
				const char* first_shape = script.segments[i].layers.empty() ? "Empty" : Capture::shape_descriptor(script.segments[i].layers.front().shape.kind).name;
				std::snprintf(label, sizeof(label), "%02zu  %s  (%s)", i + 1, script.segments[i].name.c_str(), first_shape);
				if (ImGui::Selectable(label, selected_segment_ == static_cast<int>(i), ImGuiSelectableFlags_SpanAllColumns)) {
					select_segment(static_cast<int>(i));
				}
				if (ImGui::BeginPopupContextItem()) {
					if (ImGui::MenuItem("Duplicate")) apply_segment_action(script, ListAction::Duplicate, static_cast<int>(i));
					if (ImGui::MenuItem("Delete")) apply_segment_action(script, ListAction::Delete, static_cast<int>(i));
					ImGui::Separator();
					if (ImGui::MenuItem("Move Up", nullptr, false, i > 0)) apply_segment_action(script, ListAction::MoveUp, static_cast<int>(i));
					if (ImGui::MenuItem("Move Down", nullptr, false, i + 1 < script.segments.size())) apply_segment_action(script, ListAction::MoveDown, static_cast<int>(i));
					ImGui::Separator();
					if (ImGui::MenuItem("Insert Stop After")) apply_segment_action(script, ListAction::InsertStop, static_cast<int>(i));
					if (ImGui::MenuItem(script.segments[i].enabled ? "Disable" : "Enable")) apply_segment_action(script, ListAction::Toggle, static_cast<int>(i));
					ImGui::EndPopup();
				}
				ImGui::TableNextColumn();
				ImGui::Text("%.2fs", script.segments[i].duration);
				ImGui::PopID();
			}
			ImGui::EndTable();
		}

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
		drag_double("New Segment Duration (s)", new_segment_duration_, 0.05, 0.05, 3600.0, "%.2f");
		render_setting_tooltip("Duration given to segments created with Add Segment, Add Stop and Add Leg.");
		if (ImGui::Button("Add Segment...")) {
			ImGui::OpenPopup("##AddSegmentPopup");
		}
		render_setting_tooltip("Opens a menu to insert a new trajectory segment with a chosen shape.");
		if (ImGui::BeginPopup("##AddSegmentPopup")) {
			for (const auto& group : kShapeGroups) {
				if (ImGui::BeginMenu(group.name)) {
					for (const Capture::ShapeKind kind : group.kinds) {
						if (ImGui::MenuItem(Capture::shape_descriptor(kind).name)) {
							insert_segment(script, Capture::make_script_segment(kind, new_segment_duration_));
						}
					}
					ImGui::EndMenu();
				}
			}
			ImGui::EndPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Add Stop")) {
			Capture::ScriptSegment stop = Capture::make_script_segment(Capture::ShapeKind::Hold, std::min(new_segment_duration_, 3600.0));
			stop.name = "Stop";
			stop.anchor = script.segments.empty() ? Capture::AnchorMode::World : Capture::AnchorMode::ContinuePrevious;
			insert_segment(script, std::move(stop));
		}
		render_setting_tooltip("Inserts a motionless segment holding camera pose at the previous end.");

		if (ImGui::Button("Add Leg To Cam")) {
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
		render_setting_tooltip("Adds a linear segment from current end of the script to the live camera.");

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.55f);
		static_cast<void>(edit_body_reference("##SurfaceWalkBody", surface_walk_body_));
		render_setting_tooltip("Body on whose surface the new walk segment is placed. The world origin selects the primary source radius. The segment follows the body when it moves.");
		ImGui::SameLine();
		if (ImGui::Button("Add Surface Walk")) {
			insert_segment(script, make_surface_segment(orchestrator, Capture::ShapeKind::SurfaceWalk));
		}
		render_setting_tooltip("Adds a walking segment on the chosen body, starting below the live camera in its viewing direction.");
		ImGui::SameLine();
		if (ImGui::Button("Add Surface Loop")) {
			insert_segment(script, make_surface_segment(orchestrator, Capture::ShapeKind::SurfaceLoop));
		}
		render_setting_tooltip("Adds a closed circular path around the point below the live camera, following the deformed surface of the chosen body.");
		ImGui::SameLine();
		if (ImGui::Button("Add Surface Route")) {
			insert_segment(script, make_surface_segment(orchestrator, Capture::ShapeKind::SurfaceRoute));
		}
		render_setting_tooltip("Adds a great-circle route from the coordinates below the live camera to a second coordinate, following the deformed surface of the chosen body.");

		const bool has_selection = selected_segment_ >= 0 && selected_segment_ < static_cast<int>(script.segments.size());
		ImGui::BeginDisabled(!has_selection);
		ImGui::SameLine();
		if (ImGui::Button("Dup")) {
			apply_segment_action(script, ListAction::Duplicate, selected_segment_);
		}
		render_setting_tooltip("Duplicates the selected segment.");
		ImGui::SameLine();
		if (ImGui::Button("Del")) {
			apply_segment_action(script, ListAction::Delete, selected_segment_);
		}
		render_setting_tooltip("Deletes the selected segment.");
		ImGui::SameLine();
		if (ImGui::Button("Up")) {
			apply_segment_action(script, ListAction::MoveUp, selected_segment_);
		}
		render_setting_tooltip("Moves the selected segment earlier in the script order.");
		ImGui::SameLine();
		if (ImGui::Button("Down")) {
			apply_segment_action(script, ListAction::MoveDown, selected_segment_);
		}
		render_setting_tooltip("Moves the selected segment later in the script order.");
		ImGui::EndDisabled();

		ImGui::EndChild();

		vertical_splitter("##PathSplitter", list_pane_width_, 180.0f, ImGui::GetContentRegionAvail().x - 220.0f, available_height);

		ImGui::BeginChild("##SegmentInspectorPane", ImVec2(0.0f, available_height), true);
		if (has_selection) {
			render_segment_inspector(script, static_cast<size_t>(selected_segment_), orchestrator);
		} else {
			ImGui::TextDisabled("Select a segment from the list on the left to edit its parameters.");
		}
		ImGui::EndChild();
	}

	bool edit_event(Capture::ScriptEvent& event, size_t segment_count) {
		using namespace CaptureWidgets;
		using namespace MotionScriptEditorDetail;
		bool changed = false;
		if (begin_property_grid("##EventGrid")) {
			changed |= property_text("Name", event.name);
			changed |= property_check("Enabled", event.enabled);
			changed |= property_enum("Trigger", event.trigger, kTriggerNames, "When the event is triggered.");
			if (event.trigger == Capture::EventTrigger::ScriptTime) {
				changed |= property_drag("Time (s)", event.time_seconds, 0.02, 0.0, 86400.0, "%.3f", "Exact script time when the event fires.");
			} else {
				changed |= property_u32("Segment Index", event.segment, 0U, static_cast<uint32_t>(std::max<size_t>(segment_count, 1) - 1), 0, "Segment the trigger is attached to.");
				if (event.trigger == Capture::EventTrigger::SegmentFraction) {
					changed |= property_drag("Segment Fraction", event.fraction, 0.005, 0.0, 1.0, "%.3f", "Fraction of the segment progress (0 to 1).");
				}
			}
			changed |= property_drag("Time Offset (s)", event.offset_seconds, 0.02, -86400.0, 86400.0, "%.3f", "Shifts the trigger time. Use a negative offset with Segment End to fire just before a segment finishes.");
			changed |= property_enum("Action", event.action, kActionNames, "Action executed when the event triggers.");

			switch (event.action) {
				case Capture::EventAction::SetParameter: {
					int parameter_index = static_cast<int>(Capture::event_parameter_index(event.parameter));
					const auto& names = event_parameter_names();
					if (property_row("Parameter", "Simulation parameter to modify.", [&] {
						return ImGui::Combo("##value", &parameter_index, names.data(), static_cast<int>(names.size()));
					})) {
						event.parameter = static_cast<uint32_t>(Capture::kEventParameters[static_cast<size_t>(parameter_index)].type);
						changed = true;
					}
					break;
				}
				case Capture::EventAction::StepTicks:
					changed |= property_drag("Tick Count", event.value, 1.0, 0.0, 100000.0, "%.0f");
					break;
				case Capture::EventAction::SetMetric: {
					changed |= property_text("Metric Name", event.text);
					int pick = -1;
					if (property_row("Metric Quick Pick", nullptr, [&] {
						return ImGui::Combo("##value", &pick, kMetricNames.data(), static_cast<int>(kMetricNames.size()));
					})) {
						event.text = kMetricNames[static_cast<size_t>(pick)];
						changed = true;
					}
					break;
				}
				case Capture::EventAction::SetIntegrator: {
					changed |= property_text("Integrator Name", event.text);
					int pick = -1;
					if (property_row("Integrator Quick Pick", nullptr, [&] {
						return ImGui::Combo("##value", &pick, kIntegratorNames.data(), static_cast<int>(kIntegratorNames.size()));
					})) {
						event.text = kIntegratorNames[static_cast<size_t>(pick)];
						changed = true;
					}
					break;
				}
				case Capture::EventAction::LoadScenario:
					changed |= property_text("Scenario Path", event.text);
					break;
				case Capture::EventAction::SetOverlay: {
					int overlay_index = static_cast<int>(Capture::event_overlay_index(event.parameter));
					const auto& overlay_names = event_overlay_names();
					if (property_row("Overlay", "Rendering overlay switched by this event.", [&] {
						return ImGui::Combo("##value", &overlay_index, overlay_names.data(), static_cast<int>(overlay_names.size()));
					})) {
						event.parameter = static_cast<uint32_t>(overlay_index);
						changed = true;
					}
					bool overlay_enabled = event.value > 0.5;
					if (property_check("Overlay Enabled", overlay_enabled, "Turns the overlay on or off when the event fires.")) {
						event.value = overlay_enabled ? 1.0 : 0.0;
						changed = true;
					}
					break;
				}
				case Capture::EventAction::SetPerformancePreset: {
					int preset = std::clamp(static_cast<int>(std::lround(event.value)), 0, 5);
					if (property_row("Preset", "Performance preset applied when the event fires.", [&] {
						return ImGui::Combo("##value", &preset, kPerformancePresetNames.data(), static_cast<int>(kPerformancePresetNames.size()));
					})) {
						event.value = static_cast<double>(preset);
						changed = true;
					}
					break;
				}
				case Capture::EventAction::SetTickRate:
					property_info("Unit", "Value is the scheduler tick rate in Hz (10 to 1000). The ramp fields below can glide it over time.");
					break;
				case Capture::EventAction::SetResolutionScale:
					property_info("Unit", "Value is the live render scale (0.1 to 2). The ramp fields below can glide it over time.");
					break;
				default:
					break;
			}

			if (Capture::is_ramp_action(event.action)) {
				changed |= property_drag_free("Value / Start Value", event.value, 0.01, "%.5f", "Initial target value for this parameter.");
				changed |= property_drag_free("End Value (Ramp)", event.value_end, 0.01, "%.5f", "Value reached at the end of the transition ramp.");
				changed |= property_drag("Ramp Duration (s)", event.duration, 0.02, 0.0, 86400.0, "%.3f", "Duration of the linear or eased transition (0 = instant).");
			}
			end_property_grid();
		}

		if (Capture::is_ramp_action(event.action) && event.duration > 0.0) {
			if (ImGui::TreeNode("Ramp Easing")) {
				changed |= edit_easing("EventEasing", event.easing);
				ImGui::TreePop();
			}
		}
		return changed;
	}

	void render_events_tab(Capture::MotionScript& script) {
		using namespace CaptureWidgets;
		using namespace MotionScriptEditorDetail;
		const float available_height = std::max(ImGui::GetContentRegionAvail().y, 350.0f);
		float left_w = std::clamp(list_pane_width_, 180.0f, std::max(ImGui::GetContentRegionAvail().x - 250.0f, 180.0f));

		ImGui::BeginChild("##EventListPane", ImVec2(left_w, available_height), true);
		ImGui::TextColored(kHeaderColor, "Events (%zu)", script.events.size());

		const float table_height = std::max(available_height - 80.0f, 100.0f);
		if (ImGui::BeginTable("##EventTable", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp, ImVec2(0.0f, table_height))) {
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableSetupColumn("On", ImGuiTableColumnFlags_WidthFixed, 26.0f);
			ImGui::TableSetupColumn("Event", ImGuiTableColumnFlags_WidthStretch, 1.0f);
			ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 54.0f);
			ImGui::TableHeadersRow();

			for (size_t i = 0; i < script.events.size(); ++i) {
				ImGui::PushID(static_cast<int>(i));
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				bool enabled = script.events[i].enabled;
				if (ImGui::Checkbox("##event_enabled", &enabled)) {
					script.events[i].enabled = enabled;
					mark(true);
				}
				render_setting_tooltip("Enables or disables this event.");
				ImGui::TableNextColumn();
				char label[192];
				std::snprintf(label, sizeof(label), "%02zu  %s  (%s)", i + 1, script.events[i].name.c_str(), Capture::event_action_name(script.events[i].action));
				if (ImGui::Selectable(label, selected_event_ == static_cast<int>(i), ImGuiSelectableFlags_SpanAllColumns)) {
					selected_event_ = static_cast<int>(i);
				}
				if (ImGui::BeginPopupContextItem()) {
					if (ImGui::MenuItem("Duplicate")) {
						Capture::ScriptEvent copy = script.events[i];
						copy.name += " Copy";
						script.events.insert(script.events.begin() + i + 1, std::move(copy));
						selected_event_ = static_cast<int>(i + 1);
						mark(true);
					}
					if (ImGui::MenuItem("Delete")) {
						script.events.erase(script.events.begin() + i);
						selected_event_ = std::min(static_cast<int>(i), static_cast<int>(script.events.size()) - 1);
						mark(true);
					}
					ImGui::Separator();
					if (ImGui::MenuItem(script.events[i].enabled ? "Disable" : "Enable")) {
						script.events[i].enabled = !script.events[i].enabled;
						mark(true);
					}
					ImGui::EndPopup();
				}
				ImGui::TableNextColumn();
				ImGui::Text("%.2fs", script.resolved_event_time(script.events[i]));
				ImGui::PopID();
			}
			ImGui::EndTable();
		}

		if (ImGui::Button("Add At Cursor")) {
			Capture::ScriptEvent event;
			event.time_seconds = cursor_seconds_;
			event.name = "Event " + std::to_string(script.events.size() + 1);
			script.events.push_back(std::move(event));
			selected_event_ = static_cast<int>(script.events.size()) - 1;
			mark(true);
		}
		render_setting_tooltip("Creates a new event scheduled at the current timeline cursor position.");
		ImGui::SameLine();
		if (ImGui::Button("Sort By Time")) {
			std::stable_sort(script.events.begin(), script.events.end(), [&script](const Capture::ScriptEvent& a, const Capture::ScriptEvent& b) {
				return script.resolved_event_time(a) < script.resolved_event_time(b);
			});
			selected_event_ = -1;
			mark(true);
		}
		render_setting_tooltip("Sorts all events in chronological order according to their trigger times.");

		const bool has_event = selected_event_ >= 0 && selected_event_ < static_cast<int>(script.events.size());
		ImGui::BeginDisabled(!has_event);
		if (ImGui::Button("Duplicate")) {
			Capture::ScriptEvent copy = script.events[static_cast<size_t>(selected_event_)];
			copy.name += " Copy";
			script.events.insert(script.events.begin() + selected_event_ + 1, std::move(copy));
			++selected_event_;
			mark(true);
		}
		render_setting_tooltip("Duplicates the selected event.");
		ImGui::SameLine();
		if (ImGui::Button("Delete")) {
			script.events.erase(script.events.begin() + selected_event_);
			selected_event_ = std::min(selected_event_, static_cast<int>(script.events.size()) - 1);
			mark(true);
		}
		render_setting_tooltip("Deletes the selected event.");
		ImGui::EndDisabled();

		ImGui::EndChild();

		vertical_splitter("##EventSplitter", list_pane_width_, 180.0f, ImGui::GetContentRegionAvail().x - 220.0f, available_height);

		ImGui::BeginChild("##EventInspectorPane", ImVec2(0.0f, available_height), true);
		if (has_event) {
			ImGui::TextColored(kHeaderColor, "%02d  %s", selected_event_ + 1, script.events[static_cast<size_t>(selected_event_)].name.c_str());
			ImGui::PushID("SelectedEvent");
			mark(edit_event(script.events[static_cast<size_t>(selected_event_)], script.segments.size()));
			ImGui::PopID();
		} else {
			ImGui::TextDisabled("Select an event from the list on the left to edit its parameters.");
		}
		ImGui::EndChild();
	}

	void render_script_pane(Capture::MotionScript& script) {
		using namespace CaptureWidgets;
		using namespace MotionScriptEditorDetail;
		ImGui::SeparatorText("Script Properties");
		if (begin_property_grid("##ScriptPropsGrid")) {
			if (property_text("Script Name", script.name)) mark(true);
			if (property_enum("End Behavior", script.end_behavior, kPathEndNames, "How the script behaves when the cursor passes beyond the total duration.")) mark(true);
			end_property_grid();
		}
		if (ImGui::TreeNode("Global Time Easing")) {
			if (edit_easing("GlobalEasing", script.global_easing)) mark(true);
			ImGui::TreePop();
		}

		ImGui::SeparatorText("Timing Adjustments");
		if (begin_property_grid("##ScriptTimingGrid")) {
			property_drag("Scale Factor", time_scale_factor_, 0.01, 0.01, 100.0, "%.3f", "Multiplier applied to all durations.");
			property_drag("Target Duration (s)", fit_duration_, 0.1, 0.1, 86400.0, "%.2f", "Total length to scale the script to.");
			end_property_grid();
		}
		if (ImGui::Button("Apply Time Scale")) {
			script.scale_time(time_scale_factor_);
			mark(true);
		}
		render_setting_tooltip("Multiplies every segment duration, ramp duration and timed event by the factor.");
		ImGui::SameLine();
		if (ImGui::Button("Fit To Target Duration")) {
			const double total = script.total_duration();
			if (total > 1.0e-9) {
				script.scale_time(fit_duration_ / total);
				mark(true);
			}
		}
		render_setting_tooltip("Rescales all timings so the whole script lasts the target duration.");

		ImGui::SeparatorText("Presets");
		ImGui::Combo("Preset Template", &preset_index_, Capture::kScriptPresetNames.data(), static_cast<int>(Capture::kScriptPresetCount));
		render_setting_tooltip("Predefined trajectory script template to replace or append.");
		if (ImGui::Button("Replace Script With Preset")) {
			script = Capture::MotionScript::make_preset(static_cast<Capture::ScriptPreset>(preset_index_));
			select_segment(script.segments.empty() ? -1 : 0);
			selected_event_ = -1;
			mark(true);
		}
		render_setting_tooltip("Replaces the entire current motion script with the selected preset template.");
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
		render_setting_tooltip("Appends all segments and events from the selected preset to the current script.");
		ImGui::SameLine();
		if (ImGui::Button("New Empty Script")) {
			script = Capture::MotionScript{};
			select_segment(-1);
			selected_event_ = -1;
			mark(true);
		}
		render_setting_tooltip("Clears all segments and events to start a blank motion script.");

		ImGui::SeparatorText("Import / Export & Files");
		render_script_files(script);
	}

	void render_script_files(Capture::MotionScript& script) {
		using namespace CaptureWidgets;
		if (begin_property_grid("##ScriptFilesGrid")) {
			property_text("File Path", file_path_, "Target path for script loading and saving.");
			end_property_grid();
		}
		if (ImGui::Button("Save Script To File")) {
			const std::string error = Capture::save_motion_script(file_path_, script);
			file_message_ = error.empty() ? ("Script saved to " + file_path_) : error;
		}
		render_setting_tooltip("Saves the active motion script to the specified file path on disk.");
		ImGui::SameLine();
		if (ImGui::Button("Load Script From File")) {
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
		render_setting_tooltip("Loads a motion script from the specified file path on disk.");
		if (text_buffer_.empty()) {
			text_buffer_.assign(kTextBufferCapacity, '\0');
		}
		ImGui::InputTextMultiline("##ScriptText", text_buffer_.data(), text_buffer_.size(), ImVec2(-1.0f, 180.0f));
		if (ImGui::Button("Export Text")) {
			const std::string text = Capture::motion_script_to_text(script);
			if (text.size() < text_buffer_.size()) {
				std::memcpy(text_buffer_.data(), text.c_str(), text.size() + 1U);
				file_message_ = "Script exported to the text field.";
			} else {
				file_message_ = "The script is too large for the text field, use the script file instead.";
			}
		}
		render_setting_tooltip("Serializes the script into text format and populates the text field above.");
		ImGui::SameLine();
		if (ImGui::Button("Import Text")) {
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
		render_setting_tooltip("Parses the text from the field above and replaces the active motion script.");
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

		render_summary(script);
		render_timeline(script);

		if (ImGui::BeginTabBar("##MotionScriptEditorTabs")) {
			if (ImGui::BeginTabItem("Path Editor", nullptr, tab_flags(Pane::Path))) {
				render_path_tab(script, orchestrator);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Events", nullptr, tab_flags(Pane::Events))) {
				render_events_tab(script);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Curves", nullptr, tab_flags(Pane::Curves))) {
				render_curves(script, orchestrator);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Script", nullptr, tab_flags(Pane::Script))) {
				render_script_pane(script);
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
		pane_request_ = Pane::None;
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
