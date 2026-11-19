#pragma once

#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/dynamics/pn/pn_body.hpp"
#include "relativistic/dynamics/pn/pn_nbody_system.hpp"
#include "relativistic/dynamics/body_surface_layers.hpp"
#include "relativistic/ui/body_manager/body_manager_types.hpp"
#include "relativistic/ui/body_manager/body_preset_library.hpp"
#include "relativistic/ui/numeric_slider_utils.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include "relativistic/units/unit_system.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <mutex>
#include <numbers>
#include <optional>
#include <random>
#include <string>
#include <string_view>

namespace Relativistic::UI {

struct KeplerianOrbitElements {
	bool valid{false};
	double semi_major_axis{0.0};
	double eccentricity{0.0};
	double inclination_deg{0.0};
	double period_seconds{0.0};
	double periapsis{0.0};
	double apoapsis{0.0};
	double specific_energy{0.0};
	double angular_momentum{0.0};
	bool is_bound{false};
	bool is_prograde{true};
	const char* type_name{"Unknown"};
};

struct TrajectoryHazardDiagnostic {
	bool has_hazard{false};
	bool plunge_hazard{false};
	bool roche_hazard{false};
	double periapsis{0.0};
	double horizon_radius{0.0};
	double roche_limit{0.0};
};

struct CollisionOverlapDiagnostic {
	bool has_collision{false};
	bool inside_primary_horizon{false};
	bool inside_source_horizon{false};
	bool overlaps_body{false};
	uint32_t colliding_body_id{0};
	std::string colliding_body_name{};
	double clearance_needed{0.0};
};

struct BinarySourceConfiguration {
	bool valid{false};
	double primary_mass{0.0};
	double secondary_mass{0.0};
	double mass_ratio{0.0};
	double symmetric_mass_ratio{0.0};
	double separation_distance{0.0};
	double separation_rg{0.0};
	double spin_orbit_misalignment_deg{0.0};
	double mutual_spin_angle_deg{0.0};
	double peters_coalescence_time_seconds{0.0};
};

struct RelativeOrbitalPlaneState {
	bool valid{false};
	int reference_id{-2};
	std::string reference_name{"Central Spacetime Attractor (Origin)"};
	double reference_mass{1.0};
	std::array<double, 3> reference_position{0.0, 0.0, 0.0};
	std::array<double, 3> reference_velocity{0.0, 0.0, 0.0};
	std::array<double, 3> reference_acceleration{0.0, 0.0, 0.0};
	std::array<double, 3> reference_plane_normal{0.0, 0.0, 1.0};
	double reference_inclination_world_deg{0.0};

	std::array<double, 3> relative_position{0.0, 0.0, 0.0};
	std::array<double, 3> relative_velocity{0.0, 0.0, 0.0};
	std::array<double, 3> relative_acceleration{0.0, 0.0, 0.0};
	double relative_distance{0.0};
	double relative_speed{0.0};
	double relative_radial_speed{0.0};
	double relative_tangential_speed{0.0};
	double relative_acceleration_mag{0.0};

	std::array<double, 3> orbit_normal{0.0, 0.0, 1.0};
	std::array<double, 3> node_vector{1.0, 0.0, 0.0};
	std::array<double, 3> eccentricity_vector{0.0, 0.0, 0.0};

	double target_inclination_world_deg{0.0};
	double signed_inclination_offset_deg{0.0};
	double relative_inclination_deg{0.0};
	double relative_node_deg{0.0};
	double relative_periapsis_deg{0.0};
	double relative_true_anomaly_deg{0.0};

	double semi_major_axis{0.0};
	double eccentricity{0.0};
	double periapsis_distance{0.0};
	double apoapsis_distance{0.0};
	double orbital_period_seconds{0.0};
	double specific_energy{0.0};
	double specific_angular_momentum{0.0};
	bool is_bound{false};
	bool is_prograde{true};

	double hill_radius{0.0};
	double hill_occupancy_pct{0.0};
	double tidal_dominance_ratio{0.0};
	bool is_satellite_of_reference{false};

	const char* alignment_category{"Coplanar Prograde (Aligned)"};
	bool is_in_kozai_zone{false};
	const char* kozai_warning{""};
};

struct LocalFrenetFrameState {
	bool valid{false};
	int reference_id{-2};
	std::string reference_name{"Central Spacetime Attractor (Origin)"};
	double distance{0.0};
	double speed{0.0};
	double v_radial{0.0};
	double v_tangential{0.0};
	double v_normal{0.0};
	double circular_speed{0.0};
	double escape_speed{0.0};
	double flight_path_angle_deg{0.0};
	std::array<double, 3> radial_hat{1.0, 0.0, 0.0};
	std::array<double, 3> tangential_hat{0.0, 1.0, 0.0};
	std::array<double, 3> normal_hat{0.0, 0.0, 1.0};
};

struct FrenetManeuverPreview {
	bool valid{false};
	double delta_v_magnitude{0.0};
	double current_speed{0.0};
	double predicted_speed{0.0};
	double current_eccentricity{0.0};
	double predicted_eccentricity{0.0};
	double current_semi_major_axis{0.0};
	double predicted_semi_major_axis{0.0};
	double current_periapsis{0.0};
	double predicted_periapsis{0.0};
	double current_apoapsis{0.0};
	double predicted_apoapsis{0.0};
	double current_inclination_deg{0.0};
	double predicted_inclination_deg{0.0};
	double current_period_seconds{0.0};
	double predicted_period_seconds{0.0};
	bool current_is_bound{false};
	bool predicted_is_bound{false};
	const char* predicted_orbit_type{"Unknown"};
	bool causes_plunge{false};
	bool causes_escape{false};
};

struct PatternGenerationResult {
	bool success{false};
	size_t requested_count{0};
	size_t generated_count{0};
	size_t rejected_count{0};
	double total_mass{0.0};
	double min_distance{1e30};
	double max_distance{0.0};
	double min_speed{1e30};
	double max_speed{0.0};
	std::vector<uint32_t> injected_ids{};
};

class BodyManagerContext {
public:
	static constexpr int kCentralSourceId = -2;
	static constexpr int kNoSelection = -1;

private:
	Orchestrator::SimulationOrchestrator<1024>& orchestrator_;
	int selected_object_id_{kNoSelection};
	int selected_source_id_{kCentralSourceId};
	std::vector<int> selected_object_ids_{};
	std::vector<int> selected_source_ids_{};
	int tracked_body_id_{kNoSelection};
	bool tracking_enabled_{false};
	std::optional<BodyManagerTab> requested_tab_{};
	std::mt19937_64 rng_{std::random_device{}()};
	bool pattern_generator_requested_{false};
	std::vector<uint32_t> last_injected_batch_ids_{};
	PatternGenerationResult last_generation_result_{};
	bool last_generation_active_{false};

	template <typename Engine>
	[[nodiscard]] static double engine_speed_scale(const Engine& engine) noexcept {
		if constexpr (requires { engine.speed_scale(); }) {
			return static_cast<double>(engine.speed_scale());
		} else if constexpr (requires { engine.velocity_scale(); }) {
			return static_cast<double>(engine.velocity_scale());
		} else if constexpr (requires { engine.time_scale(); }) {
			const double ts = static_cast<double>(engine.time_scale());
			return (ts > 0.0) ? (static_cast<double>(engine.length_scale()) / ts) : 299792458.0;
		} else if constexpr (requires { engine.speed_of_light(); }) {
			return static_cast<double>(engine.speed_of_light());
		} else if constexpr (requires { engine.c(); }) {
			return static_cast<double>(engine.c());
		} else {
			return 299792458.0;
		}
	}

public:
	explicit BodyManagerContext(Orchestrator::SimulationOrchestrator<1024>& orchestrator) noexcept
		: orchestrator_(orchestrator) {}

	[[nodiscard]] Orchestrator::SimulationOrchestrator<1024>& orchestrator() const noexcept {
		return orchestrator_;
	}

	[[nodiscard]] double simulation_g() const noexcept {
		const auto preset = orchestrator_.constants_engine().active_preset();
		if (preset == Core::ConstantsPreset::Custom) {
			const double g = orchestrator_.constants_engine().sim_gravitational_constant();
			return (g > 1e-12) ? g : 1.0;
		}
		return 1.0;
	}

	[[nodiscard]] double simulation_c() const noexcept {
		const auto preset = orchestrator_.constants_engine().active_preset();
		if (preset == Core::ConstantsPreset::Custom) {
			const double c = orchestrator_.constants_engine().sim_speed_of_light();
			return (c > 1e-12) ? c : 1.0;
		}
		return 1.0;
	}

	[[nodiscard]] double mass_scale() const noexcept {
		const double ms = orchestrator_.constants_engine().mass_scale();
		return (ms > 1e-30 && ms < 1e50 && std::abs(ms - 1.0) > 1e-12) ? ms : 1.98847e30;
	}

	[[nodiscard]] double length_scale() const noexcept {
		const double ls = orchestrator_.constants_engine().length_scale();
		return (ls > 1e-30 && ls < 1e50 && std::abs(ls - 1.0) > 1e-12) ? ls : 1476.62506;
	}

	[[nodiscard]] double speed_scale() const noexcept {
		return 299792458.0;
	}

	[[nodiscard]] int selected_object_id() const noexcept {
		return selected_object_id_;
	}

	[[nodiscard]] int selected_source_id() const noexcept {
		return selected_source_id_;
	}

	[[nodiscard]] const std::vector<int>& selected_object_ids() const noexcept {
		return selected_object_ids_;
	}

	[[nodiscard]] const std::vector<int>& selected_source_ids() const noexcept {
		return selected_source_ids_;
	}

	[[nodiscard]] bool is_object_selected(int id) const noexcept {
		return std::find(selected_object_ids_.begin(), selected_object_ids_.end(), id) != selected_object_ids_.end();
	}

	[[nodiscard]] bool is_source_selected(int id) const noexcept {
		if (id == kCentralSourceId) {
			return selected_source_id_ == kCentralSourceId && selected_source_ids_.empty();
		}
		return std::find(selected_source_ids_.begin(), selected_source_ids_.end(), id) != selected_source_ids_.end();
	}

	void clear_object_selection() noexcept {
		selected_object_id_ = kNoSelection;
		selected_object_ids_.clear();
	}

	void clear_source_selection() noexcept {
		selected_source_id_ = kCentralSourceId;
		selected_source_ids_.clear();
	}

	void set_selected_objects(const std::vector<int>& ids) noexcept {
		selected_object_ids_ = ids;
		selected_object_id_ = ids.empty() ? kNoSelection : ids.front();
	}

	void set_selected_sources(const std::vector<int>& ids) noexcept {
		selected_source_ids_ = ids;
		selected_source_id_ = ids.empty() ? kCentralSourceId : ids.front();
	}

	void select_object(int id, bool additive = false, bool range = false, const std::vector<int>& visible_ids = {}) noexcept {
		if (id < 0) {
			clear_object_selection();
			return;
		}
		if (range && selected_object_id_ >= 0 && !visible_ids.empty()) {
			auto start_it = std::find(visible_ids.begin(), visible_ids.end(), selected_object_id_);
			auto end_it = std::find(visible_ids.begin(), visible_ids.end(), id);
			if (start_it != visible_ids.end() && end_it != visible_ids.end()) {
				if (start_it > end_it) std::swap(start_it, end_it);
				if (!additive) selected_object_ids_.clear();
				for (auto it = start_it; it <= end_it; ++it) {
					if (std::find(selected_object_ids_.begin(), selected_object_ids_.end(), *it) == selected_object_ids_.end()) {
						selected_object_ids_.push_back(*it);
					}
				}
				selected_object_id_ = id;
				return;
			}
		}
		if (additive) {
			auto it = std::find(selected_object_ids_.begin(), selected_object_ids_.end(), id);
			if (it != selected_object_ids_.end()) {
				selected_object_ids_.erase(it);
				selected_object_id_ = selected_object_ids_.empty() ? kNoSelection : selected_object_ids_.back();
			} else {
				selected_object_ids_.push_back(id);
				selected_object_id_ = id;
			}
			return;
		}
		selected_object_id_ = id;
		selected_object_ids_ = {id};
	}

	void select_source(int id, bool additive = false, bool range = false, const std::vector<int>& visible_ids = {}) noexcept {
		if (id == kCentralSourceId) {
			clear_source_selection();
			return;
		}
		if (id < 0) {
			clear_source_selection();
			return;
		}
		if (range && selected_source_id_ >= 0 && selected_source_id_ != kCentralSourceId && !visible_ids.empty()) {
			auto start_it = std::find(visible_ids.begin(), visible_ids.end(), selected_source_id_);
			auto end_it = std::find(visible_ids.begin(), visible_ids.end(), id);
			if (start_it != visible_ids.end() && end_it != visible_ids.end()) {
				if (start_it > end_it) std::swap(start_it, end_it);
				if (!additive) selected_source_ids_.clear();
				for (auto it = start_it; it <= end_it; ++it) {
					if (std::find(selected_source_ids_.begin(), selected_source_ids_.end(), *it) == selected_source_ids_.end()) {
						selected_source_ids_.push_back(*it);
					}
				}
				selected_source_id_ = id;
				return;
			}
		}
		if (additive) {
			auto it = std::find(selected_source_ids_.begin(), selected_source_ids_.end(), id);
			if (it != selected_source_ids_.end()) {
				selected_source_ids_.erase(it);
				selected_source_id_ = selected_source_ids_.empty() ? kCentralSourceId : selected_source_ids_.back();
			} else {
				selected_source_ids_.push_back(id);
				selected_source_id_ = id;
			}
			return;
		}
		selected_source_id_ = id;
		selected_source_ids_ = {id};
	}

	void select_and_open_source(int id) noexcept {
		select_source(id);
		requested_tab_ = BodyManagerTab::SourceCatalog;
	}

	void select_and_open_object(int id) noexcept {
		select_object(id);
		requested_tab_ = BodyManagerTab::ObjectCatalog;
	}

	void look_at_selection(const std::vector<Dynamics::PostNewtonianBody*>& selection) noexcept {
		if (selection.empty()) return;
		std::array<double, 3> centroid{0.0, 0.0, 0.0};
		double total_mass = 0.0;
		for (const auto* b : selection) {
			if (b == nullptr) continue;
			const double m = std::max(b->mass, 1e-12);
			centroid[0] += b->position[0] * m;
			centroid[1] += b->position[1] * m;
			centroid[2] += b->position[2] * m;
			total_mass += m;
		}
		if (total_mass > 0.0) {
			centroid[0] /= total_mass;
			centroid[1] /= total_mass;
			centroid[2] /= total_mass;
		} else {
			for (const auto* b : selection) {
				if (b == nullptr) continue;
				centroid[0] += b->position[0];
				centroid[1] += b->position[1];
				centroid[2] += b->position[2];
			}
			const double n = static_cast<double>(selection.size());
			centroid[0] /= n;
			centroid[1] /= n;
			centroid[2] /= n;
		}
		look_at(centroid);
	}

	void request_tab(BodyManagerTab tab) noexcept {
		requested_tab_ = tab;
	}

	[[nodiscard]] std::optional<BodyManagerTab> take_requested_tab() noexcept {
		const std::optional<BodyManagerTab> tab = requested_tab_;
		requested_tab_.reset();
		return tab;
	}

	[[nodiscard]] bool is_tracking(int id) const noexcept {
		return tracking_enabled_ && tracked_body_id_ == id;
	}

	[[nodiscard]] bool tracking_active() const noexcept {
		return tracking_enabled_ && tracked_body_id_ != kNoSelection;
	}

	[[nodiscard]] int tracked_id() const noexcept {
		return tracking_enabled_ ? tracked_body_id_ : kNoSelection;
	}

	void stop_tracking() noexcept {
		tracking_enabled_ = false;
		tracked_body_id_ = kNoSelection;
	}

