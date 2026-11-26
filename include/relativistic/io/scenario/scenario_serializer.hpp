#pragma once

#include "relativistic/core/math/tensor.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <sstream>
#include <iomanip>
#include <optional>
#include <algorithm>
#include <cstring>
#include <cstdint>
#include <random>

namespace Relativistic::IO {

struct ScenarioTimeConfig {
	double warp_factor{1.0};
	double tick_rate_hz{60.0};
	bool is_paused{true};
};

struct ScenarioSkyboxConfig {
	bool enabled{true};
	bool stars_enabled{true};
	double star_density{91.064};
	double star_brightness{1.81};
	bool nebulae_enabled{true};
	double nebula_intensity{0.23};
	bool galaxies_enabled{true};
	double galaxy_density{0.5};
	double galaxy_brightness{0.07};
	bool clusters_enabled{true};
	double cluster_density{0.184};
	double cluster_brightness{0.0};
	bool dust_enabled{true};
	double dust_density{3.85};
	double dust_intensity{1.3};
	bool grid_enabled{false};
	double grid_opacity{0.5};
	double grid_line_width{1.0};
	uint32_t grid_cell_fill_mode{0};
	double grid_cell_opacity{1.0};
	uint32_t grid_cell_color_count{4};
	std::array<std::array<float, 3>, 4> grid_cell_colors{
		std::array<float, 3>{1.0f, 0.0f, 0.0f},
		std::array<float, 3>{0.0f, 0.0f, 1.0f},
		std::array<float, 3>{0.0f, 1.0f, 0.0f},
		std::array<float, 3>{1.0f, 1.0f, 0.0f}
	};
	uint32_t background_source{0};
	uint32_t panorama_id{0};
	std::array<float, 3> background_color{0.0f, 0.0f, 0.0f};
	double orientation_yaw_deg{0.0};
	double orientation_pitch_deg{0.0};
	double orientation_roll_deg{0.0};
	uint32_t seed{56};
};

struct ScenarioAccretionDiskConfig {
	bool enabled{false};
	float inner_radius_scale{1.0f};
	float outer_radius_mass_units{25.0f};
	float peak_temperature_k{12546.0f};
	float floor_temperature_k{1200.0f};
	float brightness{1.0f};
	float opacity_scale{1.0f};
	float doppler_beaming_exponent{0.0f};
	float color_saturation{1.0f};
	float edge_softness{0.3f};
	float temperature_exponent{0.75f};
	float zero_torque_strength{1.0f};
	float tint_strength{0.0f};
	std::array<float, 3> tint{1.0f, 1.0f, 1.0f};
	float rotation_speed_scale{1.0f};
	float ring_amplitude{0.0f};
	float ring_frequency{8.0f};
	float ring_sharpness{3.0f};
	int spiral_arm_count{0};
	float spiral_amplitude{0.0f};
	float spiral_pitch{1.0f};
	float turbulence_amplitude{0.0f};
	float turbulence_scale{5.0f};
	int turbulence_octaves{3};
	float radial_stretch{2.0f};
	float grain_amplitude{0.0f};
	float grain_scale{50.0f};
	uint32_t noise_seed{42};
	uint32_t hydro_model{0};
	float accretion_rate_scale{1.0f};
	float torus_inner_radius{6.0f};
	float torus_center_radius{12.0f};
	float adiabatic_index{1.333333f};
	float optical_depth_scale{1.0f};
	float hydro_sampling_density{2.0f};
};

struct ScenarioJetConfig {
	bool enabled{false};
	uint32_t geometry{0};
	float field_line_index{1.0f};
	uint32_t angular_velocity_law{0};
	float angular_velocity_fraction{0.5f};
	float angular_velocity_variation{0.0f};
	float fixed_angular_velocity{0.0f};
	float toroidal_field_scale{1.0f};
	float rotation_coupling{0.5f};
	float magnetization{10.0f};
	float magnetization_index{0.0f};
	float inner_radius_scale{1.05f};
	float footpoint_edge_deg{15.0f};
	float footpoint_softness_deg{2.0f};
	float outer_radius_mass_units{200.0f};
	float lorentz_factor_inner{1.5f};
	float lorentz_factor_max{10.0f};
	float acceleration_radius_mass_units{50.0f};
	float acceleration_index{1.0f};
	bool doppler_beaming_enabled{true};
	bool gravitational_redshift_enabled{true};
	uint32_t emission_model{0};
	float power_law_index{2.5f};
	float minimum_lorentz_factor{10.0f};
	float non_thermal_fraction{0.1f};
	float electron_temperature_k{1.0e9f};
	float temperature_index{0.0f};
	float observing_frequency_hz{2.3e11f};
	float absorption_scale{1.0f};
	float faraday_scale{1.0f};
	uint32_t display_mode{0};
	bool auto_normalization{true};
	float reference_brightness_temperature_k{1.0e10f};
	float horizon_field_tesla{1.0e4f};
	float core_density_scale{1.0f};
	float brightness{1.0f};
	std::array<float, 3> tint{1.0f, 1.0f, 1.0f};
	float polarization_coherence{0.8f};
	float sampling_density{1.0f};
	int max_samples{32};
	bool use_engine_length_scale{true};
	float meters_per_length_unit{1.477e3f};
};

struct ScenarioBodyConfig {
	std::string name{"CelestialBody"};
	uint32_t body_id{0};
	double mass{1.0};
	double radius{1.0};
	double spin{0.0};
	double charge{0.0};
	bool enabled{true};
	bool is_spacetime_source{false};
	std::array<float, 4> color{0.62f, 0.75f, 1.0f, 1.0f};
	std::array<float, 4> color_secondary{0.18f, 0.30f, 0.75f, 1.0f};
	std::array<float, 4> color_tertiary{0.95f, 0.95f, 0.98f, 1.0f};
	double magnetic_moment{0.0};
	double rotation_speed{0.0};
	double friction_coefficient{0.0};
	double restitution{0.5};
	double elasticity{0.0};
	double integrity{1.0};
	double lifetime{0.0};
	double temperature{0.0};
	double heat_capacity{0.0};
	double absorption_factor{1.0};
	double transmission_factor{0.0};
	double critical_temperature{0.0};
	double cold_resistance{0.0};
	double hot_resistance{0.0};
	double youngs_modulus_cold{1.0e9};
	double youngs_modulus_hot{1.0e9};
	std::string composition{};
	std::array<double, 4> initial_position{0.0, 10.0, std::numbers::pi_v<double> / 2.0, 0.0};
	std::array<double, 4> initial_velocity{1.0, 0.0, 0.0, 0.1};
	double quadrupole_moment{0.0};
	double j2{0.0};
	double j3{0.0};
	double j4{0.0};
	double reference_radius{0.0};
	uint32_t preset_3d{1};
	uint32_t surface_texture_mode{0};
	uint32_t geometry_model{0};
	float surface_noise_scale{10.0f};
	float surface_roughness{0.5f};
	float texture_detail_scale{1.0f};
	float polar_cap_strength{0.0f};
	float night_side_light_intensity{0.0f};
	bool ring_system_enabled{false};
	uint32_t atmosphere_mode{2};
	float atmosphere_thickness{0.0f};
	std::array<float, 4> atmosphere_color{0.3f, 0.6f, 1.0f, 1.0f};
	float emission_intensity{0.0f};
	float specular_roughness{0.5f};
	float rotation_speed_3d{0.0f};
	std::array<double, 3> rotation_axis_3d{0.0, 0.0, 1.0};
	std::array<double, 3> orientation_euler{0.0, 0.0, 0.0};
	std::array<double, 3> spin_axis{0.0, 0.0, 1.0};
	uint32_t earth_map_variant{0};
	uint32_t earth_map_quality{0};
	float earth_terminator_softness{0.15f};
	int layer_template_1{-1};
	int layer_template_2{-1};
	bool has_orbit{false};
	std::string parent_name{};
	int32_t parent_id{-2};
	double semi_major_axis{0.0};
	double eccentricity{0.0};
	double inclination_deg{0.0};
	double longitude_node_deg{0.0};
	double argument_periapsis_deg{0.0};
	double true_anomaly_deg{0.0};
	uint32_t count{1};
	bool disk_enabled{false};
	float disk_inner_scale{1.0f};
	float disk_outer_rg{25.0f};
	float disk_peak_temp{15000.0f};
	float disk_floor_temp{1200.0f};
	float disk_brightness{1.0f};
	float disk_opacity{1.0f};
	float disk_beaming{0.0f};
	float disk_saturation{1.0f};
	std::string mass_unit{"kg"};
	std::string radius_unit{"m"};
	std::string position_unit{"m"};
	std::string velocity_unit{"m/s"};
	std::string temperature_unit{"K"};
	std::string charge_unit{"C"};
};

struct ScenarioObserverConfig {
	std::string name{"PrimaryCamera"};
	std::array<double, 4> position{0.0, 50.0, std::numbers::pi_v<double> / 2.0, 0.0};
	std::array<double, 4> four_velocity{1.0, 0.0, 0.0, 0.0};
	double field_of_view_deg{60.0};
	uint32_t resolution_x{1920};
	uint32_t resolution_y{1080};
	std::array<double, 3> orientation{0.0, 180.0, 0.0};
	bool has_explicit_orientation{false};
	uint32_t orientation_convention{1};
	std::string position_unit{"m"};
	std::string fov_unit{"deg"};
	std::string target_name{};
	int32_t target_body_id{-1};
	bool has_target{false};
	std::string target_mode{"center_of_mass"};
	std::array<double, 3> look_at_offset{0.0, 0.0, 0.0};
};

struct ScenarioPatternConfig {
	std::string pattern_type{"asteroid_belt"};
	std::string preset{"main_asteroid_belt"};
	uint32_t object_count{150};
	std::string parent_name{};
	int32_t parent_id{-2};
	std::array<double, 3> center_position{0.0, 0.0, 0.0};
	std::array<double, 3> center_velocity{0.0, 0.0, 0.0};
	double plane_inclination_deg{0.0};
	double plane_longitude_node_deg{0.0};
	double plane_argument_periapsis_deg{0.0};
	double mean_semi_major_axis{32.0};
	double radial_thickness{12.0};
	std::string radial_distribution{"uniform"};
	double radial_gaussian_sigma{2.5};
	double radial_power_law_index{-1.5};
	bool kirkwood_gaps_enabled{false};
	int32_t kirkwood_gap_count{3};
	double kirkwood_resonance_depth{0.85};
	double vertical_scale_height{1.0};
	std::string vertical_distribution{"gaussian"};
	double mean_eccentricity{0.06};
	double eccentricity_dispersion{0.035};
	std::string eccentricity_distribution{"rayleigh"};
	double mean_inclination_dispersion_deg{3.5};
	std::string inclination_distribution{"rayleigh"};
	std::string angular_distribution{"uniform"};
	double angular_opening_deg{360.0};
	double angular_center_deg{0.0};
	double lagrangian_clustering_strength{0.8};
	std::string velocity_mode{"keplerian_circular"};
	double mean_velocity_multiplier{1.0};
	double velocity_dispersion_fraction{0.03};
	std::string velocity_distribution{"maxwell_boltzmann"};
	double prograde_fraction{1.0};
	std::array<double, 3> fixed_velocity_vector{0.0, 0.0, 0.0};
	double expansion_speed{0.05};
	std::string spherical_type{"plummer_sphere"};
	double thin_shell_radius{28.0};
	double plummer_scale_radius{16.0};
	double uniform_sphere_radius{25.0};
	double shell_inner_radius{14.0};
	double shell_outer_radius{42.0};
	double spherical_power_law_gamma{-2.0};
	double solid_body_rotation_speed{0.04};
	double solid_body_axis_tilt_deg{0.0};
	double solid_body_axis_azimuth_deg{0.0};
	std::string archetype{"asteroid"};
	std::string mass_distribution{"power_law"};
	double fixed_mass{1.0e-7};
	double min_mass{1.0e-9};
	double max_mass{2.0e-6};
	double mass_power_law_alpha{1.833};
	bool radius_from_density{true};
	double mean_density{2700.0};
	double density_dispersion_pct{15.0};
	double fixed_radius{0.05};
	double min_radius{0.005};
	double max_radius{0.4};
	bool temperature_equilibrium{true};
	double reference_temperature_k{250.0};
	double temperature_falloff_exponent{0.5};
	double color_jitter{0.2};
	std::string name_prefix{"Asteroid"};
	bool avoid_horizon_overlap{true};
	bool avoid_mutual_collisions{true};
	uint64_t seed{1337};
};

struct ScenarioIntegratorConfig {
	std::string scheme{"RK45"};
	double initial_step{0.01};
	double min_step{1e-8};
	double max_step{10.0};
	double relative_tolerance{1e-10};
	double absolute_tolerance{1e-14};
	uint64_t max_evaluations{1000000};
};

struct ScenarioOutputConfig {
	bool fits_enabled{true};
	bool hdf5_enabled{true};
	bool vtk_enabled{true};
	std::string export_directory{"./output"};
};

struct ScenarioInteractionConfig {
	bool electricity_enabled{false};
	bool magnetism_enabled{false};
	double vacuum_permittivity{8.8541878128e-12};
	double vacuum_permeability{1.25663706212e-6};
	bool collisions_enabled{false};
	uint32_t collision_response_model{0};
	bool collision_consider_rotation{true};
	bool collision_consider_friction{true};
	double collision_restitution_multiplier{1.0};
	double collision_stiffness_scale{1.0e-9};
	double collision_position_correction_factor{0.2};
	bool thermodynamics_enabled{false};
	double ambient_temperature_kelvin{-1.0};
	double radiative_coupling_scale{1.0};
	bool fragmentation_enabled{false};
	bool fragmentation_tidal_stress_enabled{false};
	double minimum_fragment_mass{1.0e-6};
	uint32_t max_fragments_per_event{2};
	double collision_energy_to_integrity_loss{1.0e-6};
	double tidal_stress_to_integrity_loss{1.0e-6};
	bool annihilation_enabled{false};
	double annihilation_contact_scale{1.0};
	bool annihilation_require_opposite_charge{true};
};

struct ScenarioUnitsConfig {
	std::string distance_unit{"m"};
	std::string mass_unit{"kg"};
	std::string velocity_unit{"m/s"};
	std::string time_unit{"s"};
	std::string energy_unit{"J"};
	std::string temperature_unit{"K"};
	std::string charge_unit{"C"};
	std::string angle_unit{"rad"};
};

struct ScenarioValidationResult {
	bool is_valid{true};
	std::string error_message{};
};

struct ScenarioDefinition {
	uint32_t format_version{1};
	std::string scenario_name{"RelativisticSimulation"};
	std::string description{"Physical Spacetime Simulation Scenario"};
	std::string author{"Unknown"};
	std::string created_at{};
	std::string version_tag{"1.0.0"};
	ScenarioUnitsConfig units{};
	std::string metric_type{"Schwarzschild"};
	double central_mass{1.0};
	double central_spin{0.0};
	double central_charge{0.0};
	double cosmological_lambda{0.0};
	double wormhole_throat{1.0};
	double warp_velocity{0.0};
	double warp_bubble_radius{15.0};
	double warp_wall_sharpness{0.4};
	double wormhole_tidal_potential{0.0};
	double teves_critical_acceleration{1.0e-3};
	double speed_of_light{1.0};
	double gravitational_constant{1.0};
	double pole_guard_precision_scale{5.5};
	double far_field_step_scale{7.8};

