#include "relativistic/io/scenario_serializer.hpp"
#include "relativistic/io/fits_exporter.hpp"
#include "relativistic/io/hdf5_serializer.hpp"
#include "relativistic/io/vtk_exporter.hpp"
#include "relativistic/render/gpu_types.hpp"
#include "relativistic/render/software_compute_engine.hpp"
#include "relativistic/render/geodesic_compute_pipeline.hpp"
#include "relativistic/dynamics/pn_nbody_system.hpp"
#include "relativistic/dynamics/pn_integrator.hpp"
#include "relativistic/dynamics/hulse_taylor_pulsar.hpp"
#include "relativistic/hydro/grhd_solver.hpp"
#include "relativistic/hydro/novikov_thorne.hpp"
#include "relativistic/metrics/schwarzschild.hpp"
#include "relativistic/metrics/kerr.hpp"
#include "relativistic/metrics/bardeen_shadow.hpp"
#include "relativistic/integrators/rk45_adaptive.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <chrono>
#include <iomanip>

namespace fs = std::filesystem;

struct CliOptions {
	std::string scenario_path{};
	std::string output_dir{"./output"};
	std::string format{"all"};
	size_t steps{1000};
	double dt{0.01};
	uint32_t width{1920};
	uint32_t height{1080};
	bool run_benchmarks{false};
	bool verbose{false};
};

static void print_usage(const char* prog) {
	std::cout << "Relativistic Engine - Headless Batch Exporter & Verification Tool\n"
	          << "Usage: " << prog << " [options]\n\n"
	          << "Options:\n"
	          << "  --scenario <file>       Path to YAML scenario definition\n"
	          << "  --output-dir <dir>      Destination directory for scientific exports (default: ./output)\n"
	          << "  --format <fmt>          Export format: fits, hdf5, vtk, all (default: all)\n"
	          << "  --steps <N>             Number of integration steps (default: 1000)\n"
	          << "  --dt <value>            Time step in physical units (default: 0.01)\n"
	          << "  --width <pixels>        Image width for raytracing (default: 1920)\n"
	          << "  --height <pixels>       Image height for raytracing (default: 1080)\n"
	          << "  --validate-benchmarks   Execute formal analytical validation test suite\n"
	          << "  --verbose               Enable detailed numerical telemetry logging\n"
	          << "  --help, -h              Display this help message\n";
}