	[[nodiscard]] std::string tracked_target_name() const {
		if (!tracking_enabled_ || tracked_body_id_ == kNoSelection) {
			return {};
		}
		if (tracked_body_id_ == kCentralSourceId) {
			return "Central Spacetime Source";
		}
		auto& sys = orchestrator_.nbody_system();
		std::lock_guard<std::recursive_mutex> lock(sys.bodies_mutex());
		const auto* b = find_body(static_cast<uint32_t>(tracked_body_id_));
		if (b != nullptr) {
			return display_name(*b);
		}
		return "Body #" + std::to_string(tracked_body_id_);
	}

	void set_tracking(int id, bool enabled) noexcept {
		tracking_enabled_ = enabled;
		tracked_body_id_ = enabled ? id : kNoSelection;
	}

	void toggle_tracking(int id) noexcept {
		set_tracking(id, !is_tracking(id));
	}

	void look_at(const std::array<double, 3>& target) noexcept {
		auto& camera = orchestrator_.camera();
		const double dx = target[0] - camera.position[0];
		const double dy = target[1] - camera.position[1];
		const double dz = target[2] - camera.position[2];
		const double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
		if (distance <= 1e-9) return;
		camera.target = target;
		camera.yaw = std::atan2(dy, dx) * (180.0 / std::numbers::pi);
		camera.pitch = std::asin(std::clamp(dz / distance, -1.0, 1.0)) * (180.0 / std::numbers::pi);
		camera.roll = 0.0;
	}

	void update_tracking() noexcept {
		if (!tracking_enabled_) return;
		auto& sys = orchestrator_.nbody_system();
		std::lock_guard<std::recursive_mutex> tracking_lock(sys.bodies_mutex());
		if (tracked_body_id_ == kCentralSourceId) {
			look_at({0.0, 0.0, 0.0});
			return;
		}
		for (const auto& body : sys.bodies()) {
			if (body.id == static_cast<uint32_t>(tracked_body_id_) && body.enabled) {
				look_at(body.position);
				return;
			}
		}
		tracking_enabled_ = false;
		tracked_body_id_ = kNoSelection;
	}

	[[nodiscard]] double random_real(double min_val, double max_val) noexcept {
		std::uniform_real_distribution<double> dist(min_val, std::max(min_val, max_val));
		return dist(rng_);
	}

	[[nodiscard]] int random_int(int min_val, int max_val) noexcept {
		std::uniform_int_distribution<int> dist(min_val, max_val);
		return dist(rng_);
	}

	[[nodiscard]] std::string generate_source_name() noexcept {
		char buf[48];
		static constexpr const char* const kConstellations[] = {
			"Cyg", "Sgr", "Vela", "Sco", "Mus", "LMC", "SMC", "Cir", "Her", "Mon", "Cep", "Per", "Oph", "Cru", "Car", "Carina", "Pup", "Pav", "Ara", "Aql", "Aur", "Cam", "Cas", "Cen", "Col", "Com", "CrB", "Crv", "Del", "Dor"
		};
		static constexpr const char* const kSurveys[] = {
			"MAXI", "XTE", "Swift", "GRS", "GRO", "Gaia", "RX", "CXO", "XMM", "INTEGRAL", "Fermi", "HESS", "VERITAS", "MAGIC", "CTA"
		};
		const int mode = random_int(0, 4);
		if (mode == 0) {
			const char* c = kConstellations[static_cast<size_t>(random_int(0, 14))];
			std::snprintf(buf, sizeof(buf), "%s X-%d", c, random_int(1, 9));
		} else if (mode == 1) {
			const char* c = kConstellations[static_cast<size_t>(random_int(0, 14))];
			std::snprintf(buf, sizeof(buf), "V%d %s", random_int(300, 4900), c);
		} else if (mode == 2) {
			const char* s = kSurveys[static_cast<size_t>(random_int(0, 7))];
			if (std::string_view(s) == "Gaia") {
				std::snprintf(buf, sizeof(buf), "Gaia BH%d", random_int(1, 40));
			} else {
				const char sign = (random_int(0, 1) == 0) ? '+' : '-';
				std::snprintf(buf, sizeof(buf), "%s J%02d%02d%c%02d", s, random_int(0, 23), random_int(0, 59), sign, random_int(0, 89));
			}
		} else if (mode == 3) {
			const int cat = random_int(0, 5);
			if (cat == 0) std::snprintf(buf, sizeof(buf), "M%d*", random_int(31, 105));
			else if (cat == 1) std::snprintf(buf, sizeof(buf), "NGC %d-BH", random_int(100, 7800));
			else if (cat == 2) std::snprintf(buf, sizeof(buf), "Mrk %d*", random_int(100, 999));
			else if (cat == 3) std::snprintf(buf, sizeof(buf), "PKS %02d%02d-%02d", random_int(0, 23), random_int(0, 59), random_int(0, 89));
			else if (cat == 4) std::snprintf(buf, sizeof(buf), "3C %d", random_int(48, 454));
			else std::snprintf(buf, sizeof(buf), "TON %d", random_int(100, 900));
		} else {
			const char sign = (random_int(0, 1) == 0) ? '+' : '-';
			std::snprintf(buf, sizeof(buf), "BH-J%02d%02d%c%02d", random_int(0, 23), random_int(0, 59), sign, random_int(0, 89));
		}
		return std::string(buf);
	}

	[[nodiscard]] std::string generate_object_name(Dynamics::Body3DPreset preset = Dynamics::Body3DPreset::Custom) noexcept {
		char buf[48];
		static constexpr const char* const kGreek[] = {
			"Alpha", "Beta", "Gamma", "Delta", "Epsilon", "Zeta", "Eta", "Theta", "Iota", "Kappa", "Lambda", "Mu", "Nu", "Xi", "Pi", "Rho", "Sigma", "Tau", "Upsilon", "Phi", "Chi", "Psi", "Omega"
		};
		static constexpr const char* const kConst[] = {
			"Cen", "Eri", "Pav", "Pic", "Ret", "Cru", "Cyg", "Sgr", "Ori", "Lyr", "Sco", "Car", "Cet", "And", "Cas", "Leo", "UMa", "Dra", "Aql", "Peg", "Pup", "Vel", "CMa", "Boo", "Mus", "Tuc", "Dor"
		};
		switch (preset) {
			case Dynamics::Body3DPreset::Star: {
				const int mode = random_int(0, 4);
				if (mode == 0) {
					std::snprintf(buf, sizeof(buf), "%s %s", kGreek[static_cast<size_t>(random_int(0, 22))], kConst[static_cast<size_t>(random_int(0, 26))]);
				} else if (mode == 1) {
					std::snprintf(buf, sizeof(buf), "HD %d", random_int(1000, 225000));
				} else if (mode == 2) {
					std::snprintf(buf, sizeof(buf), "HIP %d", random_int(1000, 120000));
				} else if (mode == 3) {
					std::snprintf(buf, sizeof(buf), "GJ %d", random_int(10, 4200));
				} else {
					std::snprintf(buf, sizeof(buf), "TOI-%d", random_int(100, 4500));
				}
				break;
			}
			case Dynamics::Body3DPreset::TerrestrialPlanet: {
				const char sub = static_cast<char>('b' + random_int(0, 5));
				const int mode = random_int(0, 4);
				if (mode == 0) {
					std::snprintf(buf, sizeof(buf), "Kepler-%d %c", random_int(20, 1700), sub);
				} else if (mode == 1) {
					std::snprintf(buf, sizeof(buf), "TOI-%d %c", random_int(100, 3500), sub);
				} else if (mode == 2) {
					std::snprintf(buf, sizeof(buf), "GJ %d %c", random_int(100, 3500), sub);
				} else if (mode == 3) {
					std::snprintf(buf, sizeof(buf), "K2-%d %c", random_int(10, 350), sub);
				} else {
					std::snprintf(buf, sizeof(buf), "LHS %d %c", random_int(100, 3900), sub);
				}
				break;
			}
			case Dynamics::Body3DPreset::GasGiant: {
				const char sub = static_cast<char>('b' + random_int(0, 3));
				const int mode = random_int(0, 4);
				if (mode == 0) {
					std::snprintf(buf, sizeof(buf), "WASP-%d %c", random_int(1, 190), sub);
				} else if (mode == 1) {
					std::snprintf(buf, sizeof(buf), "HAT-P-%d %c", random_int(1, 70), sub);
				} else if (mode == 2) {
					std::snprintf(buf, sizeof(buf), "HD %d %c", random_int(10000, 220000), sub);
				} else if (mode == 3) {
					std::snprintf(buf, sizeof(buf), "KELT-%d %c", random_int(1, 26), sub);
				} else {
					std::snprintf(buf, sizeof(buf), "CoRoT-%d %c", random_int(1, 35), sub);
				}
				break;
			}
			case Dynamics::Body3DPreset::IceGiant: {
				const char sub = static_cast<char>('b' + random_int(0, 3));
				const int mode = random_int(0, 2);
				if (mode == 0) {
					std::snprintf(buf, sizeof(buf), "HIP %d %c", random_int(1000, 110000), sub);
				} else if (mode == 1) {
					std::snprintf(buf, sizeof(buf), "GJ %d %c", random_int(100, 3500), sub);
				} else {
					std::snprintf(buf, sizeof(buf), "K2-%d %c", random_int(10, 300), sub);
				}
				break;
			}
			case Dynamics::Body3DPreset::Metallic:
			case Dynamics::Body3DPreset::Asteroid: {
				static constexpr const char* const kAsteroidNames[] = {
					"Ceres", "Pallas", "Juno", "Vesta", "Astraea", "Hebe", "Iris", "Flora", "Metis", "Hygiea",
					"Parthenope", "Victoria", "Egeria", "Irene", "Eunomia", "Psyche", "Thetis", "Melpomene", "Fortuna",
					"Massalia", "Lutetia", "Kalliope", "Thalia", "Themis", "Proserpina", "Bellona", "Amphitrite",
					"Urania", "Euphrosyne", "Pomona", "Polyhymnia", "Circe", "Leukothea", "Atalante", "Fides", "Leda",
					"Laetitia", "Harmonia", "Daphne", "Isis", "Ariadne", "Nysa", "Eugenia", "Hestia", "Aglaia", "Doris",
					"Pales", "Virginia", "Nemausa", "Europa", "Kalypso", "Alexandra", "Pandora", "Melete", "Mnemosyne",
					"Concordia", "Olympia", "Erato", "Ausonia", "Danae", "Niobe", "Feronia", "Klytia", "Galatea", "Euridike",
					"Freia", "Frigga", "Diana", "Eurynome", "Sappho", "Terpsichore", "Alkmene", "Clio", "Io", "Semele"
				};
				const int mode = random_int(0, 2);
				if (mode == 0) {
					const size_t idx = static_cast<size_t>(random_int(0, 74));
					std::snprintf(buf, sizeof(buf), "(%d) %s", random_int(1, 9999), kAsteroidNames[idx]);
				} else if (mode == 1) {
					static constexpr const char* const kLetters = "ABCDEFGHJKLMNOPQRSTUVWXYZ";
					const char l1 = kLetters[static_cast<size_t>(random_int(0, 24))];
					const char l2 = kLetters[static_cast<size_t>(random_int(0, 24))];
					std::snprintf(buf, sizeof(buf), "%d %c%c%d", random_int(1995, 2025), l1, l2, random_int(1, 99));
				} else {
					std::snprintf(buf, sizeof(buf), "Ast-%d", random_int(100, 9999));
				}
				break;
			}
			case Dynamics::Body3DPreset::NeutronStar:
			case Dynamics::Body3DPreset::Pulsar: {
				const char sign = (random_int(0, 1) == 0) ? '+' : '-';
				const int mode = random_int(0, 2);
				if (mode == 0) {
					std::snprintf(buf, sizeof(buf), "PSR J%02d%02d%c%02d", random_int(0, 23), random_int(0, 59), sign, random_int(0, 89));
				} else if (mode == 1) {
					std::snprintf(buf, sizeof(buf), "PSR B%02d%02d%c%02d", random_int(0, 23), random_int(0, 59), sign, random_int(0, 89));
				} else {
					std::snprintf(buf, sizeof(buf), "SGR %02d%02d%c%02d", random_int(0, 23), random_int(0, 59), sign, random_int(0, 89));
				}
				break;
			}
			case Dynamics::Body3DPreset::BlackHole:
				return generate_source_name();
			case Dynamics::Body3DPreset::Custom:
			default:
				std::snprintf(buf, sizeof(buf), "EXO-%04d", random_int(100, 9999));
				break;
		}
		return std::string(buf);
	}

	[[nodiscard]] std::string generate_procedural_name(const Dynamics::PostNewtonianBody& body) noexcept {
		if (body.is_spacetime_source) {
			return generate_source_name();
		}
		return generate_object_name(body.preset_3d);
	}

	[[nodiscard]] double sample_log_uniform(double min_val, double max_val) noexcept {
		return std::pow(10.0, random_real(std::log10(min_val), std::log10(max_val)));
	}

	[[nodiscard]] double sample_signed_uniform(double min_abs, double max_abs) noexcept {
		return (random_int(0, 1) == 0 ? -1.0 : 1.0) * random_real(min_abs, max_abs);
	}

	[[nodiscard]] double sample_uniform(double min_val, double max_val) noexcept {
		return random_real(min_val, max_val);
	}

	[[nodiscard]] double sample_gaussian(double mean, double stddev) noexcept {
		const double u1 = std::max(random_real(0.0, 1.0), 1e-15);
		const double u2 = random_real(0.0, 1.0);
		const double z0 = std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * std::numbers::pi * u2);
		return mean + stddev * z0;
	}

	[[nodiscard]] double sample_rayleigh(double sigma) noexcept {
		const double u = std::max(random_real(0.0, 1.0), 1e-15);
		return sigma * std::sqrt(-2.0 * std::log(u));
	}

	[[nodiscard]] double sample_maxwell_boltzmann(double a) noexcept {
		const double x = sample_gaussian(0.0, a);
		const double y = sample_gaussian(0.0, a);
		const double z = sample_gaussian(0.0, a);
		return std::sqrt(x * x + y * y + z * z);
	}

	[[nodiscard]] double sample_power_law(double min_val, double max_val, double alpha) noexcept {
		const double safe_min = std::max(min_val, 1e-15);
		const double safe_max = std::max(max_val, safe_min * 1.0001);
		if (std::abs(alpha - 1.0) < 1e-5) {
			return sample_log_uniform(safe_min, safe_max);
		}
		const double p = 1.0 - alpha;
		const double u = random_real(0.0, 1.0);
		const double v = std::pow(safe_min, p) + u * (std::pow(safe_max, p) - std::pow(safe_min, p));
		return std::pow(std::max(v, 1e-300), 1.0 / p);
	}

	void request_pattern_generator() noexcept {
		pattern_generator_requested_ = true;
		request_tab(BodyManagerTab::ObjectCreation);
	}

	[[nodiscard]] bool consume_pattern_generator_request() noexcept {
		const bool req = pattern_generator_requested_;
		pattern_generator_requested_ = false;
		return req;
	}

	[[nodiscard]] bool has_last_pattern_batch() const noexcept {
		return !last_injected_batch_ids_.empty();
	}

	[[nodiscard]] size_t last_pattern_batch_size() const noexcept {
		return last_injected_batch_ids_.size();
	}

	[[nodiscard]] const PatternGenerationResult& last_generation_result() const noexcept {
		return last_generation_result_;
	}

	void select_last_pattern_batch() noexcept {
		if (last_injected_batch_ids_.empty()) return;
		std::vector<int> int_ids;
		int_ids.reserve(last_injected_batch_ids_.size());
		for (uint32_t id : last_injected_batch_ids_) {
			int_ids.push_back(static_cast<int>(id));
		}
		set_selected_objects(int_ids);
	}

