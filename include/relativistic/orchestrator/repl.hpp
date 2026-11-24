#pragma once

#include "relativistic/orchestrator/command.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include <iostream>
#include <string_view>
#include <string>
#include <array>
#include <algorithm>
#include <cctype>

namespace Relativistic::Orchestrator {

template <size_t QueueCapacity = 1024>
class MasterTerminalRepl {
private:
	SimulationOrchestrator<QueueCapacity>& orchestrator_;

public:
	explicit constexpr MasterTerminalRepl(SimulationOrchestrator<QueueCapacity>& orchestrator) noexcept
		: orchestrator_(orchestrator) {}

	bool execute_line(std::string_view line, CommandResult* res = nullptr) noexcept {
		auto trim_sv = [](std::string_view s) noexcept -> std::string_view {
			while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n')) s.remove_prefix(1);
			while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n')) s.remove_suffix(1);
			return s;
		};

		const std::string_view trimmed = trim_sv(line);
		if (trimmed.starts_with("help") || trimmed.starts_with("?") || trimmed == "topics" || trimmed == "pages") {
			std::string_view query{};
			if (trimmed == "topics" || trimmed == "pages") {
				query = "topics";
			} else {
				const size_t space = trimmed.find_first_of(" \t");
				if (space != std::string_view::npos) {
					query = trim_sv(trimmed.substr(space));
				}
			}
			print_help(std::cout, query);
			if (res != nullptr) {
				res->success = true;
				std::strncpy(res->message, "Help displayed", sizeof(res->message) - 1);
			}
			return true;
		}

		if (trimmed == "status") {
			print_status(std::cout);
			if (res != nullptr) {
				res->success = true;
				std::strncpy(res->message, "Status displayed", sizeof(res->message) - 1);
			}
			return true;
		}

		CommandResult local_res{};
		const auto cmd_opt = CommandParser::parse(line, &local_res);
		if (!cmd_opt.has_value()) {
			if (res != nullptr) {
				*res = local_res;
			}
			return false;
		}

		const bool pushed = orchestrator_.enqueue_command(*cmd_opt);
		if (!pushed) {
			if (res != nullptr) {
				res->success = false;
				std::strncpy(res->message, "Command queue full", sizeof(res->message) - 1);
			}
			return false;
		}

		if (res != nullptr) {
			*res = local_res;
		}
		return true;
	}

	void print_prompt(std::ostream& out = std::cout) const {
		out << "relativistic> " << std::flush;
	}

	void print_status(std::ostream& out = std::cout) const {
		const auto snap = orchestrator_.scheduler().snapshot();
		const auto& p = orchestrator_.parameters();
		out << "=== Simulation Status ===\n"
			<< "Tick Index:       " << snap.tick_index << "\n"
			<< "Logical Time:     " << snap.logical_time << " s\n"
			<< "Tick Rate:        " << snap.tick_rate_hz << " Hz (dt = " << snap.tick_dt << " s)\n"
			<< "Warp Factor:      " << snap.warp_factor << "x\n"
			<< "Paused:           " << (snap.is_paused ? "YES" : "NO") << "\n"
			<< "Remaining Steps:  " << snap.remaining_steps << "\n"
			<< "Mass:             " << p.mass << "\n"
			<< "Spin:             " << p.spin << "\n"
			<< "Charge:           " << p.charge << "\n"
			<< "Cosmo Lambda:     " << p.cosmological_lambda << "\n"
			<< "Wormhole Throat:  " << p.wormhole_throat << "\n"
			<< "Warp Velocity:    " << p.warp_velocity << "\n"
			<< "Uptime:           " << Core::Diagnostics::uptime_seconds() << " s\n"
			<< "Warnings/Errors:  " << Core::Diagnostics::g_registry.warning_counter.load(std::memory_order_relaxed) << " / " << Core::Diagnostics::g_registry.error_counter.load(std::memory_order_relaxed) << "\n"
			<< "Dropped Commands: " << Core::Diagnostics::g_registry.dropped_command_counter.load(std::memory_order_relaxed) << "\n"
			<< "=========================\n";
	}

	struct HelpTopicEntry {
		size_t page;
		std::string_view keyword;
		std::string_view title;
		std::string_view description;
	};