	ScenarioTimeConfig time{};
	bool has_skybox{false};
	ScenarioSkyboxConfig skybox{};
	bool has_primary_disk{false};
	ScenarioAccretionDiskConfig primary_disk{};
	bool has_primary_jet{false};
	ScenarioJetConfig primary_jet{};

	std::vector<ScenarioPatternConfig> patterns{};
	std::vector<ScenarioBodyConfig> bodies{};
	std::vector<ScenarioObserverConfig> observers{};
	ScenarioIntegratorConfig integrator{};
	ScenarioOutputConfig output{};
	ScenarioInteractionConfig interactions{};
};

class ScenarioValueSampler {
public:
	[[nodiscard]] static double sample(std::string_view text, double fallback = 0.0) noexcept {
		auto trim = [](std::string_view sv) noexcept -> std::string_view {
			while (!sv.empty() && (sv.front() == ' ' || sv.front() == '\t' || sv.front() == '\r' || sv.front() == '"' || sv.front() == '\'')) sv.remove_prefix(1);
			while (!sv.empty() && (sv.back() == ' ' || sv.back() == '\t' || sv.back() == '\r' || sv.back() == '"' || sv.back() == '\'')) sv.remove_suffix(1);
			return sv;
		};
		text = trim(text);
		if (text.empty()) return fallback;

		static thread_local std::mt19937_64 rng{std::random_device{}()};

		if (text.front() == '[' && text.back() == ']') {
			std::string_view inner = text.substr(1, text.size() - 2);
			std::vector<std::string_view> tokens;
			int depth = 0;
			size_t start = 0;
			for (size_t i = 0; i < inner.size(); ++i) {
				if (inner[i] == '[' || inner[i] == '(') ++depth;
				else if (inner[i] == ']' || inner[i] == ')') --depth;
				else if (inner[i] == ',' && depth == 0) {
					tokens.push_back(trim(inner.substr(start, i - start)));
					start = i + 1;
				}
			}
			if (start < inner.size()) tokens.push_back(trim(inner.substr(start)));

			if (tokens.size() == 2) {
				double a = std::strtod(std::string(tokens[0]).c_str(), nullptr);
				double b = std::strtod(std::string(tokens[1]).c_str(), nullptr);
				if (a > b) std::swap(a, b);
				std::uniform_real_distribution<double> dist(a, b);
				return dist(rng);
			} else if (tokens.size() >= 3) {
				double a = std::strtod(std::string(tokens[0]).c_str(), nullptr);
				double b = std::strtod(std::string(tokens[1]).c_str(), nullptr);
				std::string mode;
				for (char c : tokens[2]) mode.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
				if (mode.find("log") != std::string::npos) {
					double lo = std::min(a, b);
					double hi = std::max(a, b);
					if (lo <= 0.0) lo = 1e-12;
					if (hi <= lo) hi = lo * 10.0;
					std::uniform_real_distribution<double> dist(std::log(lo), std::log(hi));
					return std::exp(dist(rng));
				} else if (mode.find("gauss") != std::string::npos || mode.find("norm") != std::string::npos) {
					std::normal_distribution<double> dist(a, std::max(b, 1e-12));
					return dist(rng);
				} else {
					if (a > b) std::swap(a, b);
					std::uniform_real_distribution<double> dist(a, b);
					return dist(rng);
				}
			}
		}

		std::string lowered;
		lowered.reserve(text.size());
		for (char c : text) lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));

		if (lowered.starts_with("uniform(") || lowered.starts_with("range(") || lowered.starts_with("log_uniform(") || lowered.starts_with("gaussian(") || lowered.starts_with("normal(") || lowered.starts_with("gauss(")) {
			size_t open_p = text.find('(');
			size_t close_p = text.rfind(')');
			if (open_p != std::string_view::npos && close_p != std::string_view::npos && close_p > open_p) {
				std::string_view inner = text.substr(open_p + 1, close_p - open_p - 1);
				size_t comma = inner.find(',');
				if (comma != std::string_view::npos) {
					double a = std::strtod(std::string(trim(inner.substr(0, comma))).c_str(), nullptr);
					double b = std::strtod(std::string(trim(inner.substr(comma + 1))).c_str(), nullptr);
					if (lowered.starts_with("log")) {
						double lo = std::min(a, b);
						double hi = std::max(a, b);
						if (lo <= 0.0) lo = 1e-12;
						if (hi <= lo) hi = lo * 10.0;
						std::uniform_real_distribution<double> dist(std::log(lo), std::log(hi));
						return std::exp(dist(rng));
					} else if (lowered.starts_with("gauss") || lowered.starts_with("norm")) {
						std::normal_distribution<double> dist(a, std::max(b, 1e-12));
						return dist(rng);
					} else {
						if (a > b) std::swap(a, b);
						std::uniform_real_distribution<double> dist(a, b);
						return dist(rng);
					}
				}
			}
		}

		char* end_ptr = nullptr;
		const double parsed = std::strtod(std::string(text).c_str(), &end_ptr);
		return (end_ptr != text.data()) ? parsed : fallback;
	}
};