	void undo_last_pattern_injection() noexcept {
		if (last_injected_batch_ids_.empty()) return;
		auto& sys = orchestrator_.nbody_system();
		std::lock_guard<std::recursive_mutex> lock(sys.bodies_mutex());
		for (uint32_t id : last_injected_batch_ids_) {
			orchestrator_.surface_layers().erase(id);
			sys.remove_body(id);
		}
		sys.update_accelerations();
		orchestrator_.notify_state_changed();
		last_injected_batch_ids_.clear();
		last_generation_active_ = false;
	}

	PatternGenerationResult inject_geometric_pattern(const GeometricPatternConfig& config) noexcept {
		PatternGenerationResult result;
		GeometricPatternConfig cfg = config;
		cfg.sanitize();
		result.requested_count = static_cast<size_t>(cfg.object_count);

		if (cfg.seed > 0) {
			rng_.seed(cfg.seed);
		}

		auto& sys = orchestrator_.nbody_system();
		std::lock_guard<std::recursive_mutex> lock(sys.bodies_mutex());

		std::array<double, 3> parent_pos{0.0, 0.0, 0.0};
		std::array<double, 3> parent_vel{0.0, 0.0, 0.0};
		double parent_mass = std::max(orchestrator_.parameters().mass, 1e-12);
		double parent_horizon = primary_horizon_radius();

		if (cfg.parent_body_id != kCentralSourceId) {
			const auto* p_body = find_body(static_cast<uint32_t>(cfg.parent_body_id));
			if (p_body != nullptr) {
				parent_pos = p_body->position;
				parent_vel = p_body->velocity;
				parent_mass = std::max(p_body->mass, 1e-12);
				parent_horizon = p_body->is_spacetime_source ? p_body->kerr_outer_horizon_radius() : p_body->radius;
			}
		}

		const std::array<double, 3> center{
			parent_pos[0] + cfg.center_position[0],
			parent_pos[1] + cfg.center_position[1],
			parent_pos[2] + cfg.center_position[2]
		};
		const std::array<double, 3> center_v{
			parent_vel[0] + cfg.center_velocity[0],
			parent_vel[1] + cfg.center_velocity[1],
			parent_vel[2] + cfg.center_velocity[2]
		};

		const double i_rad = cfg.plane_inclination_deg * (std::numbers::pi / 180.0);
		const double omega_rad = cfg.plane_longitude_node_deg * (std::numbers::pi / 180.0);
		const double w_rad = cfg.plane_argument_periapsis_deg * (std::numbers::pi / 180.0);
		const double sin_i = std::sin(i_rad);
		const double cos_i = std::cos(i_rad);
		const double sin_omega = std::sin(omega_rad);
		const double cos_omega = std::cos(omega_rad);

		const std::array<double, 3> n_hat{sin_i * sin_omega, -sin_i * cos_omega, cos_i};
		const std::array<double, 3> u_hat{cos_omega, sin_omega, 0.0};
		const std::array<double, 3> v_hat{-cos_i * sin_omega, cos_i * cos_omega, sin_i};

		const double g_sim = orchestrator_.physical_gravitational_constant();
		const double m_scale = mass_scale();
		const double l_scale = length_scale();
		const double prim_h = primary_horizon_radius();

		std::vector<Dynamics::PostNewtonianBody> generated_bodies;
		generated_bodies.reserve(static_cast<size_t>(cfg.object_count));

		for (int32_t k = 0; k < cfg.object_count; ++k) {
			double r = cfg.mean_semi_major_axis;
			if (cfg.pattern_type == PatternGeometryType::AsteroidBelt) {
				if (cfg.radial_distribution == RadialDistributionLaw::Gaussian) {
					r = cfg.mean_semi_major_axis + sample_gaussian(0.0, cfg.radial_gaussian_sigma);
				} else if (cfg.radial_distribution == RadialDistributionLaw::PowerLaw) {
					const double r_min = std::max(cfg.mean_semi_major_axis - 0.5 * cfg.radial_thickness, 0.5);
					const double r_max = cfg.mean_semi_major_axis + 0.5 * cfg.radial_thickness;
					r = sample_power_law(r_min, r_max, cfg.radial_power_law_index);
				} else if (cfg.radial_distribution == RadialDistributionLaw::KirkwoodResonant) {
					r = cfg.mean_semi_major_axis + sample_signed_uniform(0.0, 0.5 * cfg.radial_thickness);
					if (cfg.kirkwood_gaps_enabled) {
						for (int32_t g = 1; g <= cfg.kirkwood_gap_count; ++g) {
							const double gap_frac = -0.38 + 0.76 * (static_cast<double>(g) / static_cast<double>(cfg.kirkwood_gap_count + 1));
							const double gap_r = cfg.mean_semi_major_axis + gap_frac * cfg.radial_thickness;
							if (std::abs(r - gap_r) < 0.035 * cfg.radial_thickness && random_real(0.0, 1.0) < cfg.kirkwood_resonance_depth) {
								r = cfg.mean_semi_major_axis + sample_signed_uniform(0.0, 0.5 * cfg.radial_thickness);
							}
						}
					}
				} else {
					r = cfg.mean_semi_major_axis + sample_signed_uniform(0.0, 0.5 * cfg.radial_thickness);
				}
			} else if (cfg.pattern_type == PatternGeometryType::DebrisSwarm) {
				if (cfg.radial_distribution == RadialDistributionLaw::Gaussian) {
					r = cfg.mean_semi_major_axis + sample_gaussian(0.0, cfg.radial_thickness * 0.35);
				} else if (cfg.radial_distribution == RadialDistributionLaw::PowerLaw) {
					const double r_min = std::max(cfg.mean_semi_major_axis - 0.5 * cfg.radial_thickness, 0.5);
					const double r_max = cfg.mean_semi_major_axis + 0.5 * cfg.radial_thickness;
					r = sample_power_law(r_min, r_max, cfg.radial_power_law_index);
				} else {
					r = cfg.mean_semi_major_axis + sample_signed_uniform(0.0, 0.5 * cfg.radial_thickness);
				}
			} else {
				if (cfg.spherical_type == SphericalFormationType::ThinShell) {
					r = cfg.thin_shell_radius + sample_gaussian(0.0, 0.01 * cfg.thin_shell_radius);
				} else if (cfg.spherical_type == SphericalFormationType::ThickShell) {
					r = sample_power_law(cfg.shell_inner_radius, cfg.shell_outer_radius, -cfg.spherical_power_law_gamma);
				} else if (cfg.spherical_type == SphericalFormationType::PlummerSphere) {
					const double q = sample_uniform(0.001, 0.995);
					r = cfg.plummer_scale_radius * std::cbrt(q) / std::sqrt(std::max(1.0 - std::pow(q, 2.0 / 3.0), 1e-6));
				} else if (cfg.spherical_type == SphericalFormationType::UniformSphere) {
					const double q = sample_uniform(0.0001, 1.0);
					r = cfg.uniform_sphere_radius * std::cbrt(q);
				} else if (cfg.spherical_type == SphericalFormationType::FibonacciShell) {
					r = cfg.thin_shell_radius;
				} else {
					r = sample_uniform(cfg.shell_inner_radius, cfg.shell_outer_radius);
				}
			}
			r = std::max(r, 0.1);

			std::array<double, 3> pos_rel{0.0, 0.0, 0.0};
			double theta_orb = 0.0;

			if (cfg.pattern_type == PatternGeometryType::SphericalFormation) {
				if (cfg.spherical_type == SphericalFormationType::FibonacciShell) {
					const double y_unit = 1.0 - (static_cast<double>(k) / static_cast<double>(std::max(cfg.object_count - 1, 1))) * 2.0;
					const double rad_unit = std::sqrt(std::max(1.0 - y_unit * y_unit, 0.0));
					const double phi_golden = std::numbers::pi * (3.0 - std::sqrt(5.0)) * static_cast<double>(k);
					pos_rel = {r * rad_unit * std::cos(phi_golden), r * rad_unit * std::sin(phi_golden), r * y_unit};
				} else {
					const double cos_t = sample_uniform(-1.0, 1.0);
					const double sin_t = std::sqrt(std::max(1.0 - cos_t * cos_t, 0.0));
					const double phi_s = sample_uniform(0.0, 2.0 * std::numbers::pi);
					pos_rel = {r * sin_t * std::cos(phi_s), r * sin_t * std::sin(phi_s), r * cos_t};
				}
			} else {
				if (cfg.angular_distribution == AngularDistributionLaw::ClusteredLagrange) {
					const double lagrange_base = (random_int(0, 1) == 0 ? 1.0 : -1.0) * (std::numbers::pi / 3.0);
					theta_orb = lagrange_base + sample_gaussian(0.0, 0.3 * (1.0 - cfg.lagrangian_clustering_strength));
				} else if (cfg.pattern_type == PatternGeometryType::DebrisSwarm && cfg.angular_opening_deg < 359.0) {
					const double half_open = (cfg.angular_opening_deg * (std::numbers::pi / 180.0)) * 0.5;
					theta_orb = (cfg.angular_center_deg * (std::numbers::pi / 180.0)) + sample_gaussian(0.0, half_open * 0.45);
				} else {
					theta_orb = sample_uniform(0.0, 2.0 * std::numbers::pi);
				}

				double z_off = 0.0;
				if (cfg.vertical_distribution == DispersionDistributionLaw::Uniform) {
					z_off = sample_signed_uniform(0.0, cfg.vertical_scale_height);
				} else {
					z_off = sample_gaussian(0.0, cfg.vertical_scale_height);
				}

				const double cos_th = std::cos(theta_orb);
				const double sin_th = std::sin(theta_orb);
				pos_rel = {
					r * (cos_th * u_hat[0] + sin_th * v_hat[0]) + z_off * n_hat[0],
					r * (cos_th * u_hat[1] + sin_th * v_hat[1]) + z_off * n_hat[1],
					r * (cos_th * u_hat[2] + sin_th * v_hat[2]) + z_off * n_hat[2]
				};
			}

			const std::array<double, 3> pos_world{
				center[0] + pos_rel[0],
				center[1] + pos_rel[1],
				center[2] + pos_rel[2]
			};

			const double d_to_prim = std::sqrt(pos_world[0] * pos_world[0] + pos_world[1] * pos_world[1] + pos_world[2] * pos_world[2]);
			if (cfg.avoid_horizon_overlap && prim_h > 0.0 && d_to_prim <= prim_h * 1.35) {
				++result.rejected_count;
				continue;
			}
			const double d_to_parent = std::sqrt(pos_rel[0] * pos_rel[0] + pos_rel[1] * pos_rel[1] + pos_rel[2] * pos_rel[2]);
			if (cfg.avoid_horizon_overlap && parent_horizon > 0.0 && d_to_parent <= parent_horizon * 1.35) {
				++result.rejected_count;
				continue;
			}

			double m_body = cfg.fixed_mass;
			if (cfg.mass_distribution == MassDistributionLaw::Uniform) {
				m_body = sample_uniform(cfg.min_mass, cfg.max_mass);
			} else if (cfg.mass_distribution == MassDistributionLaw::LogUniform) {
				m_body = sample_log_uniform(cfg.min_mass, cfg.max_mass);
			} else if (cfg.mass_distribution == MassDistributionLaw::PowerLaw) {
				m_body = sample_power_law(cfg.min_mass, cfg.max_mass, cfg.mass_power_law_alpha);
			}

			double radius = cfg.fixed_radius;
			if (cfg.radius_from_density) {
				const double density = std::max(cfg.mean_density * (1.0 + sample_gaussian(0.0, 0.01 * cfg.density_dispersion_pct)), 10.0);
				const double mass_kg = m_body * m_scale;
				const double vol = mass_kg / density;
				const double r_m = std::cbrt(3.0 * vol / (4.0 * std::numbers::pi));
				radius = std::clamp(r_m / l_scale, cfg.min_radius, cfg.max_radius);
			} else {
				radius = sample_log_uniform(cfg.min_radius, cfg.max_radius);
			}

			if (cfg.avoid_mutual_collisions && !generated_bodies.empty()) {
				bool collided = false;
				for (const auto& prev : generated_bodies) {
					const double dx = pos_world[0] - prev.position[0];
					const double dy = pos_world[1] - prev.position[1];
					const double dz = pos_world[2] - prev.position[2];
					if (dx * dx + dy * dy + dz * dz < (radius + prev.radius) * (radius + prev.radius) * 1.3) {
						collided = true;
						break;
					}
				}
				if (collided) {
					++result.rejected_count;
					continue;
				}
			}

			const double mu = g_sim * (parent_mass + m_body);
			std::array<double, 3> vel_rel{0.0, 0.0, 0.0};

			if (cfg.velocity_mode == VelocityInjectionMode::KeplerianCircular || cfg.velocity_mode == VelocityInjectionMode::KeplerianEccentric) {
				double e = 0.0;
				if (cfg.velocity_mode == VelocityInjectionMode::KeplerianEccentric) {
					if (cfg.eccentricity_distribution == DispersionDistributionLaw::Gaussian) {
						e = std::clamp(cfg.mean_eccentricity + sample_gaussian(0.0, cfg.eccentricity_dispersion), 0.0, 0.99);
					} else if (cfg.eccentricity_distribution == DispersionDistributionLaw::Uniform) {
						e = std::clamp(cfg.mean_eccentricity + sample_signed_uniform(0.0, cfg.eccentricity_dispersion), 0.0, 0.99);
					} else {
						e = std::clamp(cfg.mean_eccentricity + sample_rayleigh(cfg.eccentricity_dispersion), 0.0, 0.99);
					}
				}

				const double nu = theta_orb - w_rad;
				const double cos_nu = std::cos(nu);
				const double sin_nu = std::sin(nu);
				const double semi_a = r * (1.0 + e * cos_nu) / std::max(1.0 - e * e, 1e-4);
				const double h_orb = std::sqrt(std::max(mu * semi_a * (1.0 - e * e), 1e-12));

				const double vr = (mu / h_orb) * e * sin_nu;
				const double vt = (h_orb / r);
				const double orb_sign = (random_real(0.0, 1.0) < cfg.prograde_fraction) ? 1.0 : -1.0;

				const double cos_th = std::cos(theta_orb);
				const double sin_th = std::sin(theta_orb);
				const std::array<double, 3> e_r{cos_th * u_hat[0] + sin_th * v_hat[0], cos_th * u_hat[1] + sin_th * v_hat[1], cos_th * u_hat[2] + sin_th * v_hat[2]};
				const std::array<double, 3> e_t{-sin_th * u_hat[0] + cos_th * v_hat[0], -sin_th * u_hat[1] + cos_th * v_hat[1], -sin_th * u_hat[2] + cos_th * v_hat[2]};

				vel_rel = {
					(vr * e_r[0] + (orb_sign * vt) * e_t[0]) * cfg.mean_velocity_multiplier,
					(vr * e_r[1] + (orb_sign * vt) * e_t[1]) * cfg.mean_velocity_multiplier,
					(vr * e_r[2] + (orb_sign * vt) * e_t[2]) * cfg.mean_velocity_multiplier
				};

				if (cfg.velocity_dispersion_fraction > 0.0) {
					const double v_circ = std::sqrt(mu / r);
					const double v_disp_mag = v_circ * cfg.velocity_dispersion_fraction;
					vel_rel[0] += sample_gaussian(0.0, v_disp_mag * 0.577);
					vel_rel[1] += sample_gaussian(0.0, v_disp_mag * 0.577);
					vel_rel[2] += sample_gaussian(0.0, v_disp_mag * 0.577);
				}
			} else if (cfg.velocity_mode == VelocityInjectionMode::RadialExpansion) {
				const double len = std::sqrt(pos_rel[0] * pos_rel[0] + pos_rel[1] * pos_rel[1] + pos_rel[2] * pos_rel[2]);
				const double inv_len = (len > 1e-9) ? (1.0 / len) : 0.0;
				vel_rel = {pos_rel[0] * inv_len * cfg.expansion_speed, pos_rel[1] * inv_len * cfg.expansion_speed, pos_rel[2] * inv_len * cfg.expansion_speed};
			} else if (cfg.velocity_mode == VelocityInjectionMode::RadialCollapse) {
				const double len = std::sqrt(pos_rel[0] * pos_rel[0] + pos_rel[1] * pos_rel[1] + pos_rel[2] * pos_rel[2]);
				const double v_fall = std::sqrt(2.0 * mu / std::max(len, 1e-6));
				const double inv_len = (len > 1e-9) ? (1.0 / len) : 0.0;
				vel_rel = {-pos_rel[0] * inv_len * v_fall, -pos_rel[1] * inv_len * v_fall, -pos_rel[2] * inv_len * v_fall};
			} else if (cfg.velocity_mode == VelocityInjectionMode::IsotropicVirial) {
				const double v_rms = std::sqrt(mu / (2.0 * std::max(r, 1e-6)));
				vel_rel = {sample_gaussian(0.0, v_rms * 0.577), sample_gaussian(0.0, v_rms * 0.577), sample_gaussian(0.0, v_rms * 0.577)};
			} else if (cfg.velocity_mode == VelocityInjectionMode::SolidBodyRotation) {
				const double rot_tilt = cfg.solid_body_axis_tilt_deg * (std::numbers::pi / 180.0);
				const double rot_az = cfg.solid_body_axis_azimuth_deg * (std::numbers::pi / 180.0);
				const std::array<double, 3> rot_axis{std::sin(rot_tilt) * std::cos(rot_az), std::sin(rot_tilt) * std::sin(rot_az), std::cos(rot_tilt)};
				vel_rel = {
					(rot_axis[1] * pos_rel[2] - rot_axis[2] * pos_rel[1]) * cfg.solid_body_rotation_speed,
					(rot_axis[2] * pos_rel[0] - rot_axis[0] * pos_rel[2]) * cfg.solid_body_rotation_speed,
					(rot_axis[0] * pos_rel[1] - rot_axis[1] * pos_rel[0]) * cfg.solid_body_rotation_speed
				};
			} else {
				vel_rel = cfg.fixed_velocity_vector;
			}

			const std::array<double, 3> vel_world{
				center_v[0] + vel_rel[0],
				center_v[1] + vel_rel[1],
				center_v[2] + vel_rel[2]
			};

			Dynamics::PostNewtonianBody b(0, m_body, radius, pos_world, vel_world, {0.0, 0.0, 0.0});
			b.reference_radius = radius;
			apply_body_appearance_preset(b, cfg.archetype);
			apply_body_material_preset(b, cfg.archetype);

			if (cfg.temperature_equilibrium) {
				b.temperature = cfg.reference_temperature_k * std::pow(std::max(cfg.mean_semi_major_axis / r, 1e-4), cfg.temperature_falloff_exponent);
			}

			if (cfg.color_jitter > 0.0) {
				const float hue_offset = static_cast<float>(sample_signed_uniform(0.0, cfg.color_jitter * 0.5));
				const auto rgb = body_hsv_to_rgb(std::clamp(0.1f + hue_offset, 0.0f, 1.0f), 0.6f, 0.85f);
				b.color = {rgb[0], rgb[1], rgb[2], 1.0f};
			}
			b.surface_noise_scale = static_cast<float>(sample_uniform(4.0, 16.0));
			b.surface_roughness = static_cast<float>(sample_uniform(0.4, 0.95));

			b.set_name(cfg.name_prefix + " " + std::to_string(k + 1));
			generated_bodies.push_back(std::move(b));
		}

		result.injected_ids.reserve(generated_bodies.size());
		for (auto& b : generated_bodies) {
			const double speed = b.speed();
			const double dist = distance_from_center(b);
			result.total_mass += b.mass;
			result.min_distance = std::min(result.min_distance, dist);
			result.max_distance = std::max(result.max_distance, dist);
			result.min_speed = std::min(result.min_speed, speed);
			result.max_speed = std::max(result.max_speed, speed);

			const uint32_t id = sys.add_body(std::move(b));
			orchestrator_.surface_layers().set(id, Dynamics::BodySurfaceLayerSet{});
			result.injected_ids.push_back(id);
		}

		sys.update_accelerations();
		orchestrator_.notify_state_changed();

		result.generated_count = result.injected_ids.size();
		result.success = (result.generated_count > 0);
		last_injected_batch_ids_ = result.injected_ids;
		last_generation_result_ = result;
		last_generation_active_ = true;
		return result;
	}