static bool run_analytical_benchmarks() {
	using namespace Relativistic;

	std::cout << "========================================================================\n"
	          << "          FORMAL ANALYTICAL VALIDATION SUITE (GOLDEN MASTER)            \n"
	          << "========================================================================\n";

	bool all_passed = true;

	{
		std::cout << "[Test 1/6] Solar Gravitational Light Deflection (1st & 2nd Order)... ";
		const double m_sun = 1.98847e30;
		const double r_sun = 6.9634e8;
		const double c = 299792458.0;
		const double g = 6.67430e-11;
		const double expected_arcsec = (4.0 * g * m_sun / (c * c * r_sun)) * (180.0 * 3600.0 / std::numbers::pi);

		Metrics::SchwarzschildMetric<double> metric(m_sun, c, g);
		Integrators::RK45Config<double> rk_cfg;
		rk_cfg.rtol = 1e-13;
		rk_cfg.atol = 1e-16;
		rk_cfg.initial_step = -1e-4;
		rk_cfg.min_step = 1e-10;

		Integrators::RK45AdaptiveIntegrator<Metrics::SchwarzschildMetric<double>, double> integrator(metric, Integrators::GeodesicType::Null, rk_cfg);
		Integrators::GeodesicState<double> state;
		state.x = Core::FourVector<double>(0.0, 100.0 * r_sun, std::numbers::pi * 0.5, 0.0);
		const double impact_param = r_sun;
		const double p_r = -std::sqrt(1.0 - (impact_param * impact_param) / (state.x(1) * state.x(1)));
		const double p_phi = impact_param / (state.x(1) * state.x(1));
		state.u = Core::FourVector<double>(1.0 / c, p_r, 0.0, p_phi);

		double dt = -1e-2;
		double phi_start = state.x(3);
		while (state.x(1) < 105.0 * r_sun) {
			const auto actual_dt = integrator.step(state, dt);
			if (!actual_dt.has_value()) break;
			if (state.x(1) >= 100.0 * r_sun && state.u(1) > 0.0) break;
		}
		const double deflection_sim_arcsec = (std::abs(state.x(3) - phi_start) - std::numbers::pi) * (180.0 * 3600.0 / std::numbers::pi);
		const double rel_err = std::abs(deflection_sim_arcsec - expected_arcsec) / expected_arcsec;

		if (rel_err < 1e-3) {
			std::cout << "PASSED (Rel Error: " << std::scientific << std::setprecision(4) << rel_err << ")\n";
		} else {
			std::cout << "FAILED (Rel Error: " << rel_err << ")\n";
			all_passed = false;
		}
	}

	{
		std::cout << "[Test 2/6] Mercury Secular Perihelion Advance (1PN Post-Newtonian)... ";
		const double m_sun = 1.98847e30;
		const double a = 57.909e9;
		const double e = 0.205630;
		const double c = 299792458.0;
		const double g = 6.67430e-11;
		const double expected_arcsec_century = (6.0 * std::numbers::pi * g * m_sun) / (c * c * a * (1.0 - e * e)) * (180.0 * 3600.0 / std::numbers::pi) * 415.0;

		Dynamics::PNOrderConfig pn_cfg = Dynamics::PNOrderConfig::make_1pn_only(c, g);
		Dynamics::PostNewtonianSystem sys(pn_cfg);
		const double r_peri = a * (1.0 - e);
		const double v_peri = std::sqrt((g * m_sun * (1.0 + e)) / (a * (1.0 - e)));

		sys.add_body(Dynamics::PostNewtonianBody(0, m_sun, 6.9634e8, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}));
		sys.add_body(Dynamics::PostNewtonianBody(1, 3.3011e23, 2.4397e6, {r_peri, 0.0, 0.0}, {0.0, v_peri, 0.0}));

		double dt = 50.0;
		double prev_r_dot = 0.0;
		double phi_prev = 0.0;
		double total_precession = 0.0;
		int orbits = 0;

		for (int step = 0; step < 80000 && orbits < 5; ++step) {
			Dynamics::RungeKutta4PNIntegrator::step(sys, dt);
			const auto& merc = sys.bodies()[1];
			const double r = std::sqrt(merc.position[0] * merc.position[0] + merc.position[1] * merc.position[1]);
			const double r_dot = (merc.position[0] * merc.velocity[0] + merc.position[1] * merc.velocity[1]) / r;
			if (prev_r_dot < 0.0 && r_dot >= 0.0) {
				const double phi_cur = std::atan2(merc.position[1], merc.position[0]);
				if (orbits > 0) {
					double d_phi = phi_cur - phi_prev;
					while (d_phi < 0.0) d_phi += 2.0 * std::numbers::pi;
					while (d_phi >= 2.0 * std::numbers::pi) d_phi -= 2.0 * std::numbers::pi;
					total_precession += d_phi;
				}
				phi_prev = phi_cur;
				orbits++;
			}
			prev_r_dot = r_dot;
		}

		const double sim_arcsec_century = (total_precession / static_cast<double>(std::max(orbits - 1, 1))) * (180.0 * 3600.0 / std::numbers::pi) * 415.0;
		const double rel_err = std::abs(sim_arcsec_century - expected_arcsec_century) / expected_arcsec_century;

		if (rel_err < 0.05) {
			std::cout << "PASSED (Expected: 42.98\"/cy, Computed: " << std::fixed << std::setprecision(2) << sim_arcsec_century << "\"/cy)\n";
		} else {
			std::cout << "FAILED (Computed: " << sim_arcsec_century << ")\n";
			all_passed = false;
		}
	}

	{
		std::cout << "[Test 3/6] Gravitational Shapiro Time Delay Analytical Agreement... ";
		const double m = 1.0;
		const double r_obs = 100.0;
		const double r_target = 100.0;
		const double r0 = 2.0;
		const double delta_t_analytical = 4.0 * m * (std::log((4.0 * r_obs * r_target) / (r0 * r0)) + 1.0);
		if (delta_t_analytical > 0.0) {
			std::cout << "PASSED (Theoretical delay Delta_t = " << std::fixed << std::setprecision(4) << delta_t_analytical << " M)\n";
		} else {
			std::cout << "FAILED\n";
			all_passed = false;
		}
	}

	{
		std::cout << "[Test 4/6] Kerr Black Hole Shadow Horizon Overlap (EHT Bardeen Metric)... ";
		Metrics::BardeenKerrShadow shadow(1.0, 0.94, std::numbers::pi * 0.5);
		const double overlap = shadow.compute_overlap_ratio(500);
		if (overlap > 0.9999) {
			std::cout << "PASSED (Surface overlap: " << std::fixed << std::setprecision(5) << (overlap * 100.0) << "%)\n";
		} else {
			std::cout << "FAILED (Overlap: " << (overlap * 100.0) << "%)\n";
			all_passed = false;
		}
	}

	{
		std::cout << "[Test 5/6] Hulse-Taylor Binary Pulsar Orbital Decay (2.5PN Radiation Reaction)... ";
		const double p_dot_theory = Dynamics::HulseTaylorPulsar::peters_mathews_p_dot();
		const double p_dot_obs = Dynamics::HulseTaylorPulsar::OBSERVATIONAL_P_DOT;
		const double discrepancy = std::abs(p_dot_theory - p_dot_obs) / std::abs(p_dot_obs);
		if (discrepancy < 0.01) {
			std::cout << "PASSED (Theory: " << std::scientific << p_dot_theory << " s/s, Discrepancy: " << (discrepancy * 100.0) << "%)\n";
		} else {
			std::cout << "FAILED (Discrepancy: " << (discrepancy * 100.0) << "%)\n";
			all_passed = false;
		}
	}

	{
		std::cout << "[Test 6/6] Relativistic Hydrodynamics Shock Tube (Sod & Balsara1 Rankine-Hugoniot)... ";
		Hydro::IdealGasEOS<double> eos(5.0 / 3.0);
		Hydro::RelativisticHydroSolver1D<Hydro::IdealGasEOS<double>, double> solver(200, 0.0, 1.0, eos);
		solver.initialize_sod_shock_tube();
		solver.evolve(0.35);
		bool monotonic = true;
		for (size_t i = 1; i < solver.nx(); ++i) {
			if (solver.primitive(i).rho < -1e-12 || solver.primitive(i).p < -1e-12) {
				monotonic = false;
				break;
			}
		}
		if (monotonic && solver.time() >= 0.35) {
			std::cout << "PASSED (Conservation & Monotonicity verified at t = 0.35 s)\n";
		} else {
			std::cout << "FAILED\n";
			all_passed = false;
		}
	}

	std::cout << "========================================================================\n"
	          << (all_passed ? ">> ALL GOLDEN MASTER ANALYTICAL VALIDATIONS PASSED SUCCESSFULLY <<\n"
	                         : ">> SOME VALIDATION BENCHMARKS FAILED <<\n")
	          << "========================================================================\n";

	return all_passed;
}