	static constexpr std::array<HelpTopicEntry, 16> kHelpTopics{{
		{1, "flow", "Simulation Clock & Execution Flow", "Pause, resume, step, warp, tick rate, clock reset, status, exit"},
		{2, "spacetime", "Spacetime Solutions & Compact Masses", "Metric manifolds, mass, Kerr spin, charge, cosmological constant, wormhole, warp"},
		{3, "solvers", "Numerical ODE Solvers & Tolerances", "Integrator algorithms, relative/absolute tolerances, step size boundaries, controller"},
		{4, "camera", "Camera Navigation & Projections", "Translation, rotation, teleportation, look-at targeting, snaps, FOV, speed, exposure"},
		{5, "walk", "Surface Walk & Planet Orbit Modes", "Surface walker physics, heights, ground speed, gravity, head bob, planet orbit drag"},
		{6, "bodies", "Celestial Body Catalog & Transformations", "Adding bodies, removal, velocity inversion, scattering, grid snap, mass averaging"},
		{7, "render", "Ray-Tracing Pipeline & Hardware GPU", "Performance presets, resolution scaling, ray step budgets, GPU compute, interlacing"},
		{8, "lod", "Level of Detail & Spacetime Skipping", "Distance-based LOD, weak-field space-skipping, far-field step scaling, polar damping"},
		{9, "lighting", "Illumination, Shadows & Atmosphere", "Light sources, positions, falloff, specular, body emission lighting, shadows, Rayleigh"},
		{10, "disk", "Accretion Disks & Relativistic Jets", "Thermal profiles, Doppler beaming exponent, inner/outer radius, Blandford-Znajek jets"},
		{11, "sky", "Procedural Skybox & Environment", "Starfield point density, brightness, volumetric gas nebulae, dust lanes, clusters, seed"},
		{12, "optics", "Projections, Tonemapping & Grading", "Pinhole, panoramic, fisheye modes, ACES/Reinhard tonemappers, lift/gamma/gain/vignette"},
		{13, "physics", "Physical Constants & Multiphysics", "Constants presets, speed of light, G, Planck, Boltzmann, multiphysics interactions"},
		{14, "units", "Unit Preferences & Coordinate Systems", "Units for distances, velocities, masses, angles, times; default and HUD coordinate frames"},
		{15, "darkmatter", "Dark Matter Halos & Modern Cosmology", "Dark matter field, gravitational lensing, visualization modes, cosmic void parameters"},
		{16, "cluster", "Distributed Compute & Data Capture", "Cluster topology, worker roles, network transport, screenshot export, scenarios"}
	}};

	[[nodiscard]] static size_t resolve_topic_page(std::string_view query) noexcept {
		if (query.empty()) {
			return 1;
		}
		auto to_lower_char = [](char c) noexcept -> char {
			return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + ('a' - 'A')) : c;
		};
		auto iequals = [&](std::string_view a, std::string_view b) noexcept -> bool {
			if (a.size() != b.size()) return false;
			for (size_t i = 0; i < a.size(); ++i) {
				if (to_lower_char(a[i]) != to_lower_char(b[i])) return false;
			}
			return true;
		};
		auto contains_sub = [&](std::string_view haystack, std::string_view needle) noexcept -> bool {
			if (needle.empty() || needle.size() > haystack.size()) return false;
			for (size_t i = 0; i <= haystack.size() - needle.size(); ++i) {
				bool match = true;
				for (size_t j = 0; j < needle.size(); ++j) {
					if (to_lower_char(haystack[i + j]) != to_lower_char(needle[j])) {
						match = false;
						break;
					}
				}
				if (match) return true;
			}
			return false;
		};

		if (query[0] >= '0' && query[0] <= '9') {
			char buf[16]{};
			const size_t len = std::min(query.size(), sizeof(buf) - 1);
			std::memcpy(buf, query.data(), len);
			const unsigned long num = std::strtoul(buf, nullptr, 10);
			if (num >= 1 && num <= kHelpTopics.size()) {
				return static_cast<size_t>(num);
			}
			return 0;
		}

		for (const auto& topic : kHelpTopics) {
			if (iequals(query, topic.keyword)) {
				return topic.page;
			}
		}