	[[nodiscard]] static std::string display_name(const Dynamics::PostNewtonianBody& body) {
		if (body.has_name()) {
			return std::string(body.name_view());
		}
		return "Body #" + std::to_string(body.id);
	}

	[[nodiscard]] static double distance_from_center(const Dynamics::PostNewtonianBody& body) noexcept {
		return std::sqrt(body.position[0] * body.position[0] + body.position[1] * body.position[1] + body.position[2] * body.position[2]);
	}

	[[nodiscard]] Dynamics::PostNewtonianBody* find_body(uint32_t id) const noexcept {
		auto& sys = orchestrator_.nbody_system();
		for (auto& body : sys.bodies()) {
			if (body.id == id) {
				return &body;
			}
		}
		return nullptr;
	}

	void count_bodies(size_t& objects, size_t& enabled_objects, size_t& sources, size_t& enabled_sources) const noexcept {
		objects = 0;
		enabled_objects = 0;
		sources = 0;
		enabled_sources = 0;
		auto& sys = orchestrator_.nbody_system();
		std::lock_guard<std::recursive_mutex> lock(sys.bodies_mutex());
		for (const auto& body : sys.bodies()) {
			if (body.is_spacetime_source) {
				++sources;
				if (body.enabled) ++enabled_sources;
			} else {
				++objects;
				if (body.enabled) ++enabled_objects;
			}
		}
	}

	[[nodiscard]] std::string unique_name(std::string_view base, uint32_t ignore_id = 0) const {
		std::string original = base.empty() ? "Body" : std::string(base);
		std::lock_guard<std::recursive_mutex> naming_lock(orchestrator_.nbody_system().bodies_mutex());
		const auto& bodies = orchestrator_.nbody_system().bodies();
		auto exists = [&](std::string_view name) {
			return std::any_of(bodies.begin(), bodies.end(), [&](const auto& b) {
				return b.id != ignore_id && b.name_view() == name;
			});
		};

		if (!exists(original)) return original;

		std::string stem = original;
		uint32_t start_suffix = 2;
		const size_t last_space = original.find_last_of(' ');
		if (last_space != std::string::npos && last_space + 1 < original.size()) {
			bool all_digits = true;
			for (size_t i = last_space + 1; i < original.size(); ++i) {
				if (!std::isdigit(static_cast<unsigned char>(original[i]))) {
					all_digits = false;
					break;
				}
			}
			if (all_digits) {
				char* endp = nullptr;
				const unsigned long parsed = std::strtoul(original.c_str() + last_space + 1, &endp, 10);
				if (parsed > 0) {
					stem = original.substr(0, last_space);
					start_suffix = static_cast<uint32_t>(parsed + 1);
				}
			}
		}

		for (uint32_t suffix = start_suffix; suffix < 1000000; ++suffix) {
			const std::string candidate = stem + " " + std::to_string(suffix);
			if (!exists(candidate)) return candidate;
		}
		return original;
	}

	[[nodiscard]] std::array<double, 3> compute_circular_orbit_velocity(const std::array<double, 3>& pos, double central_mass) const noexcept {
		const double r2 = pos[0] * pos[0] + pos[1] * pos[1] + pos[2] * pos[2];
		const double r = std::sqrt(std::max(r2, 1e-12));
		const double speed = std::sqrt(std::max(simulation_g() * central_mass, 0.0) / r);
		const std::array<double, 3> up{0.0, 0.0, 1.0};
		std::array<double, 3> tangent{
			up[1] * pos[2] - up[2] * pos[1],
			up[2] * pos[0] - up[0] * pos[2],
			up[0] * pos[1] - up[1] * pos[0]
		};
		const double t_len = std::sqrt(tangent[0] * tangent[0] + tangent[1] * tangent[1] + tangent[2] * tangent[2]);
		if (t_len < 1e-9) {
			return {0.0, speed, 0.0};
		}
		return {tangent[0] / t_len * speed, tangent[1] / t_len * speed, tangent[2] / t_len * speed};
	}

	[[nodiscard]] std::array<double, 3> compute_retrograde_circular_orbit_velocity(const std::array<double, 3>& pos, double central_mass) const noexcept {
		const auto prograde = compute_circular_orbit_velocity(pos, central_mass);
		return {-prograde[0], -prograde[1], -prograde[2]};
	}

	[[nodiscard]] std::array<double, 3> compute_circular_satellite_velocity(
		const std::array<double, 3>& sat_pos,
		const std::array<double, 3>& sat_vel,
		const std::array<double, 3>& parent_pos,
		const std::array<double, 3>& parent_vel,
		double parent_mass,
		double sat_mass
	) const noexcept {
		const std::array<double, 3> r_rel{sat_pos[0] - parent_pos[0], sat_pos[1] - parent_pos[1], sat_pos[2] - parent_pos[2]};
		const double d = std::sqrt(r_rel[0] * r_rel[0] + r_rel[1] * r_rel[1] + r_rel[2] * r_rel[2]);
		if (d < 1e-9) return parent_vel;
		const std::array<double, 3> v_rel{sat_vel[0] - parent_vel[0], sat_vel[1] - parent_vel[1], sat_vel[2] - parent_vel[2]};
		const double mu = std::max(simulation_g() * (parent_mass + sat_mass), 1e-14);
		const double v_circ = std::sqrt(mu / d);

		std::array<double, 3> h{
			r_rel[1] * v_rel[2] - r_rel[2] * v_rel[1],
			r_rel[2] * v_rel[0] - r_rel[0] * v_rel[2],
			r_rel[0] * v_rel[1] - r_rel[1] * v_rel[0]
		};
		double h_len = std::sqrt(h[0] * h[0] + h[1] * h[1] + h[2] * h[2]);
		if (h_len < 1e-9) {
			const std::array<double, 3> up{0.0, 0.0, 1.0};
			h = {up[1] * r_rel[2] - up[2] * r_rel[1], up[2] * r_rel[0] - up[0] * r_rel[2], up[0] * r_rel[1] - up[1] * r_rel[0]};
			h_len = std::sqrt(h[0] * h[0] + h[1] * h[1] + h[2] * h[2]);
			if (h_len < 1e-9) {
				h = {1.0, 0.0, 0.0};
				h_len = 1.0;
			}
		}
		const std::array<double, 3> h_hat{h[0] / h_len, h[1] / h_len, h[2] / h_len};
		std::array<double, 3> tangent{
			h_hat[1] * r_rel[2] - h_hat[2] * r_rel[1],
			h_hat[2] * r_rel[0] - h_hat[0] * r_rel[2],
			h_hat[0] * r_rel[1] - h_hat[1] * r_rel[0]
		};
		const double t_len = std::sqrt(tangent[0] * tangent[0] + tangent[1] * tangent[1] + tangent[2] * tangent[2]);
		const std::array<double, 3> t_hat = (t_len > 1e-9) ? std::array<double, 3>{tangent[0] / t_len, tangent[1] / t_len, tangent[2] / t_len} : std::array<double, 3>{0.0, 1.0, 0.0};
		return {parent_vel[0] + v_circ * t_hat[0], parent_vel[1] + v_circ * t_hat[1], parent_vel[2] + v_circ * t_hat[2]};
	}

	[[nodiscard]] std::array<double, 3> compute_escape_velocity(const std::array<double, 3>& pos, const std::array<double, 3>& current_vel, double central_mass) const noexcept {
		const double r = std::sqrt(pos[0] * pos[0] + pos[1] * pos[1] + pos[2] * pos[2]);
		if (r < 1e-12) return {0.0, 0.0, 0.0};
		const double mu = std::max(simulation_g() * central_mass, 0.0);
		const double v_esc = std::sqrt(2.0 * mu / r);
		const double cur_v = std::sqrt(current_vel[0] * current_vel[0] + current_vel[1] * current_vel[1] + current_vel[2] * current_vel[2]);
		if (cur_v > 1e-9) {
			const double scale = v_esc / cur_v;
			return {current_vel[0] * scale, current_vel[1] * scale, current_vel[2] * scale};
		}
		const auto prograde = compute_circular_orbit_velocity(pos, central_mass);
		const double pro_v = std::sqrt(prograde[0] * prograde[0] + prograde[1] * prograde[1] + prograde[2] * prograde[2]);
		if (pro_v > 1e-9) {
			const double scale = v_esc / pro_v;
			return {prograde[0] * scale, prograde[1] * scale, prograde[2] * scale};
		}
		return {0.0, v_esc, 0.0};
	}

	[[nodiscard]] static double v_peri_fallback(double v) noexcept {
		return v;
	}

	[[nodiscard]] std::array<double, 3> compute_target_eccentricity_velocity(
		const std::array<double, 3>& pos,
		const std::array<double, 3>& current_vel,
		double central_mass,
		double target_ecc
	) const noexcept {
		const double r = std::sqrt(pos[0] * pos[0] + pos[1] * pos[1] + pos[2] * pos[2]);
		if (r < 1e-12) return current_vel;
		const double mu = std::max(simulation_g() * central_mass, 1e-14);
		const double clamped_ecc = std::max(target_ecc, 0.0);
		const double v_peri = std::sqrt(mu * (1.0 + clamped_ecc) / r);

		const double r_inv = 1.0 / r;
		const std::array<double, 3> r_hat{pos[0] * r_inv, pos[1] * r_inv, pos[2] * r_inv};
		const double v_rad = current_vel[0] * r_hat[0] + current_vel[1] * r_hat[1] + current_vel[2] * r_hat[2];
		std::array<double, 3> v_tan{current_vel[0] - v_rad * r_hat[0], current_vel[1] - v_rad * r_hat[1], current_vel[2] - v_rad * r_hat[2]};
		const double v_tan_len = std::sqrt(v_tan[0] * v_tan[0] + v_tan[1] * v_tan[1] + v_tan[2] * v_tan[2]);
		if (v_tan_len > 1e-9) {
			const double scale = v_peri / v_tan_len;
			return {v_tan[0] * scale, v_tan[1] * scale, v_tan[2] * scale};
		}
		const auto prograde = compute_circular_orbit_velocity(pos, central_mass);
		const double pro_len = std::sqrt(prograde[0] * prograde[0] + prograde[1] * prograde[1] + prograde[2] * prograde[2]);
		if (pro_len > 1e-9) {
			const double scale = v_peri / pro_len;
			return {prograde[0] * scale, prograde[1] * scale, prograde[2] * scale};
		}
		return {0.0, v_peri, 0.0};
	}

