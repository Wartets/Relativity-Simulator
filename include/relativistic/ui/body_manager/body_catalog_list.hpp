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
#include <cstdio>
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
	ToggleSource,
	BatchEnable,
	BatchDisable,
	BatchSetCircularOrbit,
	BatchSafeSeparation,
	BatchDuplicate,
	BatchDelete,
	BatchLookAtCentroid,
	SelectSubsystem,
	SelectParent,
	CircularizeAroundParent,
	CircularizeSatellites
};

enum class CatalogDynamicsFilter : uint32_t {
	All = 0,
	BoundOrbits = 1,
	EscapeTrajectories = 2,
	NearHorizon = 3
};

inline constexpr std::array<const char*, 4> kCatalogDynamicsFilterNames{
	"All Trajectories",
	"Bound Orbits (e < 1)",
	"Escape Trajectories (e >= 1)",
	"Near Horizon (r < 5 rs)"
};

struct PhysicalAttributeRangeFilter {
	bool enabled{false};
	float min_val{0.0f};
	float max_val{1.0f};
	float limit_min{0.0f};
	float limit_max{1.0f};

	[[nodiscard]] bool matches(double val) const noexcept {
		if (!enabled) return true;
		const float f = static_cast<float>(val);
		const float lo = std::min(min_val, max_val);
		const float hi = std::max(min_val, max_val);
		return (f >= lo && f <= hi);
	}

	void set_limits(float l_min, float l_max) noexcept {
		if (l_min > l_max) std::swap(l_min, l_max);
		if (l_min == l_max) {
			l_min = std::max(0.0f, l_min * 0.8f);
			l_max = (l_max > 0.0f) ? (l_max * 1.2f) : 1.0f;
		}
		limit_min = l_min;
		limit_max = l_max;
		if (!enabled) {
			min_val = limit_min;
			max_val = limit_max;
		} else {
			min_val = std::clamp(min_val, limit_min, limit_max);
			max_val = std::clamp(max_val, limit_min, limit_max);
		}
	}

	void expand_limits(float val) noexcept {
		if (val < limit_min) {
			limit_min = std::max(0.0f, val * 0.9f);
			if (!enabled) min_val = limit_min;
		}
		if (val > limit_max) {
			limit_max = val * 1.1f;
			if (!enabled) max_val = limit_max;
		}
	}

	void reset_to_limits() noexcept {
		min_val = limit_min;
		max_val = limit_max;
	}

	void sanitize() noexcept {
		if (limit_min > limit_max) std::swap(limit_min, limit_max);
		min_val = std::clamp(min_val, limit_min, limit_max);
		max_val = std::clamp(max_val, limit_min, limit_max);
		if (min_val > max_val) std::swap(min_val, max_val);
	}
};

struct CatalogRangeFilters {
	bool initialized{false};
	PhysicalAttributeRangeFilter mass{};
	PhysicalAttributeRangeFilter radius{};
	PhysicalAttributeRangeFilter temperature{};
	PhysicalAttributeRangeFilter eccentricity{};
	PhysicalAttributeRangeFilter distance{};

	[[nodiscard]] bool any_active() const noexcept {
		return mass.enabled || radius.enabled || temperature.enabled || eccentricity.enabled || distance.enabled;
	}

	[[nodiscard]] size_t active_count() const noexcept {
		size_t count = 0;
		if (mass.enabled) ++count;
		if (radius.enabled) ++count;
		if (temperature.enabled) ++count;
		if (eccentricity.enabled) ++count;
		if (distance.enabled) ++count;
		return count;
	}

	void reset_all() noexcept {
		mass.enabled = false;
		mass.reset_to_limits();
		radius.enabled = false;
		radius.reset_to_limits();
		temperature.enabled = false;
		temperature.reset_to_limits();
		eccentricity.enabled = false;
		eccentricity.reset_to_limits();
		distance.enabled = false;
		distance.reset_to_limits();
	}
};

enum class CatalogViewMode : uint32_t {
	FlatList = 0,
	HierarchicalTree = 1
};

inline constexpr std::array<const char*, 2> kCatalogViewModeNames{
	"Flat Catalog List",
	"Gravitational Hierarchy Tree"
};

struct CatalogFilterState {
	char search[64]{};
	int sort_mode{0};
	bool descending{false};
	int enabled_filter{0};
	int type_filter{0};
	int dynamics_filter{0};
	int page{0};
	int page_size_index{2};
	int view_mode{0};
	std::vector<uint32_t> collapsed_parents{};
	CatalogRangeFilters ranges{};

	[[nodiscard]] bool is_parent_collapsed(uint32_t id) const noexcept {
		return std::find(collapsed_parents.begin(), collapsed_parents.end(), id) != collapsed_parents.end();
	}

	void set_parent_collapsed(uint32_t id, bool collapsed) noexcept {
		auto it = std::find(collapsed_parents.begin(), collapsed_parents.end(), id);
		if (collapsed && it == collapsed_parents.end()) {
			collapsed_parents.push_back(id);
		} else if (!collapsed && it != collapsed_parents.end()) {
			collapsed_parents.erase(it);
		}
	}

	void toggle_parent_collapsed(uint32_t id) noexcept {
		set_parent_collapsed(id, !is_parent_collapsed(id));
	}

	void expand_all() noexcept {
		collapsed_parents.clear();
	}

	void collapse_all(const std::vector<uint32_t>& parent_ids) noexcept {
		collapsed_parents = parent_ids;
	}
};

