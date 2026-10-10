#pragma once

#include "relativistic/observer/camera_projections.hpp"
#include "relativistic/ui/hud/hud_layout_config.hpp"
#include "relativistic/ui/input_actions.hpp"
#include "relativistic/ui/schematic/schematic_view_config.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <ostream>
#include <string>
#include <type_traits>
#include <unordered_map>

namespace Relativistic::UI {

class InterfacePersistence {
public:
	using Entries = std::unordered_map<std::string, std::string>;

	static void write(std::ostream& out, const HudLayoutConfig& hud, const SchematicViewConfig& schematic) {
		emit(out, "hud_ext_toolbar_padding_scale", hud.toolbar_padding_scale);
		emit(out, "hud_ext_auto_arrange", hud.auto_arrange_enabled);
		emit(out, "hud_ext_auto_arrange_spacing", hud.auto_arrange_spacing);
		emit(out, "hud_coord_primary", hud.coordinates.primary);
		emit(out, "hud_coord_secondary", hud.coordinates.secondary);
		emit(out, "hud_coord_secondary_enabled", hud.coordinates.secondary_enabled);
		emit(out, "hud_coord_axis_labels", hud.coordinates.show_axis_labels);
		visit_toolbar(hud.toolbar_buttons, [&out](const std::string& name, const auto& field) {
			emit(out, "hud_tb_" + name, field);
		});
		for (size_t i = 0; i < static_cast<size_t>(HudElementId::Count); ++i) {
			const std::string prefix = "hud_ex_" + std::to_string(i) + "_";
			visit_element(hud.element(static_cast<HudElementId>(i)), [&out, &prefix](const std::string& name, const auto& field) {
				emit(out, prefix + name, field);
			});
		}
		for (size_t i = 0; i < static_cast<size_t>(InputAction::Count); ++i) {
			emit(out, "hud_kbs_" + std::to_string(i), static_cast<bool>(hud.keybind_summary_visible[i]));
		}
		visit_schematic(schematic, [&out](const std::string& name, const auto& field) {
			emit(out, "schem_" + name, field);
		});
	}

	static void read(const Entries& entries, HudLayoutConfig& hud, SchematicViewConfig& schematic) {
		ingest(entries, "hud_ext_toolbar_padding_scale", hud.toolbar_padding_scale);
		ingest(entries, "hud_ext_auto_arrange", hud.auto_arrange_enabled);
		ingest(entries, "hud_ext_auto_arrange_spacing", hud.auto_arrange_spacing);
		ingest(entries, "hud_coord_primary", hud.coordinates.primary);
		ingest(entries, "hud_coord_secondary", hud.coordinates.secondary);
		ingest(entries, "hud_coord_secondary_enabled", hud.coordinates.secondary_enabled);
		ingest(entries, "hud_coord_axis_labels", hud.coordinates.show_axis_labels);
		hud.coordinates.primary = Observer::coordinate_system_from_index(static_cast<uint32_t>(hud.coordinates.primary));
		hud.coordinates.secondary = Observer::coordinate_system_from_index(static_cast<uint32_t>(hud.coordinates.secondary));
		visit_toolbar(hud.toolbar_buttons, [&entries](const std::string& name, auto& field) {
			ingest(entries, "hud_tb_" + name, field);
		});
		for (size_t i = 0; i < static_cast<size_t>(HudElementId::Count); ++i) {
			const std::string prefix = "hud_ex_" + std::to_string(i) + "_";
			auto& element = hud.element(static_cast<HudElementId>(i));
			visit_element(element, [&entries, &prefix](const std::string& name, auto& field) {
				ingest(entries, prefix + name, field);
			});
			element.display_mode = static_cast<HudDisplayMode>(std::min<uint32_t>(static_cast<uint32_t>(element.display_mode), 2U));
			element.decimal_precision = std::clamp(element.decimal_precision, 0, 6);
			element.warning_rule.comparison = static_cast<HudColorRuleComparison>(std::min<uint32_t>(static_cast<uint32_t>(element.warning_rule.comparison), 1U));
			element.critical_rule.comparison = static_cast<HudColorRuleComparison>(std::min<uint32_t>(static_cast<uint32_t>(element.critical_rule.comparison), 1U));
		}
		for (size_t i = 0; i < static_cast<size_t>(InputAction::Count); ++i) {
			bool visible = static_cast<bool>(hud.keybind_summary_visible[i]);
			ingest(entries, "hud_kbs_" + std::to_string(i), visible);
			hud.keybind_summary_visible[i] = visible;
		}
		visit_schematic(schematic, [&entries](const std::string& name, auto& field) {
			ingest(entries, "schem_" + name, field);
		});
		schematic.projection_mode = static_cast<Observer::ProjectionMode>(std::min<uint32_t>(static_cast<uint32_t>(schematic.projection_mode), 7U));
	}

private:
	template <typename T>
	static void emit(std::ostream& out, const std::string& key, const T& value) {
		if constexpr (std::is_enum_v<T>) {
			out << key << '=' << static_cast<uint32_t>(value) << '\n';
		} else if constexpr (std::is_same_v<T, bool>) {
			out << key << '=' << (value ? 1 : 0) << '\n';
		} else if constexpr (std::is_floating_point_v<T>) {
			out << key << '=' << std::setprecision(9) << value << '\n';
		} else {
			out << key << '=' << value << '\n';
		}
	}

