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
	std::array<double, 4> initial_position{0.0, 10.0, std::numbers::pi_v<double> / 2.0, 0.0};
	std::array<double, 4> initial_velocity{1.0, 0.0, 0.0, 0.1};
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

struct ScenarioValidationResult {
	bool is_valid{true};
	std::string error_message{};
};

struct ScenarioDefinition {
	uint32_t format_version{1};
	std::string scenario_name{"RelativisticSimulation"};
	std::string description{"Physical Spacetime Simulation Scenario"};
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
};

class ScenarioSerializer {
public:
	[[nodiscard]] static std::string to_yaml(const ScenarioDefinition& s) {
		std::ostringstream ss;
		ss << std::setprecision(15);
		ss << "format_version: " << s.format_version << "\n";
		ss << "scenario_name: \"" << s.scenario_name << "\"\n";
		ss << "description: \"" << s.description << "\"\n";
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

		ss << "bodies:\n";
		for (const auto& b : s.bodies) {
			ss << "  - name: \"" << b.name << "\"\n";
			ss << "    body_id: " << b.body_id << "\n";
			ss << "    mass: " << b.mass << "\n";
			ss << "    radius: " << b.radius << "\n";
			ss << "    spin: " << b.spin << "\n";
			ss << "    charge: " << b.charge << "\n";
			ss << "    position: [" << b.initial_position[0] << ", " << b.initial_position[1] << ", " << b.initial_position[2] << ", " << b.initial_position[3] << "]\n";
			ss << "    velocity: [" << b.initial_velocity[0] << ", " << b.initial_velocity[1] << ", " << b.initial_velocity[2] << ", " << b.initial_velocity[3] << "]\n";
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

		enum class Section : uint8_t { Root, Spacetime, Integrator, Output, Bodies, Observers };
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
					else if (key == "position") s.bodies.back().initial_position = parse_vec4(val);
					else if (key == "velocity") s.bodies.back().initial_velocity = parse_vec4(val);
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
					else if (key == "position") s.observers.back().position = parse_vec4(val);
					else if (key == "four_velocity") s.observers.back().four_velocity = parse_vec4(val);
				}
				continue;
			}

			if (key == "format_version") { s.format_version = static_cast<uint32_t>(std::strtoul(std::string(val).c_str(), nullptr, 10)); continue; }
			if (key == "scenario_name") s.scenario_name = unquote(val);
			else if (key == "description") s.description = unquote(val);
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
		}

		return s;
	}
};

}
