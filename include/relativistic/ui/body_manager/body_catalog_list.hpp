#pragma once

#include <imgui.h>
#include "relativistic/ui/body_manager/body_manager_context.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include "relativistic/ui/coordinate_aware_widgets.hpp"
#include "relativistic/units/unit_system.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace Relativistic::UI {

enum class CatalogRowAction : uint8_t {
	None = 0,
	LookAt,
	ToggleTracking,
	ToggleEnabled,
	Duplicate,
	Delete,
	WalkOnSurface,
	SetCircularOrbit,
	SafeSeparation,
	AlignSpinWithOrbit,
	ToggleSource
};

struct CatalogFilterState {
	char search[64]{};
	int sort_mode{0};
	bool descending{false};
	int enabled_filter{0};
	int type_filter{0};
	int page{0};
	int page_size_index{2};
};

struct CatalogListResult {
	int clicked_id{-1};
	int action_id{-1};
	CatalogRowAction action{CatalogRowAction::None};
};

inline constexpr std::array<const char*, 5> kCatalogSortNames{"Creation Order", "Name", "Mass", "Distance From Center", "Speed"};
inline constexpr std::array<const char*, 3> kCatalogStateNames{"All States", "Enabled Only", "Disabled Only"};
inline constexpr std::array<const char*, 5> kCatalogPageSizeNames{"25 Per Page", "50 Per Page", "100 Per Page", "250 Per Page", "Show All"};
inline constexpr std::array<int, 5> kCatalogPageSizes{25, 50, 100, 250, 0};

[[nodiscard]] inline std::string catalog_lowercase(std::string text) {
	std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return text;
}

[[nodiscard]] inline std::string catalog_info_line(BodyManagerContext& ctx, const Dynamics::PostNewtonianBody& b) {
	const auto& prefs = ctx.orchestrator().unit_preferences();
	std::string text = "M " + Units::format_mass(b.mass * ctx.mass_scale(), prefs.mass, 3);
	if (b.is_spacetime_source) {
		text += "  |  a/M " + std::to_string(b.kerr_spin_parameter()).substr(0, 5);
		text += "  |  r_h " + Units::format_distance(b.kerr_outer_horizon_radius() * ctx.length_scale(), prefs.distance, 3);
	} else {
		text += "  |  r " + Units::format_distance(BodyManagerContext::distance_from_center(b) * ctx.length_scale(), prefs.distance, 3);
		text += "  |  v " + Units::format_velocity(b.speed() * ctx.speed_scale(), prefs.velocity);
	}
	return text;
}