		if (iequals(query, "sim") || iequals(query, "clock") || iequals(query, "pause") || iequals(query, "step") || iequals(query, "warp") || iequals(query, "tickrate")) return 1;
		if (iequals(query, "metric") || iequals(query, "gravity") || iequals(query, "mass") || iequals(query, "spin") || iequals(query, "charge") || iequals(query, "wormhole") || iequals(query, "teves")) return 2;
		if (iequals(query, "integrator") || iequals(query, "ode") || iequals(query, "rtol") || iequals(query, "atol") || iequals(query, "step_ctrl")) return 3;
		if (iequals(query, "nav") || iequals(query, "flight") || iequals(query, "fov") || iequals(query, "speed") || iequals(query, "teleport") || iequals(query, "lookat")) return 4;
		if (iequals(query, "surface") || iequals(query, "planet") || iequals(query, "ground") || iequals(query, "jump")) return 5;
		if (iequals(query, "body") || iequals(query, "nbody") || iequals(query, "catalog") || iequals(query, "solar") || iequals(query, "orbit")) return 6;
		if (iequals(query, "pipeline") || iequals(query, "gpu") || iequals(query, "preset") || iequals(query, "resolution") || iequals(query, "scale") || iequals(query, "steps")) return 7;
		if (iequals(query, "skip") || iequals(query, "far_field") || iequals(query, "pole") || iequals(query, "optimization")) return 8;
		if (iequals(query, "shadows") || iequals(query, "atmosphere") || iequals(query, "emission") || iequals(query, "illumination")) return 9;
		if (iequals(query, "accretion") || iequals(query, "jet") || iequals(query, "jets") || iequals(query, "magnetosphere") || iequals(query, "blandford")) return 10;
		if (iequals(query, "skybox") || iequals(query, "stars") || iequals(query, "nebula") || iequals(query, "galaxy") || iequals(query, "dust")) return 11;
		if (iequals(query, "projection") || iequals(query, "tonemap") || iequals(query, "post") || iequals(query, "color") || iequals(query, "grade")) return 12;
		if (iequals(query, "constants") || iequals(query, "interaction") || iequals(query, "collision") || iequals(query, "thermo") || iequals(query, "fragmentation")) return 13;
		if (iequals(query, "coords") || iequals(query, "frames") || iequals(query, "display") || iequals(query, "preferences")) return 14;
		if (iequals(query, "cosmology") || iequals(query, "halos") || iequals(query, "evaporation") || iequals(query, "void")) return 15;
		if (iequals(query, "capture") || iequals(query, "export") || iequals(query, "screenshot") || iequals(query, "scenario")) return 16;

		for (const auto& topic : kHelpTopics) {
			if (contains_sub(topic.title, query) || contains_sub(topic.description, query)) {
				return topic.page;
			}
		}

		return 0;
	}

	void print_topics(std::ostream& out) const {
		out << "====================== Available Help Topics & Pages ======================\n";
		for (const auto& topic : kHelpTopics) {
			out << "  Page " << (topic.page < 10 ? " " : "") << topic.page
				<< " [" << topic.keyword << (topic.keyword.size() < 10 ? std::string(10 - topic.keyword.size(), ' ') : "") << "] : "
				<< topic.title << "\n";
		}
		out << "---------------------------------------------------------------------------\n"
			<< "Usage: 'help <1-16>' to view a specific numbered page.\n"
			<< "       'help <topic>' (e.g. 'help camera', 'help lighting') to jump directly.\n"
			<< "       'help topics' or 'topics' to display this index.\n"
			<< "===========================================================================\n";
	}