class ScenarioSerializer {
public:
	[[nodiscard]] static std::string to_yaml(const ScenarioDefinition& s) {
		std::ostringstream ss;
		ss << std::setprecision(15);
		ss << "format_version: " << s.format_version << "\n";
		ss << "scenario_name: \"" << s.scenario_name << "\"\n";
		ss << "description: \"" << s.description << "\"\n";
		ss << "author: \"" << s.author << "\"\n";
		ss << "created_at: \"" << s.created_at << "\"\n";
		ss << "version_tag: \"" << s.version_tag << "\"\n";
		ss << "units:\n";
		ss << "  distance: \"" << s.units.distance_unit << "\"\n";
		ss << "  mass: \"" << s.units.mass_unit << "\"\n";
		ss << "  velocity: \"" << s.units.velocity_unit << "\"\n";
		ss << "  time: \"" << s.units.time_unit << "\"\n";
		ss << "  energy: \"" << s.units.energy_unit << "\"\n";
		ss << "  temperature: \"" << s.units.temperature_unit << "\"\n";
		ss << "  charge: \"" << s.units.charge_unit << "\"\n";
		ss << "  angle: \"" << s.units.angle_unit << "\"\n";
		ss << "spacetime:\n";
		ss << "  metric_type: \"" << s.metric_type << "\"\n";
		ss << "  central_mass: " << s.central_mass << "\n";
		ss << "  central_spin: " << s.central_spin << "\n";
		ss << "  central_charge: " << s.central_charge << "\n";
		ss << "  cosmological_lambda: " << s.cosmological_lambda << "\n";
		ss << "  wormhole_throat: " << s.wormhole_throat << "\n";
		ss << "  warp_velocity: " << s.warp_velocity << "\n";
		ss << "  warp_bubble_radius: " << s.warp_bubble_radius << "\n";
		ss << "  warp_wall_sharpness: " << s.warp_wall_sharpness << "\n";
		ss << "  wormhole_tidal_potential: " << s.wormhole_tidal_potential << "\n";
		ss << "  teves_critical_acceleration: " << s.teves_critical_acceleration << "\n";
		ss << "  pole_guard_precision_scale: " << s.pole_guard_precision_scale << "\n";
		ss << "  far_field_step_scale: " << s.far_field_step_scale << "\n";
		ss << "  speed_of_light: " << s.speed_of_light << "\n";
		ss << "  gravitational_constant: " << s.gravitational_constant << "\n";

		ss << "time:\n";
		ss << "  warp_factor: " << s.time.warp_factor << "\n";
		ss << "  tick_rate: " << s.time.tick_rate_hz << "\n";
		ss << "  is_paused: " << (s.time.is_paused ? "true" : "false") << "\n";

		if (s.has_skybox) {
			ss << "skybox:\n";
			ss << "  enabled: " << (s.skybox.enabled ? "true" : "false") << "\n";
			ss << "  stars_enabled: " << (s.skybox.stars_enabled ? "true" : "false") << "\n";
			ss << "  star_density: " << s.skybox.star_density << "\n";
			ss << "  star_brightness: " << s.skybox.star_brightness << "\n";
			ss << "  nebula_enabled: " << (s.skybox.nebulae_enabled ? "true" : "false") << "\n";
			ss << "  nebula_intensity: " << s.skybox.nebula_intensity << "\n";
			ss << "  galaxies_enabled: " << (s.skybox.galaxies_enabled ? "true" : "false") << "\n";
			ss << "  galaxy_density: " << s.skybox.galaxy_density << "\n";
			ss << "  galaxy_brightness: " << s.skybox.galaxy_brightness << "\n";
			ss << "  clusters_enabled: " << (s.skybox.clusters_enabled ? "true" : "false") << "\n";
			ss << "  cluster_density: " << s.skybox.cluster_density << "\n";
			ss << "  cluster_brightness: " << s.skybox.cluster_brightness << "\n";
			ss << "  dust_enabled: " << (s.skybox.dust_enabled ? "true" : "false") << "\n";
			ss << "  dust_density: " << s.skybox.dust_density << "\n";
			ss << "  dust_intensity: " << s.skybox.dust_intensity << "\n";
			ss << "  grid_enabled: " << (s.skybox.grid_enabled ? "true" : "false") << "\n";
			ss << "  grid_opacity: " << s.skybox.grid_opacity << "\n";
			ss << "  grid_line_width: " << s.skybox.grid_line_width << "\n";
			ss << "  grid_cell_fill_mode: " << s.skybox.grid_cell_fill_mode << "\n";
			ss << "  grid_cell_opacity: " << s.skybox.grid_cell_opacity << "\n";
			ss << "  grid_cell_color_count: " << s.skybox.grid_cell_color_count << "\n";
			ss << "  grid_cell_color_0: [" << s.skybox.grid_cell_colors[0][0] << ", " << s.skybox.grid_cell_colors[0][1] << ", " << s.skybox.grid_cell_colors[0][2] << "]\n";
			ss << "  grid_cell_color_1: [" << s.skybox.grid_cell_colors[1][0] << ", " << s.skybox.grid_cell_colors[1][1] << ", " << s.skybox.grid_cell_colors[1][2] << "]\n";
			ss << "  grid_cell_color_2: [" << s.skybox.grid_cell_colors[2][0] << ", " << s.skybox.grid_cell_colors[2][1] << ", " << s.skybox.grid_cell_colors[2][2] << "]\n";
			ss << "  grid_cell_color_3: [" << s.skybox.grid_cell_colors[3][0] << ", " << s.skybox.grid_cell_colors[3][1] << ", " << s.skybox.grid_cell_colors[3][2] << "]\n";
			ss << "  background_source: " << s.skybox.background_source << "\n";
			ss << "  panorama_id: " << s.skybox.panorama_id << "\n";
			ss << "  background_color: [" << s.skybox.background_color[0] << ", " << s.skybox.background_color[1] << ", " << s.skybox.background_color[2] << "]\n";
			ss << "  orientation_yaw_deg: " << s.skybox.orientation_yaw_deg << "\n";
			ss << "  seed: " << s.skybox.seed << "\n";
		}

		if (s.has_primary_disk) {
			ss << "primary_disk:\n";
			ss << "  enabled: " << (s.primary_disk.enabled ? "true" : "false") << "\n";
			ss << "  inner_radius_scale: " << s.primary_disk.inner_radius_scale << "\n";
			ss << "  outer_radius_mass_units: " << s.primary_disk.outer_radius_mass_units << "\n";
			ss << "  peak_temperature_k: " << s.primary_disk.peak_temperature_k << "\n";
			ss << "  floor_temperature_k: " << s.primary_disk.floor_temperature_k << "\n";
			ss << "  brightness: " << s.primary_disk.brightness << "\n";
			ss << "  opacity_scale: " << s.primary_disk.opacity_scale << "\n";
			ss << "  doppler_beaming_exponent: " << s.primary_disk.doppler_beaming_exponent << "\n";
			ss << "  color_saturation: " << s.primary_disk.color_saturation << "\n";
			ss << "  edge_softness: " << s.primary_disk.edge_softness << "\n";
			ss << "  temperature_exponent: " << s.primary_disk.temperature_exponent << "\n";
			ss << "  zero_torque_strength: " << s.primary_disk.zero_torque_strength << "\n";
			ss << "  tint_strength: " << s.primary_disk.tint_strength << "\n";
			ss << "  tint: [" << s.primary_disk.tint[0] << ", " << s.primary_disk.tint[1] << ", " << s.primary_disk.tint[2] << "]\n";
			ss << "  rotation_speed_scale: " << s.primary_disk.rotation_speed_scale << "\n";
			ss << "  ring_amplitude: " << s.primary_disk.ring_amplitude << "\n";
			ss << "  ring_frequency: " << s.primary_disk.ring_frequency << "\n";
			ss << "  ring_sharpness: " << s.primary_disk.ring_sharpness << "\n";
			ss << "  spiral_arm_count: " << s.primary_disk.spiral_arm_count << "\n";
			ss << "  spiral_amplitude: " << s.primary_disk.spiral_amplitude << "\n";
			ss << "  spiral_pitch: " << s.primary_disk.spiral_pitch << "\n";
			ss << "  turbulence_amplitude: " << s.primary_disk.turbulence_amplitude << "\n";
			ss << "  turbulence_scale: " << s.primary_disk.turbulence_scale << "\n";
			ss << "  turbulence_octaves: " << s.primary_disk.turbulence_octaves << "\n";
			ss << "  radial_stretch: " << s.primary_disk.radial_stretch << "\n";
			ss << "  grain_amplitude: " << s.primary_disk.grain_amplitude << "\n";
			ss << "  grain_scale: " << s.primary_disk.grain_scale << "\n";
			ss << "  noise_seed: " << s.primary_disk.noise_seed << "\n";
			ss << "  hydro_model: " << s.primary_disk.hydro_model << "\n";
			ss << "  accretion_rate_scale: " << s.primary_disk.accretion_rate_scale << "\n";
			ss << "  torus_inner_radius: " << s.primary_disk.torus_inner_radius << "\n";
			ss << "  torus_center_radius: " << s.primary_disk.torus_center_radius << "\n";
			ss << "  adiabatic_index: " << s.primary_disk.adiabatic_index << "\n";
			ss << "  optical_depth_scale: " << s.primary_disk.optical_depth_scale << "\n";
			ss << "  hydro_sampling_density: " << s.primary_disk.hydro_sampling_density << "\n";
		}

		if (s.has_primary_jet) {
			ss << "primary_jet:\n";
			ss << "  enabled: " << (s.primary_jet.enabled ? "true" : "false") << "\n";
			ss << "  geometry: " << s.primary_jet.geometry << "\n";
			ss << "  field_line_index: " << s.primary_jet.field_line_index << "\n";
			ss << "  angular_velocity_law: " << s.primary_jet.angular_velocity_law << "\n";
			ss << "  angular_velocity_fraction: " << s.primary_jet.angular_velocity_fraction << "\n";
			ss << "  angular_velocity_variation: " << s.primary_jet.angular_velocity_variation << "\n";
			ss << "  fixed_angular_velocity: " << s.primary_jet.fixed_angular_velocity << "\n";
			ss << "  toroidal_field_scale: " << s.primary_jet.toroidal_field_scale << "\n";
			ss << "  rotation_coupling: " << s.primary_jet.rotation_coupling << "\n";
			ss << "  magnetization: " << s.primary_jet.magnetization << "\n";
			ss << "  magnetization_index: " << s.primary_jet.magnetization_index << "\n";
			ss << "  inner_radius_scale: " << s.primary_jet.inner_radius_scale << "\n";
			ss << "  footpoint_edge_deg: " << s.primary_jet.footpoint_edge_deg << "\n";
			ss << "  footpoint_softness_deg: " << s.primary_jet.footpoint_softness_deg << "\n";
			ss << "  outer_radius_mass_units: " << s.primary_jet.outer_radius_mass_units << "\n";
			ss << "  lorentz_factor_inner: " << s.primary_jet.lorentz_factor_inner << "\n";
			ss << "  lorentz_factor_max: " << s.primary_jet.lorentz_factor_max << "\n";
			ss << "  acceleration_radius_mass_units: " << s.primary_jet.acceleration_radius_mass_units << "\n";
			ss << "  acceleration_index: " << s.primary_jet.acceleration_index << "\n";
			ss << "  doppler_beaming_enabled: " << (s.primary_jet.doppler_beaming_enabled ? "true" : "false") << "\n";
			ss << "  gravitational_redshift_enabled: " << (s.primary_jet.gravitational_redshift_enabled ? "true" : "false") << "\n";
			ss << "  emission_model: " << s.primary_jet.emission_model << "\n";
			ss << "  power_law_index: " << s.primary_jet.power_law_index << "\n";
			ss << "  minimum_lorentz_factor: " << s.primary_jet.minimum_lorentz_factor << "\n";
			ss << "  non_thermal_fraction: " << s.primary_jet.non_thermal_fraction << "\n";
			ss << "  electron_temperature_k: " << s.primary_jet.electron_temperature_k << "\n";
			ss << "  temperature_index: " << s.primary_jet.temperature_index << "\n";
			ss << "  observing_frequency_hz: " << s.primary_jet.observing_frequency_hz << "\n";
			ss << "  absorption_scale: " << s.primary_jet.absorption_scale << "\n";
			ss << "  faraday_scale: " << s.primary_jet.faraday_scale << "\n";
			ss << "  display_mode: " << s.primary_jet.display_mode << "\n";
			ss << "  auto_normalization: " << (s.primary_jet.auto_normalization ? "true" : "false") << "\n";
			ss << "  reference_brightness_temperature_k: " << s.primary_jet.reference_brightness_temperature_k << "\n";
			ss << "  horizon_field_tesla: " << s.primary_jet.horizon_field_tesla << "\n";
			ss << "  core_density_scale: " << s.primary_jet.core_density_scale << "\n";
			ss << "  brightness: " << s.primary_jet.brightness << "\n";
			ss << "  tint: [" << s.primary_jet.tint[0] << ", " << s.primary_jet.tint[1] << ", " << s.primary_jet.tint[2] << "]\n";
			ss << "  polarization_coherence: " << s.primary_jet.polarization_coherence << "\n";
			ss << "  sampling_density: " << s.primary_jet.sampling_density << "\n";
			ss << "  max_samples: " << s.primary_jet.max_samples << "\n";
			ss << "  use_engine_length_scale: " << (s.primary_jet.use_engine_length_scale ? "true" : "false") << "\n";
			ss << "  meters_per_length_unit: " << s.primary_jet.meters_per_length_unit << "\n";
		}

		ss << "integrator:\n";
		ss << "  scheme: \"" << s.integrator.scheme << "\"\n";
		ss << "  initial_step: " << s.integrator.initial_step << "\n";
		ss << "  min_step: " << s.integrator.min_step << "\n";
		ss << "  max_step: " << s.integrator.max_step << "\n";
		ss << "  relative_tolerance: " << s.integrator.relative_tolerance << "\n";
		ss << "  absolute_tolerance: " << s.integrator.absolute_tolerance << "\n";

		ss << "output:\n";
		ss << "  fits_enabled: " << (s.output.fits_enabled ? "true" : "false") << "\n";
		ss << "  hdf5_enabled: " << (s.output.hdf5_enabled ? "true" : "false") << "\n";
		ss << "  vtk_enabled: " << (s.output.vtk_enabled ? "true" : "false") << "\n";
		ss << "  export_directory: \"" << s.output.export_directory << "\"\n";

		ss << "interactions:\n";
		ss << "  electricity_enabled: " << (s.interactions.electricity_enabled ? "true" : "false") << "\n";
		ss << "  magnetism_enabled: " << (s.interactions.magnetism_enabled ? "true" : "false") << "\n";
		ss << "  vacuum_permittivity: " << s.interactions.vacuum_permittivity << "\n";
		ss << "  vacuum_permeability: " << s.interactions.vacuum_permeability << "\n";
		ss << "  collisions_enabled: " << (s.interactions.collisions_enabled ? "true" : "false") << "\n";
		ss << "  collision_response_model: " << s.interactions.collision_response_model << "\n";
		ss << "  collision_consider_rotation: " << (s.interactions.collision_consider_rotation ? "true" : "false") << "\n";
		ss << "  collision_consider_friction: " << (s.interactions.collision_consider_friction ? "true" : "false") << "\n";
		ss << "  collision_restitution_multiplier: " << s.interactions.collision_restitution_multiplier << "\n";
		ss << "  collision_stiffness_scale: " << s.interactions.collision_stiffness_scale << "\n";
		ss << "  collision_position_correction_factor: " << s.interactions.collision_position_correction_factor << "\n";
		ss << "  thermodynamics_enabled: " << (s.interactions.thermodynamics_enabled ? "true" : "false") << "\n";
		ss << "  ambient_temperature_kelvin: " << s.interactions.ambient_temperature_kelvin << "\n";
		ss << "  radiative_coupling_scale: " << s.interactions.radiative_coupling_scale << "\n";
		ss << "  fragmentation_enabled: " << (s.interactions.fragmentation_enabled ? "true" : "false") << "\n";
		ss << "  fragmentation_tidal_stress_enabled: " << (s.interactions.fragmentation_tidal_stress_enabled ? "true" : "false") << "\n";
		ss << "  minimum_fragment_mass: " << s.interactions.minimum_fragment_mass << "\n";
		ss << "  max_fragments_per_event: " << s.interactions.max_fragments_per_event << "\n";
		ss << "  collision_energy_to_integrity_loss: " << s.interactions.collision_energy_to_integrity_loss << "\n";
		ss << "  tidal_stress_to_integrity_loss: " << s.interactions.tidal_stress_to_integrity_loss << "\n";
		ss << "  annihilation_enabled: " << (s.interactions.annihilation_enabled ? "true" : "false") << "\n";
		ss << "  annihilation_contact_scale: " << s.interactions.annihilation_contact_scale << "\n";
		ss << "  annihilation_require_opposite_charge: " << (s.interactions.annihilation_require_opposite_charge ? "true" : "false") << "\n";

		ss << "bodies:\n";
		for (const auto& b : s.bodies) {
			ss << "  - name: \"" << b.name << "\"\n";
			ss << "    body_id: " << b.body_id << "\n";
			ss << "    mass: " << b.mass << "\n";
			ss << "    radius: " << b.radius << "\n";
			ss << "    spin: " << b.spin << "\n";
			ss << "    charge: " << b.charge << "\n";
			ss << "    enabled: " << (b.enabled ? "true" : "false") << "\n";
			ss << "    color: [" << b.color[0] << ", " << b.color[1] << ", " << b.color[2] << ", " << b.color[3] << "]\n";
			ss << "    color_secondary: [" << b.color_secondary[0] << ", " << b.color_secondary[1] << ", " << b.color_secondary[2] << ", " << b.color_secondary[3] << "]\n";
			ss << "    color_tertiary: [" << b.color_tertiary[0] << ", " << b.color_tertiary[1] << ", " << b.color_tertiary[2] << ", " << b.color_tertiary[3] << "]\n";
			ss << "    is_spacetime_source: " << (b.is_spacetime_source ? "true" : "false") << "\n";
			ss << "    preset_3d: " << b.preset_3d << "\n";
			ss << "    surface_texture_mode: " << b.surface_texture_mode << "\n";
			ss << "    geometry_model: " << b.geometry_model << "\n";
			ss << "    surface_noise_scale: " << b.surface_noise_scale << "\n";
			ss << "    surface_roughness: " << b.surface_roughness << "\n";
			ss << "    texture_detail_scale: " << b.texture_detail_scale << "\n";
			ss << "    polar_cap_strength: " << b.polar_cap_strength << "\n";
			ss << "    night_side_light_intensity: " << b.night_side_light_intensity << "\n";
			ss << "    ring_system_enabled: " << (b.ring_system_enabled ? "true" : "false") << "\n";
			ss << "    atmosphere_mode: " << b.atmosphere_mode << "\n";
			ss << "    atmosphere_thickness: " << b.atmosphere_thickness << "\n";
			ss << "    atmosphere_color: [" << b.atmosphere_color[0] << ", " << b.atmosphere_color[1] << ", " << b.atmosphere_color[2] << ", " << b.atmosphere_color[3] << "]\n";
			ss << "    emission_intensity: " << b.emission_intensity << "\n";
			ss << "    specular_roughness: " << b.specular_roughness << "\n";
			ss << "    rotation_speed_3d: " << b.rotation_speed_3d << "\n";
			ss << "    rotation_axis_3d: [" << b.rotation_axis_3d[0] << ", " << b.rotation_axis_3d[1] << ", " << b.rotation_axis_3d[2] << "]\n";
			ss << "    orientation_euler: [" << b.orientation_euler[0] << ", " << b.orientation_euler[1] << ", " << b.orientation_euler[2] << "]\n";
			ss << "    spin_axis: [" << b.spin_axis[0] << ", " << b.spin_axis[1] << ", " << b.spin_axis[2] << "]\n";
			ss << "    earth_map_variant: " << b.earth_map_variant << "\n";
			ss << "    earth_map_quality: " << b.earth_map_quality << "\n";
			ss << "    earth_terminator_softness: " << b.earth_terminator_softness << "\n";
			ss << "    layer_template_1: " << b.layer_template_1 << "\n";
			ss << "    layer_template_2: " << b.layer_template_2 << "\n";
			if (b.has_orbit) {
				ss << "    has_orbit: true\n";
				ss << "    parent_name: \"" << b.parent_name << "\"\n";
				ss << "    parent_id: " << b.parent_id << "\n";
				ss << "    semi_major_axis: " << b.semi_major_axis << "\n";
				ss << "    eccentricity: " << b.eccentricity << "\n";
				ss << "    inclination_deg: " << b.inclination_deg << "\n";
				ss << "    longitude_node_deg: " << b.longitude_node_deg << "\n";
				ss << "    argument_periapsis_deg: " << b.argument_periapsis_deg << "\n";
				ss << "    true_anomaly_deg: " << b.true_anomaly_deg << "\n";
			}
			if (b.is_spacetime_source && b.disk_enabled) {
				ss << "    disk_enabled: true\n";
				ss << "    disk_inner_scale: " << b.disk_inner_scale << "\n";
				ss << "    disk_outer_rg: " << b.disk_outer_rg << "\n";
				ss << "    disk_peak_temp: " << b.disk_peak_temp << "\n";
				ss << "    disk_floor_temp: " << b.disk_floor_temp << "\n";
				ss << "    disk_brightness: " << b.disk_brightness << "\n";
				ss << "    disk_opacity: " << b.disk_opacity << "\n";
				ss << "    disk_beaming: " << b.disk_beaming << "\n";
				ss << "    disk_saturation: " << b.disk_saturation << "\n";
			}
			ss << "    magnetic_moment: " << b.magnetic_moment << "\n";
			ss << "    rotation_speed: " << b.rotation_speed << "\n";
			ss << "    friction_coefficient: " << b.friction_coefficient << "\n";
			ss << "    restitution: " << b.restitution << "\n";
			ss << "    elasticity: " << b.elasticity << "\n";
			ss << "    integrity: " << b.integrity << "\n";
			ss << "    lifetime: " << b.lifetime << "\n";
			ss << "    temperature: " << b.temperature << "\n";
			ss << "    heat_capacity: " << b.heat_capacity << "\n";
			ss << "    absorption_factor: " << b.absorption_factor << "\n";
			ss << "    transmission_factor: " << b.transmission_factor << "\n";
			ss << "    critical_temperature: " << b.critical_temperature << "\n";
			ss << "    cold_resistance: " << b.cold_resistance << "\n";
			ss << "    hot_resistance: " << b.hot_resistance << "\n";
			ss << "    youngs_modulus_cold: " << b.youngs_modulus_cold << "\n";
			ss << "    youngs_modulus_hot: " << b.youngs_modulus_hot << "\n";
			ss << "    composition: \"" << b.composition << "\"\n";
			ss << "    position: [" << b.initial_position[0] << ", " << b.initial_position[1] << ", " << b.initial_position[2] << ", " << b.initial_position[3] << "]\n";
			ss << "    velocity: [" << b.initial_velocity[0] << ", " << b.initial_velocity[1] << ", " << b.initial_velocity[2] << ", " << b.initial_velocity[3] << "]\n";
			ss << "    quadrupole: " << b.quadrupole_moment << "\n";
			ss << "    j2: " << b.j2 << "\n";
			ss << "    j3: " << b.j3 << "\n";
			ss << "    j4: " << b.j4 << "\n";
			ss << "    reference_radius: " << b.reference_radius << "\n";
			ss << "    mass_unit: \"" << b.mass_unit << "\"\n";
			ss << "    radius_unit: \"" << b.radius_unit << "\"\n";
			ss << "    position_unit: \"" << b.position_unit << "\"\n";
			ss << "    velocity_unit: \"" << b.velocity_unit << "\"\n";
			ss << "    temperature_unit: \"" << b.temperature_unit << "\"\n";
			ss << "    charge_unit: \"" << b.charge_unit << "\"\n";
		}

		if (!s.patterns.empty()) {
			ss << "patterns:\n";
			for (const auto& p : s.patterns) {
				ss << "  - pattern_type: \"" << p.pattern_type << "\"\n";
				ss << "    preset: \"" << p.preset << "\"\n";
				ss << "    object_count: " << p.object_count << "\n";
				if (!p.parent_name.empty()) ss << "    parent_name: \"" << p.parent_name << "\"\n";
				if (p.parent_id != -2) ss << "    parent_id: " << p.parent_id << "\n";
				ss << "    center_position: [" << p.center_position[0] << ", " << p.center_position[1] << ", " << p.center_position[2] << "]\n";
				ss << "    center_velocity: [" << p.center_velocity[0] << ", " << p.center_velocity[1] << ", " << p.center_velocity[2] << "]\n";
				ss << "    plane_inclination_deg: " << p.plane_inclination_deg << "\n";
				ss << "    plane_longitude_node_deg: " << p.plane_longitude_node_deg << "\n";
				ss << "    plane_argument_periapsis_deg: " << p.plane_argument_periapsis_deg << "\n";
				ss << "    mean_semi_major_axis: " << p.mean_semi_major_axis << "\n";
				ss << "    radial_thickness: " << p.radial_thickness << "\n";
				ss << "    radial_distribution: \"" << p.radial_distribution << "\"\n";
				ss << "    radial_gaussian_sigma: " << p.radial_gaussian_sigma << "\n";
				ss << "    radial_power_law_index: " << p.radial_power_law_index << "\n";
				ss << "    kirkwood_gaps_enabled: " << (p.kirkwood_gaps_enabled ? "true" : "false") << "\n";
				ss << "    kirkwood_gap_count: " << p.kirkwood_gap_count << "\n";
				ss << "    kirkwood_resonance_depth: " << p.kirkwood_resonance_depth << "\n";
				ss << "    vertical_scale_height: " << p.vertical_scale_height << "\n";
				ss << "    vertical_distribution: \"" << p.vertical_distribution << "\"\n";
				ss << "    mean_eccentricity: " << p.mean_eccentricity << "\n";
				ss << "    eccentricity_dispersion: " << p.eccentricity_dispersion << "\n";
				ss << "    eccentricity_distribution: \"" << p.eccentricity_distribution << "\"\n";
				ss << "    mean_inclination_dispersion_deg: " << p.mean_inclination_dispersion_deg << "\n";
				ss << "    inclination_distribution: \"" << p.inclination_distribution << "\"\n";
				ss << "    angular_distribution: \"" << p.angular_distribution << "\"\n";
				ss << "    angular_opening_deg: " << p.angular_opening_deg << "\n";
				ss << "    angular_center_deg: " << p.angular_center_deg << "\n";
				ss << "    lagrangian_clustering_strength: " << p.lagrangian_clustering_strength << "\n";
				ss << "    velocity_mode: \"" << p.velocity_mode << "\"\n";
				ss << "    mean_velocity_multiplier: " << p.mean_velocity_multiplier << "\n";
				ss << "    velocity_dispersion_fraction: " << p.velocity_dispersion_fraction << "\n";
				ss << "    velocity_distribution: \"" << p.velocity_distribution << "\"\n";
				ss << "    prograde_fraction: " << p.prograde_fraction << "\n";
				ss << "    fixed_velocity_vector: [" << p.fixed_velocity_vector[0] << ", " << p.fixed_velocity_vector[1] << ", " << p.fixed_velocity_vector[2] << "]\n";
				ss << "    expansion_speed: " << p.expansion_speed << "\n";
				ss << "    spherical_type: \"" << p.spherical_type << "\"\n";
				ss << "    thin_shell_radius: " << p.thin_shell_radius << "\n";
				ss << "    plummer_scale_radius: " << p.plummer_scale_radius << "\n";
				ss << "    uniform_sphere_radius: " << p.uniform_sphere_radius << "\n";
				ss << "    shell_inner_radius: " << p.shell_inner_radius << "\n";
				ss << "    shell_outer_radius: " << p.shell_outer_radius << "\n";
				ss << "    spherical_power_law_gamma: " << p.spherical_power_law_gamma << "\n";
				ss << "    solid_body_rotation_speed: " << p.solid_body_rotation_speed << "\n";
				ss << "    solid_body_axis_tilt_deg: " << p.solid_body_axis_tilt_deg << "\n";
				ss << "    solid_body_axis_azimuth_deg: " << p.solid_body_axis_azimuth_deg << "\n";
				ss << "    archetype: \"" << p.archetype << "\"\n";
				ss << "    mass_distribution: \"" << p.mass_distribution << "\"\n";
				ss << "    fixed_mass: " << p.fixed_mass << "\n";
				ss << "    min_mass: " << p.min_mass << "\n";
				ss << "    max_mass: " << p.max_mass << "\n";
				ss << "    mass_power_law_alpha: " << p.mass_power_law_alpha << "\n";
				ss << "    radius_from_density: " << (p.radius_from_density ? "true" : "false") << "\n";
				ss << "    mean_density: " << p.mean_density << "\n";
				ss << "    density_dispersion_pct: " << p.density_dispersion_pct << "\n";
				ss << "    fixed_radius: " << p.fixed_radius << "\n";
				ss << "    min_radius: " << p.min_radius << "\n";
				ss << "    max_radius: " << p.max_radius << "\n";
				ss << "    temperature_equilibrium: " << (p.temperature_equilibrium ? "true" : "false") << "\n";
				ss << "    reference_temperature_k: " << p.reference_temperature_k << "\n";
				ss << "    temperature_falloff_exponent: " << p.temperature_falloff_exponent << "\n";
				ss << "    color_jitter: " << p.color_jitter << "\n";
				ss << "    name_prefix: \"" << p.name_prefix << "\"\n";
				ss << "    avoid_horizon_overlap: " << (p.avoid_horizon_overlap ? "true" : "false") << "\n";
				ss << "    avoid_mutual_collisions: " << (p.avoid_mutual_collisions ? "true" : "false") << "\n";
				ss << "    seed: " << p.seed << "\n";
			}
		}

		ss << "observers:\n";
		for (const auto& o : s.observers) {
			ss << "  - name: \"" << o.name << "\"\n";
			ss << "    fov_deg: " << o.field_of_view_deg << "\n";
			ss << "    resolution: [" << o.resolution_x << ", " << o.resolution_y << "]\n";
			ss << "    position: [" << o.position[0] << ", " << o.position[1] << ", " << o.position[2] << ", " << o.position[3] << "]\n";
			ss << "    orientation: [" << o.orientation[0] << ", " << o.orientation[1] << ", " << o.orientation[2] << "]\n";
			ss << "    orientation_convention: " << o.orientation_convention << "\n";
			if (o.has_target) {
				ss << "    target_name: \"" << o.target_name << "\"\n";
				ss << "    target_body_id: " << o.target_body_id << "\n";
				ss << "    target_mode: \"" << o.target_mode << "\"\n";
				ss << "    look_at_offset: [" << o.look_at_offset[0] << ", " << o.look_at_offset[1] << ", " << o.look_at_offset[2] << "]\n";
			}
			ss << "    position_unit: \"" << o.position_unit << "\"\n";
			ss << "    fov_unit: \"" << o.fov_unit << "\"\n";
			ss << "    four_velocity: [" << o.four_velocity[0] << ", " << o.four_velocity[1] << ", " << o.four_velocity[2] << ", " << o.four_velocity[3] << "]\n";
		}

		return ss.str();
	}