	[[nodiscard]] KeplerianOrbitElements compute_keplerian_elements(
		const std::array<double, 3>& pos,
		const std::array<double, 3>& vel,
		double central_mass
	) const noexcept {
		KeplerianOrbitElements el;
		const double r2 = pos[0] * pos[0] + pos[1] * pos[1] + pos[2] * pos[2];
		const double r = std::sqrt(r2);
		if (r < 1e-12) return el;
		const double v2 = vel[0] * vel[0] + vel[1] * vel[1] + vel[2] * vel[2];
		const double mu = std::max(simulation_g() * central_mass, 1e-14);

		const std::array<double, 3> h_vec{
			pos[1] * vel[2] - pos[2] * vel[1],
			pos[2] * vel[0] - pos[0] * vel[2],
			pos[0] * vel[1] - pos[1] * vel[0]
		};
		const double h2 = h_vec[0] * h_vec[0] + h_vec[1] * h_vec[1] + h_vec[2] * h_vec[2];
		const double h = std::sqrt(h2);
		el.angular_momentum = h;

		const double energy = 0.5 * v2 - mu / r;
		el.specific_energy = energy;

		const std::array<double, 3> v_cross_h{
			vel[1] * h_vec[2] - vel[2] * h_vec[1],
			vel[2] * h_vec[0] - vel[0] * h_vec[2],
			vel[0] * h_vec[1] - vel[1] * h_vec[0]
		};
		const std::array<double, 3> e_vec{
			v_cross_h[0] / mu - pos[0] / r,
			v_cross_h[1] / mu - pos[1] / r,
			v_cross_h[2] / mu - pos[2] / r
		};
		const double ecc = std::sqrt(e_vec[0] * e_vec[0] + e_vec[1] * e_vec[1] + e_vec[2] * e_vec[2]);
		el.eccentricity = ecc;

		const bool bound = (energy < -1e-14) && (ecc < 0.999999);
		el.is_bound = bound;

		if (std::abs(energy) > 1e-14) {
			el.semi_major_axis = -mu / (2.0 * energy);
		} else {
			el.semi_major_axis = 1e30;
		}

		if (bound && el.semi_major_axis > 0.0) {
			el.periapsis = el.semi_major_axis * (1.0 - ecc);
			el.apoapsis = el.semi_major_axis * (1.0 + ecc);
			const double sim_period = 2.0 * std::numbers::pi * std::sqrt((el.semi_major_axis * el.semi_major_axis * el.semi_major_axis) / mu);
			const double time_scale = length_scale() / speed_scale();
			el.period_seconds = sim_period * time_scale;
		} else {
			el.periapsis = (h2 > 1e-12) ? (h2 / (mu * (1.0 + ecc))) : (r / (1.0 + ecc));
			el.apoapsis = std::numeric_limits<double>::infinity();
			el.period_seconds = 0.0;
		}

		if (h > 1e-12) {
			const double cos_i = std::clamp(h_vec[2] / h, -1.0, 1.0);
			el.inclination_deg = std::acos(cos_i) * (180.0 / std::numbers::pi);
		} else {
			el.inclination_deg = 0.0;
		}
		el.is_prograde = (el.inclination_deg <= 90.0);

		if (ecc < 0.005) {
			el.type_name = "Circular";
		} else if (ecc < 0.999) {
			el.type_name = "Elliptic";
		} else if (ecc <= 1.001) {
			el.type_name = "Parabolic (Borderline)";
		} else {
			el.type_name = "Hyperbolic (Escape)";
		}
		el.valid = true;
		return el;
	}

	[[nodiscard]] TrajectoryHazardDiagnostic evaluate_trajectory_hazard(
		const std::array<double, 3>& pos,
		const std::array<double, 3>& vel,
		double body_mass,
		double body_radius,
		double central_mass
	) const noexcept {
		TrajectoryHazardDiagnostic diag;
		const auto el = compute_keplerian_elements(pos, vel, central_mass);
		if (!el.valid) return diag;

		diag.periapsis = el.periapsis;
		diag.horizon_radius = primary_horizon_radius();

		if (diag.horizon_radius > 0.0 && el.periapsis <= diag.horizon_radius) {
			diag.has_hazard = true;
			diag.plunge_hazard = true;
		}

		if (body_mass > 1e-15 && body_radius > 1e-9 && central_mass > 1e-12) {
			const double mass_ratio = central_mass / body_mass;
			diag.roche_limit = 1.8 * body_radius * std::cbrt(std::max(mass_ratio, 1.0));
			if (!diag.plunge_hazard && el.periapsis <= diag.roche_limit) {
				diag.has_hazard = true;
				diag.roche_hazard = true;
			}
		}
		return diag;
	}

	[[nodiscard]] BinarySourceConfiguration compute_binary_configuration(const Dynamics::PostNewtonianBody& secondary) const noexcept {
		BinarySourceConfiguration conf;
		const double m1 = std::max(orchestrator_.parameters().mass, 1e-9);
		const double m2 = std::max(secondary.mass, 1e-9);
		const double d = distance_from_center(secondary);
		if (d < 1e-9) return conf;

		conf.valid = true;
		conf.primary_mass = m1;
		conf.secondary_mass = m2;
		conf.mass_ratio = m2 / m1;
		const double m_tot = m1 + m2;
		conf.symmetric_mass_ratio = (m1 * m2) / (m_tot * m_tot);
		conf.separation_distance = d;
		conf.separation_rg = d / m1;

		const std::array<double, 3> l_vec{
			secondary.position[1] * secondary.velocity[2] - secondary.position[2] * secondary.velocity[1],
			secondary.position[2] * secondary.velocity[0] - secondary.position[0] * secondary.velocity[2],
			secondary.position[0] * secondary.velocity[1] - secondary.position[1] * secondary.velocity[0]
		};
		const double l_norm = std::sqrt(l_vec[0] * l_vec[0] + l_vec[1] * l_vec[1] + l_vec[2] * l_vec[2]);
		const std::array<double, 3> s2_hat = secondary.spin_axis_unit();

		if (l_norm > 1e-12) {
			const double dot_ls = (s2_hat[0] * l_vec[0] + s2_hat[1] * l_vec[1] + s2_hat[2] * l_vec[2]) / l_norm;
			conf.spin_orbit_misalignment_deg = std::acos(std::clamp(dot_ls, -1.0, 1.0)) * (180.0 / std::numbers::pi);
		}

		const double s1_val = orchestrator_.parameters().spin;
		const std::array<double, 3> s1_hat{0.0, 0.0, (s1_val >= 0.0) ? 1.0 : -1.0};
		const double dot_s1_s2 = s1_hat[0] * s2_hat[0] + s1_hat[1] * s2_hat[1] + s1_hat[2] * s2_hat[2];
		conf.mutual_spin_angle_deg = std::acos(std::clamp(dot_s1_s2, -1.0, 1.0)) * (180.0 / std::numbers::pi);

		const double g_sim = simulation_g();
		const double c_sim = simulation_c();
		const auto el = compute_keplerian_elements(secondary.position, secondary.velocity, m_tot);
		const double a_orb = (el.valid && el.is_bound && el.semi_major_axis > 0.0) ? el.semi_major_axis : d;
		const double a4 = a_orb * a_orb * a_orb * a_orb;
		const double denom = 256.0 * (g_sim * g_sim * g_sim) * m1 * m2 * m_tot;
		if (denom > 1e-30) {
			const double c5 = c_sim * c_sim * c_sim * c_sim * c_sim;
			const double t_sim = (5.0 * c5 * a4) / denom;
			const double time_scale = length_scale() / speed_scale();
			const double ecc = el.valid ? std::clamp(el.eccentricity, 0.0, 0.999) : 0.0;
			const double ecc_factor = std::pow(std::max(1.0 - ecc * ecc, 1e-4), 3.5);
			conf.peters_coalescence_time_seconds = t_sim * time_scale * ecc_factor;
		}
		return conf;
	}

	[[nodiscard]] double primary_horizon_radius() const noexcept {
		const auto& p = orchestrator_.parameters();
		if (p.mass <= 0.0) return 0.0;
		const double a = std::clamp(p.spin, -0.999 * p.mass, 0.999 * p.mass);
		return p.mass + std::sqrt(std::max(p.mass * p.mass - a * a, 0.0));
	}

	[[nodiscard]] int find_dominant_parent_id(const Dynamics::PostNewtonianBody& target) const noexcept {
		auto& sys = orchestrator_.nbody_system();
		const auto& all_bodies = sys.bodies();
		const double safe_central_mass = std::max(orchestrator_.parameters().mass, 1e-12);
		const double safe_g = std::max(orchestrator_.physical_gravitational_constant(), 1e-14);
		const double m_i = std::max(target.mass, 0.0);

		int best_parent = kCentralSourceId;
		double best_hill_ratio = 1e30;

		for (const auto& b_j : all_bodies) {
			if (b_j.id == target.id || !b_j.enabled) continue;
			const double m_j = std::max(b_j.mass, 0.0);
			if (m_j <= 0.0) continue;
			if (m_i > 1e-9 && m_j < m_i * 1.05) continue;
			if (m_i <= 1e-9 && m_j <= 1e-9) continue;

			const double dx = target.position[0] - b_j.position[0];
			const double dy = target.position[1] - b_j.position[1];
			const double dz = target.position[2] - b_j.position[2];
			const double d_ij = std::sqrt(dx * dx + dy * dy + dz * dz);
			if (d_ij < 1e-9) continue;

			const double r_j = std::max(distance_from_center(b_j), 1e-6);
			const double r_hill_j = r_j * std::cbrt(m_j / (3.0 * (safe_central_mass + m_j)));
			const double r_soi_j = r_j * std::pow(m_j / (safe_central_mass + m_j), 0.4);
			const double r_dom = std::max(r_hill_j, r_soi_j);

			if (d_ij > r_dom * 1.35) continue;

			const double dvx = target.velocity[0] - b_j.velocity[0];
			const double dvy = target.velocity[1] - b_j.velocity[1];
			const double dvz = target.velocity[2] - b_j.velocity[2];
			const double v_ij = std::sqrt(dvx * dvx + dvy * dvy + dvz * dvz);

			const double mu = safe_g * (m_j + m_i);
			const double energy = 0.5 * v_ij * v_ij - mu / d_ij;
			const double a_parent = mu / (d_ij * d_ij);
			const double a_tide = 2.0 * safe_g * safe_central_mass * d_ij / (r_j * r_j * r_j);
			const double dominance = a_parent / std::max(a_tide, 1e-12);

			if (energy >= 0.0 && dominance < 1.0) continue;

			const double hill_ratio = d_ij / std::max(r_dom, 1e-9);
			if (hill_ratio < best_hill_ratio) {
				best_hill_ratio = hill_ratio;
				best_parent = static_cast<int>(b_j.id);
			}
		}
		return best_parent;
	}