	void print_page_group_1_to_4(std::ostream& out, size_t page) const {
		switch (page) {
			case 1:
				out << "--- Page 1: Simulation Flow & Time Execution ---\n"
					<< "  pause                             - Halt simulation physical clock\n"
					<< "  resume                            - Unpause and advance simulation clock\n"
					<< "  step [N]                          - Execute exactly N simulation ticks (default: 1)\n"
					<< "  warp <factor>                     - Set temporal multiplier (e.g. 0.1, 1.0, 10.0, 100.0)\n"
					<< "  tickrate <Hz>                     - Set fixed scheduler integration frequency (10.0 - 1000.0 Hz)\n"
					<< "  reset                             - Restore all bodies, parameters and clocks to initial state\n"
					<< "  status                            - Output comprehensive snapshot of simulation and scheduler\n"
					<< "  shutdown / quit / exit            - Terminate simulator runtime cleanly\n";
				break;
			case 2:
				out << "--- Page 2: Spacetime Solutions & Primary Compact Objects ---\n"
					<< "  metric <name>                     - Select manifold (Schwarzschild, Kerr, ReissnerNordstrom,\n"
					<< "                                      KerrNewman, SchwarzschildDeSitter, FLRW, MorrisThorne,\n"
					<< "                                      Alcubierre, LemaitreTolmanBondi, QuintessenceCosmology,\n"
					<< "                                      ChaplyginCosmology, EvaporatingBlackHole, TeVeS, etc.)\n"
					<< "  mass <val>                        - Central gravitational mass M in geometric simulation units\n"
					<< "  spin <val>                        - Kerr specific angular momentum a (bounded by |a| <= M)\n"
					<< "  charge <val>                      - Central electrostatic charge Q (bounded by Q^2 + a^2 <= M^2)\n"
					<< "  lambda <val>                      - Cosmological constant Lambda for de Sitter / FLRW manifolds\n"
					<< "  throat <val>                      - Morris-Thorne traversable wormhole throat radius b0\n"
					<< "  wormhole_tidal <val>              - Wormhole redshift potential parameter Phi0 (-5.0 to 5.0)\n"
					<< "  warp_vel <val>                    - Alcubierre bubble apparent coordinate shift velocity (c units)\n"
					<< "  warp_radius <val>                 - Radius of Alcubierre bubble in simulation length units\n"
					<< "  warp_sharpness <val>              - Wall thickness parameter sigma in tanh shaping envelope\n"
					<< "  teves_a0 <val>                    - TeVeS critical MOND acceleration scale parameter\n";
				break;
			case 3:
				out << "--- Page 3: Numerical ODE Integrators & Tolerance Tuning ---\n"
					<< "  integrator <name>                 - Numerical solver (Dormand-Prince RK45, Cash-Karp 5(4),\n"
					<< "                                      Vernier 9(8), Symplectic Gauss-Legendre 4th/6th, Hermite 4th)\n"
					<< "  rtol <val>                        - Relative local truncation error tolerance (e.g. 1e-8)\n"
					<< "  atol <val>                        - Absolute error floor safeguarding zero crossings (e.g. 1e-12)\n"
					<< "  min_step <val>                    - Minimum permitted geodesic integration step size\n"
					<< "  max_step <val>                    - Maximum permitted geodesic integration step size\n"
					<< "  step_factor <val>                 - Proportional adaptive step regulator factor (0.002 to 0.5)\n"
					<< "  step_controller <mode>            - Controller filter: standard, pi30, pid42\n";
				break;
			case 4:
				out << "--- Page 4: Camera Navigation, Flight & Projections ---\n"
					<< "  camera move <dx> <dy> <dz>        - Relative Cartesian translation in camera frame\n"
					<< "  camera rotate <p> <y> [r]         - Set pitch, yaw and roll deltas in degrees\n"
					<< "  camera teleport <x> <y> <z>       - Set absolute Cartesian coordinates in world frame\n"
					<< "  camera lookat <x> <y> <z>         - Orient observer optical gaze vector toward target position\n"
					<< "  camera snap <isco|equatorial> [d] - Snap camera to ISCO orbit or equatorial front view\n"
					<< "  camera mode <mode>                - Navigation model: free, orbit, spherical, rocket, walk, planet\n"
					<< "  camera fov <deg>                  - Optical field of view aperture angle (5.0 to 175.0 degrees)\n"
					<< "  camera speed <val>                - Traversal movement speed in simulation units per second\n"
					<< "  camera exposure <EV>              - Photometric exposure compensation in EV (-6.0 to +6.0 EV)\n"
					<< "  camera collision <on|off>         - Toggle horizon and celestial body collision prevention\n"
					<< "  camera clearance <val>            - Minimum buffer clearance distance to obstacle surfaces\n"
					<< "  camera motion_refresh <on|off>    - Toggle live viewport redraws during continuous walker motion\n"
					<< "  camera reset                      - Reset observer position, speed and orientation to defaults\n";
				break;
			default:
				break;
		}
	}

