#pragma once

#include "relativistic/core/tensor.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <sstream>
#include <iomanip>
#include <optional>
#include <algorithm>
#include <cstring>
#include <cstdint>

namespace Relativistic::IO {

struct ScenarioBodyConfig {
	std::string name{"CelestialBody"};
	uint32_t body_id{0};
	double mass{1.0};
	double radius{1.0};
	double spin{0.0};
	double charge{0.0};
	bool enabled{true};
	std::array<float, 4> color{0.62f, 0.75f, 1.0f, 1.0f};
	std::array<float, 4> color_secondary{0.18f, 0.30f, 0.75f, 1.0f};
	double magnetic_moment{0.0};
	double rotation_speed{0.0};
	double friction_coefficient{0.0};
	double restitution{0.5};
	double integrity{1.0};
	double lifetime{0.0};
	double temperature{0.0};
	double heat_capacity{0.0};
	double absorption_factor{1.0};
	std::string composition{};
	std::array<double, 4> initial_position{0.0, 10.0, std::numbers::pi_v<double> / 2.0, 0.0};
	std::array<double, 4> initial_velocity{1.0, 0.0, 0.0, 0.1};
	double quadrupole_moment{0.0};
	double j2{0.0};
	double j3{0.0};
	double j4{0.0};
	double reference_radius{0.0};
};

struct ScenarioObserverConfig {
	std::string name{"PrimaryCamera"};
	std::array<double, 4> position{0.0, 50.0, std::numbers::pi_v<double> / 2.0, 0.0};
	std::array<double, 4> four_velocity{1.0, 0.0, 0.0, 0.0};
	double field_of_view_deg{60.0};
	uint32_t resolution_x{1920};
	uint32_t resolution_y{1080};
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
	std::string metric_type{"Schwarzschild"};
	double central_mass{1.0};
	double central_spin{0.0};
	double central_charge{0.0};
	double cosmological_lambda{0.0};
	double wormhole_throat{1.0};
	double warp_velocity{0.0};
	double speed_of_light{1.0};
	double gravitational_constant{1.0};

	std::vector<ScenarioBodyConfig> bodies{};
	std::vector<ScenarioObserverConfig> observers{};
	ScenarioIntegratorConfig integrator{};
	ScenarioOutputConfig output{};
	ScenarioInteractionConfig interactions{};
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
		ss << "spacetime:\n";
		ss << "  metric_type: \"" << s.metric_type << "\"\n";
		ss << "  central_mass: " << s.central_mass << "\n";
		ss << "  central_spin: " << s.central_spin << "\n";
		ss << "  central_charge: " << s.central_charge << "\n";
		ss << "  cosmological_lambda: " << s.cosmological_lambda << "\n";
		ss << "  wormhole_throat: " << s.wormhole_throat << "\n";
		ss << "  warp_velocity: " << s.warp_velocity << "\n";
		ss << "  speed_of_light: " << s.speed_of_light << "\n";
		ss << "  gravitational_constant: " << s.gravitational_constant << "\n";

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
			ss << "    magnetic_moment: " << b.magnetic_moment << "\n";
			ss << "    rotation_speed: " << b.rotation_speed << "\n";
			ss << "    friction_coefficient: " << b.friction_coefficient << "\n";
			ss << "    restitution: " << b.restitution << "\n";
			ss << "    integrity: " << b.integrity << "\n";
			ss << "    lifetime: " << b.lifetime << "\n";
			ss << "    temperature: " << b.temperature << "\n";
			ss << "    heat_capacity: " << b.heat_capacity << "\n";
			ss << "    absorption_factor: " << b.absorption_factor << "\n";
			ss << "    composition: \"" << b.composition << "\"\n";
			ss << "    position: [" << b.initial_position[0] << ", " << b.initial_position[1] << ", " << b.initial_position[2] << ", " << b.initial_position[3] << "]\n";
			ss << "    velocity: [" << b.initial_velocity[0] << ", " << b.initial_velocity[1] << ", " << b.initial_velocity[2] << ", " << b.initial_velocity[3] << "]\n";
			ss << "    quadrupole: " << b.quadrupole_moment << "\n";
			ss << "    j2: " << b.j2 << "\n";
			ss << "    j3: " << b.j3 << "\n";
			ss << "    j4: " << b.j4 << "\n";
			ss << "    reference_radius: " << b.reference_radius << "\n";
		}