struct CatalogListResult {
	int clicked_id{-1};
	int action_id{-1};
	CatalogRowAction action{CatalogRowAction::None};
	bool ctrl_held{false};
	bool shift_held{false};
	std::vector<int> visible_ids{};
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

struct BodyHierarchyNode {
	size_t body_index{0};
	uint32_t body_id{0};
	int parent_body_index{-1};
	int parent_id{-2};
	int tier{0};
	double distance_to_parent{0.0};
	double relative_speed{0.0};
	double relative_eccentricity{0.0};
	double relative_semi_major_axis{0.0};
	double hill_radius{0.0};
	double hill_occupancy_pct{0.0};
	double dominance_ratio{0.0};
	bool bound_to_parent{false};
	std::string parent_name{};
	std::vector<size_t> children_indices{};
};

[[nodiscard]] inline std::vector<BodyHierarchyNode> build_gravitational_hierarchy(
	std::span<const Dynamics::PostNewtonianBody> all_bodies,
	double central_mass,
	double g_sim
) noexcept {
	const size_t count = all_bodies.size();
	std::vector<BodyHierarchyNode> nodes(count);
	const double safe_central_mass = std::max(central_mass, 1e-12);
	const double safe_g = std::max(g_sim, 1e-14);

	for (size_t i = 0; i < count; ++i) {
		const auto& b = all_bodies[i];
		nodes[i].body_index = i;
		nodes[i].body_id = b.id;
		nodes[i].parent_body_index = -1;
		nodes[i].parent_id = -2;
		nodes[i].tier = 0;
		nodes[i].distance_to_parent = BodyManagerContext::distance_from_center(b);
		nodes[i].relative_speed = b.speed();
		nodes[i].parent_name = "Central Attractor";

		const double r_center = std::max(nodes[i].distance_to_parent, 1e-6);
		const double m_b = std::max(b.mass, 0.0);
		if (m_b > 0.0) {
			nodes[i].hill_radius = r_center * std::cbrt(m_b / (3.0 * (safe_central_mass + m_b)));
		}
	}

	for (size_t i = 0; i < count; ++i) {
		const auto& b_i = all_bodies[i];
		const double m_i = std::max(b_i.mass, 0.0);

		int best_parent = -1;
		double best_hill_ratio = 1e30;
		double best_d = 0.0;
		double best_v = 0.0;
		double best_ecc = 0.0;
		double best_sma = 0.0;
		double best_r_hill = 0.0;
		double best_dominance = 0.0;
		bool best_bound = false;

		for (size_t j = 0; j < count; ++j) {
			if (j == i) continue;
			const auto& b_j = all_bodies[j];
			const double m_j = std::max(b_j.mass, 0.0);
			if (m_j <= 0.0) continue;
			if (m_i > 1e-9 && m_j < m_i * 1.05) continue;
			if (m_i <= 1e-9 && m_j <= 1e-9) continue;

			const double dx = b_i.position[0] - b_j.position[0];
			const double dy = b_i.position[1] - b_j.position[1];
			const double dz = b_i.position[2] - b_j.position[2];
			const double d_ij = std::sqrt(dx * dx + dy * dy + dz * dz);
			if (d_ij < 1e-9) continue;

			const double r_j = std::max(BodyManagerContext::distance_from_center(b_j), 1e-6);
			const double r_hill_j = r_j * std::cbrt(m_j / (3.0 * (safe_central_mass + m_j)));
			const double r_soi_j = r_j * std::pow(m_j / (safe_central_mass + m_j), 0.4);
			const double r_dom = std::max(r_hill_j, r_soi_j);

			if (d_ij > r_dom * 1.35) continue;

			const double dvx = b_i.velocity[0] - b_j.velocity[0];
			const double dvy = b_i.velocity[1] - b_j.velocity[1];
			const double dvz = b_i.velocity[2] - b_j.velocity[2];
			const double v_ij = std::sqrt(dvx * dvx + dvy * dvy + dvz * dvz);

			const double mu = safe_g * (m_j + m_i);
			const double energy = 0.5 * v_ij * v_ij - mu / d_ij;

			const double hx = dy * dvz - dz * dvy;
			const double hy = dz * dvx - dx * dvz;
			const double hz = dx * dvy - dy * dvx;

			const double v_cross_hx = dvy * hz - dvz * hy;
			const double v_cross_hy = dvz * hx - dvx * hz;
			const double v_cross_hz = dvx * hy - dvy * hx;

			const double ex = v_cross_hx / mu - dx / d_ij;
			const double ey = v_cross_hy / mu - dy / d_ij;
			const double ez = v_cross_hz / mu - dz / d_ij;
			const double ecc = std::sqrt(ex * ex + ey * ey + ez * ez);

			const double a_parent = mu / (d_ij * d_ij);
			const double a_tide = 2.0 * safe_g * safe_central_mass * d_ij / (r_j * r_j * r_j);
			const double dominance = a_parent / std::max(a_tide, 1e-12);

			const bool dynamically_bound = (energy < 0.0) || (ecc <= 1.15);
			if (!dynamically_bound && dominance < 1.0) continue;

			const double hill_ratio = d_ij / std::max(r_dom, 1e-9);
			if (hill_ratio < best_hill_ratio) {
				best_hill_ratio = hill_ratio;
				best_parent = static_cast<int>(j);
				best_d = d_ij;
				best_v = v_ij;
				best_ecc = ecc;
				best_r_hill = r_hill_j;
				best_dominance = dominance;
				best_bound = (energy < 0.0);
				best_sma = (energy < 0.0) ? (-mu / (2.0 * energy)) : d_ij;
			}
		}

		if (best_parent >= 0) {
			nodes[i].parent_body_index = best_parent;
			nodes[i].parent_id = static_cast<int>(all_bodies[best_parent].id);
			nodes[i].distance_to_parent = best_d;
			nodes[i].relative_speed = best_v;
			nodes[i].relative_eccentricity = best_ecc;
			nodes[i].relative_semi_major_axis = best_sma;
			nodes[i].hill_radius = best_r_hill;
			nodes[i].hill_occupancy_pct = (best_d / std::max(best_r_hill, 1e-9)) * 100.0;
			nodes[i].dominance_ratio = best_dominance;
			nodes[i].bound_to_parent = best_bound;
			nodes[i].parent_name = BodyManagerContext::display_name(all_bodies[best_parent]);
		}
	}

	for (size_t i = 0; i < count; ++i) {
		const int p = nodes[i].parent_body_index;
		if (p >= 0 && static_cast<size_t>(p) < count) {
			nodes[p].children_indices.push_back(i);
		}
	}

	for (size_t i = 0; i < count; ++i) {
		int p = nodes[i].parent_body_index;
		int depth = 0;
		while (p >= 0 && depth < 10) {
			++depth;
			p = (static_cast<size_t>(p) < count) ? nodes[p].parent_body_index : -1;
		}
		nodes[i].tier = depth;
	}

	return nodes;
}

struct TreeFlattenItem {
	size_t body_index{0};
	int depth{0};
	bool has_children{false};
	bool is_collapsed{false};
	size_t child_count{0};
	int parent_id{-2};
	double distance_to_parent{0.0};
	double relative_speed{0.0};
	double relative_eccentricity{0.0};
	double relative_semi_major_axis{0.0};
	double hill_radius{0.0};
	double hill_occupancy_pct{0.0};
	double dominance_ratio{0.0};
	bool bound_to_parent{false};
	std::string parent_name{};
};

template <typename KeyLessFunc>
inline void flatten_tree_branch(
	size_t node_idx,
	int depth,
	const std::vector<BodyHierarchyNode>& hierarchy,
	const std::vector<uint8_t>& matching_mask,
	const CatalogFilterState& filter,
	const KeyLessFunc& key_less,
	std::vector<TreeFlattenItem>& out_rows
) noexcept {
	const auto& node = hierarchy[node_idx];
	std::vector<size_t> valid_children;
	for (size_t child_idx : node.children_indices) {
		if (matching_mask[child_idx] != 0U) {
			valid_children.push_back(child_idx);
		}
	}

	std::stable_sort(valid_children.begin(), valid_children.end(), [&](size_t a, size_t b) noexcept {
		return filter.descending ? key_less(b, a) : key_less(a, b);
	});

	const bool has_children = !valid_children.empty();
	const bool is_collapsed = filter.is_parent_collapsed(node.body_id);

	TreeFlattenItem item;
	item.body_index = node_idx;
	item.depth = depth;
	item.has_children = has_children;
	item.is_collapsed = is_collapsed;
	item.child_count = valid_children.size();
	item.parent_id = node.parent_id;
	item.distance_to_parent = node.distance_to_parent;
	item.relative_speed = node.relative_speed;
	item.relative_eccentricity = node.relative_eccentricity;
	item.relative_semi_major_axis = node.relative_semi_major_axis;
	item.hill_radius = node.hill_radius;
	item.hill_occupancy_pct = node.hill_occupancy_pct;
	item.dominance_ratio = node.dominance_ratio;
	item.bound_to_parent = node.bound_to_parent;
	item.parent_name = node.parent_name;
	out_rows.push_back(std::move(item));

	if (has_children && !is_collapsed) {
		for (size_t child_idx : valid_children) {
			flatten_tree_branch(child_idx, depth + 1, hierarchy, matching_mask, filter, key_less, out_rows);
		}
	}
}

[[nodiscard]] inline CatalogListResult render_body_catalog_list(BodyManagerContext& ctx, std::span<Dynamics::PostNewtonianBody> bodies, bool sources, CatalogFilterState& filter, int selected_id) noexcept {
	static_cast<void>(selected_id);
	CatalogListResult result;
	filter.sort_mode = std::clamp(filter.sort_mode, 0, static_cast<int>(kCatalogSortNames.size()) - 1);
	filter.enabled_filter = std::clamp(filter.enabled_filter, 0, static_cast<int>(kCatalogStateNames.size()) - 1);
	filter.dynamics_filter = std::clamp(filter.dynamics_filter, 0, static_cast<int>(kCatalogDynamicsFilterNames.size()) - 1);
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
	if (ImGui::Combo("##CatalogDynamics", &filter.dynamics_filter, kCatalogDynamicsFilterNames.data(), static_cast<int>(kCatalogDynamicsFilterNames.size()))) {
		filter.page = 0;
	}
	render_setting_tooltip("Filters bodies by orbital dynamics: all trajectories, bound orbits (e < 1), escape trajectories (e >= 1), or close horizon proximity (r < 5 rs).");

	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::Combo("##CatalogPageSize", &filter.page_size_index, kCatalogPageSizeNames.data(), static_cast<int>(kCatalogPageSizeNames.size()))) {
		filter.page = 0;
	}
	render_setting_tooltip("Number of entries displayed on one page. Large catalogs stay responsive because only the visible rows are drawn.");

	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::Combo("##CatalogViewMode", &filter.view_mode, kCatalogViewModeNames.data(), static_cast<int>(kCatalogViewModeNames.size()))) {
		filter.page = 0;
	}
	render_setting_tooltip("Toggles between a flat sequential list and a dynamic gravitational hierarchy tree grouping secondary bodies and satellites under their primary parent attractors.");

	const double central_mass = ctx.orchestrator().parameters().mass;
	const double rs = std::max(2.0 * central_mass, 1e-6);

	struct BodyFilterMetric {
		size_t body_index{0};
		double mass{0.0};
		double radius{0.0};
		double temperature{0.0};
		double eccentricity{0.0};
		double distance{0.0};
		bool bound_orbit{false};
	};

	std::vector<BodyFilterMetric> metrics;
	metrics.reserve(bodies.size());
	for (size_t i = 0; i < bodies.size(); ++i) {
		const auto& b = bodies[i];
		if (b.is_spacetime_source != sources) continue;
		const double effective_central = central_mass + (b.is_spacetime_source ? b.mass : 0.0);
		const auto el = ctx.compute_keplerian_elements(b.position, b.velocity, effective_central);
		metrics.push_back(BodyFilterMetric{
			i,
			b.mass,
			b.radius,
			b.temperature,
			el.valid ? el.eccentricity : 0.0,
			BodyManagerContext::distance_from_center(b),
			el.valid && el.is_bound
		});
	}

	if (!metrics.empty()) {
		if (!filter.ranges.initialized) {
			double min_m = metrics.front().mass, max_m = metrics.front().mass;
			double min_r = metrics.front().radius, max_r = metrics.front().radius;
			double min_t = metrics.front().temperature, max_t = metrics.front().temperature;
			double min_e = metrics.front().eccentricity, max_e = metrics.front().eccentricity;
			double min_d = metrics.front().distance, max_d = metrics.front().distance;
			for (const auto& m : metrics) {
				min_m = std::min(min_m, m.mass); max_m = std::max(max_m, m.mass);
				min_r = std::min(min_r, m.radius); max_r = std::max(max_r, m.radius);
				min_t = std::min(min_t, m.temperature); max_t = std::max(max_t, m.temperature);
				min_e = std::min(min_e, m.eccentricity); max_e = std::max(max_e, m.eccentricity);
				min_d = std::min(min_d, m.distance); max_d = std::max(max_d, m.distance);
			}
			filter.ranges.mass.set_limits(static_cast<float>(min_m * 0.95), static_cast<float>(max_m * 1.05));
			filter.ranges.radius.set_limits(static_cast<float>(min_r * 0.95), static_cast<float>(max_r * 1.05));
			filter.ranges.temperature.set_limits(static_cast<float>(std::max(0.0, min_t * 0.95)), static_cast<float>(max_t * 1.05));
			filter.ranges.eccentricity.set_limits(0.0f, static_cast<float>(std::max(max_e * 1.1, 2.0)));
			filter.ranges.distance.set_limits(static_cast<float>(min_d * 0.95), static_cast<float>(max_d * 1.05));
			filter.ranges.initialized = true;
		} else {
			for (const auto& m : metrics) {
				filter.ranges.mass.expand_limits(static_cast<float>(m.mass));
				filter.ranges.radius.expand_limits(static_cast<float>(m.radius));
				filter.ranges.temperature.expand_limits(static_cast<float>(m.temperature));
				filter.ranges.eccentricity.expand_limits(static_cast<float>(m.eccentricity));
				filter.ranges.distance.expand_limits(static_cast<float>(m.distance));
			}
		}
	}

	const size_t total_category_bodies = metrics.size();
	size_t pass_mass_count = 0;
	size_t pass_radius_count = 0;
	size_t pass_temp_count = 0;
	size_t pass_ecc_count = 0;
	size_t pass_dist_count = 0;
	size_t pass_all_range_count = 0;

	for (const auto& m : metrics) {
		const bool pm = filter.ranges.mass.matches(m.mass);
		const bool pr = filter.ranges.radius.matches(m.radius);
		const bool pt = filter.ranges.temperature.matches(m.temperature);
		const bool pe = filter.ranges.eccentricity.matches(m.eccentricity);
		const bool pd = filter.ranges.distance.matches(m.distance);
		if (pm) ++pass_mass_count;
		if (pr) ++pass_radius_count;
		if (pt) ++pass_temp_count;
		if (pe) ++pass_ecc_count;
		if (pd) ++pass_dist_count;
		if (pm && pr && pt && pe && pd) ++pass_all_range_count;
	}

	const size_t active_criteria_count = filter.ranges.active_count();
	const double pass_pct = (total_category_bodies > 0) ? (100.0 * static_cast<double>(pass_all_range_count) / static_cast<double>(total_category_bodies)) : 0.0;
	const ImVec4 badge_col = (pass_pct >= 75.0)
		? ImVec4(0.35f, 0.95f, 0.45f, 1.0f)
		: ((pass_pct >= 25.0) ? ImVec4(1.0f, 0.8f, 0.25f, 1.0f) : ImVec4(1.0f, 0.4f, 0.35f, 1.0f));

	char header_label[96];
	if (active_criteria_count > 0) {
		std::snprintf(header_label, sizeof(header_label), "Physical Attribute Range Filters (%zu active)###PhysicalRanges", active_criteria_count);
	} else {
		std::snprintf(header_label, sizeof(header_label), "Physical Attribute Range Filters###PhysicalRanges");
	}

	const bool header_open = ImGui::CollapsingHeader(header_label);
	render_setting_tooltip("Filters bodies dynamically by continuous physical parameter intervals (mass, radius, temperature, orbital eccentricity, and distance from center).");

	if (!header_open && active_criteria_count > 0) {
		ImGui::TextColored(badge_col, "Physical Filter Match: %zu / %zu (%.1f%% passing)", pass_all_range_count, total_category_bodies, pass_pct);
	}

	if (header_open) {
		ImGui::Spacing();
		if (ImGui::SmallButton("Auto-Fit Limits to Population")) {
			filter.ranges.initialized = false;
			filter.page = 0;
		}
		render_setting_tooltip("Recalibrates all slider bounds to tightly fit the current catalog population.");
		ImGui::SameLine();
		if (ImGui::SmallButton("Reset All Ranges")) {
			filter.ranges.reset_all();
			filter.page = 0;
		}
		render_setting_tooltip("Disables all physical range filters and resets sliders to their bounds.");

		ImGui::Spacing();
		ImGui::TextColored(badge_col, "Population Match: %zu / %zu (%.1f%%)", pass_all_range_count, total_category_bodies, pass_pct);
		char progress_buf[48];
		std::snprintf(progress_buf, sizeof(progress_buf), "%.1f%% (%zu/%zu)", pass_pct, pass_all_range_count, total_category_bodies);
		ImGui::ProgressBar(static_cast<float>(pass_pct * 0.01), ImVec2(-1.0f, 14.0f), progress_buf);

		if (active_criteria_count > 0) {
			ImGui::TextDisabled("Active Criteria Pass Breakdown:");
			const auto& prefs = ctx.orchestrator().unit_preferences();
			if (filter.ranges.mass.enabled) {
				const std::string s_min = Units::format_mass(static_cast<double>(filter.ranges.mass.min_val) * ctx.mass_scale(), prefs.mass, 2);
				const std::string s_max = Units::format_mass(static_cast<double>(filter.ranges.mass.max_val) * ctx.mass_scale(), prefs.mass, 2);
				ImGui::TextDisabled("  • Mass: %zu/%zu passing [%s .. %s]", pass_mass_count, total_category_bodies, s_min.c_str(), s_max.c_str());
			}
			if (filter.ranges.radius.enabled) {
				const std::string s_min = Units::format_distance(static_cast<double>(filter.ranges.radius.min_val) * ctx.length_scale(), prefs.distance, 2);
				const std::string s_max = Units::format_distance(static_cast<double>(filter.ranges.radius.max_val) * ctx.length_scale(), prefs.distance, 2);
				ImGui::TextDisabled("  • Radius: %zu/%zu passing [%s .. %s]", pass_radius_count, total_category_bodies, s_min.c_str(), s_max.c_str());
			}
			if (filter.ranges.temperature.enabled) {
				const std::string s_min = Units::format_temperature(static_cast<double>(filter.ranges.temperature.min_val), prefs.temperature);
				const std::string s_max = Units::format_temperature(static_cast<double>(filter.ranges.temperature.max_val), prefs.temperature);
				ImGui::TextDisabled("  • Temp: %zu/%zu passing [%s .. %s]", pass_temp_count, total_category_bodies, s_min.c_str(), s_max.c_str());
			}
			if (filter.ranges.eccentricity.enabled) {
				ImGui::TextDisabled("  • Eccentricity: %zu/%zu passing [%.3f .. %.3f]", pass_ecc_count, total_category_bodies, static_cast<double>(filter.ranges.eccentricity.min_val), static_cast<double>(filter.ranges.eccentricity.max_val));
			}
			if (filter.ranges.distance.enabled) {
				const std::string s_min = Units::format_distance(static_cast<double>(filter.ranges.distance.min_val) * ctx.length_scale(), prefs.distance, 2);
				const std::string s_max = Units::format_distance(static_cast<double>(filter.ranges.distance.max_val) * ctx.length_scale(), prefs.distance, 2);
				ImGui::TextDisabled("  • Distance: %zu/%zu passing [%s .. %s]", pass_dist_count, total_category_bodies, s_min.c_str(), s_max.c_str());
			}
			if (pass_all_range_count < total_category_bodies) {
				ImGui::TextDisabled("  Excluded by active ranges: %zu bodies", total_category_bodies - pass_all_range_count);
			}
		} else {
			ImGui::TextDisabled("All range filters are currently inactive (100%% passing).");
		}

		ImGui::Separator();

		const auto render_range_row = [&](const char* id, const char* name, PhysicalAttributeRangeFilter& rf, const auto& formatter, const char* format) noexcept {
			ImGui::PushID(id);
			bool en = rf.enabled;
			if (ImGui::Checkbox("##En", &en)) {
				rf.enabled = en;
				filter.page = 0;
			}
			ImGui::SameLine();
			ImGui::TextColored(en ? ImVec4(0.4f, 0.85f, 1.0f, 1.0f) : ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "%s", name);
			if (!en) {
				ImGui::SameLine();
				ImGui::TextDisabled("[%s .. %s]", formatter(rf.limit_min).c_str(), formatter(rf.limit_max).c_str());
			} else {
				ImGui::SameLine();
				if (ImGui::SmallButton("Reset")) {
					rf.reset_to_limits();
					filter.page = 0;
				}
				float speed = (rf.limit_max - rf.limit_min) * 0.005f;
				if (speed <= 0.0f) speed = 0.01f;
				ImGui::SetNextItemWidth(-1.0f);
				if (ImGui::DragFloatRange2("##Range", &rf.min_val, &rf.max_val, speed, rf.limit_min, rf.limit_max, format, format, ImGuiSliderFlags_AlwaysClamp)) {
					rf.sanitize();
					filter.page = 0;
				}
				ImGui::TextDisabled("  Span: %s  ..  %s", formatter(rf.min_val).c_str(), formatter(rf.max_val).c_str());
			}
			ImGui::PopID();
		};

		const auto& prefs = ctx.orchestrator().unit_preferences();
		render_range_row("RMass", "Mass Filter", filter.ranges.mass,
			[&](float v) { return Units::format_mass(static_cast<double>(v) * ctx.mass_scale(), prefs.mass, 3); }, "Min: %.4g");
		render_range_row("RRad", "Physical Radius Filter", filter.ranges.radius,
			[&](float v) { return Units::format_distance(static_cast<double>(v) * ctx.length_scale(), prefs.distance, 3); }, "Min: %.4g");
		render_range_row("RTemp", "Temperature Filter", filter.ranges.temperature,
			[&](float v) { return Units::format_temperature(static_cast<double>(v), prefs.temperature); }, "Min: %.0f K");
		render_range_row("REcc", "Eccentricity Filter", filter.ranges.eccentricity,
			[&](float v) {
				char buf[32];
				std::snprintf(buf, sizeof(buf), "e = %.3f", static_cast<double>(v));
				return std::string(buf);
			}, "Min: %.3f");
		render_range_row("RDist", "Distance from Center Filter", filter.ranges.distance,
			[&](float v) { return Units::format_distance(static_cast<double>(v) * ctx.length_scale(), prefs.distance, 3); }, "Min: %.4g");
		ImGui::Separator();
	}

	const double g_sim = ctx.orchestrator().physical_gravitational_constant();
	const auto hierarchy = build_gravitational_hierarchy(bodies, central_mass, g_sim);

	std::vector<uint8_t> self_matches(bodies.size(), 0U);
	const std::string needle = catalog_lowercase(filter.search);

	for (const auto& m : metrics) {
		const auto& b = bodies[m.body_index];
		if (filter.enabled_filter == 1 && !b.enabled) continue;
		if (filter.enabled_filter == 2 && b.enabled) continue;
		if (filter.dynamics_filter > 0) {
			if (filter.dynamics_filter == 1 && !m.bound_orbit) continue;
			if (filter.dynamics_filter == 2 && m.bound_orbit) continue;
			if (filter.dynamics_filter == 3 && m.distance >= 5.0 * rs) continue;
		}
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
		if (!filter.ranges.mass.matches(m.mass)) continue;
		if (!filter.ranges.radius.matches(m.radius)) continue;
		if (!filter.ranges.temperature.matches(m.temperature)) continue;
		if (!filter.ranges.eccentricity.matches(m.eccentricity)) continue;
		if (!filter.ranges.distance.matches(m.distance)) continue;

		self_matches[m.body_index] = 1U;
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

	std::vector<size_t> visible;
	std::vector<TreeFlattenItem> tree_rows;

	if (filter.view_mode == static_cast<int>(CatalogViewMode::FlatList)) {
		visible.reserve(bodies.size());
		for (size_t i = 0; i < bodies.size(); ++i) {
			if (self_matches[i] != 0U) {
				visible.push_back(i);
			}
		}
		std::stable_sort(visible.begin(), visible.end(), [&](size_t a, size_t b) noexcept {
			return filter.descending ? key_less(b, a) : key_less(a, b);
		});
		result.visible_ids.reserve(visible.size());
		for (size_t v_idx : visible) {
			result.visible_ids.push_back(static_cast<int>(bodies[v_idx].id));
		}
	} else {
		std::vector<uint8_t> matching_mask = self_matches;
		for (size_t pass = 0; pass < 6; ++pass) {
			for (size_t i = 0; i < bodies.size(); ++i) {
				if (matching_mask[i] != 0U) {
					const int p = hierarchy[i].parent_body_index;
					if (p >= 0 && static_cast<size_t>(p) < bodies.size()) {
						matching_mask[p] = 1U;
					}
				}
			}
		}

		std::vector<size_t> top_level;
		for (size_t i = 0; i < bodies.size(); ++i) {
			if (matching_mask[i] == 0U) continue;
			if (bodies[i].is_spacetime_source != sources) continue;
			const int p = hierarchy[i].parent_body_index;
			if (p < 0 || static_cast<size_t>(p) >= bodies.size() || bodies[p].is_spacetime_source != sources || matching_mask[p] == 0U) {
				top_level.push_back(i);
			}
		}

		std::stable_sort(top_level.begin(), top_level.end(), [&](size_t a, size_t b) noexcept {
			return filter.descending ? key_less(b, a) : key_less(a, b);
		});

		tree_rows.reserve(bodies.size());
		for (size_t root_idx : top_level) {
			flatten_tree_branch(root_idx, 0, hierarchy, matching_mask, filter, key_less, tree_rows);
		}

		result.visible_ids.reserve(tree_rows.size());
		for (const auto& row_item : tree_rows) {
			result.visible_ids.push_back(static_cast<int>(bodies[row_item.body_index].id));
		}
	}

	const size_t total = (filter.view_mode == static_cast<int>(CatalogViewMode::HierarchicalTree)) ? tree_rows.size() : visible.size();
	const int page_size = kCatalogPageSizes[static_cast<size_t>(filter.page_size_index)];
	const size_t per_page = (page_size > 0) ? static_cast<size_t>(page_size) : std::max<size_t>(total, 1);
	const size_t page_count = std::max<size_t>((total + per_page - 1) / per_page, 1);
	filter.page = std::clamp(filter.page, 0, static_cast<int>(page_count) - 1);
	const size_t first = static_cast<size_t>(filter.page) * per_page;
	const size_t last = std::min(first + per_page, total);

	if (filter.view_mode == static_cast<int>(CatalogViewMode::HierarchicalTree)) {
		size_t parent_count = 0;
		size_t satellite_count = 0;
		size_t sub_satellite_count = 0;
		for (const auto& item : tree_rows) {
			if (item.depth == 0) ++parent_count;
			else if (item.depth == 1) ++satellite_count;
			else ++sub_satellite_count;
		}

		ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Hierarchy: %zu Primary Attractors | %zu Secondary Bodies / Satellites | %zu Sub-satellites", parent_count, satellite_count, sub_satellite_count);
		if (ImGui::SmallButton("Expand All Sub-systems")) {
			filter.expand_all();
		}
		render_setting_tooltip("Expands every parent body tree node to display all satellites and secondary bodies.");
		ImGui::SameLine();
		if (ImGui::SmallButton("Collapse All Sub-systems")) {
			std::vector<uint32_t> parent_ids;
			for (const auto& row_item : tree_rows) {
				if (row_item.has_children) {
					parent_ids.push_back(bodies[row_item.body_index].id);
				}
			}
			filter.collapse_all(parent_ids);
		}
		render_setting_tooltip("Collapses all parent body tree nodes to hide satellites and streamline dense catalog views.");
	}

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
			const size_t item_idx = first + static_cast<size_t>(row);
			const bool is_tree_mode = (filter.view_mode == static_cast<int>(CatalogViewMode::HierarchicalTree));
			const size_t body_idx = is_tree_mode ? tree_rows[item_idx].body_index : visible[item_idx];
			const auto& b = bodies[body_idx];
			const auto* tree_item = is_tree_mode ? &tree_rows[item_idx] : nullptr;
			const int id = static_cast<int>(b.id);
			ImGui::PushID(id);
			const bool is_selected = sources ? ctx.is_source_selected(id) : ctx.is_object_selected(id);

			const float indent_px = (tree_item != nullptr) ? (static_cast<float>(tree_item->depth) * 18.0f) : 0.0f;
			const float toggle_btn_w = (tree_item != nullptr && tree_item->has_children) ? 20.0f : 0.0f;

			if (ImGui::Selectable("##CatalogRow", is_selected, ImGuiSelectableFlags_None, ImVec2(0.0f, row_height))) {
				const float mouse_click_x = ImGui::GetIO().MousePos.x;
				const ImVec2 item_min_pos = ImGui::GetItemRectMin();
				const float toggle_x_start = item_min_pos.x + pad_x + indent_px;
				const float toggle_x_end = toggle_x_start + toggle_btn_w;
				if (tree_item != nullptr && tree_item->has_children && mouse_click_x >= toggle_x_start && mouse_click_x <= toggle_x_end) {
					filter.toggle_parent_collapsed(b.id);
				} else {
					result.clicked_id = id;
					result.ctrl_held = ImGui::GetIO().KeyCtrl;
					result.shift_held = ImGui::GetIO().KeyShift;
				}
			}
			if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
				if (!is_selected) {
					result.clicked_id = id;
					result.ctrl_held = false;
					result.shift_held = false;
				}
			}
			const bool hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort);
			const ImVec2 rect_min = ImGui::GetItemRectMin();
			const ImVec2 rect_max = ImGui::GetItemRectMax();

			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			const std::string id_label = "#" + std::to_string(b.id);
			const float id_width = ImGui::CalcTextSize(id_label.c_str()).x;
			const std::string name = BodyManagerContext::display_name(b);

			const bool is_tracked = ctx.is_tracking(id);
			const bool is_luminous = b.is_spacetime_source
				? (b.accretion_disk.enabled || b.emission_intensity > 0.0f)
				: (b.emission_intensity > 0.0f || b.preset_3d == Dynamics::Body3DPreset::Star || b.preset_3d == Dynamics::Body3DPreset::Pulsar || b.preset_3d == Dynamics::Body3DPreset::NeutronStar);
			const bool has_atmo = !b.is_spacetime_source && (b.atmosphere_mode != Dynamics::Body3DAtmosphereMode::Off) && (b.atmosphere_thickness > 0.0f);
			const bool collisions_enabled = ctx.orchestrator().interaction_config().collisions.enabled;

			struct CatalogRowBadge {
				char text[32]{};
				ImU32 text_col{0};
				ImU32 bg_col{0};
				ImU32 border_col{0};
				float width{0.0f};
			};
			std::array<CatalogRowBadge, 8> badge_items{};
			size_t badge_count = 0;

			const float badge_h = line_height - 2.0f;
			const float badge_pad_x = 4.0f;

			auto add_badge = [&](const char* badge_text, ImU32 text_col, ImU32 bg_col, ImU32 border_col) noexcept {
				if (badge_count < badge_items.size()) {
					auto& b_slot = badge_items[badge_count++];
					std::strncpy(b_slot.text, badge_text, sizeof(b_slot.text) - 1);
					b_slot.text_col = text_col;
					b_slot.bg_col = bg_col;
					b_slot.border_col = border_col;
					b_slot.width = ImGui::CalcTextSize(b_slot.text).x + 2.0f * badge_pad_x;
				}
			};

			if (tree_item != nullptr) {
				if (tree_item->has_children) {
					char sub_badge[32];
					std::snprintf(sub_badge, sizeof(sub_badge), "DOM: %zu", tree_item->child_count);
					add_badge(sub_badge, IM_COL32(255, 215, 80, 255), IM_COL32(80, 65, 10, 220), IM_COL32(180, 150, 30, 230));
				} else if (tree_item->depth == 1) {
					add_badge("SEC", IM_COL32(70, 230, 230, 255), IM_COL32(10, 70, 70, 220), IM_COL32(30, 160, 160, 230));
				} else if (tree_item->depth >= 2) {
					add_badge("SUB", IM_COL32(200, 150, 255, 255), IM_COL32(60, 20, 85, 220), IM_COL32(140, 50, 190, 230));
				}
			}

			if (!b.enabled) {
				add_badge("OFF", IM_COL32(255, 110, 110, 255), IM_COL32(70, 20, 20, 220), IM_COL32(160, 45, 45, 230));
			}
			if (is_tracked) {
				add_badge("TRK", IM_COL32(255, 255, 100, 255), IM_COL32(90, 85, 10, 220), IM_COL32(220, 210, 40, 255));
			}
			if (b.is_spacetime_source) {
				add_badge("BH", IM_COL32(255, 175, 60, 255), IM_COL32(85, 40, 10, 220), IM_COL32(180, 85, 20, 230));
			}
			if (is_luminous) {
				add_badge("LUM", IM_COL32(255, 225, 70, 255), IM_COL32(75, 65, 15, 220), IM_COL32(160, 140, 30, 230));
			}
			if (has_atmo) {
				add_badge("ATM", IM_COL32(70, 220, 255, 255), IM_COL32(15, 60, 75, 220), IM_COL32(30, 130, 165, 230));
			}
			if (collisions_enabled) {
				add_badge("COLL", IM_COL32(95, 235, 130, 255), IM_COL32(20, 65, 30, 220), IM_COL32(40, 140, 60, 230));
			} else {
				add_badge("COLL", IM_COL32(150, 150, 150, 180), IM_COL32(40, 40, 40, 180), IM_COL32(75, 75, 75, 200));
			}

			const float id_x = rect_max.x - id_width - pad_x;
			float badge_right = id_x - 5.0f;
			for (size_t bi = badge_count; bi > 0; --bi) {
				const auto& item = badge_items[bi - 1];
				const float badge_x = badge_right - item.width;
				if (badge_x < rect_min.x + pad_x + 50.0f) {
					break;
				}
				const float badge_y = rect_min.y + pad_y + 1.0f;
				draw_list->AddRectFilled(ImVec2(badge_x, badge_y), ImVec2(badge_x + item.width, badge_y + badge_h), item.bg_col, 2.5f);
				draw_list->AddRect(ImVec2(badge_x, badge_y), ImVec2(badge_x + item.width, badge_y + badge_h), item.border_col, 2.5f, 0, 1.0f);
				draw_list->AddText(ImVec2(badge_x + badge_pad_x, badge_y - 0.5f), item.text_col, item.text);
				badge_right -= item.width + 3.0f;
			}

			if (tree_item != nullptr) {
				if (tree_item->depth > 0) {
					draw_list->AddText(ImVec2(rect_min.x + pad_x + indent_px - 14.0f, rect_min.y + pad_y), muted_color, "└─");
				}
				if (tree_item->has_children) {
					const bool is_collapsed = filter.is_parent_collapsed(b.id);
					const float toggle_x = rect_min.x + pad_x + indent_px;
					const bool toggle_hovered = hovered && (ImGui::GetIO().MousePos.x >= toggle_x) && (ImGui::GetIO().MousePos.x <= toggle_x + toggle_btn_w);
					const ImU32 toggle_col = toggle_hovered ? IM_COL32(255, 230, 80, 255) : IM_COL32(190, 190, 200, 240);
					draw_list->AddText(ImVec2(toggle_x, rect_min.y + pad_y), toggle_col, is_collapsed ? "[+]" : "[-]");
				}
			}

			const float text_offset_x = indent_px + toggle_btn_w;
			const float name_limit_x = std::max(rect_min.x + pad_x + text_offset_x + 10.0f, badge_right - 4.0f);
			draw_list->PushClipRect(rect_min, ImVec2(name_limit_x, rect_max.y), true);
			draw_list->AddText(ImVec2(rect_min.x + pad_x + text_offset_x, rect_min.y + pad_y), b.enabled ? text_color : muted_color, name.c_str());
			draw_list->PopClipRect();

			draw_list->PushClipRect(rect_min, rect_max, true);
			draw_list->AddText(ImVec2(id_x, rect_min.y + pad_y), accent_color, id_label.c_str());

			std::string info;
			if (tree_item != nullptr && tree_item->depth > 0) {
				char rel_buf[192];
				const std::string dist_str = Units::format_distance(tree_item->distance_to_parent * length_scale, prefs.distance, 2);
				const std::string speed_str = Units::format_velocity(tree_item->relative_speed * ctx.speed_scale(), prefs.velocity);
				std::snprintf(rel_buf, sizeof(rel_buf), "Parent: #%d  |  r_rel: %s  |  v_rel: %s  |  e: %.3f  |  Hill: %.0f%%",
					tree_item->parent_id, dist_str.c_str(), speed_str.c_str(), tree_item->relative_eccentricity, tree_item->hill_occupancy_pct);
				info = std::string(rel_buf);
			} else if (tree_item != nullptr && tree_item->has_children) {
				const std::string hill_str = Units::format_distance(tree_item->hill_radius * length_scale, prefs.distance, 2);
				info = catalog_info_line(ctx, b) + "  |  Sub-system: " + std::to_string(tree_item->child_count) + " satellite(s) (r_Hill: " + hill_str + ")";
			} else {
				info = catalog_info_line(ctx, b);
			}

			draw_list->AddText(ImVec2(rect_min.x + pad_x + text_offset_x, rect_min.y + pad_y + line_height), muted_color, info.c_str());
			draw_list->PopClipRect();

			if (hovered) {
				ImGui::BeginTooltip();
				ImGui::Text("Identifier: #%u", b.id);
				ImGui::Text("Mass: %.6e", b.mass);
				ImGui::Text("Distance from center: %.6f", BodyManagerContext::distance_from_center(b));
				ImGui::TextUnformatted(("Position: " + describe_position(ctx.orchestrator().coordinate_preferences().widget_default, b.position, make_coordinate_frame(ctx.orchestrator()), length_scale, prefs, 4)).c_str());
				ImGui::Text("Speed: %.6e", b.speed());

				if (tree_item != nullptr) {
					if (tree_item->depth > 0) {
						ImGui::Separator();
						ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.9f, 1.0f), "Gravitational Hierarchy & Parent Orbit:");
						ImGui::Text("Dominant Parent Attractor: #%d (%s)", tree_item->parent_id, tree_item->parent_name.c_str());
						ImGui::Text("Relative Distance: %s", Units::format_distance(tree_item->distance_to_parent * length_scale, prefs.distance).c_str());
						ImGui::Text("Relative Speed: %s", Units::format_velocity(tree_item->relative_speed * ctx.speed_scale(), prefs.velocity).c_str());
						ImGui::Text("Relative Eccentricity: %.4f (%s)", tree_item->relative_eccentricity, tree_item->bound_to_parent ? "Bound Orbit" : "Hyperbolic Encounter");
						ImGui::Text("Parent Hill Radius: %s", Units::format_distance(tree_item->hill_radius * length_scale, prefs.distance).c_str());
						ImGui::Text("Hill Sphere Occupancy: %.1f%% (%s)", tree_item->hill_occupancy_pct, (tree_item->hill_occupancy_pct < 50.0) ? "Deeply Stable Satellite" : ((tree_item->hill_occupancy_pct < 100.0) ? "Stable Satellite Orbit" : "Near Hill Sphere Margin"));
						ImGui::Text("Gravitational Dominance Ratio: %.1f x central tide", tree_item->dominance_ratio);
					}
					if (tree_item->has_children) {
						ImGui::Separator();
						ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Sub-system Satellite Family:");
						ImGui::Text("Direct Satellites / Secondary Bodies: %zu", tree_item->child_count);
						ImGui::Text("Hill Sphere Radius: %s", Units::format_distance(tree_item->hill_radius * length_scale, prefs.distance).c_str());
						ImGui::Text("Safe Satellite Boundary (0.5 r_Hill): %s", Units::format_distance(tree_item->hill_radius * 0.5 * length_scale, prefs.distance).c_str());
					}
				}

				ImGui::Separator();
				ImGui::TextDisabled("Status Badges:");
				if (!b.enabled) ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.45f, 1.0f), "[OFF] Body is currently disabled");
				if (is_tracked) ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.35f, 1.0f), "[TRK] Camera is currently tracking this body");
				if (b.is_spacetime_source) ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.25f, 1.0f), "[BH] Independent Kerr spacetime source");
				if (is_luminous) ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.3f, 1.0f), "[LUM] Emissive / luminous surface");
				if (has_atmo) ImGui::TextColored(ImVec4(0.3f, 0.85f, 1.0f, 1.0f), "[ATM] Atmospheric scattering active");
				if (collisions_enabled) ImGui::TextColored(ImVec4(0.4f, 0.95f, 0.5f, 1.0f), "[COLL] Physical collisions enabled");
				else ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "[COLL] Physical collisions disabled globally");
				ImGui::EndTooltip();
			}

			if (ImGui::BeginPopupContextItem("##CatalogRowMenu")) {
				const size_t batch_sel_count = sources ? ctx.selected_source_ids().size() : ctx.selected_object_ids().size();
				if (batch_sel_count > 1 && is_selected) {
					ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Batch Selection (%zu %s)", batch_sel_count, sources ? "Sources" : "Objects");
					ImGui::Separator();
					if (ImGui::MenuItem("Look At Group Centroid")) {
						result.action = CatalogRowAction::BatchLookAtCentroid;
						result.action_id = id;
					}
					if (ImGui::MenuItem("Enable All Selected")) {
						result.action = CatalogRowAction::BatchEnable;
						result.action_id = id;
					}
					if (ImGui::MenuItem("Disable All Selected")) {
						result.action = CatalogRowAction::BatchDisable;
						result.action_id = id;
					}
					if (ImGui::MenuItem("Set Circular Orbit For All Selected")) {
						result.action = CatalogRowAction::BatchSetCircularOrbit;
						result.action_id = id;
					}
					if (sources && ImGui::MenuItem("Move All Selected To Safe Separation")) {
						result.action = CatalogRowAction::BatchSafeSeparation;
						result.action_id = id;
					}
					ImGui::Separator();
					if (ImGui::MenuItem("Duplicate All Selected")) {
						result.action = CatalogRowAction::BatchDuplicate;
						result.action_id = id;
					}
					if (ImGui::MenuItem("Delete All Selected")) {
						result.action = CatalogRowAction::BatchDelete;
						result.action_id = id;
					}
				} else {
					ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", BodyManagerContext::display_name(b).c_str());
					ImGui::Separator();
					if (tree_item != nullptr && tree_item->has_children) {
						if (ImGui::MenuItem("Select Entire Sub-system")) {
							result.action = CatalogRowAction::SelectSubsystem;
							result.action_id = id;
						}
						if (ImGui::MenuItem("Circularize Satellites Around This Body")) {
							result.action = CatalogRowAction::CircularizeSatellites;
							result.action_id = id;
						}
						if (ImGui::MenuItem(filter.is_parent_collapsed(static_cast<uint32_t>(id)) ? "Expand Sub-system" : "Collapse Sub-system")) {
							filter.toggle_parent_collapsed(static_cast<uint32_t>(id));
						}
						ImGui::Separator();
					}
					if (tree_item != nullptr && tree_item->depth > 0) {
						if (tree_item->parent_id >= 0 && ImGui::MenuItem("Select Parent Body")) {
							result.action = CatalogRowAction::SelectParent;
							result.action_id = tree_item->parent_id;
						}
						if (tree_item->parent_id >= 0 && ImGui::MenuItem("Set Circular Orbit Around Parent")) {
							result.action = CatalogRowAction::CircularizeAroundParent;
							result.action_id = id;
						}
						ImGui::Separator();
					}
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
		case CatalogRowAction::BatchEnable: {
			for (int sid : (body->is_spacetime_source ? ctx.selected_source_ids() : ctx.selected_object_ids())) {
				auto* b = ctx.find_body(static_cast<uint32_t>(sid));
				if (b != nullptr) b->enabled = true;
			}
			sys.update_accelerations();
			ctx.orchestrator().notify_state_changed();
			break;
		}
		case CatalogRowAction::BatchDisable: {
			for (int sid : (body->is_spacetime_source ? ctx.selected_source_ids() : ctx.selected_object_ids())) {
				auto* b = ctx.find_body(static_cast<uint32_t>(sid));
				if (b != nullptr) b->enabled = false;
			}
			sys.update_accelerations();
			ctx.orchestrator().notify_state_changed();
			break;
		}
		case CatalogRowAction::BatchSetCircularOrbit: {
			const double central_mass = ctx.orchestrator().parameters().mass;
			for (int sid : (body->is_spacetime_source ? ctx.selected_source_ids() : ctx.selected_object_ids())) {
				auto* b = ctx.find_body(static_cast<uint32_t>(sid));
				if (b != nullptr) {
					b->velocity = ctx.compute_circular_orbit_velocity(b->position, central_mass + (b->is_spacetime_source ? b->mass : 0.0));
				}
			}
			sys.update_accelerations();
			ctx.orchestrator().notify_state_changed();
			break;
		}
		case CatalogRowAction::BatchSafeSeparation: {
			for (int sid : ctx.selected_source_ids()) {
				auto* b = ctx.find_body(static_cast<uint32_t>(sid));
				if (b != nullptr && b->is_spacetime_source) {
					ctx.resolve_source_placement(*b);
				}
			}
			sys.update_accelerations();
			ctx.orchestrator().notify_state_changed();
			break;
		}
		case CatalogRowAction::BatchDuplicate: {
			std::vector<int> target_ids = (body->is_spacetime_source ? ctx.selected_source_ids() : ctx.selected_object_ids());
			std::vector<int> new_selected;
			for (int sid : target_ids) {
				auto* b = ctx.find_body(static_cast<uint32_t>(sid));
				if (b != nullptr) {
					Dynamics::PostNewtonianBody clone = *b;
					clone.position[0] += std::max(clone.radius, 1e-6) * 4.0;
					const Dynamics::BodySurfaceLayerSet layers = ctx.orchestrator().surface_layers().get(static_cast<uint32_t>(sid));
					const uint32_t new_id = ctx.spawn_body(clone, layers);
					new_selected.push_back(static_cast<int>(new_id));
				}
			}
			if (body->is_spacetime_source) ctx.set_selected_sources(new_selected);
			else ctx.set_selected_objects(new_selected);
			break;
		}
		case CatalogRowAction::BatchDelete: {
			std::vector<int> target_ids = (body->is_spacetime_source ? ctx.selected_source_ids() : ctx.selected_object_ids());
			for (int sid : target_ids) {
				ctx.remove_body_by_id(static_cast<uint32_t>(sid));
			}
			break;
		}
		case CatalogRowAction::BatchLookAtCentroid: {
			std::vector<Dynamics::PostNewtonianBody*> ptrs;
			for (int sid : (body->is_spacetime_source ? ctx.selected_source_ids() : ctx.selected_object_ids())) {
				auto* b = ctx.find_body(static_cast<uint32_t>(sid));
				if (b != nullptr) ptrs.push_back(b);
			}
			ctx.look_at_selection(ptrs);
			break;
		}
		case CatalogRowAction::SelectSubsystem: {
			const double central_mass = ctx.orchestrator().parameters().mass;
			const double g_sim = ctx.orchestrator().physical_gravitational_constant();
			const auto hierarchy = build_gravitational_hierarchy(sys.bodies(), central_mass, g_sim);
			std::vector<int> subsystem_ids{static_cast<int>(id)};
			std::vector<size_t> queue;
			for (size_t i = 0; i < hierarchy.size(); ++i) {
				if (hierarchy[i].body_id == id) {
					queue.push_back(i);
					break;
				}
			}
			while (!queue.empty()) {
				const size_t curr = queue.back();
				queue.pop_back();
				for (size_t child_idx : hierarchy[curr].children_indices) {
					subsystem_ids.push_back(static_cast<int>(hierarchy[child_idx].body_id));
					queue.push_back(child_idx);
				}
			}
			if (body->is_spacetime_source) {
				ctx.set_selected_sources(subsystem_ids);
			} else {
				ctx.set_selected_objects(subsystem_ids);
			}
			break;
		}
		case CatalogRowAction::SelectParent: {
			if (body->is_spacetime_source) {
				ctx.select_source(request.action_id);
			} else {
				ctx.select_object(request.action_id);
			}
			break;
		}
		case CatalogRowAction::CircularizeAroundParent: {
			const double central_mass = ctx.orchestrator().parameters().mass;
			const double g_sim = ctx.orchestrator().physical_gravitational_constant();
			const auto hierarchy = build_gravitational_hierarchy(sys.bodies(), central_mass, g_sim);
			for (const auto& node : hierarchy) {
				if (node.body_id == id && node.parent_body_index >= 0) {
					const auto& parent_body = sys.bodies()[node.parent_body_index];
					body->velocity = ctx.compute_circular_satellite_velocity(
						body->position, body->velocity,
						parent_body.position, parent_body.velocity,
						parent_body.mass, body->mass
					);
					sys.update_accelerations();
					ctx.orchestrator().notify_state_changed();
					break;
				}
			}
			break;
		}
		case CatalogRowAction::CircularizeSatellites: {
			const double central_mass = ctx.orchestrator().parameters().mass;
			const double g_sim = ctx.orchestrator().physical_gravitational_constant();
			const auto hierarchy = build_gravitational_hierarchy(sys.bodies(), central_mass, g_sim);
			size_t parent_idx = hierarchy.size();
			for (size_t i = 0; i < hierarchy.size(); ++i) {
				if (hierarchy[i].body_id == id) {
					parent_idx = i;
					break;
				}
			}
			if (parent_idx < hierarchy.size()) {
				for (size_t child_idx : hierarchy[parent_idx].children_indices) {
					auto* child = ctx.find_body(hierarchy[child_idx].body_id);
					if (child != nullptr) {
						child->velocity = ctx.compute_circular_satellite_velocity(
							child->position, child->velocity,
							body->position, body->velocity,
							body->mass, child->mass
						);
					}
				}
				sys.update_accelerations();
				ctx.orchestrator().notify_state_changed();
			}
			break;
		}
		case CatalogRowAction::None:
		default:
			break;
	}
}

}