	void print_page_group_5_to_8(std::ostream& out, size_t page) const {
		switch (page) {
			case 5:
				out << "--- Page 5: Surface Walk & Planet Orbit Navigation Systems ---\n"
					<< "  camera walk start                 - Place walker on designated celestial body surface\n"
					<< "  camera walk stop                  - Exit surface walk mode and return to 6-DOF free flight\n"
					<< "  camera walk target <id>           - Select celestial body identifier to bind surface walker\n"
					<< "  camera walk height <val>          - Total physical height of the surface walker\n"
					<< "  camera walk speed <val>           - Ground traversal velocity in simulation units per second\n"
					<< "  camera walk jump <val>            - Peak vertical jump height under local body surface gravity\n"
					<< "  camera walk gravity <val>         - Manual override gravitational acceleration magnitude\n"
					<< "  camera walk auto_gravity <on|off> - Automatically compute surface gravity from body mass and radius\n"
					<< "  camera walk bob <on|off>          - Toggle rhythmic vertical eye-height head bobbing\n"
					<< "  camera walk air_control <val>     - Directional steering authority fraction while airborne (0..1)\n"
					<< "  camera walk follow_rotation <on|off> - Synchronize walker position with body sidereal rotation\n"
					<< "  camera planet speed <val>         - Planet orbit navigation angular traversal rate (deg/s)\n"
					<< "  camera planet drag <val>          - Viewport mouse drag sensitivity in degrees per pixel\n"
					<< "  camera planet zoom <val>          - Altitude scaling rate for keyboard PageUp/PageDown\n"
					<< "  camera planet min_alt <val>       - Minimum permitted orbital altitude in mean body radii\n"
					<< "  camera planet max_alt <val>       - Maximum permitted orbital altitude in mean body radii\n";
				break;
			case 6:
				out << "--- Page 6: Celestial Body Catalog & N-Body Transformations ---\n"
					<< "  body add <m> <r> <x> <y> <z> [nm] - Insert celestial body with mass, radius and position\n"
					<< "  body remove <id>                  - Delete body from active catalog by unique identifier\n"
					<< "  body clear                        - Delete all celestial bodies from simulation catalog\n"
					<< "  body invert                       - Reverse orbital velocity vectors for all active bodies\n"
					<< "  body scatter                      - Randomize spatial positions within bounding domain\n"
					<< "  body snap [cell]                  - Snap coordinates of all bodies to discrete spatial grid\n"
					<< "  body cull [radius]                - Remove bodies exceeding specified radial distance limit\n"
					<< "  body equalize                     - Equalize all body masses to the mean catalog mass\n"
					<< "  body average                      - Replace each individual body mass with the system average\n"
					<< "  body zero_spin                    - Set spin angular momentum of every body to zero\n";
				break;
			case 7:
				out << "--- Page 7: Ray-Tracing Pipeline, Resolution & Hardware Acceleration ---\n"
					<< "  preset <potato|perf|balanced|high|ultra|extreme> - Apply performance profile\n"
					<< "  scale <val> / resolution <val>    - Primary viewport render resolution scale factor (0.1 - 2.0)\n"
					<< "  steps <val>                       - Maximum geodesic ray integration step limit (64 - 65536)\n"
					<< "  gpu <on|off>                      - Toggle Vulkan GPU compute shader offload\n"
					<< "  interlace <on|off>                - Toggle temporal checkerboard interlaced ray generation\n"
					<< "  dynamic_res <on|off>              - Toggle automatic target-FPS dynamic resolution scaling\n"
					<< "  dynamic_fps <val>                 - Target frame rate for dynamic resolution regulator\n"
					<< "  adaptive_tile <on|off>            - Toggle adaptive 32x32 screen tile classification prepass\n"
					<< "  low_power <on|off>                - Force simplified Lambertian shading across all bodies\n"
					<< "  bodies_only <on|off>              - Bypass black hole raytracer and render bodies only\n";
				break;
			case 8:
				out << "--- Page 8: Level of Detail (LOD) & Spacetime Skipping ---\n"
					<< "  lod <on|off>                      - Toggle distance-based ray step budget reduction\n"
					<< "  lod_dist <val>                    - Radial distance threshold where LOD reduction begins\n"
					<< "  lod_steps <val>                   - Step budget allocated to rays exceeding LOD distance\n"
					<< "  skip <on|off>                     - Toggle weak-field asymptotic spatial skipping\n"
					<< "  skip_radius <val>                 - Radial bounding sphere radius threshold for space skipping\n"
					<< "  far_field <val>                   - Geodesic step multiplier in asymptotic weak field (1..8x)\n"
					<< "  pole_precision <val>              - Polar damping precision scale for rotating spacetime axes\n"
					<< "  body_lod <val>                    - On-screen pixel threshold switching bodies to simplified LOD\n"
					<< "  point_lod <val>                   - Pixel threshold rendering distant bodies as single points\n"
					<< "  body_noise <val>                  - Surface procedural noise octave count (1 to 6 layers)\n";
				break;
			default:
				break;
		}
	}