	template <typename T>
	static void ingest(const Entries& entries, const std::string& key, T& target) {
		const auto it = entries.find(key);
		if (it == entries.end()) {
			return;
		}
		const char* text = it->second.c_str();
		if constexpr (std::is_enum_v<T>) {
			target = static_cast<T>(std::strtoul(text, nullptr, 10));
		} else if constexpr (std::is_same_v<T, bool>) {
			target = std::strtoul(text, nullptr, 10) != 0UL;
		} else if constexpr (std::is_floating_point_v<T>) {
			target = static_cast<T>(std::strtod(text, nullptr));
		} else {
			target = static_cast<T>(std::strtoll(text, nullptr, 10));
		}
	}

	template <typename Rule, typename Visitor>
	static void visit_rule(const std::string& prefix, Rule& rule, Visitor& visitor) {
		visitor(prefix + "enabled", rule.enabled);
		visitor(prefix + "comparison", rule.comparison);
		visitor(prefix + "threshold", rule.threshold);
		visitor(prefix + "r", rule.color[0]);
		visitor(prefix + "g", rule.color[1]);
		visitor(prefix + "b", rule.color[2]);
		visitor(prefix + "a", rule.color[3]);
	}

	template <typename Element, typename Visitor>
	static void visit_element(Element& element, Visitor&& visitor) {
		visitor("nudge_x", element.nudge_x);
		visitor("nudge_y", element.nudge_y);
		visitor("display_mode", element.display_mode);
		visitor("decimal_precision", element.decimal_precision);
		visitor("show_label", element.show_label);
		visitor("horizontal_layout", element.horizontal_layout);
		visitor("draw_priority", element.draw_priority);
		visitor("refresh_interval", element.refresh_interval_seconds);
		visit_rule("warn_", element.warning_rule, visitor);
		visit_rule("crit_", element.critical_rule, visitor);
	}

	template <typename Toolbar, typename Visitor>
	static void visit_toolbar(Toolbar& tb, Visitor&& visitor) {
		visitor("play_pause", tb.play_pause);
		visitor("step", tb.step);
		visitor("reset_view", tb.reset_view);
		visitor("look_at_target_combo", tb.look_at_target_combo);
		visitor("jump_to_target", tb.jump_to_target);
		visitor("camera_mode_combo", tb.camera_mode_combo);
		visitor("hud_master_toggle", tb.hud_master_toggle);
		visitor("screenshot", tb.screenshot);
		visitor("fullscreen_toggle", tb.fullscreen_toggle);
		visitor("gpu_compute_toggle", tb.gpu_compute_toggle);
		visitor("space_skip_toggle", tb.space_skip_toggle);
		visitor("lod_toggle", tb.lod_toggle);
		visitor("exposure_controls", tb.exposure_controls);
		visitor("warp_controls", tb.warp_controls);
		visitor("tonemapper_cycle", tb.tonemapper_cycle);
		visitor("projection_cycle", tb.projection_cycle);
		visitor("skybox_cycle", tb.skybox_cycle);
		visitor("metric_cycle", tb.metric_cycle);
		visitor("integrator_cycle", tb.integrator_cycle);
		visitor("performance_preset_combo", tb.performance_preset_combo);
		visitor("quicksave_quickload", tb.quicksave_quickload);
		visitor("step_controller_cycle", tb.step_controller_cycle);
		visitor("render_distance_toggle", tb.render_distance_toggle);
		visitor("pole_precision_nudge", tb.pole_precision_nudge);
	}

	template <typename Schematic, typename Visitor>
	static void visit_schematic(Schematic& s, Visitor&& visitor) {
		visitor("projection_mode", s.projection_mode);
		visitor("human_perspective_mode", s.human_perspective_mode);
		visitor("show_overlay_in_raytraced_view", s.show_overlay_in_raytraced_view);
		visitor("lens_body_overlays_in_raytraced_view", s.lens_body_overlays_in_raytraced_view);
		visitor("show_central_object", s.show_central_object);
		visitor("show_bodies", s.show_bodies);
		visitor("show_background_grid", s.show_background_grid);
		visitor("show_field_lines", s.show_field_lines);
		visitor("show_trails", s.show_trails);
		visitor("show_orbit_predictions", s.show_orbit_predictions);
		visitor("show_vectors", s.show_vectors);
		visitor("show_tags", s.show_tags);
	}
};

}