[[nodiscard]] inline CatalogListResult render_body_catalog_list(BodyManagerContext& ctx, std::span<Dynamics::PostNewtonianBody> bodies, bool sources, CatalogFilterState& filter, int selected_id) noexcept {
	CatalogListResult result;
	filter.sort_mode = std::clamp(filter.sort_mode, 0, static_cast<int>(kCatalogSortNames.size()) - 1);
	filter.enabled_filter = std::clamp(filter.enabled_filter, 0, static_cast<int>(kCatalogStateNames.size()) - 1);
	filter.page_size_index = std::clamp(filter.page_size_index, 0, static_cast<int>(kCatalogPageSizes.size()) - 1);

	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::InputTextWithHint("##CatalogSearch", sources ? "Filter sources by name or ID..." : "Filter objects by name or ID...", filter.search, sizeof(filter.search))) {
		filter.page = 0;
	}
	render_setting_tooltip("Narrows the list below to entries whose name or numeric identifier contains this text. The match ignores letter case.");

	ImGui::SetNextItemWidth(-34.0f);
	ImGui::Combo("##CatalogSort", &filter.sort_mode, kCatalogSortNames.data(), static_cast<int>(kCatalogSortNames.size()));
	ImGui::SameLine();
	if (ImGui::ArrowButton("##CatalogSortDirection", filter.descending ? ImGuiDir_Down : ImGuiDir_Up)) {
		filter.descending = !filter.descending;
	}
	render_setting_tooltip("Chooses how the list is ordered, and toggles between ascending and descending order.");

	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::Combo("##CatalogState", &filter.enabled_filter, kCatalogStateNames.data(), static_cast<int>(kCatalogStateNames.size()))) {
		filter.page = 0;
	}
	render_setting_tooltip("Restricts the list to enabled or disabled entries.");

	if (!sources) {
		ImGui::SetNextItemWidth(-1.0f);
		if (ImGui::Combo("##CatalogCategory", &filter.type_filter, kCatalogTypeCategoryNames.data(), static_cast<int>(kCatalogTypeCategoryNames.size()))) {
			filter.page = 0;
		}
		render_setting_tooltip("Filters objects by their physical archetype category.");
	}

	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::Combo("##CatalogPageSize", &filter.page_size_index, kCatalogPageSizeNames.data(), static_cast<int>(kCatalogPageSizeNames.size()))) {
		filter.page = 0;
	}
	render_setting_tooltip("Number of entries displayed on one page. Large catalogs stay responsive because only the visible rows are drawn.");

	std::vector<size_t> visible;
	visible.reserve(bodies.size());
	const std::string needle = catalog_lowercase(filter.search);
	for (size_t i = 0; i < bodies.size(); ++i) {
		const auto& b = bodies[i];
		if (b.is_spacetime_source != sources) continue;
		if (filter.enabled_filter == 1 && !b.enabled) continue;
		if (filter.enabled_filter == 2 && b.enabled) continue;
		if (!sources && filter.type_filter > 0) {
			const auto cat = static_cast<CatalogTypeCategory>(filter.type_filter);
			bool match = false;
			switch (cat) {
				case CatalogTypeCategory::Terrestrial: match = (b.preset_3d == Dynamics::Body3DPreset::TerrestrialPlanet); break;
				case CatalogTypeCategory::GasGiant: match = (b.preset_3d == Dynamics::Body3DPreset::GasGiant); break;
				case CatalogTypeCategory::IceGiant: match = (b.preset_3d == Dynamics::Body3DPreset::IceGiant); break;
				case CatalogTypeCategory::Star: match = (b.preset_3d == Dynamics::Body3DPreset::Star); break;
				case CatalogTypeCategory::Compact: match = (b.preset_3d == Dynamics::Body3DPreset::NeutronStar || b.preset_3d == Dynamics::Body3DPreset::Pulsar); break;
				case CatalogTypeCategory::SmallBody: match = (b.preset_3d == Dynamics::Body3DPreset::Metallic || b.preset_3d == Dynamics::Body3DPreset::Asteroid); break;
				case CatalogTypeCategory::Custom: match = (b.preset_3d == Dynamics::Body3DPreset::Custom || b.preset_3d == Dynamics::Body3DPreset::BlackHole); break;
				case CatalogTypeCategory::All: default: match = true; break;
			}
			if (!match) continue;
		}
		if (!needle.empty()) {
			const std::string name = catalog_lowercase(BodyManagerContext::display_name(b));
			const std::string id_text = std::to_string(b.id);
			if (name.find(needle) == std::string::npos && id_text.find(needle) == std::string::npos) continue;
		}
		visible.push_back(i);
	}

	const auto key_less = [&](size_t a, size_t b) noexcept -> bool {
		switch (static_cast<BodyCatalogSortMode>(filter.sort_mode)) {
			case BodyCatalogSortMode::Name:
				return BodyManagerContext::display_name(bodies[a]) < BodyManagerContext::display_name(bodies[b]);
			case BodyCatalogSortMode::Mass:
				return bodies[a].mass < bodies[b].mass;
			case BodyCatalogSortMode::Distance:
				return BodyManagerContext::distance_from_center(bodies[a]) < BodyManagerContext::distance_from_center(bodies[b]);
			case BodyCatalogSortMode::Speed:
				return bodies[a].speed() < bodies[b].speed();
			case BodyCatalogSortMode::CreationOrder:
			default:
				return a < b;
		}
	};
	std::stable_sort(visible.begin(), visible.end(), [&](size_t a, size_t b) noexcept {
		return filter.descending ? key_less(b, a) : key_less(a, b);
	});

	const size_t total = visible.size();
	const int page_size = kCatalogPageSizes[static_cast<size_t>(filter.page_size_index)];
	const size_t per_page = (page_size > 0) ? static_cast<size_t>(page_size) : std::max<size_t>(total, 1);
	const size_t page_count = std::max<size_t>((total + per_page - 1) / per_page, 1);
	filter.page = std::clamp(filter.page, 0, static_cast<int>(page_count) - 1);
	const size_t first = static_cast<size_t>(filter.page) * per_page;
	const size_t last = std::min(first + per_page, total);

	if (total == 0) {
		ImGui::TextDisabled("No entry matches the current filter.");
	} else {
		ImGui::TextDisabled("Showing %zu-%zu of %zu", first + 1, last, total);
		if (page_count > 1) {
			ImGui::SameLine();
			if (ImGui::SmallButton("Prev") && filter.page > 0) {
				--filter.page;
			}
			ImGui::SameLine();
			ImGui::TextDisabled("%d / %zu", filter.page + 1, page_count);
			ImGui::SameLine();
			if (ImGui::SmallButton("Next") && static_cast<size_t>(filter.page) + 1 < page_count) {
				++filter.page;
			}
		}
	}

	ImGui::BeginChild("##CatalogRows", ImVec2(0.0f, 0.0f), false);
	const ImGuiStyle& style = ImGui::GetStyle();
	const float line_height = ImGui::GetTextLineHeight();
	const float row_height = line_height * 2.0f + style.FramePadding.y * 2.0f + 2.0f;
	const float pad_x = style.FramePadding.x + 2.0f;
	const float pad_y = style.FramePadding.y * 0.5f + 1.0f;
	const ImU32 text_color = ImGui::GetColorU32(ImGuiCol_Text);
	const ImU32 muted_color = ImGui::GetColorU32(ImGuiCol_TextDisabled);
	const ImU32 accent_color = ImGui::ColorConvertFloat4ToU32(sources ? ImVec4(1.0f, 0.65f, 0.4f, 1.0f) : ImVec4(0.55f, 0.85f, 1.0f, 1.0f));
	const double length_scale = ctx.length_scale();
	const auto& prefs = ctx.orchestrator().unit_preferences();

	ImGuiListClipper clipper;
	clipper.Begin(static_cast<int>(last - first), row_height + style.ItemSpacing.y);
	while (clipper.Step()) {
		for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
			const auto& b = bodies[visible[first + static_cast<size_t>(row)]];
			const int id = static_cast<int>(b.id);
			ImGui::PushID(id);
			const bool is_selected = (id == selected_id);
			if (ImGui::Selectable("##CatalogRow", is_selected, ImGuiSelectableFlags_None, ImVec2(0.0f, row_height))) {
				result.clicked_id = id;
			}
			if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
				result.clicked_id = id;
			}
			const bool hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort);
			const ImVec2 rect_min = ImGui::GetItemRectMin();
			const ImVec2 rect_max = ImGui::GetItemRectMax();

			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			const std::string id_label = "#" + std::to_string(b.id);
			const float id_width = ImGui::CalcTextSize(id_label.c_str()).x;
			std::string name = BodyManagerContext::display_name(b);
			if (!b.enabled) {
				name += "  (disabled)";
			}
			draw_list->PushClipRect(rect_min, ImVec2(std::max(rect_min.x, rect_max.x - id_width - 2.0f * pad_x), rect_max.y), true);
			draw_list->AddText(ImVec2(rect_min.x + pad_x, rect_min.y + pad_y), b.enabled ? text_color : muted_color, name.c_str());
			draw_list->PopClipRect();
			draw_list->PushClipRect(rect_min, rect_max, true);
			draw_list->AddText(ImVec2(rect_max.x - id_width - pad_x, rect_min.y + pad_y), accent_color, id_label.c_str());
			const std::string info = catalog_info_line(ctx, b);
			draw_list->AddText(ImVec2(rect_min.x + pad_x, rect_min.y + pad_y + line_height), muted_color, info.c_str());
			draw_list->PopClipRect();

			if (hovered) {
				ImGui::BeginTooltip();
				ImGui::Text("Identifier: #%u", b.id);
				ImGui::Text("Mass: %.6e", b.mass);
				ImGui::Text("Distance from center: %.6f", BodyManagerContext::distance_from_center(b));
				ImGui::TextUnformatted(("Position: " + describe_position(ctx.orchestrator().coordinate_preferences().widget_default, b.position, make_coordinate_frame(ctx.orchestrator()), length_scale, prefs, 4)).c_str());
				ImGui::Text("Speed: %.6e", b.speed());
				ImGui::EndTooltip();
			}

			if (ImGui::BeginPopupContextItem("##CatalogRowMenu")) {
				ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", BodyManagerContext::display_name(b).c_str());
				ImGui::Separator();
				if (ImGui::MenuItem("Look At")) {
					result.action = CatalogRowAction::LookAt;
					result.action_id = id;
				}
				if (ImGui::MenuItem("Track", nullptr, ctx.is_tracking(id))) {
					result.action = CatalogRowAction::ToggleTracking;
					result.action_id = id;
				}
				if (ImGui::MenuItem("Enabled", nullptr, b.enabled)) {
					result.action = CatalogRowAction::ToggleEnabled;
					result.action_id = id;
				}
				ImGui::Separator();
				if (!sources && b.enabled && ImGui::MenuItem("Walk On Surface")) {
					result.action = CatalogRowAction::WalkOnSurface;
					result.action_id = id;
				}
				if (ImGui::MenuItem("Set Circular Orbit Velocity")) {
					result.action = CatalogRowAction::SetCircularOrbit;
					result.action_id = id;
				}
				if (sources) {
					if (ImGui::MenuItem("Move To Safe Separation")) {
						result.action = CatalogRowAction::SafeSeparation;
						result.action_id = id;
					}
					if (ImGui::MenuItem("Align Spin With Orbit")) {
						result.action = CatalogRowAction::AlignSpinWithOrbit;
						result.action_id = id;
					}
					if (ImGui::MenuItem("Revert To Ordinary Body")) {
						result.action = CatalogRowAction::ToggleSource;
						result.action_id = id;
					}
				} else {
					if (ImGui::MenuItem("Convert To Spacetime Source")) {
						result.action = CatalogRowAction::ToggleSource;
						result.action_id = id;
					}
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Duplicate")) {
					result.action = CatalogRowAction::Duplicate;
					result.action_id = id;
				}
				if (ImGui::MenuItem("Delete")) {
					result.action = CatalogRowAction::Delete;
					result.action_id = id;
				}
				ImGui::EndPopup();
			}
			ImGui::PopID();
		}
	}
	ImGui::EndChild();
	return result;
}