	void print_page_group_9_to_12(std::ostream& out, size_t page) const {
		switch (page) {
			case 9:
				out << "--- Page 9: Scene Illumination, Shadows & Atmosphere ---\n"
					<< "  light intensity <val>             - Direct illuminance intensity multiplier\n"
					<< "  light ambient <val>               - Ambient illumination floor on night hemispheres (0..1)\n"
					<< "  light softness <val>              - Twilight terminator transition blending width (0..1)\n"
					<< "  light specular <val>              - Surface specular gloss reflection multiplier\n"
					<< "  light mode <mode>                 - Light origin: central, headlight, direction, point, nearest, body, unlit\n"
					<< "  light attenuation <mode>          - Distance falloff: none, inverse_square\n"
					<< "  light ref_dist <val>              - Reference distance where irradiance equals unit intensity\n"
					<< "  light color <r> <g> <b>           - Direct light RGB chromatic emission color\n"
					<< "  light direction <az> <el>         - Spherical azimuth and elevation angles for directional key\n"
					<< "  light position <x> <y> <z>        - Fixed world Cartesian coordinates for point light source\n"
					<< "  light body <id>                   - Designate specific celestial body ID as primary light emitter\n"
					<< "  light emission <on|off>           - Toggle mutual secondary illumination from luminous bodies\n"
					<< "  light emission_gain <val>         - Global luminance gain multiplier for emissive bodies\n"
					<< "  light emission_ref <val>          - Falloff reference distance for mutual body emission\n"
					<< "  shadows <on|off>                  - Toggle celestial body shadow occlusion casting\n"
					<< "  disk_occlusion <on|off>           - Toggle accretion disk occlusion behind spherical bodies\n"
					<< "  atmosphere <on|off|val>           - Atmospheric Rayleigh limb scattering intensity factor\n";
				break;
			case 10:
				out << "--- Page 10: Accretion Disks, Hydrodynamics & Relativistic Jets ---\n"
					<< "  disk temp_scale <val>             - Novikov-Thorne disk peak thermal scale in Kelvin\n"
					<< "  disk temp_floor <val>             - Minimum outer disk temperature floor in Kelvin\n"
					<< "  disk beaming <val>                - Relativistic Doppler boosting exponent (-2.0 to 8.0)\n"
					<< "  disk saturation <val>             - Disk blackbody radiation color saturation multiplier\n"
					<< "  disk inner <val>                  - Inner disk truncation boundary radius factor\n"
					<< "  disk outer <val>                  - Outer disk physical radius in gravitational radii (M)\n"
					<< "  jet <on|off>                      - Toggle Blandford-Znajek relativistic plasma jet rendering\n"
					<< "  jet length <val>                  - Collimated jet physical extent in gravitational radii (1..500 M)\n"
					<< "  jet angle <val>                   - Jet half-opening cone angle in degrees (0.5 to 45.0 deg)\n"
					<< "  jet lorentz <val>                 - Bulk plasma outflow Lorentz factor gamma (1.0 to 50.0)\n"
					<< "  jet bfield <val>                  - Helical magnetic field induction strength magnitude\n"
					<< "  jet precession <val>              - Jet orientation precession half-cone angle in degrees\n";
				break;
			case 11:
				out << "--- Page 11: Deep Space Procedural Skybox & Environment ---\n"
					<< "  sky <on|off>                      - Master toggle for celestial sky background rendering\n"
					<< "  sky density <val>                 - Procedural star point spatial density parameter\n"
					<< "  sky brightness <val>              - Master stellar luminosity brightness multiplier\n"
					<< "  sky nebula <val>                  - Fractal volumetric interstellar gas nebula opacity\n"
					<< "  sky grid <val>                    - Celestial coordinate grid line opacity (0..1)\n"
					<< "  sky rotation <deg>                - Sky orientation yaw rotation angle in degrees\n"
					<< "  sky hue <deg>                     - Global sky chromatic hue shift angle in degrees\n"
					<< "  sky saturation <val>              - Sky color saturation scaling factor (0.0 to 3.0)\n"
					<< "  sky seed <val>                    - Deterministic pseudo-random seed for sky features\n"
					<< "  sky stars <on|off>                - Toggle point-like background stars\n"
					<< "  sky galaxies <on|off>             - Toggle distant deep-field spiral and elliptical galaxies\n"
					<< "  sky dust <on|off>                 - Toggle dark interstellar dust absorption lanes\n"
					<< "  sky clusters <on|off>             - Toggle dense globular star clusters\n"
					<< "  sky bg <r> <g> <b>                - Cosmic background baseline color floor\n";
				break;
			case 12:
				out << "--- Page 12: Optical Projections, Tonemapping & Color Grading ---\n"
					<< "  projection <mode>                 - Camera projection: pinhole, autozoom, stereographic,\n"
					<< "                                      equirectangular, equidistant, orthographic, panini, hammer\n"
					<< "  tonemap <mode>                    - Operator: linear, aces, logarithmic, reinhard\n"
					<< "  post contrast <val>               - Post-tonemapping contrast curve slope (0.1 to 3.0x)\n"
					<< "  post saturation <val>             - Post-tonemapping color saturation multiplier (0.0 to 3.0x)\n"
					<< "  post lift <val>                   - Dark shadow floor black level offset (-0.5 to +0.5)\n"
					<< "  post gamma <val>                  - Midtone power curve gamma exponent (0.2 to 3.0)\n"
					<< "  post gain <val>                   - Highlight signal gain multiplier (0.1 to 3.0x)\n"
					<< "  post vignette <val>               - Radial lens optical corner vignette strength (0.0 to 1.0)\n"
					<< "  post highlights <val>             - Highlight detail compression / recovery (-1.0 to +1.0)\n"
					<< "  post shadows <val>                - Shadow detail compression / recovery (-1.0 to +1.0)\n"
					<< "  post reset                        - Restore all color grading curves to neutral defaults\n";
				break;
			default:
				break;
		}
	}