		ss << "observers:\n";
		for (const auto& o : s.observers) {
			ss << "  - name: \"" << o.name << "\"\n";
			ss << "    fov_deg: " << o.field_of_view_deg << "\n";
			ss << "    resolution: [" << o.resolution_x << ", " << o.resolution_y << "]\n";
			ss << "    position: [" << o.position[0] << ", " << o.position[1] << ", " << o.position[2] << ", " << o.position[3] << "]\n";
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
		    m.find("Wormhole") == std::string::npos && m.find("Warp") == std::string::npos) {
			if (s.central_mass <= 0.0) {
				return {false, "Central mass must be positive for black hole metric."};
			}
			if (m.find("Kerr") != std::string::npos && m.find("Newman") == std::string::npos) {
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

		auto parse_vec4 = [](std::string_view text) noexcept -> std::array<double, 4> {
			std::array<double, 4> res{0.0, 0.0, 0.0, 0.0};
			const size_t start = text.find('[');
			const size_t end = text.find(']');
			if (start == std::string_view::npos || end == std::string_view::npos || end <= start) {
				return res;
			}
			std::string_view inner = text.substr(start + 1, end - start - 1);
			size_t idx = 0;
			while (!inner.empty() && idx < 4) {
				const size_t comma = inner.find(',');
				const std::string_view token = (comma == std::string_view::npos) ? inner : inner.substr(0, comma);
				inner = (comma == std::string_view::npos) ? std::string_view{} : inner.substr(comma + 1);
				res[idx++] = std::strtod(std::string(token).c_str(), nullptr);
			}
			return res;
		};
		auto parse_color = [&](std::string_view text) noexcept -> std::array<float, 4> {
			const auto values = parse_vec4(text);
			return {static_cast<float>(values[0]), static_cast<float>(values[1]), static_cast<float>(values[2]), static_cast<float>(values[3])};
		};

		enum class Section : uint8_t { Root, Spacetime, Integrator, Output, Interactions, Bodies, Observers };
		Section current_section = Section::Root;

		while (std::getline(stream, line)) {
			std::string_view sv = trim(line);
			if (sv.empty() || sv.front() == '#') continue;

			const size_t colon = sv.find(':');
			if (colon == std::string_view::npos) continue;

			const std::string_view key = trim(sv.substr(0, colon));
			const std::string_view val = trim(sv.substr(colon + 1));

			if (key == "spacetime") { current_section = Section::Spacetime; continue; }
			if (key == "integrator") { current_section = Section::Integrator; continue; }
			if (key == "output") { current_section = Section::Output; continue; }
			if (key == "interactions") { current_section = Section::Interactions; continue; }
			if (key == "bodies") { current_section = Section::Bodies; continue; }
			if (key == "observers") { current_section = Section::Observers; continue; }

			if (current_section == Section::Bodies) {
				if (sv.starts_with("-")) {
					s.bodies.emplace_back();
					const size_t sub_colon = sv.find(':');
					if (sub_colon != std::string_view::npos) {
						const std::string_view skey = trim(sv.substr(1, sub_colon - 1));
						const std::string_view sval = trim(sv.substr(sub_colon + 1));
						if (skey == "name") s.bodies.back().name = unquote(sval);
					}
					continue;
				}
				if (!s.bodies.empty()) {
					if (key == "name") s.bodies.back().name = unquote(val);
					else if (key == "body_id") s.bodies.back().body_id = static_cast<uint32_t>(std::strtoul(std::string(val).c_str(), nullptr, 10));
					else if (key == "mass") s.bodies.back().mass = std::strtod(std::string(val).c_str(), nullptr);
					else if (key == "radius") s.bodies.back().radius = std::strtod(std::string(val).c_str(), nullptr);
					else if (key == "spin") s.bodies.back().spin = std::strtod(std::string(val).c_str(), nullptr);
					else if (key == "charge") s.bodies.back().charge = std::strtod(std::string(val).c_str(), nullptr);
					else if (key == "enabled") s.bodies.back().enabled = (val != "false");
					else if (key == "color") s.bodies.back().color = parse_color(val);
					else if (key == "color_secondary") s.bodies.back().color_secondary = parse_color(val);
					else if (key == "magnetic_moment") s.bodies.back().magnetic_moment = std::strtod(std::string(val).c_str(), nullptr);
					else if (key == "rotation_speed") s.bodies.back().rotation_speed = std::strtod(std::string(val).c_str(), nullptr);
					else if (key == "friction_coefficient") s.bodies.back().friction_coefficient = std::strtod(std::string(val).c_str(), nullptr);
					else if (key == "restitution") s.bodies.back().restitution = std::strtod(std::string(val).c_str(), nullptr);
					else if (key == "integrity") s.bodies.back().integrity = std::strtod(std::string(val).c_str(), nullptr);
					else if (key == "lifetime") s.bodies.back().lifetime = std::strtod(std::string(val).c_str(), nullptr);
					else if (key == "temperature") s.bodies.back().temperature = std::strtod(std::string(val).c_str(), nullptr);
					else if (key == "heat_capacity") s.bodies.back().heat_capacity = std::strtod(std::string(val).c_str(), nullptr);
					else if (key == "absorption_factor") s.bodies.back().absorption_factor = std::strtod(std::string(val).c_str(), nullptr);
					else if (key == "composition") s.bodies.back().composition = unquote(val);
					else if (key == "position") s.bodies.back().initial_position = parse_vec4(val);
					else if (key == "velocity") s.bodies.back().initial_velocity = parse_vec4(val);
					else if (key == "quadrupole") s.bodies.back().quadrupole_moment = std::strtod(std::string(val).c_str(), nullptr);
					else if (key == "j2") s.bodies.back().j2 = std::strtod(std::string(val).c_str(), nullptr);
					else if (key == "j3") s.bodies.back().j3 = std::strtod(std::string(val).c_str(), nullptr);
					else if (key == "j4") s.bodies.back().j4 = std::strtod(std::string(val).c_str(), nullptr);
					else if (key == "reference_radius") s.bodies.back().reference_radius = std::strtod(std::string(val).c_str(), nullptr);
				}
				continue;
			}

			if (current_section == Section::Observers) {
				if (sv.starts_with("-")) {
					s.observers.emplace_back();
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
					else if (key == "fov_deg") s.observers.back().field_of_view_deg = std::strtod(std::string(val).c_str(), nullptr);
					else if (key == "resolution") {
						const auto res_vals = parse_vec4(val);
						s.observers.back().resolution_x = static_cast<uint32_t>(res_vals[0]);
						s.observers.back().resolution_y = static_cast<uint32_t>(res_vals[1]);
					}
					else if (key == "position") s.observers.back().position = parse_vec4(val);
					else if (key == "four_velocity") s.observers.back().four_velocity = parse_vec4(val);
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
			else if (key == "central_mass") s.central_mass = std::strtod(std::string(val).c_str(), nullptr);
			else if (key == "central_spin") s.central_spin = std::strtod(std::string(val).c_str(), nullptr);
			else if (key == "central_charge") s.central_charge = std::strtod(std::string(val).c_str(), nullptr);
			else if (key == "cosmological_lambda") s.cosmological_lambda = std::strtod(std::string(val).c_str(), nullptr);
			else if (key == "wormhole_throat") s.wormhole_throat = std::strtod(std::string(val).c_str(), nullptr);
			else if (key == "warp_velocity") s.warp_velocity = std::strtod(std::string(val).c_str(), nullptr);
			else if (key == "speed_of_light") s.speed_of_light = std::strtod(std::string(val).c_str(), nullptr);
			else if (key == "gravitational_constant") s.gravitational_constant = std::strtod(std::string(val).c_str(), nullptr);
			else if (key == "scheme") s.integrator.scheme = unquote(val);
			else if (key == "initial_step") s.integrator.initial_step = std::strtod(std::string(val).c_str(), nullptr);
			else if (key == "min_step") s.integrator.min_step = std::strtod(std::string(val).c_str(), nullptr);
			else if (key == "max_step") s.integrator.max_step = std::strtod(std::string(val).c_str(), nullptr);
			else if (key == "relative_tolerance") s.integrator.relative_tolerance = std::strtod(std::string(val).c_str(), nullptr);
			else if (key == "absolute_tolerance") s.integrator.absolute_tolerance = std::strtod(std::string(val).c_str(), nullptr);
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

		return s;
	}
};

}