inline void apply_catalog_row_action(BodyManagerContext& ctx, const CatalogListResult& request) noexcept {
	if (request.action == CatalogRowAction::None || request.action_id < 0) {
		return;
	}
	auto& sys = ctx.orchestrator().nbody_system();
	std::lock_guard<std::recursive_mutex> lock(sys.bodies_mutex());
	const uint32_t id = static_cast<uint32_t>(request.action_id);
	Dynamics::PostNewtonianBody* body = ctx.find_body(id);
	if (body == nullptr) {
		return;
	}
	switch (request.action) {
		case CatalogRowAction::LookAt:
			ctx.look_at(body->position);
			break;
		case CatalogRowAction::ToggleTracking:
			ctx.toggle_tracking(request.action_id);
			break;
		case CatalogRowAction::ToggleEnabled:
			body->enabled = !body->enabled;
			sys.update_accelerations();
			ctx.orchestrator().notify_state_changed();
			break;
		case CatalogRowAction::WalkOnSurface:
			ctx.orchestrator().parameters().surface_walk_body_id = id;
			static_cast<void>(ctx.orchestrator().enqueue_command(Orchestrator::Command::make_set_camera_mode(4U)));
			break;
		case CatalogRowAction::SetCircularOrbit: {
			const double central_mass = ctx.orchestrator().parameters().mass + (body->is_spacetime_source ? body->mass : 0.0);
			body->velocity = ctx.compute_circular_orbit_velocity(body->position, central_mass);
			sys.update_accelerations();
			ctx.orchestrator().notify_state_changed();
			break;
		}
		case CatalogRowAction::SafeSeparation:
			ctx.resolve_source_placement(*body);
			sys.update_accelerations();
			ctx.orchestrator().notify_state_changed();
			break;
		case CatalogRowAction::AlignSpinWithOrbit: {
			const std::array<double, 3> angular_momentum{
				body->position[1] * body->velocity[2] - body->position[2] * body->velocity[1],
				body->position[2] * body->velocity[0] - body->position[0] * body->velocity[2],
				body->position[0] * body->velocity[1] - body->position[1] * body->velocity[0]
			};
			if (std::sqrt(angular_momentum[0] * angular_momentum[0] + angular_momentum[1] * angular_momentum[1] + angular_momentum[2] * angular_momentum[2]) > 1e-12) {
				body->set_spin_state(body->kerr_spin_parameter(), angular_momentum);
				sys.update_accelerations();
				ctx.orchestrator().notify_state_changed();
			}
			break;
		}
		case CatalogRowAction::ToggleSource: {
			body->is_spacetime_source = !body->is_spacetime_source;
			if (body->is_spacetime_source) {
				apply_body_appearance_preset(*body, Dynamics::Body3DPreset::BlackHole);
				body->enforce_spacetime_source_invariants();
				ctx.resolve_source_placement(*body);
				ctx.select_and_open_source(static_cast<int>(id));
			} else {
				apply_body_appearance_preset(*body, Dynamics::Body3DPreset::Metallic);
				ctx.select_and_open_object(static_cast<int>(id));
			}
			sys.update_accelerations();
			ctx.orchestrator().notify_state_changed();
			break;
		}
		case CatalogRowAction::Duplicate: {
			Dynamics::PostNewtonianBody clone = *body;
			clone.position[0] += std::max(clone.radius, 1e-6) * 4.0;
			const Dynamics::BodySurfaceLayerSet layers = ctx.orchestrator().surface_layers().get(id);
			const bool is_source = clone.is_spacetime_source;
			const uint32_t new_id = ctx.spawn_body(clone, layers);
			if (is_source) {
				ctx.select_source(static_cast<int>(new_id));
			} else {
				ctx.select_object(static_cast<int>(new_id));
			}
			break;
		}
		case CatalogRowAction::Delete:
			ctx.remove_body_by_id(id);
			break;
		case CatalogRowAction::None:
		default:
			break;
	}
}

}