	void print_page_group_13_to_16(std::ostream& out, size_t page) const {
		switch (page) {
			case 13:
				out << "--- Page 13: Physical Constants Engine & Multiphysics Interactions ---\n"
					<< "  constants preset <si|planck|custom> - Set active physical unit normalization\n"
					<< "  constant <c|g|h|kb|na|ke|kcd> <val> - Override individual simulation physical constant\n"
					<< "  interaction electricity <on|off>  - Toggle electrostatic Coulomb force interactions\n"
					<< "  interaction magnetism <on|off>    - Toggle magnetic dipole-dipole Lorentz interactions\n"
					<< "  interaction permittivity <val>    - Vacuum electric permittivity epsilon_0\n"
					<< "  interaction permeability <val>    - Vacuum magnetic permeability mu_0\n"
					<< "  interaction collisions <on|off>   - Toggle physical body contact collisions\n"
					<< "  interaction collision_model <md>  - Collision response impulse model: elastic, inelastic\n"
					<< "  interaction collision_rotation <on|off> - Transfer angular torque during contact collisions\n"
					<< "  interaction collision_friction <on|off> - Apply tangential Coulomb contact friction\n"
					<< "  interaction restitution <val>     - Collision restitution coefficient multiplier (0.0 to 4.0)\n"
					<< "  interaction stiffness <val>       - Penalty contact spring stiffness scale\n"
					<< "  interaction correction <val>      - Interpenetration position correction factor (0.0 to 1.0)\n"
					<< "  interaction thermo <on|off>       - Toggle thermodynamic radiative heat exchange\n"
					<< "  interaction ambient_temp <val>    - Cosmic background ambient temperature in Kelvin\n"
					<< "  interaction radiative_scale <val> - Stefan-Boltzmann radiative coupling scaling factor\n"
					<< "  interaction frag <on|off>         - Toggle mechanical collision and tidal fragmentation\n"
					<< "  interaction min_frag_mass <val>   - Minimum fragment debris mass cutoff\n"
					<< "  interaction max_frags <val>       - Maximum fragment debris count produced per event (1..8)\n"
					<< "  interaction tidal_stress <on|off> - Toggle Roche limit tidal disruption stress\n"
					<< "  interaction annihilation <on|off> - Toggle particle-antiparticle annihilation upon contact\n";
				break;
			case 14:
				out << "--- Page 14: Unit Display Preferences & Coordinate Systems ---\n"
					<< "  unit <dimension> <unit_name>      - Set display formatting unit for physical dimension\n"
					<< "                                      Dimensions: distance, velocity, mass, energy, temp,\n"
					<< "                                      angle, charge, current, time, accel, density, pressure,\n"
					<< "                                      power, force, magnetic, voltage, fps\n"
					<< "  unit reset                        - Restore all display units to international SI standards\n"
					<< "  coord default <system>            - Set default widget input coordinate system\n"
					<< "                                      Systems: cartesian, spherical, cylindrical,\n"
					<< "                                      boyer_lindquist, tortoise, isotropic, kruskal,\n"
					<< "                                      painleve, eddington\n";
				break;
			case 15:
				out << "--- Page 15: Dark Matter Halo Models & Modern Cosmology ---\n"
					<< "  darkmatter <on|off>               - Master toggle for dark matter halo field\n"
					<< "  darkmatter lensing <on|off>       - Enable photon geodesic deflection by dark matter halo\n"
					<< "  darkmatter viz <on|off>           - Toggle volumetric dark matter halo visualization\n"
					<< "  darkmatter viz_mode <mode>        - Rendering style: density, potential, force, distortion\n"
					<< "  darkmatter bodies <on|off>        - Apply dark matter gravitational acceleration to N-body\n"
					<< "  cosmology void_radius <val>       - Lemaitre-Tolman-Bondi cosmic void radius scale\n"
					<< "  cosmology void_width <val>        - LTB density transition profile wall thickness\n"
					<< "  cosmology hubble <val>            - Present-day cosmological Hubble expansion rate H0\n"
					<< "  cosmology evaporation <on|off>    - Enable Hawking radiation black hole mass loss\n"
					<< "  cosmology evap_accel <val>        - Temporal acceleration multiplier for evaporation\n";
				break;
			case 16:
				out << "--- Page 16: Distributed Cluster, Capture Studio & Data Export ---\n"
					<< "  cluster <on|off>                  - Toggle distributed network cluster compute offload\n"
					<< "  cluster role <master|worker|peer> - Set local cluster network hierarchy role\n"
					<< "  cluster port <val>                - Local node network transport listening UDP port\n"
					<< "  cluster discovery <on|off>        - Toggle automatic LAN multicast peer discovery\n"
					<< "  cluster balancing <on|off>        - Toggle latency-aware dynamic work redistribution\n"
					<< "  cluster stealing <on|off>         - Toggle inter-worker job queue work stealing\n"
					<< "  cluster timeout <ms>              - Remote tile compute timeout before local fallback\n"
					<< "  cluster compression <codec>       - Network packet compression: none, lz4, lz4hc, zstd\n"
					<< "  cluster encrypt <on|off>          - Toggle ChaCha20-Poly1305 symmetric packet encryption\n"
					<< "  cluster route <task> <on|off>     - Toggle workload routing: viewport, capture, ensembles,\n"
					<< "                                      nbody, metrics\n"
					<< "  screenshot [path]                 - Write uncompressed snapshot of primary viewport\n"
					<< "  quicksave                         - Save current simulation state to scenarios/quicksave.yaml\n"
					<< "  quickload                         - Restore state from scenarios/quicksave.yaml\n"
					<< "  load <path>                       - Load scenario configuration YAML file from disk\n"
					<< "  save <path>                       - Serialize current simulation state to YAML file\n"
					<< "  export <fits|hdf5|vtk|all>        - Export scientific data products to disk\n"
					<< "  snapshot [reason]                 - Capture non-terminating diagnostic snapshot\n"
					<< "  set <param> <value>               - Direct assignment to any parameter by name\n";
				break;
			default:
				break;
		}
	}