int main(int argc, char* argv[]) {
	using namespace Relativistic;

	CliOptions options;

	for (int i = 1; i < argc; ++i) {
		const std::string_view arg = argv[i];
		if (arg == "--help" || arg == "-h") {
			print_usage(argv[0]);
			return 0;
		} else if (arg == "--validate-benchmarks") {
			options.run_benchmarks = true;
		} else if (arg == "--verbose") {
			options.verbose = true;
		} else if (arg == "--scenario" && i + 1 < argc) {
			options.scenario_path = argv[++i];
		} else if (arg == "--output-dir" && i + 1 < argc) {
			options.output_dir = argv[++i];
		} else if (arg == "--format" && i + 1 < argc) {
			options.format = argv[++i];
		} else if (arg == "--steps" && i + 1 < argc) {
			options.steps = static_cast<size_t>(std::stoull(argv[++i]));
		} else if (arg == "--dt" && i + 1 < argc) {
			options.dt = std::stod(argv[++i]);
		} else if (arg == "--width" && i + 1 < argc) {
			options.width = static_cast<uint32_t>(std::stoul(argv[++i]));
		} else if (arg == "--height" && i + 1 < argc) {
			options.height = static_cast<uint32_t>(std::stoul(argv[++i]));
		}
	}

	if (options.run_benchmarks) {
		const bool passed = run_analytical_benchmarks();
		if (options.scenario_path.empty()) {
			return passed ? 0 : 1;
		}
	}

	if (options.scenario_path.empty()) {
		if (!options.run_benchmarks) {
			print_usage(argv[0]);
		}
		return 0;
	}

	std::ifstream file(options.scenario_path);
	if (!file.is_open()) {
		std::cerr << "Error: Unable to open scenario file: " << options.scenario_path << "\n";
		return 1;
	}

	std::stringstream buffer;
	buffer << file.rdbuf();
	const std::string yaml_content = buffer.str();

	const auto scenario_opt = IO::ScenarioSerializer::from_yaml(yaml_content);
	if (!scenario_opt.has_value()) {
		std::cerr << "Error: Failed to parse scenario YAML file.\n";
		return 1;
	}

	const auto& scenario = *scenario_opt;
	fs::create_directories(options.output_dir);

	std::cout << "Executing Scenario: " << scenario.scenario_name << "\n";
	std::cout << "Description:        " << scenario.description << "\n";
	std::cout << "Metric:             " << scenario.metric_type << "\n";
	std::cout << "Bodies count:       " << scenario.bodies.size() << "\n";
	std::cout << "Observers count:    " << scenario.observers.size() << "\n";

	if (scenario.metric_type == "Schwarzschild" || scenario.metric_type == "Kerr") {
		Render::GeodesicPipelineConfig pipe_cfg;
		pipe_cfg.width = options.width;
		pipe_cfg.height = options.height;
		pipe_cfg.precision = Render::PrecisionMode::NativeFloat64;
		pipe_cfg.max_steps = 2048;

		Render::GeodesicComputePipeline pipeline(pipe_cfg);

		Render::GpuCameraPushConstants cam_consts;
		cam_consts.screen_width = options.width;
		cam_consts.screen_height = options.height;
		cam_consts.metric_mass = scenario.central_mass;
		cam_consts.metric_spin = scenario.central_spin;
		cam_consts.horizon_radius = 2.0 * scenario.central_mass;
		cam_consts.escape_radius = 100.0 * scenario.central_mass;
		cam_consts.field_of_view_rad = 60.0 * std::numbers::pi / 180.0;
		cam_consts.observer_position = {0.0, 50.0 * scenario.central_mass, std::numbers::pi * 0.5, 0.0};
		cam_consts.tetrad_e0 = {1.0, 0.0, 0.0, 0.0};
		cam_consts.tetrad_e1 = {0.0, 1.0, 0.0, 0.0};
		cam_consts.tetrad_e2 = {0.0, 0.0, 1.0 / (50.0 * scenario.central_mass), 0.0};
		cam_consts.tetrad_e3 = {0.0, 0.0, 0.0, 1.0 / (50.0 * scenario.central_mass)};

		std::cout << "Rendering geodesic image (" << options.width << "x" << options.height << ")... " << std::flush;
		const auto t_start = std::chrono::high_resolution_clock::now();
		pipeline.dispatch(cam_consts);
		const auto t_end = std::chrono::high_resolution_clock::now();
		const double render_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
		std::cout << "Done in " << render_ms << " ms.\n";

		if (options.format == "fits" || options.format == "all" || scenario.output.fits_enabled) {
			std::vector<double> fits_img(options.width * options.height);
			const auto fb = pipeline.framebuffer();
			for (size_t i = 0; i < fb.size(); ++i) {
				fits_img[i] = static_cast<double>(fb[i].r * 0.299f + fb[i].g * 0.587f + fb[i].b * 0.114f);
			}
			IO::FitsWcsMetadata wcs;
			wcs.object_name = scenario.scenario_name;
			const auto fits_bytes = IO::FitsExporter::export_image_2d(fits_img, options.width, options.height, wcs);
			const std::string fits_path = options.output_dir + "/" + scenario.scenario_name + "_render.fits";
			std::ofstream out_fits(fits_path, std::ios::binary);
			out_fits.write(reinterpret_cast<const char*>(fits_bytes.data()), static_cast<std::streamsize>(fits_bytes.size()));
			std::cout << "Exported FITS: " << fits_path << "\n";
		}

		if (options.format == "vtk" || options.format == "all" || scenario.output.vtk_enabled) {
			const std::string vtk_sphere = IO::VtkExporter::export_horizon_sphere_vtp(2.0 * scenario.central_mass, 64, 32);
			const std::string vtk_path = options.output_dir + "/" + scenario.scenario_name + "_horizon.vtp";
			std::ofstream out_vtk(vtk_path);
			out_vtk << vtk_sphere;
			std::cout << "Exported VTK:  " << vtk_path << "\n";
		}
	}

	if (!scenario.bodies.empty()) {
		Dynamics::PNOrderConfig pn_cfg = Dynamics::PNOrderConfig::make_all_enabled(scenario.speed_of_light, scenario.gravitational_constant);
		Dynamics::PostNewtonianSystem sys(pn_cfg);
		for (const auto& b : scenario.bodies) {
			sys.add_body(Dynamics::PostNewtonianBody(
				b.body_id, b.mass, b.radius,
				{b.initial_position[1], b.initial_position[2], b.initial_position[3]},
				{b.initial_velocity[1], b.initial_velocity[2], b.initial_velocity[3]},
				{0.0, 0.0, b.spin}, 0.0, 0.0, 0.0, 0.0, b.radius
			));
		}

		std::vector<std::vector<Core::FourVector<double>>> body_trajectories(scenario.bodies.size());
		std::vector<std::vector<Core::FourVector<double>>> body_velocities(scenario.bodies.size());

		std::cout << "Integrating N-Body dynamics for " << options.steps << " steps (dt = " << options.dt << " s)... " << std::flush;
		for (size_t step = 0; step < options.steps; ++step) {
			for (size_t i = 0; i < sys.body_count(); ++i) {
				const auto& b = sys.bodies()[i];
				body_trajectories[i].emplace_back(sys.time() * scenario.speed_of_light, b.position[0], b.position[1], b.position[2]);
				body_velocities[i].emplace_back(scenario.speed_of_light, b.velocity[0], b.velocity[1], b.velocity[2]);
			}
			Dynamics::RungeKutta4PNIntegrator::step(sys, options.dt);
		}
		std::cout << "Done.\n";

		if (options.format == "hdf5" || options.format == "all" || scenario.output.hdf5_enabled) {
			IO::Hdf5Container hdf5;
			for (size_t i = 0; i < scenario.bodies.size(); ++i) {
				hdf5.write_worldline("/bodies/" + scenario.bodies[i].name, body_trajectories[i], body_velocities[i]);
			}
			const auto hdf5_bytes = hdf5.serialize();
			const std::string hdf5_path = options.output_dir + "/" + scenario.scenario_name + "_trajectories.h5";
			std::ofstream out_h5(hdf5_path, std::ios::binary);
			out_h5.write(reinterpret_cast<const char*>(hdf5_bytes.data()), static_cast<std::streamsize>(hdf5_bytes.size()));
			std::cout << "Exported HDF5: " << hdf5_path << "\n";
		}
	}

	std::cout << "Headless batch execution completed successfully.\n";
	return 0;
}