	[[nodiscard]] RelativeOrbitalPlaneState compute_relative_orbital_plane_state(
		const Dynamics::PostNewtonianBody& target,
		int reference_id
	) const noexcept {
		RelativeOrbitalPlaneState st;
		st.reference_id = reference_id;
		const double safe_central_mass = std::max(orchestrator_.parameters().mass, 1e-12);
		const double safe_g = std::max(simulation_g(), 1e-14);
		const double length_scale_val = length_scale();
		const double speed_scale_val = speed_scale();
		const double time_scale_val = length_scale_val / std::max(speed_scale_val, 1e-9);

		const Dynamics::PostNewtonianBody* ref_body = nullptr;
		if (reference_id >= 0) {
			ref_body = find_body(static_cast<uint32_t>(reference_id));
		}

		if (ref_body != nullptr && ref_body->id != target.id) {
			st.reference_id = static_cast<int>(ref_body->id);
			st.reference_name = display_name(*ref_body);
			st.reference_mass = std::max(ref_body->mass, 1e-12);
			st.reference_position = ref_body->position;
			st.reference_velocity = ref_body->velocity;
			st.reference_acceleration = ref_body->acceleration;

			const double rx = ref_body->position[0], ry = ref_body->position[1], rz = ref_body->position[2];
			const double vx = ref_body->velocity[0], vy = ref_body->velocity[1], vz = ref_body->velocity[2];
			const std::array<double, 3> h_p{ry * vz - rz * vy, rz * vx - rx * vz, rx * vy - ry * vx};
			const double h_p_len = std::sqrt(h_p[0] * h_p[0] + h_p[1] * h_p[1] + h_p[2] * h_p[2]);
			if (h_p_len > 1e-9) {
				st.reference_plane_normal = {h_p[0] / h_p_len, h_p[1] / h_p_len, h_p[2] / h_p_len};
			} else {
				st.reference_plane_normal = ref_body->spin_axis_unit();
			}
			const double cos_i_ref = std::clamp(st.reference_plane_normal[2], -1.0, 1.0);
			st.reference_inclination_world_deg = std::acos(cos_i_ref) * (180.0 / std::numbers::pi);
		} else {
			st.reference_id = kCentralSourceId;
			st.reference_name = "Central Spacetime Attractor (Origin)";
			st.reference_mass = safe_central_mass;
			st.reference_position = {0.0, 0.0, 0.0};
			st.reference_velocity = {0.0, 0.0, 0.0};
			st.reference_acceleration = {0.0, 0.0, 0.0};
			st.reference_plane_normal = {0.0, 0.0, 1.0};
			st.reference_inclination_world_deg = 0.0;
		}

		st.relative_position = {
			target.position[0] - st.reference_position[0],
			target.position[1] - st.reference_position[1],
			target.position[2] - st.reference_position[2]
		};
		st.relative_distance = std::sqrt(
			st.relative_position[0] * st.relative_position[0] +
			st.relative_position[1] * st.relative_position[1] +
			st.relative_position[2] * st.relative_position[2]
		);

		st.relative_velocity = {
			target.velocity[0] - st.reference_velocity[0],
			target.velocity[1] - st.reference_velocity[1],
			target.velocity[2] - st.reference_velocity[2]
		};
		st.relative_speed = std::sqrt(
			st.relative_velocity[0] * st.relative_velocity[0] +
			st.relative_velocity[1] * st.relative_velocity[1] +
			st.relative_velocity[2] * st.relative_velocity[2]
		);

		st.relative_acceleration = {
			target.acceleration[0] - st.reference_acceleration[0],
			target.acceleration[1] - st.reference_acceleration[1],
			target.acceleration[2] - st.reference_acceleration[2]
		};
		st.relative_acceleration_mag = std::sqrt(
			st.relative_acceleration[0] * st.relative_acceleration[0] +
			st.relative_acceleration[1] * st.relative_acceleration[1] +
			st.relative_acceleration[2] * st.relative_acceleration[2]
		);

		if (st.relative_distance > 1e-12) {
			const double inv_d = 1.0 / st.relative_distance;
			st.relative_radial_speed = (st.relative_position[0] * st.relative_velocity[0] +
				st.relative_position[1] * st.relative_velocity[1] +
				st.relative_position[2] * st.relative_velocity[2]) * inv_d;
			const double rad_vx = st.relative_radial_speed * st.relative_position[0] * inv_d;
			const double rad_vy = st.relative_radial_speed * st.relative_position[1] * inv_d;
			const double rad_vz = st.relative_radial_speed * st.relative_position[2] * inv_d;
			const double tan_vx = st.relative_velocity[0] - rad_vx;
			const double tan_vy = st.relative_velocity[1] - rad_vy;
			const double tan_vz = st.relative_velocity[2] - rad_vz;
			st.relative_tangential_speed = std::sqrt(tan_vx * tan_vx + tan_vy * tan_vy + tan_vz * tan_vz);
		}

		const std::array<double, 3> h_rel{
			st.relative_position[1] * st.relative_velocity[2] - st.relative_position[2] * st.relative_velocity[1],
			st.relative_position[2] * st.relative_velocity[0] - st.relative_position[0] * st.relative_velocity[2],
			st.relative_position[0] * st.relative_velocity[1] - st.relative_position[1] * st.relative_velocity[0]
		};
		const double h_rel_len = std::sqrt(h_rel[0] * h_rel[0] + h_rel[1] * h_rel[1] + h_rel[2] * h_rel[2]);
		st.specific_angular_momentum = h_rel_len;

		if (h_rel_len > 1e-12) {
			st.orbit_normal = {h_rel[0] / h_rel_len, h_rel[1] / h_rel_len, h_rel[2] / h_rel_len};
		} else {
			st.orbit_normal = st.reference_plane_normal;
		}

		const double cos_i_tgt = std::clamp(st.orbit_normal[2], -1.0, 1.0);
		st.target_inclination_world_deg = std::acos(cos_i_tgt) * (180.0 / std::numbers::pi);
		st.signed_inclination_offset_deg = st.target_inclination_world_deg - st.reference_inclination_world_deg;

		const double dot_normals = std::clamp(
			st.orbit_normal[0] * st.reference_plane_normal[0] +
			st.orbit_normal[1] * st.reference_plane_normal[1] +
			st.orbit_normal[2] * st.reference_plane_normal[2],
			-1.0, 1.0
		);
		st.relative_inclination_deg = std::acos(dot_normals) * (180.0 / std::numbers::pi);
		st.is_prograde = (st.relative_inclination_deg <= 90.0);

		std::array<double, 3> u_ref{1.0, 0.0, 0.0};
		std::array<double, 3> v_ref{0.0, 1.0, 0.0};
		if (ref_body != nullptr) {
			const double rx = ref_body->position[0], ry = ref_body->position[1], rz = ref_body->position[2];
			const double dot_rn = rx * st.reference_plane_normal[0] + ry * st.reference_plane_normal[1] + rz * st.reference_plane_normal[2];
			std::array<double, 3> r_proj{rx - dot_rn * st.reference_plane_normal[0], ry - dot_rn * st.reference_plane_normal[1], rz - dot_rn * st.reference_plane_normal[2]};
			const double r_proj_len = std::sqrt(r_proj[0] * r_proj[0] + r_proj[1] * r_proj[1] + r_proj[2] * r_proj[2]);
			if (r_proj_len > 1e-6) {
				u_ref = {r_proj[0] / r_proj_len, r_proj[1] / r_proj_len, r_proj[2] / r_proj_len};
			} else {
				const std::array<double, 3> helper = (std::abs(st.reference_plane_normal[2]) > 0.9) ? std::array<double, 3>{1.0, 0.0, 0.0} : std::array<double, 3>{0.0, 0.0, 1.0};
				const std::array<double, 3> cp{
					helper[1] * st.reference_plane_normal[2] - helper[2] * st.reference_plane_normal[1],
					helper[2] * st.reference_plane_normal[0] - helper[0] * st.reference_plane_normal[2],
					helper[0] * st.reference_plane_normal[1] - helper[1] * st.reference_plane_normal[0]
				};
				const double cp_len = std::sqrt(cp[0] * cp[0] + cp[1] * cp[1] + cp[2] * cp[2]);
				u_ref = {cp[0] / cp_len, cp[1] / cp_len, cp[2] / cp_len};
			}
			v_ref = {
				st.reference_plane_normal[1] * u_ref[2] - st.reference_plane_normal[2] * u_ref[1],
				st.reference_plane_normal[2] * u_ref[0] - st.reference_plane_normal[0] * u_ref[2],
				st.reference_plane_normal[0] * u_ref[1] - st.reference_plane_normal[1] * u_ref[0]
			};
		}

		const std::array<double, 3> n_node{
			st.reference_plane_normal[1] * st.orbit_normal[2] - st.reference_plane_normal[2] * st.orbit_normal[1],
			st.reference_plane_normal[2] * st.orbit_normal[0] - st.reference_plane_normal[0] * st.orbit_normal[2],
			st.reference_plane_normal[0] * st.orbit_normal[1] - st.reference_plane_normal[1] * st.orbit_normal[0]
		};
		const double node_len = std::sqrt(n_node[0] * n_node[0] + n_node[1] * n_node[1] + n_node[2] * n_node[2]);
		if (node_len > 1e-12) {
			st.node_vector = {n_node[0] / node_len, n_node[1] / node_len, n_node[2] / node_len};
			const double cos_node = st.node_vector[0] * u_ref[0] + st.node_vector[1] * u_ref[1] + st.node_vector[2] * u_ref[2];
			const double sin_node = st.node_vector[0] * v_ref[0] + st.node_vector[1] * v_ref[1] + st.node_vector[2] * v_ref[2];
			double node_deg = std::atan2(sin_node, cos_node) * (180.0 / std::numbers::pi);
			if (node_deg < 0.0) node_deg += 360.0;
			st.relative_node_deg = node_deg;
		} else {
			st.node_vector = u_ref;
			st.relative_node_deg = 0.0;
		}

		const double mu = safe_g * (st.reference_mass + target.mass);
		if (mu > 1e-14 && st.relative_distance > 1e-12) {
			const double v_cross_h_x = st.relative_velocity[1] * h_rel[2] - st.relative_velocity[2] * h_rel[1];
			const double v_cross_h_y = st.relative_velocity[2] * h_rel[0] - st.relative_velocity[0] * h_rel[2];
			const double v_cross_h_z = st.relative_velocity[0] * h_rel[1] - st.relative_velocity[1] * h_rel[0];

			const double inv_mu = 1.0 / mu;
			const double inv_d = 1.0 / st.relative_distance;
			st.eccentricity_vector = {
				v_cross_h_x * inv_mu - st.relative_position[0] * inv_d,
				v_cross_h_y * inv_mu - st.relative_position[1] * inv_d,
				v_cross_h_z * inv_mu - st.relative_position[2] * inv_d
			};
			st.eccentricity = std::sqrt(
				st.eccentricity_vector[0] * st.eccentricity_vector[0] +
				st.eccentricity_vector[1] * st.eccentricity_vector[1] +
				st.eccentricity_vector[2] * st.eccentricity_vector[2]
			);

			const double v2 = st.relative_speed * st.relative_speed;
			st.specific_energy = 0.5 * v2 - mu * inv_d;

			if (std::abs(st.specific_energy) > 1e-12) {
				st.semi_major_axis = -mu / (2.0 * st.specific_energy);
			} else {
				st.semi_major_axis = 1e30;
			}

			if (st.eccentricity < 1.0) {
				st.is_bound = true;
				st.periapsis_distance = st.semi_major_axis * (1.0 - st.eccentricity);
				st.apoapsis_distance = st.semi_major_axis * (1.0 + st.eccentricity);
				if (st.semi_major_axis > 0.0) {
					const double sim_t = 2.0 * std::numbers::pi * std::sqrt((st.semi_major_axis * st.semi_major_axis * st.semi_major_axis) / mu);
					st.orbital_period_seconds = sim_t * time_scale_val;
				}
			} else {
				st.is_bound = false;
				st.periapsis_distance = (h_rel_len > 1e-12) ? (h_rel_len * h_rel_len / (mu * (1.0 + st.eccentricity))) : (0.5 * st.relative_distance);
				st.apoapsis_distance = std::numeric_limits<double>::infinity();
				st.orbital_period_seconds = 0.0;
			}

			if (st.eccentricity > 1e-5 && node_len > 1e-9) {
				const double inv_e = 1.0 / st.eccentricity;
				const double cos_peri = std::clamp((st.node_vector[0] * st.eccentricity_vector[0] + st.node_vector[1] * st.eccentricity_vector[1] + st.node_vector[2] * st.eccentricity_vector[2]) * inv_e, -1.0, 1.0);
				const std::array<double, 3> node_cross_e{
					st.node_vector[1] * st.eccentricity_vector[2] - st.node_vector[2] * st.eccentricity_vector[1],
					st.node_vector[2] * st.eccentricity_vector[0] - st.node_vector[0] * st.eccentricity_vector[2],
					st.node_vector[0] * st.eccentricity_vector[1] - st.node_vector[1] * st.eccentricity_vector[0]
				};
				const double sin_peri = std::clamp((node_cross_e[0] * st.orbit_normal[0] + node_cross_e[1] * st.orbit_normal[1] + node_cross_e[2] * st.orbit_normal[2]) * inv_e, -1.0, 1.0);
				double peri_deg = std::atan2(sin_peri, cos_peri) * (180.0 / std::numbers::pi);
				if (peri_deg < 0.0) peri_deg += 360.0;
				st.relative_periapsis_deg = peri_deg;

				const double cos_nu = std::clamp((st.eccentricity_vector[0] * st.relative_position[0] + st.eccentricity_vector[1] * st.relative_position[1] + st.eccentricity_vector[2] * st.relative_position[2]) * inv_e * inv_d, -1.0, 1.0);
				const std::array<double, 3> e_cross_r{
					st.eccentricity_vector[1] * st.relative_position[2] - st.eccentricity_vector[2] * st.relative_position[1],
					st.eccentricity_vector[2] * st.relative_position[0] - st.eccentricity_vector[0] * st.relative_position[2],
					st.eccentricity_vector[0] * st.relative_position[1] - st.eccentricity_vector[1] * st.relative_position[0]
				};
				const double sin_nu = std::clamp((e_cross_r[0] * st.orbit_normal[0] + e_cross_r[1] * st.orbit_normal[1] + e_cross_r[2] * st.orbit_normal[2]) * inv_e * inv_d, -1.0, 1.0);
				double nu_deg = std::atan2(sin_nu, cos_nu) * (180.0 / std::numbers::pi);
				if (nu_deg < 0.0) nu_deg += 360.0;
				st.relative_true_anomaly_deg = nu_deg;
			} else {
				st.relative_periapsis_deg = 0.0;
				st.relative_true_anomaly_deg = 0.0;
			}
		}

		if (ref_body != nullptr && ref_body->mass > 0.0) {
			const double r_ref_dist = std::max(distance_from_center(*ref_body), 1e-6);
			st.hill_radius = r_ref_dist * std::cbrt(st.reference_mass / (3.0 * (safe_central_mass + st.reference_mass)));
			st.hill_occupancy_pct = (st.relative_distance / std::max(st.hill_radius, 1e-9)) * 100.0;
			const double a_parent = mu / std::max(st.relative_distance * st.relative_distance, 1e-12);
			const double a_tide = 2.0 * safe_g * safe_central_mass * st.relative_distance / std::max(r_ref_dist * r_ref_dist * r_ref_dist, 1e-12);
			st.tidal_dominance_ratio = a_parent / std::max(a_tide, 1e-12);
			st.is_satellite_of_reference = (st.is_bound && st.hill_occupancy_pct <= 100.0) || (st.tidal_dominance_ratio >= 1.0);
		}

		if (st.relative_inclination_deg < 0.8) {
			st.alignment_category = "Coplanar Prograde (Aligned)";
		} else if (st.relative_inclination_deg < 15.0) {
			st.alignment_category = "Low-Inclination Prograde";
		} else if (st.relative_inclination_deg < 39.23) {
			st.alignment_category = "Moderate Inclination (Sub-Kozai)";
		} else if (st.relative_inclination_deg <= 140.77) {
			st.alignment_category = "Kozai-Lidov Resonant Inclination";
			st.is_in_kozai_zone = true;
			st.kozai_warning = "Kozai-Lidov Resonance Zone: Relative inclination falls in [39.2 deg, 140.8 deg]. Coupled secular eccentricity-inclination oscillations may drive periapsis to collision or plunge!";
		} else if (st.relative_inclination_deg < 165.0) {
			st.alignment_category = "Moderate-Inclination Retrograde";
		} else if (st.relative_inclination_deg < 179.2) {
			st.alignment_category = "Low-Inclination Retrograde";
		} else {
			st.alignment_category = "Coplanar Retrograde (Counter-Rotating)";
		}

		st.valid = true;
		return st;
	}

	void apply_relative_orbital_plane_rotation(
		Dynamics::PostNewtonianBody& target,
		int reference_id,
		double target_inc_deg,
		double target_node_deg,
		double target_peri_deg
	) const noexcept {
		const auto st = compute_relative_orbital_plane_state(target, reference_id);
		if (!st.valid || st.relative_distance < 1e-12) return;

		std::array<double, 3> u_ref{1.0, 0.0, 0.0};
		std::array<double, 3> v_ref{0.0, 1.0, 0.0};
		const auto* ref_body = (reference_id >= 0) ? find_body(static_cast<uint32_t>(reference_id)) : nullptr;
		if (ref_body != nullptr) {
			const double rx = ref_body->position[0], ry = ref_body->position[1], rz = ref_body->position[2];
			const double dot_rn = rx * st.reference_plane_normal[0] + ry * st.reference_plane_normal[1] + rz * st.reference_plane_normal[2];
			std::array<double, 3> r_proj{rx - dot_rn * st.reference_plane_normal[0], ry - dot_rn * st.reference_plane_normal[1], rz - dot_rn * st.reference_plane_normal[2]};
			const double r_proj_len = std::sqrt(r_proj[0] * r_proj[0] + r_proj[1] * r_proj[1] + r_proj[2] * r_proj[2]);
			if (r_proj_len > 1e-6) {
				u_ref = {r_proj[0] / r_proj_len, r_proj[1] / r_proj_len, r_proj[2] / r_proj_len};
			} else {
				const std::array<double, 3> helper = (std::abs(st.reference_plane_normal[2]) > 0.9) ? std::array<double, 3>{1.0, 0.0, 0.0} : std::array<double, 3>{0.0, 0.0, 1.0};
				const std::array<double, 3> cp{
					helper[1] * st.reference_plane_normal[2] - helper[2] * st.reference_plane_normal[1],
					helper[2] * st.reference_plane_normal[0] - helper[0] * st.reference_plane_normal[2],
					helper[0] * st.reference_plane_normal[1] - helper[1] * st.reference_plane_normal[0]
				};
				const double cp_len = std::sqrt(cp[0] * cp[0] + cp[1] * cp[1] + cp[2] * cp[2]);
				u_ref = {cp[0] / cp_len, cp[1] / cp_len, cp[2] / cp_len};
			}
			v_ref = {
				st.reference_plane_normal[1] * u_ref[2] - st.reference_plane_normal[2] * u_ref[1],
				st.reference_plane_normal[2] * u_ref[0] - st.reference_plane_normal[0] * u_ref[2],
				st.reference_plane_normal[0] * u_ref[1] - st.reference_plane_normal[1] * u_ref[0]
			};
		}

		const double inc_rad = std::clamp(target_inc_deg, 0.0, 180.0) * (std::numbers::pi / 180.0);
		const double node_rad = target_node_deg * (std::numbers::pi / 180.0);
		const double peri_rad = target_peri_deg * (std::numbers::pi / 180.0);

		const double sin_i = std::sin(inc_rad);
		const double cos_i = std::cos(inc_rad);
		const double sin_o = std::sin(node_rad);
		const double cos_o = std::cos(node_rad);
		const double sin_w = std::sin(peri_rad);
		const double cos_w = std::cos(peri_rad);

		const std::array<double, 3> n_node{
			cos_o * u_ref[0] + sin_o * v_ref[0],
			cos_o * u_ref[1] + sin_o * v_ref[1],
			cos_o * u_ref[2] + sin_o * v_ref[2]
		};

		const std::array<double, 3> m_perp_ref{
			-sin_o * u_ref[0] + cos_o * v_ref[0],
			-sin_o * u_ref[1] + cos_o * v_ref[1],
			-sin_o * u_ref[2] + cos_o * v_ref[2]
		};

		const std::array<double, 3> m_new{
			cos_i * m_perp_ref[0] + sin_i * st.reference_plane_normal[0],
			cos_i * m_perp_ref[1] + sin_i * st.reference_plane_normal[1],
			cos_i * m_perp_ref[2] + sin_i * st.reference_plane_normal[2]
		};

		const std::array<double, 3> p_hat{
			cos_w * n_node[0] + sin_w * m_new[0],
			cos_w * n_node[1] + sin_w * m_new[1],
			cos_w * n_node[2] + sin_w * m_new[2]
		};

		const std::array<double, 3> q_hat{
			-sin_w * n_node[0] + cos_w * m_new[0],
			-sin_w * n_node[1] + cos_w * m_new[1],
			-sin_w * n_node[2] + cos_w * m_new[2]
		};

		const double nu_rad = st.relative_true_anomaly_deg * (std::numbers::pi / 180.0);
		const double cos_nu = std::cos(nu_rad);
		const double sin_nu = std::sin(nu_rad);

		const std::array<double, 3> u_orb{
			cos_nu * p_hat[0] + sin_nu * q_hat[0],
			cos_nu * p_hat[1] + sin_nu * q_hat[1],
			cos_nu * p_hat[2] + sin_nu * q_hat[2]
		};

		const std::array<double, 3> v_orb{
			-sin_nu * p_hat[0] + cos_nu * q_hat[0],
			-sin_nu * p_hat[1] + cos_nu * q_hat[1],
			-sin_nu * p_hat[2] + cos_nu * q_hat[2]
		};

		const std::array<double, 3> new_rel_pos{
			st.relative_distance * u_orb[0],
			st.relative_distance * u_orb[1],
			st.relative_distance * u_orb[2]
		};

		const double vr = st.relative_radial_speed;
		const double vt = (st.is_prograde ? 1.0 : -1.0) * st.relative_tangential_speed;

		const std::array<double, 3> new_rel_vel{
			vr * u_orb[0] + vt * v_orb[0],
			vr * u_orb[1] + vt * v_orb[1],
			vr * u_orb[2] + vt * v_orb[2]
		};

		target.position = {
			st.reference_position[0] + new_rel_pos[0],
			st.reference_position[1] + new_rel_pos[1],
			st.reference_position[2] + new_rel_pos[2]
		};
		target.velocity = {
			st.reference_velocity[0] + new_rel_vel[0],
			st.reference_velocity[1] + new_rel_vel[1],
			st.reference_velocity[2] + new_rel_vel[2]
		};
	}