	void print_page(std::ostream& out, size_t page) const {
		if (page >= 1 && page <= 4) {
			print_page_group_1_to_4(out, page);
		} else if (page >= 5 && page <= 8) {
			print_page_group_5_to_8(out, page);
		} else if (page >= 9 && page <= 12) {
			print_page_group_9_to_12(out, page);
		} else if (page >= 13 && page <= 16) {
			print_page_group_13_to_16(out, page);
		}
	}

	void print_help(std::ostream& out = std::cout, std::string_view query = {}) const {
		auto to_lower = [](std::string_view s) noexcept -> std::string {
			std::string res;
			res.reserve(s.size());
			for (char c : s) {
				res.push_back((c >= 'A' && c <= 'Z') ? static_cast<char>(c + ('a' - 'A')) : c);
			}
			return res;
		};

		const std::string lower_query = to_lower(query);
		if (lower_query == "topics" || lower_query == "pages" || lower_query == "index" || lower_query == "categories") {
			print_topics(out);
			return;
		}

		const size_t page = resolve_topic_page(query);
		if (page == 0 || page > kHelpTopics.size()) {
			out << "Unrecognized help topic or invalid page number: '" << query << "'.\n"
				<< "Type 'help topics' to view the full directory of available topics.\n\n";
			print_topics(out);
			return;
		}

		out << "=== Relativistic Engine Manual (Page " << page << " of " << kHelpTopics.size() << ") ===\n";
		print_page(out, page);
		out << "---------------------------------------------------------------------------\n"
			<< "Navigate: 'help <1-16>' for specific page | 'help <topic>' | 'help topics'\n"
			<< "===========================================================================\n";
	}
};

}