	[[nodiscard]] static ScenarioValidationResult validate(const ScenarioDefinition& s) noexcept {
		if (s.format_version == 0 || s.format_version > 1) {
			return {false, "Scenario file format version is incompatible with this build."};
		}
		if (s.scenario_name.empty()) {
			return {false, "Scenario name cannot be empty."};
		}
		if (s.metric_type.empty()) {
			return {false, "Spacetime metric type is missing."};
		}
		const std::string& m = s.metric_type;
		const bool known = (m == "Schwarzschild" || m == "Kerr" || m == "FlatMinkowski" ||
		                    m == "Minkowski" || m == "ReissnerNordstrom" || m == "KerrNewman" ||
		                    m == "SchwarzschildDeSitter" || m == "FLRW" || m == "MorrisThorne" ||
		                    m == "Alcubierre" || m == "BSSN" ||
		                    m.find("Schwarzschild") != std::string::npos ||
		                    m.find("Kerr") != std::string::npos ||
		                    m.find("Minkowski") != std::string::npos ||
		                    m.find("Wormhole") != std::string::npos ||
		                    m.find("Warp") != std::string::npos);
		if (!known) {
			return {false, "Unknown or unsupported metric type: '" + m + "'."};
		}
		if ((m.find("Schwarzschild") != std::string::npos || m.find("Kerr") != std::string::npos) &&
		    m.find("Wormhole") == std::string::npos && m.find("Warp") == std::string::npos &&
		    m.find("Minkowski") == std::string::npos) {
			if (s.central_mass <= 0.0 && s.bodies.empty()) {
				return {false, "Central mass must be positive for black hole metric."};
			}
			if (m.find("Kerr") != std::string::npos && m.find("Newman") == std::string::npos && s.central_mass > 0.0) {
				if (std::abs(s.central_spin) > s.central_mass * 1.0001) {
					return {false, "Kerr spin exceeds physical extremality limit (|a| <= M)."};
				}
			}
		}
		if (m.find("Morris") != std::string::npos || m.find("Wormhole") != std::string::npos) {
			if (s.wormhole_throat <= 0.0) {
				return {false, "Wormhole throat radius must be positive (b0 > 0)."};
			}
		}
		if (m.find("Alcubierre") != std::string::npos || m.find("Warp") != std::string::npos) {
			if (s.warp_velocity < 0.0) {
				return {false, "Warp velocity cannot be negative."};
			}
		}
		if (s.speed_of_light <= 0.0) {
			return {false, "Speed of light must be strictly positive."};
		}
		if (s.gravitational_constant <= 0.0) {
			return {false, "Gravitational constant must be strictly positive."};
		}
		for (const auto& obs : s.observers) {
			if (obs.field_of_view_deg <= 0.0 || obs.field_of_view_deg >= 180.0) {
				return {false, "Observer FOV must be between 0 and 180 degrees."};
			}
		}
		for (const auto& b : s.bodies) {
			if (b.mass < 0.0 || b.radius < 0.0) {
				return {false, "Body mass and radius must be non-negative."};
			}
		}
		return {true, "Compatible"};
	}