	void circularize_around_reference(Dynamics::PostNewtonianBody& target, int reference_id) noexcept {
		const auto st = compute_relative_orbital_plane_state(target, reference_id);
		if (!st.valid || st.relative_distance < 1e-12) return;
		const double mu = std::max(simulation_g() * (st.reference_mass + target.mass), 1e-14);
		const double v_circ = std::sqrt(mu / st.relative_distance);

		std::array<double, 3> r_hat{
			st.relative_position[0] / st.relative_distance,
			st.relative_position[1] / st.relative_distance,
			st.relative_position[2] / st.relative_distance
		};
		std::array<double, 3> tangent{
			st.orbit_normal[1] * r_hat[2] - st.orbit_normal[2] * r_hat[1],
			st.orbit_normal[2] * r_hat[0] - st.orbit_normal[0] * r_hat[2],
			st.orbit_normal[0] * r_hat[1] - st.orbit_normal[1] * r_hat[0]
		};
		const double t_len = std::sqrt(tangent[0] * tangent[0] + tangent[1] * tangent[1] + tangent[2] * tangent[2]);
		if (t_len > 1e-9) {
			tangent[0] /= t_len; tangent[1] /= t_len; tangent[2] /= t_len;
		} else {
			tangent = {0.0, 1.0, 0.0};
		}
		const double dir = st.is_prograde ? 1.0 : -1.0;
		target.velocity = {
			st.reference_velocity[0] + dir * v_circ * tangent[0],
			st.reference_velocity[1] + dir * v_circ * tangent[1],
			st.reference_velocity[2] + dir * v_circ * tangent[2]
		};
	}

	void match_reference_velocity(Dynamics::PostNewtonianBody& target, int reference_id) noexcept {
		const auto st = compute_relative_orbital_plane_state(target, reference_id);
		if (!st.valid) return;
		target.velocity = st.reference_velocity;
	}

	[[nodiscard]] LocalFrenetFrameState compute_local_frenet_frame(
		const Dynamics::PostNewtonianBody& body,
		int reference_id
	) const noexcept {
		LocalFrenetFrameState fr;
		fr.reference_id = reference_id;

		const auto plane_st = compute_relative_orbital_plane_state(body, reference_id);
		if (!plane_st.valid || plane_st.relative_distance < 1e-12) return fr;

		fr.valid = true;
		fr.reference_name = plane_st.reference_name;
		fr.distance = plane_st.relative_distance;
		fr.speed = plane_st.relative_speed;
		fr.v_radial = plane_st.relative_radial_speed;
		fr.v_tangential = (plane_st.is_prograde ? 1.0 : -1.0) * plane_st.relative_tangential_speed;
		fr.v_normal = 0.0;

		const double inv_r = 1.0 / fr.distance;
		fr.radial_hat = {
			plane_st.relative_position[0] * inv_r,
			plane_st.relative_position[1] * inv_r,
			plane_st.relative_position[2] * inv_r
		};

		fr.normal_hat = plane_st.orbit_normal;

		fr.tangential_hat = {
			fr.normal_hat[1] * fr.radial_hat[2] - fr.normal_hat[2] * fr.radial_hat[1],
			fr.normal_hat[2] * fr.radial_hat[0] - fr.normal_hat[0] * fr.radial_hat[2],
			fr.normal_hat[0] * fr.radial_hat[1] - fr.normal_hat[1] * fr.radial_hat[0]
		};
		const double t_len = std::sqrt(
			fr.tangential_hat[0] * fr.tangential_hat[0] +
			fr.tangential_hat[1] * fr.tangential_hat[1] +
			fr.tangential_hat[2] * fr.tangential_hat[2]
		);
		if (t_len > 1e-9) {
			fr.tangential_hat[0] /= t_len;
			fr.tangential_hat[1] /= t_len;
			fr.tangential_hat[2] /= t_len;
		} else {
			fr.tangential_hat = {0.0, 1.0, 0.0};
		}

		const double mu = std::max(simulation_g() * (plane_st.reference_mass + body.mass), 1e-14);
		fr.circular_speed = std::sqrt(mu / fr.distance);
		fr.escape_speed = std::sqrt(2.0 * mu / fr.distance);

		if (fr.speed > 1e-12) {
			const double sin_gamma = std::clamp(fr.v_radial / fr.speed, -1.0, 1.0);
			fr.flight_path_angle_deg = std::asin(sin_gamma) * (180.0 / std::numbers::pi);
		} else {
			fr.flight_path_angle_deg = 0.0;
		}

		return fr;
	}

	[[nodiscard]] FrenetManeuverPreview preview_frenet_maneuver(
		const Dynamics::PostNewtonianBody& body,
		int reference_id,
		double delta_vr_sim,
		double delta_vt_sim,
		double delta_vn_sim
	) const noexcept {
		FrenetManeuverPreview prev;
		const auto fr = compute_local_frenet_frame(body, reference_id);
		if (!fr.valid) return prev;

		const auto plane_st = compute_relative_orbital_plane_state(body, reference_id);
		if (!plane_st.valid) return prev;

		const auto curr_orbit = compute_keplerian_elements(plane_st.relative_position, plane_st.relative_velocity, plane_st.reference_mass + body.mass);

		const std::array<double, 3> dv_cart{
			delta_vr_sim * fr.radial_hat[0] + delta_vt_sim * fr.tangential_hat[0] + delta_vn_sim * fr.normal_hat[0],
			delta_vr_sim * fr.radial_hat[1] + delta_vt_sim * fr.tangential_hat[1] + delta_vn_sim * fr.normal_hat[1],
			delta_vr_sim * fr.radial_hat[2] + delta_vt_sim * fr.tangential_hat[2] + delta_vn_sim * fr.normal_hat[2]
		};

		const std::array<double, 3> v_pred_rel{
			plane_st.relative_velocity[0] + dv_cart[0],
			plane_st.relative_velocity[1] + dv_cart[1],
			plane_st.relative_velocity[2] + dv_cart[2]
		};

		const auto pred_orbit = compute_keplerian_elements(plane_st.relative_position, v_pred_rel, plane_st.reference_mass + body.mass);
		if (!curr_orbit.valid || !pred_orbit.valid) return prev;

		prev.valid = true;
		prev.delta_v_magnitude = std::sqrt(delta_vr_sim * delta_vr_sim + delta_vt_sim * delta_vt_sim + delta_vn_sim * delta_vn_sim);
		prev.current_speed = fr.speed;
		prev.predicted_speed = std::sqrt(v_pred_rel[0] * v_pred_rel[0] + v_pred_rel[1] * v_pred_rel[1] + v_pred_rel[2] * v_pred_rel[2]);

		prev.current_eccentricity = curr_orbit.eccentricity;
		prev.predicted_eccentricity = pred_orbit.eccentricity;
		prev.current_semi_major_axis = curr_orbit.semi_major_axis;
		prev.predicted_semi_major_axis = pred_orbit.semi_major_axis;
		prev.current_periapsis = curr_orbit.periapsis;
		prev.predicted_periapsis = pred_orbit.periapsis;
		prev.current_apoapsis = curr_orbit.apoapsis;
		prev.predicted_apoapsis = pred_orbit.apoapsis;
		prev.current_inclination_deg = curr_orbit.inclination_deg;
		prev.predicted_inclination_deg = pred_orbit.inclination_deg;
		prev.current_period_seconds = curr_orbit.period_seconds;
		prev.predicted_period_seconds = pred_orbit.period_seconds;
		prev.current_is_bound = curr_orbit.is_bound;
		prev.predicted_is_bound = pred_orbit.is_bound;
		prev.predicted_orbit_type = pred_orbit.type_name;

		const double horizon_r = primary_horizon_radius();
		prev.causes_plunge = (horizon_r > 0.0 && pred_orbit.periapsis <= horizon_r * 1.05);
		prev.causes_escape = (!curr_orbit.valid || curr_orbit.is_bound) && !pred_orbit.is_bound;

		return prev;
	}

	void apply_frenet_delta_v(
		Dynamics::PostNewtonianBody& body,
		int reference_id,
		double delta_vr_sim,
		double delta_vt_sim,
		double delta_vn_sim
	) const noexcept {
		const auto fr = compute_local_frenet_frame(body, reference_id);
		if (!fr.valid) return;

		const double dvx = delta_vr_sim * fr.radial_hat[0] + delta_vt_sim * fr.tangential_hat[0] + delta_vn_sim * fr.normal_hat[0];
		const double dvy = delta_vr_sim * fr.radial_hat[1] + delta_vt_sim * fr.tangential_hat[1] + delta_vn_sim * fr.normal_hat[1];
		const double dvz = delta_vr_sim * fr.radial_hat[2] + delta_vt_sim * fr.tangential_hat[2] + delta_vn_sim * fr.normal_hat[2];

		body.velocity[0] += dvx;
		body.velocity[1] += dvy;
		body.velocity[2] += dvz;
	}

	void set_frenet_velocity_components(
		Dynamics::PostNewtonianBody& body,
		int reference_id,
		double target_vr_sim,
		double target_vt_sim,
		double target_vn_sim
	) const noexcept {
		const auto fr = compute_local_frenet_frame(body, reference_id);
		if (!fr.valid) return;

		const auto plane_st = compute_relative_orbital_plane_state(body, reference_id);
		if (!plane_st.valid) return;

		const double vx_rel = target_vr_sim * fr.radial_hat[0] + target_vt_sim * fr.tangential_hat[0] + target_vn_sim * fr.normal_hat[0];
		const double vy_rel = target_vr_sim * fr.radial_hat[1] + target_vt_sim * fr.tangential_hat[1] + target_vn_sim * fr.normal_hat[1];
		const double vz_rel = target_vr_sim * fr.radial_hat[2] + target_vt_sim * fr.tangential_hat[2] + target_vn_sim * fr.normal_hat[2];

		body.velocity[0] = plane_st.reference_velocity[0] + vx_rel;
		body.velocity[1] = plane_st.reference_velocity[1] + vy_rel;
		body.velocity[2] = plane_st.reference_velocity[2] + vz_rel;
	}

	void apply_frenet_hohmann_transfer(
		Dynamics::PostNewtonianBody& body,
		int reference_id,
		double target_radius_sim
	) const noexcept {
		const auto fr = compute_local_frenet_frame(body, reference_id);
		if (!fr.valid || fr.distance < 1e-12) return;

		const auto plane_st = compute_relative_orbital_plane_state(body, reference_id);
		if (!plane_st.valid) return;

		const double r1 = fr.distance;
		const double r2 = std::max(target_radius_sim, 1e-6);
		const double mu = std::max(simulation_g() * (plane_st.reference_mass + body.mass), 1e-14);

		const double a_hohmann = 0.5 * (r1 + r2);
		const double v_req = std::sqrt(std::max(mu * (2.0 / r1 - 1.0 / a_hohmann), 1e-14));
		const double sign = (fr.v_tangential >= 0.0) ? 1.0 : -1.0;

		set_frenet_velocity_components(body, reference_id, 0.0, sign * v_req, 0.0);
	}

	void apply_frenet_plane_change_burn(
		Dynamics::PostNewtonianBody& body,
		int reference_id,
		double delta_inclination_deg
	) const noexcept {
		const auto fr = compute_local_frenet_frame(body, reference_id);
		if (!fr.valid) return;

		const double di_rad = delta_inclination_deg * (std::numbers::pi / 180.0);
		const double sin_half = std::sin(0.5 * di_rad);
		const double cos_di = std::cos(di_rad);

		const double current_vt = fr.v_tangential;
		const double new_vt = current_vt * cos_di;
		const double new_vn = 2.0 * std::abs(current_vt) * sin_half * (delta_inclination_deg >= 0.0 ? 1.0 : -1.0);

		set_frenet_velocity_components(body, reference_id, fr.v_radial, new_vt, new_vn);
	}

	[[nodiscard]] std::string source_overlap_warning(const Dynamics::PostNewtonianBody& b) const {
		const double horizon = b.kerr_outer_horizon_radius();
		const double primary_horizon = primary_horizon_radius();
		if (primary_horizon > 0.0 && distance_from_center(b) < primary_horizon + horizon) {
			return "This horizon overlaps the primary source horizon; overlapping sources merge when the simulation runs, and new sources are moved to a safe separation when spawned.";
		}
		auto& sys = orchestrator_.nbody_system();
		{
			std::lock_guard<std::recursive_mutex> lock(sys.bodies_mutex());
			for (const auto& other : sys.bodies()) {
				if (!other.enabled || !other.is_spacetime_source || other.id == b.id) continue;
				const double dx = b.position[0] - other.position[0];
				const double dy = b.position[1] - other.position[1];
				const double dz = b.position[2] - other.position[2];
				if (std::sqrt(dx * dx + dy * dy + dz * dz) < horizon + other.kerr_outer_horizon_radius()) {
					return "This horizon overlaps another independent black hole; overlapping sources merge when the simulation runs, and new sources are moved to a safe separation when spawned.";
				}
			}
		}
		const auto& camera = orchestrator_.camera();
		const double cx = b.position[0] - camera.position[0];
		const double cy = b.position[1] - camera.position[1];
		const double cz = b.position[2] - camera.position[2];
		if (std::sqrt(cx * cx + cy * cy + cz * cz) < horizon) {
			return "The observer is located inside this event horizon.";
		}
		return {};
	}

	void resolve_source_placement(Dynamics::PostNewtonianBody& body) const noexcept {
		constexpr double separation_margin = 1.25;
		const double horizon = body.kerr_outer_horizon_radius();
		const double primary_horizon = primary_horizon_radius();
		auto& sys = orchestrator_.nbody_system();
		std::lock_guard<std::recursive_mutex> lock(sys.bodies_mutex());
		const auto separate_from = [&body](const std::array<double, 3>& center, double required_distance) noexcept -> bool {
			const std::array<double, 3> offset{body.position[0] - center[0], body.position[1] - center[1], body.position[2] - center[2]};
			const double distance = std::sqrt(offset[0] * offset[0] + offset[1] * offset[1] + offset[2] * offset[2]);
			if (distance >= required_distance) return false;
			const std::array<double, 3> direction = (distance > 1e-9)
				? std::array<double, 3>{offset[0] / distance, offset[1] / distance, offset[2] / distance}
				: std::array<double, 3>{1.0, 0.0, 0.0};
			body.position = {center[0] + direction[0] * required_distance, center[1] + direction[1] * required_distance, center[2] + direction[2] * required_distance};
			return true;
		};
		for (int pass = 0; pass < 16; ++pass) {
			bool moved = false;
			if (primary_horizon > 0.0) {
				moved = separate_from({0.0, 0.0, 0.0}, (primary_horizon + horizon) * separation_margin);
			}
			for (const auto& other : sys.bodies()) {
				if (!other.enabled || !other.is_spacetime_source || other.id == body.id) continue;
				moved = separate_from(other.position, (other.kerr_outer_horizon_radius() + horizon) * separation_margin) || moved;
			}
			if (!moved) break;
		}
	}

	[[nodiscard]] CollisionOverlapDiagnostic check_placement_collision(
		const std::array<double, 3>& pos,
		double radius,
		bool is_source,
		uint32_t ignore_id = 0
	) const noexcept {
		CollisionOverlapDiagnostic diag;
		const double primary_h = primary_horizon_radius();
		const double d_center = std::sqrt(pos[0] * pos[0] + pos[1] * pos[1] + pos[2] * pos[2]);
		const double primary_margin = is_source ? 1.25 : 1.05;
		if (primary_h > 0.0 && d_center < (primary_h + radius) * primary_margin) {
			diag.has_collision = true;
			diag.inside_primary_horizon = true;
			diag.clearance_needed = (primary_h + radius) * primary_margin - d_center;
			return diag;
		}
		auto& sys = orchestrator_.nbody_system();
		std::lock_guard<std::recursive_mutex> lock(sys.bodies_mutex());
		for (const auto& other : sys.bodies()) {
			if (!other.enabled || other.id == ignore_id) continue;
			const double dx = pos[0] - other.position[0];
			const double dy = pos[1] - other.position[1];
			const double dz = pos[2] - other.position[2];
			const double dist = std::sqrt(dx * dx + dy * dy + dz * dz);
			if (other.is_spacetime_source) {
				const double other_h = other.kerr_outer_horizon_radius();
				const double margin = is_source ? 1.25 : 1.10;
				const double req = (other_h + radius) * margin;
				if (dist < req) {
					diag.has_collision = true;
					diag.inside_source_horizon = true;
					diag.colliding_body_id = other.id;
					diag.colliding_body_name = display_name(other);
					diag.clearance_needed = req - dist;
					return diag;
				}
			} else {
				const double margin = is_source ? 1.25 : 1.10;
				const double req = (other.radius + radius) * margin;
				if (dist < req) {
					diag.has_collision = true;
					diag.overlaps_body = true;
					diag.colliding_body_id = other.id;
					diag.colliding_body_name = display_name(other);
					diag.clearance_needed = req - dist;
					return diag;
				}
			}
		}
		return diag;
	}

	void resolve_safe_placement(
		std::array<double, 3>& pos,
		double radius,
		bool is_source,
		uint32_t ignore_id = 0
	) const noexcept {
		const double primary_margin = is_source ? 1.25 : 1.15;
		const double source_margin = is_source ? 1.25 : 1.15;
		const double body_margin = is_source ? 1.20 : 1.10;
		auto& sys = orchestrator_.nbody_system();
		std::lock_guard<std::recursive_mutex> lock(sys.bodies_mutex());
		const double primary_h = primary_horizon_radius();
		const auto push_away = [&pos](const std::array<double, 3>& center, double req) noexcept -> bool {
			const std::array<double, 3> offset{pos[0] - center[0], pos[1] - center[1], pos[2] - center[2]};
			const double dist = std::sqrt(offset[0] * offset[0] + offset[1] * offset[1] + offset[2] * offset[2]);
			if (dist >= req) return false;
			const std::array<double, 3> dir = (dist > 1e-9)
				? std::array<double, 3>{offset[0] / dist, offset[1] / dist, offset[2] / dist}
				: std::array<double, 3>{1.0, 0.0, 0.0};
			pos = {center[0] + dir[0] * req, center[1] + dir[1] * req, center[2] + dir[2] * req};
			return true;
		};
		for (int pass = 0; pass < 32; ++pass) {
			bool moved = false;
			if (primary_h > 0.0) {
				moved = push_away({0.0, 0.0, 0.0}, (primary_h + radius) * primary_margin) || moved;
			}
			for (const auto& other : sys.bodies()) {
				if (!other.enabled || other.id == ignore_id) continue;
				const bool other_is_source = other.is_spacetime_source;
				const double other_extent = other_is_source ? other.kerr_outer_horizon_radius() : other.radius;
				const double margin = other_is_source ? source_margin : body_margin;
				moved = push_away(other.position, (other_extent + radius) * margin) || moved;
			}
			if (!moved) break;
		}
	}

	void shift_to_center_of_mass_frame(BulkActionScope scope = BulkActionScope::AllBodies, CatalogTypeCategory category = CatalogTypeCategory::All) noexcept {
		auto& sys = orchestrator_.nbody_system();
		std::lock_guard<std::recursive_mutex> lock(sys.bodies_mutex());
		double m_tot = 0.0;
		std::array<double, 3> p_tot{0.0, 0.0, 0.0};
		for (const auto& b : sys.bodies()) {
			if (!b.enabled || !matches_bulk_scope(b, scope, category)) continue;
			m_tot += b.mass;
			p_tot[0] += b.mass * b.velocity[0];
			p_tot[1] += b.mass * b.velocity[1];
			p_tot[2] += b.mass * b.velocity[2];
		}
		if (m_tot > 1e-12) {
			const std::array<double, 3> v_cm{p_tot[0] / m_tot, p_tot[1] / m_tot, p_tot[2] / m_tot};
			for (auto& b : sys.bodies()) {
				if (!b.enabled || !matches_bulk_scope(b, scope, category)) continue;
				b.velocity[0] -= v_cm[0];
				b.velocity[1] -= v_cm[1];
				b.velocity[2] -= v_cm[2];
			}
			sys.update_accelerations();
			orchestrator_.notify_state_changed();
		}
	}

	[[nodiscard]] uint32_t spawn_body(Dynamics::PostNewtonianBody body, const Dynamics::BodySurfaceLayerSet& layers) noexcept {
		auto& sys = orchestrator_.nbody_system();
		body.id = 0;
		body.acceleration = {0.0, 0.0, 0.0};
		body.set_name(unique_name(body.has_name() ? body.name_view() : std::string_view("Body")));
		if (body.is_spacetime_source) {
			body.enforce_spacetime_source_invariants();
			resolve_source_placement(body);
		}
		const uint32_t id = sys.add_body(std::move(body));
		orchestrator_.surface_layers().set(id, layers);
		sys.update_accelerations();
		orchestrator_.notify_state_changed();
		return id;
	}

	void remove_body_by_id(uint32_t id) noexcept {
		auto& sys = orchestrator_.nbody_system();
		orchestrator_.surface_layers().erase(id);
		if (sys.remove_body(id)) {
			sys.update_accelerations();
		}
		if (tracked_body_id_ == static_cast<int>(id)) {
			tracking_enabled_ = false;
			tracked_body_id_ = kNoSelection;
		}
		auto it_obj = std::find(selected_object_ids_.begin(), selected_object_ids_.end(), static_cast<int>(id));
		if (it_obj != selected_object_ids_.end()) selected_object_ids_.erase(it_obj);
		auto it_src = std::find(selected_source_ids_.begin(), selected_source_ids_.end(), static_cast<int>(id));
		if (it_src != selected_source_ids_.end()) selected_source_ids_.erase(it_src);
		if (selected_object_id_ == static_cast<int>(id)) {
			selected_object_id_ = selected_object_ids_.empty() ? kNoSelection : selected_object_ids_.back();
		}
		if (selected_source_id_ == static_cast<int>(id)) {
			selected_source_id_ = selected_source_ids_.empty() ? kCentralSourceId : selected_source_ids_.back();
		}
		orchestrator_.notify_state_changed();
	}

	void clear_all_bodies() noexcept {
		orchestrator_.surface_layers().clear();
		orchestrator_.nbody_system().clear_bodies();
		selected_object_id_ = kNoSelection;
		selected_source_id_ = kNoSelection;
		selected_object_ids_.clear();
		selected_source_ids_.clear();
		if (tracked_body_id_ != kCentralSourceId) {
			tracking_enabled_ = false;
			tracked_body_id_ = kNoSelection;
		}
		orchestrator_.notify_state_changed();
	}

	void spawn_companion_source() noexcept {
		const double primary_mass = std::max(orchestrator_.parameters().mass, 1e-6);
		const double companion_mass = primary_mass * 0.5;
		const double orbit_radius = std::max(primary_mass * 40.0, 20.0);
		const double theta = random_real(std::numbers::pi * 0.35, std::numbers::pi * 0.65);
		const double phi = random_real(0.0, 2.0 * std::numbers::pi);
		const std::array<double, 3> position{
			orbit_radius * std::sin(theta) * std::cos(phi),
			orbit_radius * std::sin(theta) * std::sin(phi),
			orbit_radius * std::cos(theta)
		};
		const auto velocity = compute_circular_orbit_velocity(position, primary_mass + companion_mass);

		Dynamics::PostNewtonianBody body(
			0, companion_mass, companion_mass * 2.0,
			position, velocity,
			{0.0, 0.0, companion_mass * 0.3}
		);
		body.set_name("Companion Black Hole");
		body.is_spacetime_source = true;
		apply_body_appearance_preset(body, Dynamics::Body3DPreset::BlackHole);
		const uint32_t id = spawn_body(body, Dynamics::BodySurfaceLayerSet{});
		selected_source_id_ = static_cast<int>(id);
	}

	void populate_reference_system() noexcept {
		auto& sys = orchestrator_.nbody_system();
		orchestrator_.surface_layers().clear();
		sys.clear_bodies();

		const double central_mass = std::max(orchestrator_.parameters().mass, 1.0);
		const auto spawn_planet = [&](std::string_view name, Dynamics::Body3DPreset preset, double mass_ratio, double size_fraction, double orbit_in_central_masses) noexcept {
			const double mass = central_mass * mass_ratio;
			const double orbit_radius = central_mass * orbit_in_central_masses;
			const double radius = std::max(orbit_radius * size_fraction, 4.0 * mass);
			const std::array<double, 3> position{orbit_radius, 0.0, 0.0};
			Dynamics::PostNewtonianBody planet(0, mass, radius, position, compute_circular_orbit_velocity(position, central_mass), {0.0, 0.0, 0.0});
			apply_body_appearance_preset(planet, preset);
			apply_body_material_preset(planet, preset);
			planet.set_name(name);
			static_cast<void>(spawn_body(planet, Dynamics::BodySurfaceLayerSet{}));
		};
		spawn_planet("Inner Planet", Dynamics::Body3DPreset::TerrestrialPlanet, 3e-7, 0.006, 14.0);
		spawn_planet("Outer Planet", Dynamics::Body3DPreset::GasGiant, 4e-5, 0.02, 32.0);
		selected_object_id_ = kNoSelection;
		selected_source_id_ = kNoSelection;
	}
};

template <typename Getter>
[[nodiscard]] inline bool batch_are_equal(const std::vector<Dynamics::PostNewtonianBody*>& bodies, Getter getter, double eps = 1e-9) noexcept {
	if (bodies.size() <= 1) return true;
	const double first = static_cast<double>(getter(*bodies.front()));
	for (size_t i = 1; i < bodies.size(); ++i) {
		if (std::abs(static_cast<double>(getter(*bodies[i])) - first) > eps) {
			return false;
		}
	}
	return true;
}

template <typename Getter>
[[nodiscard]] inline bool batch_are_equal_exact(const std::vector<Dynamics::PostNewtonianBody*>& bodies, Getter getter) noexcept {
	if (bodies.size() <= 1) return true;
	const auto first = getter(*bodies.front());
	for (size_t i = 1; i < bodies.size(); ++i) {
		if (getter(*bodies[i]) != first) return false;
	}
	return true;
}

template <typename Getter, typename Formatter>
inline void batch_hover_tooltip(const std::vector<Dynamics::PostNewtonianBody*>& bodies, Getter getter, Formatter formatter, const char* prop_name) noexcept {
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
		ImGui::BeginTooltip();
		ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "%s (Mixed across %zu bodies):", prop_name, bodies.size());
		ImGui::Separator();
		constexpr size_t kMaxDisplay = 20;
		const size_t count = std::min(bodies.size(), kMaxDisplay);
		for (size_t i = 0; i < count; ++i) {
			const auto* b = bodies[i];
			const std::string name = BodyManagerContext::display_name(*b);
			const std::string val_str = formatter(getter(*b));
			ImGui::TextDisabled("[#%u %s]: %s", b->id, name.c_str(), val_str.c_str());
		}
		if (bodies.size() > kMaxDisplay) {
			ImGui::TextDisabled("... and %zu more bodies", bodies.size() - kMaxDisplay);
		}
		ImGui::EndTooltip();
	}
}

template <typename Getter, typename Setter, typename Formatter>
inline bool render_batch_scalar_slider(
	const char* label,
	const std::vector<Dynamics::PostNewtonianBody*>& bodies,
	Getter getter,
	Setter setter,
	Formatter formatter,
	float min_val,
	float max_val,
	const char* format = "%.4f",
	bool* log_mode = nullptr,
	float log_min = 0.0f,
	float log_max = 0.0f
) noexcept {
	bool changed = false;
	const bool uniform = batch_are_equal(bodies, getter);
	const double common_val = uniform ? static_cast<double>(getter(*bodies.front())) : 0.0;
	float display_val = static_cast<float>(common_val);

	ImGui::PushID(label);
	if (uniform) {
		if (slider_float_with_input(label, &display_val, min_val, max_val, format, log_mode, log_min, log_max)) {
			const double new_val = static_cast<double>(display_val);
			for (auto* b : bodies) {
				setter(*b, new_val);
			}
			changed = true;
		}
	} else {
		constexpr float input_width = 92.0f;
		const float avail = ImGui::CalcItemWidth();
		ImGui::SetNextItemWidth(std::max(avail - input_width - 8.0f, 60.0f));

		float dummy_slider = 0.5f;
		if (ImGui::SliderFloat("##slider", &dummy_slider, 0.0f, 1.0f, "---")) {
			const float lerped = min_val + dummy_slider * (max_val - min_val);
			for (auto* b : bodies) {
				setter(*b, static_cast<double>(lerped));
			}
			changed = true;
		}
		batch_hover_tooltip(bodies, getter, formatter, label);

		ImGui::SameLine();
		ImGui::SetNextItemWidth(input_width);
		char input_buf[32] = "---";
		if (ImGui::InputText("##input", input_buf, sizeof(input_buf), ImGuiInputTextFlags_EnterReturnsTrue)) {
			if (std::strcmp(input_buf, "---") != 0 && std::strcmp(input_buf, "*") != 0 && input_buf[0] != '\0') {
				char* end_ptr = nullptr;
				const double parsed = std::strtod(input_buf, &end_ptr);
				if (end_ptr != input_buf) {
					for (auto* b : bodies) {
						setter(*b, parsed);
					}
					changed = true;
				}
			}
		}
		batch_hover_tooltip(bodies, getter, formatter, label);

		ImGui::SameLine();
		ImGui::TextUnformatted(label);
	}

	ImGui::SameLine();
	if (ImGui::SmallButton("+/-")) {
		ImGui::OpenPopup("##DeltaPopup");
	}
	render_setting_tooltip("Apply a relative additive offset (+Delta) or scaling factor (*Factor) to this property across all selected bodies.");
	if (ImGui::BeginPopup("##DeltaPopup")) {
		ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Relative Delta: %s", label);
		ImGui::Separator();
		static float delta_offset = 0.0f;
		static float delta_factor = 1.0f;
		ImGui::InputFloat("Additive Delta (+)", &delta_offset, 0.0f, 0.0f, "%.4g");
		if (ImGui::Button("Apply Additive Offset", ImVec2(-1.0f, 22.0f))) {
			for (auto* b : bodies) {
				setter(*b, static_cast<double>(getter(*b)) + static_cast<double>(delta_offset));
			}
			changed = true;
			ImGui::CloseCurrentPopup();
		}
		ImGui::Spacing();
		ImGui::InputFloat("Scale Factor (*)", &delta_factor, 0.0f, 0.0f, "%.4gx");
		if (ImGui::Button("Apply Multiplicative Scale", ImVec2(-1.0f, 22.0f))) {
			for (auto* b : bodies) {
				setter(*b, static_cast<double>(getter(*b)) * static_cast<double>(delta_factor));
			}
			changed = true;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
	ImGui::PopID();
	return changed;
}

}