	[[nodiscard]] static std::optional<ScenarioDefinition> from_yaml(std::string_view yaml_text) {
		ScenarioDefinition s;
		std::istringstream stream{std::string(yaml_text)};
		std::string line;

		auto trim = [](std::string_view sv) noexcept -> std::string_view {
			while (!sv.empty() && (sv.front() == ' ' || sv.front() == '\t' || sv.front() == '\r')) sv.remove_prefix(1);
			while (!sv.empty() && (sv.back() == ' ' || sv.back() == '\t' || sv.back() == '\r')) sv.remove_suffix(1);
			return sv;
		};

		auto unquote = [](std::string_view sv) noexcept -> std::string {
			if (sv.size() >= 2 && sv.front() == '"' && sv.back() == '"') {
				return std::string(sv.substr(1, sv.size() - 2));
			}
			return std::string(sv);
		};

		auto lower_str = [](std::string_view text) noexcept -> std::string {
			std::string res;
			res.reserve(text.size());
			for (char c : text) res.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
			return res;
		};

		auto parse_texture_mode = [&](std::string_view text) noexcept -> uint32_t {
			const std::string str = lower_str(unquote(text));
			if (str == "procedural" || str == "proceduralnoise") return 0U;
			if (str == "solidcolor" || str == "solid") return 1U;
			if (str == "colorpaletteblend" || str == "palette") return 2U;
			if (str == "bandedgasgiant" || str == "gasgiant") return 3U;
			if (str == "crateredterrestrial" || str == "cratered") return 4U;
			if (str == "stellargranulation" || str == "star") return 5U;
			if (str == "accretionflow") return 6U;
			if (str == "marbledstone" || str == "marbled") return 7U;
			if (str == "ringedgasgiant") return 8U;
			if (str == "icycracked" || str == "ice") return 9U;
			if (str == "volcanicmagma" || str == "volcanic" || str == "magma") return 10U;
			if (str == "citylights") return 11U;
			if (str == "nebulousgascloud" || str == "nebula") return 12U;
			if (str == "earthbluemarble" || str == "earth") return 13U;
			if (str == "sunphotosphere" || str == "sun") return 14U;
			if (str == "mercurycratered" || str == "mercury") return 15U;
			if (str == "venusatmosphere" || str == "venus") return 16U;
			if (str == "venusradarsurface") return 17U;
			if (str == "marsdesertbasalt" || str == "mars") return 18U;
			if (str == "jupiterstormbands" || str == "jupiter") return 19U;
			if (str == "saturncloudbelts" || str == "saturn") return 20U;
			if (str == "uranusmethanehaze" || str == "uranus") return 21U;
			if (str == "neptunevortexbands" || str == "neptune") return 22U;
			if (str == "ceresdarkregolith" || str == "ceres") return 23U;
			if (str == "eristransneptunian" || str == "eris") return 24U;
			if (str == "haumeacrystallineice" || str == "haumea") return 25U;
			if (str == "makemaketholinfrost" || str == "makemake") return 26U;
			char* endp = nullptr;
			const unsigned long v = std::strtoul(str.c_str(), &endp, 10);
			return (endp != str.c_str()) ? static_cast<uint32_t>(v) : 0U;
		};

		auto parse_body_preset = [&](std::string_view text) noexcept -> uint32_t {
			const std::string str = lower_str(unquote(text));
			if (str == "star") return 0U;
			if (str == "terrestrial" || str == "terrestrialplanet") return 1U;
			if (str == "gasgiant") return 2U;
			if (str == "icegiant") return 3U;
			if (str == "metallic" || str == "moon") return 4U;
			if (str == "asteroid") return 5U;
			if (str == "neutronstar") return 6U;
			if (str == "pulsar") return 7U;
			if (str == "blackhole") return 8U;
			if (str == "custom") return 9U;
			char* endp = nullptr;
			const unsigned long v = std::strtoul(str.c_str(), &endp, 10);
			return (endp != str.c_str()) ? static_cast<uint32_t>(v) : 1U;
		};

		auto parse_atmosphere_mode = [&](std::string_view text) noexcept -> uint32_t {
			const std::string str = lower_str(unquote(text));
			if (str == "rayleigh" || str == "rayleighlimbshell" || str == "limb") return 0U;
			if (str == "volumetric" || str == "volumetricscattering") return 1U;
			if (str == "off" || str == "none") return 2U;
			if (str == "haze" || str == "thickhaze") return 3U;
			if (str == "mie" || str == "volumetricmie") return 4U;
			if (str == "corona" || str == "glowingcorona") return 5U;
			char* endp = nullptr;
			const unsigned long v = std::strtoul(str.c_str(), &endp, 10);
			return (endp != str.c_str()) ? static_cast<uint32_t>(v) : 2U;
		};

		auto parse_geometry_model = [&](std::string_view text) noexcept -> uint32_t {
			const std::string str = lower_str(unquote(text));
			if (str == "oblate" || str == "oblatespheroid") return 0U;
			if (str == "sphere" || str == "rigidsphere") return 1U;
			if (str == "prolate" || str == "prolatespheroid") return 2U;
			if (str == "triaxial" || str == "triaxialellipsoid") return 3U;
			char* endp = nullptr;
			const unsigned long v = std::strtoul(str.c_str(), &endp, 10);
			return (endp != str.c_str()) ? static_cast<uint32_t>(v) : 0U;
		};

		auto parse_vec4 = [&](std::string_view text) noexcept -> std::array<double, 4> {
			std::array<double, 4> res{0.0, 0.0, 0.0, 0.0};
			const size_t start = text.find('[');
			const size_t end = text.rfind(']');
			if (start == std::string_view::npos || end == std::string_view::npos || end <= start) {
				return res;
			}
			std::string_view inner = text.substr(start + 1, end - start - 1);
			int depth = 0;
			size_t token_start = 0;
			size_t idx = 0;
			for (size_t i = 0; i < inner.size() && idx < 4; ++i) {
				if (inner[i] == '[' || inner[i] == '(') ++depth;
				else if (inner[i] == ']' || inner[i] == ')') --depth;
				else if (inner[i] == ',' && depth == 0) {
					res[idx++] = ScenarioValueSampler::sample(inner.substr(token_start, i - token_start));
					token_start = i + 1;
				}
			}
			if (token_start < inner.size() && idx < 4) {
				res[idx++] = ScenarioValueSampler::sample(inner.substr(token_start));
			}
			return res;
		};
		auto parse_color = [&](std::string_view text) noexcept -> std::array<float, 4> {
			const auto values = parse_vec4(text);
			return {static_cast<float>(values[0]), static_cast<float>(values[1]), static_cast<float>(values[2]), static_cast<float>(values[3])};
		};

		enum class Section : uint8_t { Root, Spacetime, Integrator, Output, Interactions, Bodies, Sources, Observers, Units, Time, Skybox, PrimaryDisk, PrimaryJet, Patterns };
		Section current_section = Section::Root;

		std::vector<std::pair<std::string, std::string>> current_body_raw_props;

		auto apply_body_prop = [&](ScenarioBodyConfig& b, std::string_view key, std::string_view val) {
			if (key == "name") b.name = unquote(val);
			else if (key == "count") b.count = static_cast<uint32_t>(std::max(1.0, ScenarioValueSampler::sample(val, 1.0)));
			else if (key == "body_id") b.body_id = static_cast<uint32_t>(ScenarioValueSampler::sample(val, b.body_id));
			else if (key == "mass") b.mass = ScenarioValueSampler::sample(val, b.mass);
			else if (key == "radius") b.radius = ScenarioValueSampler::sample(val, b.radius);
			else if (key == "spin") b.spin = ScenarioValueSampler::sample(val, b.spin);
			else if (key == "charge") b.charge = ScenarioValueSampler::sample(val, b.charge);
			else if (key == "enabled") b.enabled = (val != "false");
			else if (key == "is_spacetime_source" || key == "is_source") b.is_spacetime_source = (val == "true");
			else if (key == "color") b.color = parse_color(val);
			else if (key == "color_secondary") b.color_secondary = parse_color(val);
			else if (key == "color_tertiary") b.color_tertiary = parse_color(val);
			else if (key == "preset" || key == "preset_3d") b.preset_3d = parse_body_preset(val);
			else if (key == "texture" || key == "texture_mode" || key == "surface_texture_mode") b.surface_texture_mode = parse_texture_mode(val);
			else if (key == "geometry" || key == "geometry_model") b.geometry_model = parse_geometry_model(val);
			else if (key == "surface_noise_scale" || key == "noise_scale") b.surface_noise_scale = static_cast<float>(ScenarioValueSampler::sample(val, b.surface_noise_scale));
			else if (key == "surface_roughness" || key == "roughness") b.surface_roughness = static_cast<float>(ScenarioValueSampler::sample(val, b.surface_roughness));
			else if (key == "texture_detail_scale" || key == "detail_scale") b.texture_detail_scale = static_cast<float>(ScenarioValueSampler::sample(val, b.texture_detail_scale));
			else if (key == "polar_cap_strength" || key == "polar_cap") b.polar_cap_strength = static_cast<float>(ScenarioValueSampler::sample(val, b.polar_cap_strength));
			else if (key == "night_side_light_intensity" || key == "night_lights") b.night_side_light_intensity = static_cast<float>(ScenarioValueSampler::sample(val, b.night_side_light_intensity));
			else if (key == "ring_system_enabled" || key == "ring_system" || key == "rings") b.ring_system_enabled = (val == "true");
			else if (key == "atmosphere_mode" || key == "atmosphere") b.atmosphere_mode = parse_atmosphere_mode(val);
			else if (key == "atmosphere_thickness") b.atmosphere_thickness = static_cast<float>(ScenarioValueSampler::sample(val, b.atmosphere_thickness));
			else if (key == "atmosphere_color") b.atmosphere_color = parse_color(val);
			else if (key == "emission_intensity" || key == "emission") b.emission_intensity = static_cast<float>(ScenarioValueSampler::sample(val, b.emission_intensity));
			else if (key == "specular_roughness") b.specular_roughness = static_cast<float>(ScenarioValueSampler::sample(val, b.specular_roughness));
			else if (key == "rotation_speed_3d") b.rotation_speed_3d = static_cast<float>(ScenarioValueSampler::sample(val, b.rotation_speed_3d));
			else if (key == "rotation_axis_3d") { auto v = parse_vec4(val); b.rotation_axis_3d = {v[0], v[1], v[2]}; }
			else if (key == "orientation_euler" || key == "orientation") { auto v = parse_vec4(val); b.orientation_euler = {v[0], v[1], v[2]}; }
			else if (key == "spin_axis") { auto v = parse_vec4(val); b.spin_axis = {v[0], v[1], v[2]}; }
			else if (key == "earth_map_variant") b.earth_map_variant = static_cast<uint32_t>(ScenarioValueSampler::sample(val, 0.0));
			else if (key == "earth_map_quality") b.earth_map_quality = static_cast<uint32_t>(ScenarioValueSampler::sample(val, 0.0));
			else if (key == "earth_terminator_softness") b.earth_terminator_softness = static_cast<float>(ScenarioValueSampler::sample(val, 0.15));
			else if (key == "layer_template_1") b.layer_template_1 = static_cast<int>(ScenarioValueSampler::sample(val, -1.0));
			else if (key == "layer_template_2") b.layer_template_2 = static_cast<int>(ScenarioValueSampler::sample(val, -1.0));
			else if (key == "parent_name" || key == "parent") { b.parent_name = unquote(val); b.has_orbit = true; }
			else if (key == "parent_id") { b.parent_id = static_cast<int32_t>(ScenarioValueSampler::sample(val, -2.0)); b.has_orbit = true; }
			else if (key == "semi_major_axis" || key == "a") { b.semi_major_axis = ScenarioValueSampler::sample(val, 0.0); b.has_orbit = true; }
			else if (key == "eccentricity" || key == "e") { b.eccentricity = ScenarioValueSampler::sample(val, 0.0); b.has_orbit = true; }
			else if (key == "inclination_deg" || key == "inclination" || key == "inc") { b.inclination_deg = ScenarioValueSampler::sample(val, 0.0); b.has_orbit = true; }
			else if (key == "longitude_node_deg" || key == "longitude_node" || key == "node") { b.longitude_node_deg = ScenarioValueSampler::sample(val, 0.0); b.has_orbit = true; }
			else if (key == "argument_periapsis_deg" || key == "argument_periapsis" || key == "periapsis") { b.argument_periapsis_deg = ScenarioValueSampler::sample(val, 0.0); b.has_orbit = true; }
			else if (key == "true_anomaly_deg" || key == "true_anomaly" || key == "nu") { b.true_anomaly_deg = ScenarioValueSampler::sample(val, 0.0); b.has_orbit = true; }
			else if (key == "disk_enabled") b.disk_enabled = (val == "true");
			else if (key == "disk_inner_scale") b.disk_inner_scale = static_cast<float>(ScenarioValueSampler::sample(val, 1.0));
			else if (key == "disk_outer_rg") b.disk_outer_rg = static_cast<float>(ScenarioValueSampler::sample(val, 25.0));
			else if (key == "disk_peak_temp") b.disk_peak_temp = static_cast<float>(ScenarioValueSampler::sample(val, 15000.0));
			else if (key == "disk_floor_temp") b.disk_floor_temp = static_cast<float>(ScenarioValueSampler::sample(val, 1200.0));
			else if (key == "disk_brightness") b.disk_brightness = static_cast<float>(ScenarioValueSampler::sample(val, 1.0));
			else if (key == "disk_opacity") b.disk_opacity = static_cast<float>(ScenarioValueSampler::sample(val, 1.0));
			else if (key == "disk_beaming" || key == "disk_doppler_beaming") b.disk_beaming = static_cast<float>(ScenarioValueSampler::sample(val, 0.0));
			else if (key == "disk_saturation") b.disk_saturation = static_cast<float>(ScenarioValueSampler::sample(val, 1.0));
			else if (key == "magnetic_moment") b.magnetic_moment = ScenarioValueSampler::sample(val, b.magnetic_moment);
			else if (key == "rotation_speed") b.rotation_speed = ScenarioValueSampler::sample(val, b.rotation_speed);
			else if (key == "friction_coefficient") b.friction_coefficient = ScenarioValueSampler::sample(val, b.friction_coefficient);
			else if (key == "restitution") b.restitution = ScenarioValueSampler::sample(val, b.restitution);
			else if (key == "elasticity") b.elasticity = ScenarioValueSampler::sample(val, b.elasticity);
			else if (key == "integrity") b.integrity = ScenarioValueSampler::sample(val, b.integrity);
			else if (key == "lifetime") b.lifetime = ScenarioValueSampler::sample(val, b.lifetime);
			else if (key == "temperature") b.temperature = ScenarioValueSampler::sample(val, b.temperature);
			else if (key == "heat_capacity") b.heat_capacity = ScenarioValueSampler::sample(val, b.heat_capacity);
			else if (key == "absorption_factor") b.absorption_factor = ScenarioValueSampler::sample(val, b.absorption_factor);
			else if (key == "transmission_factor") b.transmission_factor = ScenarioValueSampler::sample(val, b.transmission_factor);
			else if (key == "critical_temperature") b.critical_temperature = ScenarioValueSampler::sample(val, b.critical_temperature);
			else if (key == "cold_resistance") b.cold_resistance = ScenarioValueSampler::sample(val, b.cold_resistance);
			else if (key == "hot_resistance") b.hot_resistance = ScenarioValueSampler::sample(val, b.hot_resistance);
			else if (key == "youngs_modulus_cold" || key == "youngs_modulus") b.youngs_modulus_cold = ScenarioValueSampler::sample(val, b.youngs_modulus_cold);
			else if (key == "youngs_modulus_hot") b.youngs_modulus_hot = ScenarioValueSampler::sample(val, b.youngs_modulus_hot);
			else if (key == "composition") b.composition = unquote(val);
			else if (key == "position") b.initial_position = parse_vec4(val);
			else if (key == "velocity") b.initial_velocity = parse_vec4(val);
			else if (key == "quadrupole") b.quadrupole_moment = ScenarioValueSampler::sample(val, b.quadrupole_moment);
			else if (key == "j2") b.j2 = ScenarioValueSampler::sample(val, b.j2);
			else if (key == "j3") b.j3 = ScenarioValueSampler::sample(val, b.j3);
			else if (key == "j4") b.j4 = ScenarioValueSampler::sample(val, b.j4);
			else if (key == "reference_radius") b.reference_radius = ScenarioValueSampler::sample(val, b.reference_radius);
			else if (key == "mass_unit") b.mass_unit = unquote(val);
			else if (key == "radius_unit") b.radius_unit = unquote(val);
			else if (key == "position_unit") b.position_unit = unquote(val);
			else if (key == "velocity_unit") b.velocity_unit = unquote(val);
			else if (key == "temperature_unit") b.temperature_unit = unquote(val);
			else if (key == "charge_unit") b.charge_unit = unquote(val);
		};

		auto expand_current_body = [&]() {
			if (s.bodies.empty() || current_body_raw_props.empty()) return;
			const uint32_t total_count = s.bodies.back().count;
			if (total_count <= 1) {
				current_body_raw_props.clear();
				return;
			}
			const auto base_props = current_body_raw_props;
			current_body_raw_props.clear();
			const uint32_t base_id = s.bodies.back().body_id;
			const std::string base_name = s.bodies.back().name;
			const bool is_source = s.bodies.back().is_spacetime_source;
			s.bodies.back().count = 1;
			s.bodies.back().name = base_name + " 1";
			for (uint32_t rep = 1; rep < total_count; ++rep) {
				s.bodies.emplace_back();
				auto& nb = s.bodies.back();
				nb.is_spacetime_source = is_source;
				for (const auto& [k, v] : base_props) {
					apply_body_prop(nb, k, v);
				}
				nb.count = 1;
				nb.body_id = base_id + rep;
				nb.name = base_name + " " + std::to_string(rep + 1);
			}
		};

		while (std::getline(stream, line)) {
			std::string_view sv = trim(line);
			if (sv.empty() || sv.front() == '#') continue;

			const size_t colon = sv.find(':');
			if (colon == std::string_view::npos) continue;

			const std::string_view key = trim(sv.substr(0, colon));
			const std::string_view val = trim(sv.substr(colon + 1));

			if (key == "units") { expand_current_body(); current_section = Section::Units; continue; }
			if (key == "spacetime") { expand_current_body(); current_section = Section::Spacetime; continue; }
			if (key == "time" || key == "scheduler") { expand_current_body(); current_section = Section::Time; continue; }
			if (key == "skybox" || key == "sky") { expand_current_body(); current_section = Section::Skybox; s.has_skybox = true; continue; }
			if (key == "primary_disk" || key == "accretion_disk") { expand_current_body(); current_section = Section::PrimaryDisk; s.has_primary_disk = true; continue; }
			if (key == "primary_jet" || key == "jet") { expand_current_body(); current_section = Section::PrimaryJet; s.has_primary_jet = true; continue; }
			if (key == "integrator") { expand_current_body(); current_section = Section::Integrator; continue; }
			if (key == "output") { expand_current_body(); current_section = Section::Output; continue; }
			if (key == "interactions") { expand_current_body(); current_section = Section::Interactions; continue; }
			if (key == "patterns" || key == "formations") { expand_current_body(); current_section = Section::Patterns; continue; }
			if (key == "bodies") { expand_current_body(); current_section = Section::Bodies; continue; }
			if (key == "sources") { expand_current_body(); current_section = Section::Sources; continue; }
			if (key == "observers") { expand_current_body(); current_section = Section::Observers; continue; }

			if (current_section == Section::Units) {
				if (key == "distance") s.units.distance_unit = unquote(val);
				else if (key == "mass") s.units.mass_unit = unquote(val);
				else if (key == "velocity") s.units.velocity_unit = unquote(val);
				else if (key == "time") s.units.time_unit = unquote(val);
				else if (key == "energy") s.units.energy_unit = unquote(val);
				else if (key == "temperature") s.units.temperature_unit = unquote(val);
				else if (key == "charge") s.units.charge_unit = unquote(val);
				else if (key == "angle") s.units.angle_unit = unquote(val);
				continue;
			}

			if (current_section == Section::Time) {
				if (key == "warp_factor" || key == "warp") s.time.warp_factor = ScenarioValueSampler::sample(val, s.time.warp_factor);
				else if (key == "tick_rate" || key == "tick_rate_hz" || key == "rate") s.time.tick_rate_hz = ScenarioValueSampler::sample(val, s.time.tick_rate_hz);
				else if (key == "is_paused" || key == "paused") s.time.is_paused = (val == "true");
				continue;
			}

			if (current_section == Section::Skybox) {
				s.has_skybox = true;
				if (key == "enabled") s.skybox.enabled = (val != "false");
				else if (key == "stars_enabled") s.skybox.stars_enabled = (val != "false");
				else if (key == "star_density") s.skybox.star_density = ScenarioValueSampler::sample(val, s.skybox.star_density);
				else if (key == "star_brightness") s.skybox.star_brightness = ScenarioValueSampler::sample(val, s.skybox.star_brightness);
				else if (key == "nebulae_enabled" || key == "nebula_enabled") s.skybox.nebulae_enabled = (val != "false");
				else if (key == "nebula_intensity") s.skybox.nebula_intensity = ScenarioValueSampler::sample(val, s.skybox.nebula_intensity);
				else if (key == "galaxies_enabled") s.skybox.galaxies_enabled = (val != "false");
				else if (key == "galaxy_density") s.skybox.galaxy_density = ScenarioValueSampler::sample(val, s.skybox.galaxy_density);
				else if (key == "galaxy_brightness") s.skybox.galaxy_brightness = ScenarioValueSampler::sample(val, s.skybox.galaxy_brightness);
				else if (key == "clusters_enabled") s.skybox.clusters_enabled = (val != "false");
				else if (key == "cluster_density") s.skybox.cluster_density = ScenarioValueSampler::sample(val, s.skybox.cluster_density);
				else if (key == "cluster_brightness") s.skybox.cluster_brightness = ScenarioValueSampler::sample(val, s.skybox.cluster_brightness);
				else if (key == "dust_enabled") s.skybox.dust_enabled = (val != "false");
				else if (key == "dust_density") s.skybox.dust_density = ScenarioValueSampler::sample(val, s.skybox.dust_density);
				else if (key == "dust_intensity") s.skybox.dust_intensity = ScenarioValueSampler::sample(val, s.skybox.dust_intensity);
				else if (key == "grid_enabled") s.skybox.grid_enabled = (val == "true");
				else if (key == "grid_opacity") s.skybox.grid_opacity = ScenarioValueSampler::sample(val, s.skybox.grid_opacity);
				else if (key == "grid_line_width") s.skybox.grid_line_width = ScenarioValueSampler::sample(val, s.skybox.grid_line_width);
				else if (key == "grid_cell_fill_mode") s.skybox.grid_cell_fill_mode = static_cast<uint32_t>(ScenarioValueSampler::sample(val, 0.0));
				else if (key == "grid_cell_opacity") s.skybox.grid_cell_opacity = ScenarioValueSampler::sample(val, s.skybox.grid_cell_opacity);
				else if (key == "grid_cell_color_count") s.skybox.grid_cell_color_count = static_cast<uint32_t>(ScenarioValueSampler::sample(val, 4.0));
				else if (key == "grid_cell_color_0") { auto c = parse_color(val); s.skybox.grid_cell_colors[0] = {c[0], c[1], c[2]}; }
				else if (key == "grid_cell_color_1") { auto c = parse_color(val); s.skybox.grid_cell_colors[1] = {c[0], c[1], c[2]}; }
				else if (key == "grid_cell_color_2") { auto c = parse_color(val); s.skybox.grid_cell_colors[2] = {c[0], c[1], c[2]}; }
				else if (key == "grid_cell_color_3") { auto c = parse_color(val); s.skybox.grid_cell_colors[3] = {c[0], c[1], c[2]}; }
				else if (key == "background_source") s.skybox.background_source = static_cast<uint32_t>(ScenarioValueSampler::sample(val, 0.0));
				else if (key == "panorama_id") s.skybox.panorama_id = static_cast<uint32_t>(ScenarioValueSampler::sample(val, 0.0));
				else if (key == "background_color") { auto c = parse_color(val); s.skybox.background_color = {c[0], c[1], c[2]}; }
				else if (key == "orientation_yaw_deg") s.skybox.orientation_yaw_deg = ScenarioValueSampler::sample(val, 0.0);
				else if (key == "orientation_pitch_deg") s.skybox.orientation_pitch_deg = ScenarioValueSampler::sample(val, 0.0);
				else if (key == "orientation_roll_deg") s.skybox.orientation_roll_deg = ScenarioValueSampler::sample(val, 0.0);
				else if (key == "seed") s.skybox.seed = static_cast<uint32_t>(ScenarioValueSampler::sample(val, 56.0));
				continue;
			}

			if (current_section == Section::PrimaryDisk) {
				s.has_primary_disk = true;
				if (key == "enabled") s.primary_disk.enabled = (val != "false");
				else if (key == "inner_radius_scale") s.primary_disk.inner_radius_scale = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_disk.inner_radius_scale));
				else if (key == "outer_radius_mass_units") s.primary_disk.outer_radius_mass_units = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_disk.outer_radius_mass_units));
				else if (key == "peak_temperature_k") s.primary_disk.peak_temperature_k = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_disk.peak_temperature_k));
				else if (key == "floor_temperature_k") s.primary_disk.floor_temperature_k = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_disk.floor_temperature_k));
				else if (key == "brightness") s.primary_disk.brightness = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_disk.brightness));
				else if (key == "opacity_scale") s.primary_disk.opacity_scale = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_disk.opacity_scale));
				else if (key == "doppler_beaming_exponent") s.primary_disk.doppler_beaming_exponent = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_disk.doppler_beaming_exponent));
				else if (key == "color_saturation") s.primary_disk.color_saturation = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_disk.color_saturation));
				else if (key == "edge_softness") s.primary_disk.edge_softness = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_disk.edge_softness));
				else if (key == "temperature_exponent") s.primary_disk.temperature_exponent = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_disk.temperature_exponent));
				else if (key == "zero_torque_strength" || key == "zero_torque") s.primary_disk.zero_torque_strength = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_disk.zero_torque_strength));
				else if (key == "tint_strength") s.primary_disk.tint_strength = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_disk.tint_strength));
				else if (key == "tint") { auto c = parse_color(val); s.primary_disk.tint = {c[0], c[1], c[2]}; }
				else if (key == "rotation_speed_scale" || key == "pattern_rotation") s.primary_disk.rotation_speed_scale = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_disk.rotation_speed_scale));
				else if (key == "ring_amplitude") s.primary_disk.ring_amplitude = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_disk.ring_amplitude));
				else if (key == "ring_frequency") s.primary_disk.ring_frequency = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_disk.ring_frequency));
				else if (key == "ring_sharpness") s.primary_disk.ring_sharpness = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_disk.ring_sharpness));
				else if (key == "spiral_arm_count" || key == "spiral_arms") s.primary_disk.spiral_arm_count = static_cast<int>(ScenarioValueSampler::sample(val, s.primary_disk.spiral_arm_count));
				else if (key == "spiral_amplitude") s.primary_disk.spiral_amplitude = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_disk.spiral_amplitude));
				else if (key == "spiral_pitch") s.primary_disk.spiral_pitch = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_disk.spiral_pitch));
				else if (key == "turbulence_amplitude" || key == "turbulence") s.primary_disk.turbulence_amplitude = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_disk.turbulence_amplitude));
				else if (key == "turbulence_scale") s.primary_disk.turbulence_scale = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_disk.turbulence_scale));
				else if (key == "turbulence_octaves") s.primary_disk.turbulence_octaves = static_cast<int>(ScenarioValueSampler::sample(val, s.primary_disk.turbulence_octaves));
				else if (key == "radial_stretch") s.primary_disk.radial_stretch = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_disk.radial_stretch));
				else if (key == "grain_amplitude" || key == "grain") s.primary_disk.grain_amplitude = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_disk.grain_amplitude));
				else if (key == "grain_scale") s.primary_disk.grain_scale = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_disk.grain_scale));
				else if (key == "noise_seed") s.primary_disk.noise_seed = static_cast<uint32_t>(ScenarioValueSampler::sample(val, s.primary_disk.noise_seed));
				else if (key == "hydro_model" || key == "model") {
					std::string m = lower_str(unquote(val));
					if (m == "novikov_thorne" || m == "novikov" || m == "thorne" || m == "1") s.primary_disk.hydro_model = 1U;
					else if (m == "fishbone_moncrief" || m == "torus" || m == "2") s.primary_disk.hydro_model = 2U;
					else s.primary_disk.hydro_model = 0U;
				}
				else if (key == "accretion_rate_scale" || key == "accretion_rate") s.primary_disk.accretion_rate_scale = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_disk.accretion_rate_scale));
				else if (key == "torus_inner_radius") s.primary_disk.torus_inner_radius = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_disk.torus_inner_radius));
				else if (key == "torus_center_radius") s.primary_disk.torus_center_radius = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_disk.torus_center_radius));
				else if (key == "adiabatic_index") s.primary_disk.adiabatic_index = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_disk.adiabatic_index));
				else if (key == "optical_depth_scale") s.primary_disk.optical_depth_scale = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_disk.optical_depth_scale));
				else if (key == "hydro_sampling_density") s.primary_disk.hydro_sampling_density = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_disk.hydro_sampling_density));
				continue;
			}

			if (current_section == Section::PrimaryJet) {
				s.has_primary_jet = true;
				if (key == "enabled") s.primary_jet.enabled = (val != "false");
				else if (key == "footpoint_edge_deg") s.primary_jet.footpoint_edge_deg = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_jet.footpoint_edge_deg));
				else if (key == "outer_radius_mass_units") s.primary_jet.outer_radius_mass_units = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_jet.outer_radius_mass_units));
				else if (key == "lorentz_factor_max") s.primary_jet.lorentz_factor_max = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_jet.lorentz_factor_max));
				else if (key == "horizon_field_tesla") s.primary_jet.horizon_field_tesla = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_jet.horizon_field_tesla));
				else if (key == "core_density_scale") s.primary_jet.core_density_scale = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_jet.core_density_scale));
				else if (key == "geometry") {
					std::string g = lower_str(unquote(val));
					if (g == "paraboloidal" || g == "1") s.primary_jet.geometry = 1U;
					else if (g == "splitmonopole" || g == "split" || g == "2") s.primary_jet.geometry = 2U;
					else s.primary_jet.geometry = 0U;
				}
				else if (key == "field_line_index") s.primary_jet.field_line_index = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_jet.field_line_index));
				else if (key == "angular_velocity_law") {
					std::string a = lower_str(unquote(val));
					if (a == "fixed" || a == "constant" || a == "1") s.primary_jet.angular_velocity_law = 1U;
					else s.primary_jet.angular_velocity_law = 0U;
				}
				else if (key == "angular_velocity_fraction") s.primary_jet.angular_velocity_fraction = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_jet.angular_velocity_fraction));
				else if (key == "angular_velocity_variation") s.primary_jet.angular_velocity_variation = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_jet.angular_velocity_variation));
				else if (key == "fixed_angular_velocity") s.primary_jet.fixed_angular_velocity = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_jet.fixed_angular_velocity));
				else if (key == "toroidal_field_scale") s.primary_jet.toroidal_field_scale = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_jet.toroidal_field_scale));
				else if (key == "rotation_coupling") s.primary_jet.rotation_coupling = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_jet.rotation_coupling));
				else if (key == "magnetization" || key == "sigma") s.primary_jet.magnetization = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_jet.magnetization));
				else if (key == "magnetization_index") s.primary_jet.magnetization_index = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_jet.magnetization_index));
				else if (key == "inner_radius_scale") s.primary_jet.inner_radius_scale = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_jet.inner_radius_scale));
				else if (key == "footpoint_softness_deg") s.primary_jet.footpoint_softness_deg = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_jet.footpoint_softness_deg));
				else if (key == "lorentz_factor_inner") s.primary_jet.lorentz_factor_inner = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_jet.lorentz_factor_inner));
				else if (key == "acceleration_radius_mass_units") s.primary_jet.acceleration_radius_mass_units = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_jet.acceleration_radius_mass_units));
				else if (key == "acceleration_index") s.primary_jet.acceleration_index = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_jet.acceleration_index));
				else if (key == "doppler_beaming_enabled") s.primary_jet.doppler_beaming_enabled = (val != "false");
				else if (key == "gravitational_redshift_enabled") s.primary_jet.gravitational_redshift_enabled = (val != "false");
				else if (key == "emission_model") {
					std::string em = lower_str(unquote(val));
					if (em == "thermalsynchrotron" || em == "thermal" || em == "1") s.primary_jet.emission_model = 1U;
					else if (em == "hybrid" || em == "hybridthermalnonthermal" || em == "2") s.primary_jet.emission_model = 2U;
					else s.primary_jet.emission_model = 0U;
				}
				else if (key == "power_law_index") s.primary_jet.power_law_index = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_jet.power_law_index));
				else if (key == "minimum_lorentz_factor") s.primary_jet.minimum_lorentz_factor = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_jet.minimum_lorentz_factor));
				else if (key == "non_thermal_fraction") s.primary_jet.non_thermal_fraction = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_jet.non_thermal_fraction));
				else if (key == "electron_temperature_k") s.primary_jet.electron_temperature_k = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_jet.electron_temperature_k));
				else if (key == "temperature_index") s.primary_jet.temperature_index = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_jet.temperature_index));
				else if (key == "observing_frequency_hz") s.primary_jet.observing_frequency_hz = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_jet.observing_frequency_hz));
				else if (key == "absorption_scale") s.primary_jet.absorption_scale = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_jet.absorption_scale));
				else if (key == "faraday_scale") s.primary_jet.faraday_scale = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_jet.faraday_scale));
				else if (key == "display_mode") s.primary_jet.display_mode = static_cast<uint32_t>(ScenarioValueSampler::sample(val, 0.0));
				else if (key == "auto_normalization") s.primary_jet.auto_normalization = (val != "false");
				else if (key == "reference_brightness_temperature_k") s.primary_jet.reference_brightness_temperature_k = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_jet.reference_brightness_temperature_k));
				else if (key == "brightness") s.primary_jet.brightness = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_jet.brightness));
				else if (key == "tint") { auto c = parse_color(val); s.primary_jet.tint = {c[0], c[1], c[2]}; }
				else if (key == "polarization_coherence") s.primary_jet.polarization_coherence = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_jet.polarization_coherence));
				else if (key == "sampling_density") s.primary_jet.sampling_density = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_jet.sampling_density));
				else if (key == "max_samples") s.primary_jet.max_samples = static_cast<int>(ScenarioValueSampler::sample(val, s.primary_jet.max_samples));
				else if (key == "use_engine_length_scale") s.primary_jet.use_engine_length_scale = (val != "false");
				else if (key == "meters_per_length_unit") s.primary_jet.meters_per_length_unit = static_cast<float>(ScenarioValueSampler::sample(val, s.primary_jet.meters_per_length_unit));
				continue;
			}

			if (current_section == Section::Patterns) {
				if (sv.starts_with("-")) {
					s.patterns.emplace_back();
					const size_t sub_colon = sv.find(':');
					if (sub_colon != std::string_view::npos) {
						const std::string_view skey = trim(sv.substr(1, sub_colon - 1));
						const std::string_view sval = trim(sv.substr(sub_colon + 1));
						if (skey == "pattern_type" || skey == "type") s.patterns.back().pattern_type = unquote(sval);
						else if (skey == "preset") s.patterns.back().preset = unquote(sval);
					}
					continue;
				}
				if (!s.patterns.empty()) {
					auto& p = s.patterns.back();
					if (key == "pattern_type" || key == "type") p.pattern_type = unquote(val);
					else if (key == "preset") p.preset = unquote(val);
					else if (key == "object_count" || key == "count") p.object_count = static_cast<uint32_t>(ScenarioValueSampler::sample(val, p.object_count));
					else if (key == "parent_name" || key == "parent") p.parent_name = unquote(val);
					else if (key == "parent_id") p.parent_id = static_cast<int32_t>(ScenarioValueSampler::sample(val, p.parent_id));
					else if (key == "center_position" || key == "center") { auto v = parse_vec4(val); p.center_position = {v[0], v[1], v[2]}; }
					else if (key == "center_velocity") { auto v = parse_vec4(val); p.center_velocity = {v[0], v[1], v[2]}; }
					else if (key == "plane_inclination_deg" || key == "inclination") p.plane_inclination_deg = ScenarioValueSampler::sample(val, p.plane_inclination_deg);
					else if (key == "plane_longitude_node_deg" || key == "longitude_node") p.plane_longitude_node_deg = ScenarioValueSampler::sample(val, p.plane_longitude_node_deg);
					else if (key == "plane_argument_periapsis_deg" || key == "argument_periapsis") p.plane_argument_periapsis_deg = ScenarioValueSampler::sample(val, p.plane_argument_periapsis_deg);
					else if (key == "mean_semi_major_axis" || key == "semi_major_axis" || key == "radius") p.mean_semi_major_axis = ScenarioValueSampler::sample(val, p.mean_semi_major_axis);
					else if (key == "radial_thickness" || key == "thickness") p.radial_thickness = ScenarioValueSampler::sample(val, p.radial_thickness);
					else if (key == "radial_distribution") p.radial_distribution = unquote(val);
					else if (key == "radial_gaussian_sigma") p.radial_gaussian_sigma = ScenarioValueSampler::sample(val, p.radial_gaussian_sigma);
					else if (key == "radial_power_law_index") p.radial_power_law_index = ScenarioValueSampler::sample(val, p.radial_power_law_index);
					else if (key == "kirkwood_gaps_enabled") p.kirkwood_gaps_enabled = (val == "true");
					else if (key == "kirkwood_gap_count") p.kirkwood_gap_count = static_cast<int32_t>(ScenarioValueSampler::sample(val, p.kirkwood_gap_count));
					else if (key == "kirkwood_resonance_depth") p.kirkwood_resonance_depth = ScenarioValueSampler::sample(val, p.kirkwood_resonance_depth);
					else if (key == "vertical_scale_height" || key == "scale_height") p.vertical_scale_height = ScenarioValueSampler::sample(val, p.vertical_scale_height);
					else if (key == "vertical_distribution") p.vertical_distribution = unquote(val);
					else if (key == "mean_eccentricity" || key == "eccentricity") p.mean_eccentricity = ScenarioValueSampler::sample(val, p.mean_eccentricity);
					else if (key == "eccentricity_dispersion") p.eccentricity_dispersion = ScenarioValueSampler::sample(val, p.eccentricity_dispersion);
					else if (key == "eccentricity_distribution") p.eccentricity_distribution = unquote(val);
					else if (key == "mean_inclination_dispersion_deg" || key == "inclination_dispersion") p.mean_inclination_dispersion_deg = ScenarioValueSampler::sample(val, p.mean_inclination_dispersion_deg);
					else if (key == "inclination_distribution") p.inclination_distribution = unquote(val);
					else if (key == "angular_distribution") p.angular_distribution = unquote(val);
					else if (key == "angular_opening_deg") p.angular_opening_deg = ScenarioValueSampler::sample(val, p.angular_opening_deg);
					else if (key == "angular_center_deg") p.angular_center_deg = ScenarioValueSampler::sample(val, p.angular_center_deg);
					else if (key == "lagrangian_clustering_strength") p.lagrangian_clustering_strength = ScenarioValueSampler::sample(val, p.lagrangian_clustering_strength);
					else if (key == "velocity_mode") p.velocity_mode = unquote(val);
					else if (key == "mean_velocity_multiplier") p.mean_velocity_multiplier = ScenarioValueSampler::sample(val, p.mean_velocity_multiplier);
					else if (key == "velocity_dispersion_fraction") p.velocity_dispersion_fraction = ScenarioValueSampler::sample(val, p.velocity_dispersion_fraction);
					else if (key == "velocity_distribution") p.velocity_distribution = unquote(val);
					else if (key == "prograde_fraction") p.prograde_fraction = ScenarioValueSampler::sample(val, p.prograde_fraction);
					else if (key == "fixed_velocity_vector") { auto v = parse_vec4(val); p.fixed_velocity_vector = {v[0], v[1], v[2]}; }
					else if (key == "expansion_speed") p.expansion_speed = ScenarioValueSampler::sample(val, p.expansion_speed);
					else if (key == "spherical_type") p.spherical_type = unquote(val);
					else if (key == "thin_shell_radius") p.thin_shell_radius = ScenarioValueSampler::sample(val, p.thin_shell_radius);
					else if (key == "plummer_scale_radius") p.plummer_scale_radius = ScenarioValueSampler::sample(val, p.plummer_scale_radius);
					else if (key == "uniform_sphere_radius") p.uniform_sphere_radius = ScenarioValueSampler::sample(val, p.uniform_sphere_radius);
					else if (key == "shell_inner_radius") p.shell_inner_radius = ScenarioValueSampler::sample(val, p.shell_inner_radius);
					else if (key == "shell_outer_radius") p.shell_outer_radius = ScenarioValueSampler::sample(val, p.shell_outer_radius);
					else if (key == "spherical_power_law_gamma") p.spherical_power_law_gamma = ScenarioValueSampler::sample(val, p.spherical_power_law_gamma);
					else if (key == "solid_body_rotation_speed") p.solid_body_rotation_speed = ScenarioValueSampler::sample(val, p.solid_body_rotation_speed);
					else if (key == "solid_body_axis_tilt_deg") p.solid_body_axis_tilt_deg = ScenarioValueSampler::sample(val, p.solid_body_axis_tilt_deg);
					else if (key == "solid_body_axis_azimuth_deg") p.solid_body_axis_azimuth_deg = ScenarioValueSampler::sample(val, p.solid_body_axis_azimuth_deg);
					else if (key == "archetype") p.archetype = unquote(val);
					else if (key == "mass_distribution") p.mass_distribution = unquote(val);
					else if (key == "fixed_mass") p.fixed_mass = ScenarioValueSampler::sample(val, p.fixed_mass);
					else if (key == "min_mass") p.min_mass = ScenarioValueSampler::sample(val, p.min_mass);
					else if (key == "max_mass") p.max_mass = ScenarioValueSampler::sample(val, p.max_mass);
					else if (key == "mass_power_law_alpha") p.mass_power_law_alpha = ScenarioValueSampler::sample(val, p.mass_power_law_alpha);
					else if (key == "radius_from_density") p.radius_from_density = (val != "false");
					else if (key == "mean_density") p.mean_density = ScenarioValueSampler::sample(val, p.mean_density);
					else if (key == "density_dispersion_pct") p.density_dispersion_pct = ScenarioValueSampler::sample(val, p.density_dispersion_pct);
					else if (key == "fixed_radius") p.fixed_radius = ScenarioValueSampler::sample(val, p.fixed_radius);
					else if (key == "min_radius") p.min_radius = ScenarioValueSampler::sample(val, p.min_radius);
					else if (key == "max_radius") p.max_radius = ScenarioValueSampler::sample(val, p.max_radius);
					else if (key == "temperature_equilibrium") p.temperature_equilibrium = (val != "false");
					else if (key == "reference_temperature_k") p.reference_temperature_k = ScenarioValueSampler::sample(val, p.reference_temperature_k);
					else if (key == "temperature_falloff_exponent") p.temperature_falloff_exponent = ScenarioValueSampler::sample(val, p.temperature_falloff_exponent);
					else if (key == "color_jitter") p.color_jitter = ScenarioValueSampler::sample(val, p.color_jitter);
					else if (key == "name_prefix") p.name_prefix = unquote(val);
					else if (key == "avoid_horizon_overlap") p.avoid_horizon_overlap = (val != "false");
					else if (key == "avoid_mutual_collisions") p.avoid_mutual_collisions = (val != "false");
					else if (key == "seed") p.seed = static_cast<uint64_t>(ScenarioValueSampler::sample(val, static_cast<double>(p.seed)));
				}
				continue;
			}

			if (current_section == Section::Bodies || current_section == Section::Sources) {
				if (sv.starts_with("-")) {
					expand_current_body();
					s.bodies.emplace_back();
					if (current_section == Section::Sources) {
						s.bodies.back().is_spacetime_source = true;
					}
					const size_t sub_colon = sv.find(':');
					if (sub_colon != std::string_view::npos) {
						const std::string_view skey = trim(sv.substr(1, sub_colon - 1));
						const std::string_view sval = trim(sv.substr(sub_colon + 1));
						current_body_raw_props.emplace_back(std::string(skey), std::string(sval));
						apply_body_prop(s.bodies.back(), skey, sval);
					}
					continue;
				}
				if (!s.bodies.empty()) {
					current_body_raw_props.emplace_back(std::string(key), std::string(val));
					apply_body_prop(s.bodies.back(), key, val);
				}
				continue;
			}

			if (current_section == Section::Observers) {
				if (sv.starts_with("-")) {
					s.observers.emplace_back();
					s.observers.back().orientation_convention = 1;
					const size_t sub_colon = sv.find(':');
					if (sub_colon != std::string_view::npos) {
						const std::string_view skey = trim(sv.substr(1, sub_colon - 1));
						const std::string_view sval = trim(sv.substr(sub_colon + 1));
						if (skey == "name") s.observers.back().name = unquote(sval);
					}
					continue;
				}
				if (!s.observers.empty()) {
					if (key == "name") s.observers.back().name = unquote(val);
					else if (key == "fov_deg") s.observers.back().field_of_view_deg = ScenarioValueSampler::sample(val, s.observers.back().field_of_view_deg);
					else if (key == "resolution") {
						const auto res_vals = parse_vec4(val);
						s.observers.back().resolution_x = static_cast<uint32_t>(res_vals[0]);
						s.observers.back().resolution_y = static_cast<uint32_t>(res_vals[1]);
					}
					else if (key == "position") s.observers.back().position = parse_vec4(val);
					else if (key == "orientation") {
						const auto v = parse_vec4(val);
						s.observers.back().orientation = {v[0], v[1], v[2]};
						s.observers.back().has_explicit_orientation = true;
					}
					else if (key == "target_name" || key == "target" || key == "look_at") {
						s.observers.back().target_name = unquote(val);
						s.observers.back().has_target = true;
					}
					else if (key == "target_body_id" || key == "target_id") {
						s.observers.back().target_body_id = static_cast<int32_t>(ScenarioValueSampler::sample(val, -1.0));
						s.observers.back().has_target = true;
					}
					else if (key == "target_mode" || key == "barycenter_mode") {
						s.observers.back().target_mode = unquote(val);
					}
					else if (key == "look_at_offset" || key == "target_offset") {
						const auto v = parse_vec4(val);
						s.observers.back().look_at_offset = {v[0], v[1], v[2]};
					}
					else if (key == "four_velocity") s.observers.back().four_velocity = parse_vec4(val);
					else if (key == "orientation_convention") s.observers.back().orientation_convention = static_cast<uint32_t>(std::strtoul(std::string(val).c_str(), nullptr, 10));
					else if (key == "position_unit") s.observers.back().position_unit = unquote(val);
					else if (key == "fov_unit") s.observers.back().fov_unit = unquote(val);
				}
				continue;
			}

			if (key == "format_version") { s.format_version = static_cast<uint32_t>(std::strtoul(std::string(val).c_str(), nullptr, 10)); continue; }
			if (key == "scenario_name") s.scenario_name = unquote(val);
			else if (key == "description") s.description = unquote(val);
			else if (key == "author") s.author = unquote(val);
			else if (key == "created_at") s.created_at = unquote(val);
			else if (key == "version_tag") s.version_tag = unquote(val);
			else if (key == "metric_type") s.metric_type = unquote(val);
			else if (key == "central_mass") s.central_mass = ScenarioValueSampler::sample(val, s.central_mass);
			else if (key == "central_spin") s.central_spin = ScenarioValueSampler::sample(val, s.central_spin);
			else if (key == "central_charge") s.central_charge = ScenarioValueSampler::sample(val, s.central_charge);
			else if (key == "cosmological_lambda") s.cosmological_lambda = ScenarioValueSampler::sample(val, s.cosmological_lambda);
			else if (key == "wormhole_throat") s.wormhole_throat = ScenarioValueSampler::sample(val, s.wormhole_throat);
			else if (key == "warp_velocity") s.warp_velocity = ScenarioValueSampler::sample(val, s.warp_velocity);
			else if (key == "warp_bubble_radius") s.warp_bubble_radius = ScenarioValueSampler::sample(val, s.warp_bubble_radius);
			else if (key == "warp_wall_sharpness") s.warp_wall_sharpness = ScenarioValueSampler::sample(val, s.warp_wall_sharpness);
			else if (key == "wormhole_tidal_potential") s.wormhole_tidal_potential = ScenarioValueSampler::sample(val, s.wormhole_tidal_potential);
			else if (key == "teves_critical_acceleration") s.teves_critical_acceleration = ScenarioValueSampler::sample(val, s.teves_critical_acceleration);
			else if (key == "pole_guard_precision_scale") s.pole_guard_precision_scale = ScenarioValueSampler::sample(val, s.pole_guard_precision_scale);
			else if (key == "far_field_step_scale") s.far_field_step_scale = ScenarioValueSampler::sample(val, s.far_field_step_scale);
			else if (key == "warp_factor") s.time.warp_factor = ScenarioValueSampler::sample(val, s.time.warp_factor);
			else if (key == "tick_rate" || key == "tick_rate_hz") s.time.tick_rate_hz = ScenarioValueSampler::sample(val, s.time.tick_rate_hz);
			else if (key == "speed_of_light") s.speed_of_light = ScenarioValueSampler::sample(val, s.speed_of_light);
			else if (key == "gravitational_constant") s.gravitational_constant = ScenarioValueSampler::sample(val, s.gravitational_constant);
			else if (key == "scheme") s.integrator.scheme = unquote(val);
			else if (key == "initial_step") s.integrator.initial_step = ScenarioValueSampler::sample(val, s.integrator.initial_step);
			else if (key == "min_step") s.integrator.min_step = ScenarioValueSampler::sample(val, s.integrator.min_step);
			else if (key == "max_step") s.integrator.max_step = ScenarioValueSampler::sample(val, s.integrator.max_step);
			else if (key == "relative_tolerance") s.integrator.relative_tolerance = ScenarioValueSampler::sample(val, s.integrator.relative_tolerance);
			else if (key == "absolute_tolerance") s.integrator.absolute_tolerance = ScenarioValueSampler::sample(val, s.integrator.absolute_tolerance);
			else if (key == "fits_enabled") s.output.fits_enabled = (val == "true");
			else if (key == "hdf5_enabled") s.output.hdf5_enabled = (val == "true");
			else if (key == "vtk_enabled") s.output.vtk_enabled = (val == "true");
			else if (key == "export_directory") s.output.export_directory = unquote(val);
			else if (key == "electricity_enabled") s.interactions.electricity_enabled = (val == "true");
			else if (key == "magnetism_enabled") s.interactions.magnetism_enabled = (val == "true");
			else if (key == "vacuum_permittivity") s.interactions.vacuum_permittivity = std::strtod(std::string(val).c_str(), nullptr);
			else if (key == "vacuum_permeability") s.interactions.vacuum_permeability = std::strtod(std::string(val).c_str(), nullptr);
			else if (key == "collisions_enabled") s.interactions.collisions_enabled = (val == "true");
			else if (key == "collision_response_model") s.interactions.collision_response_model = static_cast<uint32_t>(std::strtoul(std::string(val).c_str(), nullptr, 10));
			else if (key == "collision_consider_rotation") s.interactions.collision_consider_rotation = (val == "true");
			else if (key == "collision_consider_friction") s.interactions.collision_consider_friction = (val == "true");
			else if (key == "collision_restitution_multiplier") s.interactions.collision_restitution_multiplier = std::strtod(std::string(val).c_str(), nullptr);
			else if (key == "collision_stiffness_scale") s.interactions.collision_stiffness_scale = std::strtod(std::string(val).c_str(), nullptr);
			else if (key == "collision_position_correction_factor") s.interactions.collision_position_correction_factor = std::strtod(std::string(val).c_str(), nullptr);
			else if (key == "thermodynamics_enabled") s.interactions.thermodynamics_enabled = (val == "true");
			else if (key == "ambient_temperature_kelvin") s.interactions.ambient_temperature_kelvin = std::strtod(std::string(val).c_str(), nullptr);
			else if (key == "radiative_coupling_scale") s.interactions.radiative_coupling_scale = std::strtod(std::string(val).c_str(), nullptr);
			else if (key == "fragmentation_enabled") s.interactions.fragmentation_enabled = (val == "true");
			else if (key == "fragmentation_tidal_stress_enabled") s.interactions.fragmentation_tidal_stress_enabled = (val == "true");
			else if (key == "minimum_fragment_mass") s.interactions.minimum_fragment_mass = std::strtod(std::string(val).c_str(), nullptr);
			else if (key == "max_fragments_per_event") s.interactions.max_fragments_per_event = static_cast<uint32_t>(std::strtoul(std::string(val).c_str(), nullptr, 10));
			else if (key == "collision_energy_to_integrity_loss") s.interactions.collision_energy_to_integrity_loss = std::strtod(std::string(val).c_str(), nullptr);
			else if (key == "tidal_stress_to_integrity_loss") s.interactions.tidal_stress_to_integrity_loss = std::strtod(std::string(val).c_str(), nullptr);
			else if (key == "annihilation_enabled") s.interactions.annihilation_enabled = (val == "true");
			else if (key == "annihilation_contact_scale") s.interactions.annihilation_contact_scale = std::strtod(std::string(val).c_str(), nullptr);
			else if (key == "annihilation_require_opposite_charge") s.interactions.annihilation_require_opposite_charge = (val == "true");
		}
		expand_current_body();

		return s;
	}
};

}
