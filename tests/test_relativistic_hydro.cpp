#include "relativistic/hydro/eos.hpp"
#include "relativistic/hydro/hydro_types.hpp"
#include "relativistic/hydro/con2prim.hpp"
#include "relativistic/hydro/reconstruction.hpp"
#include "relativistic/hydro/riemann_solvers.hpp"
#include "relativistic/hydro/constrained_transport.hpp"
#include "relativistic/hydro/grhd_solver.hpp"
#include <cassert>
#include <iostream>
#include <cmath>
#include <numbers>
#include <vector>

void test_equation_of_state() {
	using namespace Relativistic::Hydro;
	IdealGasEOS<double> eos(5.0 / 3.0);

	const double rho = 2.0;
	const double eps = 1.5;
	const double p = eos.pressure(rho, eps);
	assert(std::abs(p - (2.0 / 3.0) * 2.0 * 1.5) < 1e-12);

	const double eps_recovered = eos.specific_internal_energy(rho, p);
	assert(std::abs(eps_recovered - eps) < 1e-12);

	const double h = eos.specific_enthalpy(rho, p);
	assert(std::abs(h - (1.0 + eps + p / rho)) < 1e-12);

	const double cs2 = eos.sound_speed_squared(rho, p);
	assert(cs2 > 0.0 && cs2 < (5.0 / 3.0 - 1.0));

	const double p_h = eos.pressure_from_enthalpy(rho, h);
	assert(std::abs(p_h - p) < 1e-12);

	SyngeEOS<double> synge;
	const double h_synge = synge.specific_enthalpy(rho, p);
	assert(h_synge > 1.0);
	const double cs2_synge = synge.sound_speed_squared(rho, p);
	assert(cs2_synge > 0.0 && cs2_synge <= (1.0 / 3.0));
}

void test_primitive_conserved_round_trip() {
	using namespace Relativistic::Hydro;
	IdealGasEOS<double> eos(5.0 / 3.0);
	Con2PrimSolver<IdealGasEOS<double>, double> solver(eos);

	{
		PrimitiveVariables<double> p_orig(1.2, 0.8, 0.4, 0.1, -0.2);
		const auto c = prim_to_con_flat(p_orig);
		const auto p_rec = solver.solve(c);

		assert(std::abs(p_rec.rho - p_orig.rho) < 1e-10);
		assert(std::abs(p_rec.p - p_orig.p) < 1e-10);
		assert(std::abs(p_rec.vx - p_orig.vx) < 1e-10);
		assert(std::abs(p_rec.vy - p_orig.vy) < 1e-10);
		assert(std::abs(p_rec.vz - p_orig.vz) < 1e-10);
	}

	{
		PrimitiveVariables<double> p_ultra(0.1, 0.05, 0.999, 0.0, 0.0);
		const auto c = prim_to_con_flat(p_ultra);
		const auto p_rec = solver.solve(c);

		assert(std::abs(p_rec.rho - p_ultra.rho) < 1e-8);
		assert(std::abs(p_rec.p - p_ultra.p) < 1e-8);
		assert(std::abs(p_rec.vx - p_ultra.vx) < 1e-8);
	}

	{
		PrimitiveVariables<double> p_mhd(1.5, 2.0, 0.3, -0.2, 0.1, 0.5, 1.0, -0.4);
		const auto c = prim_to_con_flat(p_mhd);
		const auto p_rec = solver.solve(c);

		assert(std::abs(p_rec.rho - p_mhd.rho) < 1e-6);
		assert(std::abs(p_rec.p - p_mhd.p) < 1e-6);
		assert(std::abs(p_rec.vx - p_mhd.vx) < 1e-6);
		assert(std::abs(p_rec.vy - p_mhd.vy) < 1e-6);
		assert(std::abs(p_rec.vz - p_mhd.vz) < 1e-6);
		assert(std::abs(p_rec.bx - p_mhd.bx) < 1e-12);
		assert(std::abs(p_rec.by - p_mhd.by) < 1e-12);
		assert(std::abs(p_rec.bz - p_mhd.bz) < 1e-12);
	}
}

void test_spatial_reconstruction_accuracy() {
	using namespace Relativistic::Hydro;

	const auto smooth_cell_average = [](double x_left, double x_right) noexcept -> double {
		const double two_pi = 2.0 * std::numbers::pi_v<double>;
		return (std::cos(two_pi * x_left) - std::cos(two_pi * x_right)) / (two_pi * (x_right - x_left));
	};

	const size_t n = 64;
	const double dx = 1.0 / static_cast<double>(n);
	std::vector<double> vals(n);
	for (size_t i = 0; i < n; ++i) {
		vals[i] = smooth_cell_average(static_cast<double>(i) * dx, static_cast<double>(i + 1) * dx);
	}

	double max_err_weno = 0.0;
	double max_err_mp5 = 0.0;

	for (size_t i = 2; i < n - 2; ++i) {
		const double x_face = static_cast<double>(i + 1) * dx;
		const double exact_face = std::sin(2.0 * std::numbers::pi_v<double> * x_face);

		const double weno_val = WENO5Reconstructor<double>::reconstruct_left_face(
			vals[i - 2], vals[i - 1], vals[i], vals[i + 1], vals[i + 2], true
		);
		const double mp5_val = MP5Reconstructor<double>::reconstruct_left_face(
			vals[i - 2], vals[i - 1], vals[i], vals[i + 1], vals[i + 2]
		);

		max_err_weno = std::max(max_err_weno, std::abs(weno_val - exact_face));
		max_err_mp5 = std::max(max_err_mp5, std::abs(mp5_val - exact_face));
	}

	assert(max_err_weno < 1e-6);
	assert(max_err_mp5 < 1e-6);

	const double q_disc_m2 = 1.0, q_disc_m1 = 1.0, q_disc_0 = 1.0, q_disc_p1 = 0.0, q_disc_p2 = 0.0;
	const double weno_step = WENO5Reconstructor<double>::reconstruct_left_face(
		q_disc_m2, q_disc_m1, q_disc_0, q_disc_p1, q_disc_p2, true
	);
	assert(weno_step >= 0.0 && weno_step <= 1.0);
}

void test_riemann_solvers_consistency() {
	using namespace Relativistic::Hydro;
	IdealGasEOS<double> eos(1.4);

	HLLRiemannSolver<IdealGasEOS<double>, double> hll(eos);
	HLLCRiemannSolver<IdealGasEOS<double>, double> hllc(eos);
	HLLDRiemannSolver<IdealGasEOS<double>, double> hlld(eos);

	PrimitiveVariables<double> p_left(1.0, 1.0, 0.0);
	PrimitiveVariables<double> p_right(1.0, 1.0, 0.0);

	const auto f_hll = hll.solve_1d_x(p_left, p_right);
	const auto f_hllc = hllc.solve_1d_x(p_left, p_right);
	const auto f_hlld = hlld.solve_1d_x(p_left, p_right);

	assert(std::abs(f_hll.fd) < 1e-12);
	assert(std::abs(f_hll.fsx - 1.0) < 1e-12);
	assert(std::abs(f_hllc.fd) < 1e-12);
	assert(std::abs(f_hllc.fsx - 1.0) < 1e-12);
	assert(std::abs(f_hlld.fd) < 1e-12);
	assert(std::abs(f_hlld.fsx - 1.0) < 1e-12);

	PrimitiveVariables<double> p_sod_l(10.0, 40.0 / 3.0, 0.0);
	PrimitiveVariables<double> p_sod_r(1.0, 1e-6, 0.0);

	const auto f_sod_hll = hll.solve_1d_x(p_sod_l, p_sod_r);
	const auto f_sod_hllc = hllc.solve_1d_x(p_sod_l, p_sod_r);

	assert(f_sod_hllc.fsx > 0.0);
	assert(f_sod_hll.fsx > 0.0);
}

void test_constrained_transport_divergence() {
	using namespace Relativistic::Hydro;
	const size_t nx = 32;
	const size_t ny = 32;
	const double dx = 1.0 / static_cast<double>(nx);
	const double dy = 1.0 / static_cast<double>(ny);

	ConstrainedTransport2D<double> ct(nx, ny, dx, dy);
	std::vector<PrimitiveVariables<double>> cells(nx * ny);

	auto eval_az = [](double x, double y) noexcept -> double {
		const double r = std::sqrt(x * x + y * y);
		return (r < 0.3) ? ((0.3 - r) * (0.3 - r)) : 0.0;
	};

	std::vector<double> az_grid((nx + 1) * (ny + 1), 0.0);
	for (size_t j = 0; j <= ny; ++j) {
		for (size_t i = 0; i <= nx; ++i) {
			const double x = static_cast<double>(i) * dx - 0.5;
			const double y = static_cast<double>(j) * dy - 0.5;
			az_grid[j * (nx + 1) + i] = eval_az(x, y);
		}
	}

	for (size_t j = 0; j < ny; ++j) {
		for (size_t i = 0; i <= nx; ++i) {
			const double az_top = az_grid[(j + 1) * (nx + 1) + i];
			const double az_bot = az_grid[j * (nx + 1) + i];
			ct.bx_face(i, j) = (az_top - az_bot) / dy;
		}
	}

	for (size_t j = 0; j <= ny; ++j) {
		for (size_t i = 0; i < nx; ++i) {
			const double az_right = az_grid[j * (nx + 1) + (i + 1)];
			const double az_left = az_grid[j * (nx + 1) + i];
			ct.by_face(i, j) = -(az_right - az_left) / dx;
		}
	}

	for (size_t j = 0; j < ny; ++j) {
		for (size_t i = 0; i < nx; ++i) {
			auto& c = cells[j * nx + i];
			c.rho = 1.0;
			c.p = 1.0;
			c.vx = 0.1;
			c.vy = -0.1;
			c.bx = 0.5 * (ct.bx_face(i, j) + ct.bx_face(i + 1, j));
			c.by = 0.5 * (ct.by_face(i, j) + ct.by_face(i, j + 1));
			c.recompute_derived();
		}
	}

	const double div_b_init = ct.compute_max_divergence();
	assert(div_b_init < 1e-12);

	const double dt = 0.001;
	for (int step = 0; step < 50; ++step) {
		ct.compute_vertex_electric_fields_from_cell_centered(cells);
		ct.update_face_magnetic_fields(dt);
		ct.average_to_cell_centers(cells);

		const double div_b = ct.compute_max_divergence();
		assert(div_b < 1e-12);
	}
}

void test_relativistic_sod_shock_tube() {
	using namespace Relativistic::Hydro;
	const size_t nx = 200;
	IdealGasEOS<double> eos(5.0 / 3.0);
	RelativisticHydroSolver1D<IdealGasEOS<double>, double> solver(
		nx, 0.0, 1.0, eos, ReconstructionMethod::WENO5_Z, RiemannSolverType::HLLC, 0.4
	);

	solver.initialize_sod_shock_tube(10.0, 40.0 / 3.0, 1.0, 1e-6, 0.5);
	solver.evolve(0.4);

	const auto& state_left = solver.primitive(0);
	const auto& state_right = solver.primitive(nx - 1);

	assert(std::abs(state_left.rho - 10.0) < 0.1);
	assert(std::abs(state_left.p - 40.0 / 3.0) < 0.1);
	assert(std::abs(state_right.rho - 1.0) < 0.1);

	bool found_intermediate = false;
	for (size_t i = 0; i < nx; ++i) {
		const auto& p = solver.primitive(i);
		if (p.vx > 0.60 && p.vx < 0.80) {
			found_intermediate = true;
			assert(p.p > 0.01 && p.p < 5.0);
		}
	}
	assert(found_intermediate);

	for (size_t i = 0; i < nx; ++i) {
		const auto& p = solver.primitive(i);
		assert(!std::isnan(p.rho) && p.rho > 0.0);
		assert(!std::isnan(p.p) && p.p > 0.0);
		assert(!std::isnan(p.vx) && std::abs(p.vx) < 1.0);
	}
}

void test_relativistic_mhd_balsara1_shock_tube() {
	using namespace Relativistic::Hydro;
	const size_t nx = 200;
	IdealGasEOS<double> eos(2.0);
	RelativisticHydroSolver1D<IdealGasEOS<double>, double> solver(
		nx, 0.0, 1.0, eos, ReconstructionMethod::WENO5_Z, RiemannSolverType::HLLD, 0.4
	);

	solver.initialize_balsara1_mhd(0.5);
	solver.evolve(0.4);

	for (size_t i = 0; i < nx; ++i) {
		const auto& p = solver.primitive(i);
		assert(!std::isnan(p.rho) && p.rho > 0.0);
		assert(!std::isnan(p.p) && p.p > 0.0);
		assert(!std::isnan(p.vx) && std::abs(p.vx) < 1.0);
		assert(!std::isnan(p.by));
	}
}

int main() {
	std::cout << "[RUN] Testing Equation of State models...\n";
	test_equation_of_state();
	std::cout << "[PASS] Equation of State validated.\n";

	std::cout << "[RUN] Testing Primitive <-> Conserved variable inversion (Con2Prim)...\n";
	test_primitive_conserved_round_trip();
	std::cout << "[PASS] Con2Prim inversion round-trip verified.\n";

	std::cout << "[RUN] Testing High-Order Spatial Reconstruction (WENO5 / MP5)...\n";
	test_spatial_reconstruction_accuracy();
	std::cout << "[PASS] 5th-order spatial reconstruction verified.\n";

	std::cout << "[RUN] Testing Approximate Riemann Solvers (HLL / HLLC / HLLD)...\n";
	test_riemann_solvers_consistency();
	std::cout << "[PASS] Riemann solvers consistency verified.\n";

	std::cout << "[RUN] Testing Constrained Transport divergence preservation...\n";
	test_constrained_transport_divergence();
	std::cout << "[PASS] Constrained transport zero-divergence verified.\n";

	std::cout << "[RUN] Testing 1D Relativistic Sod Shock Tube...\n";
	test_relativistic_sod_shock_tube();
	std::cout << "[PASS] Relativistic Sod Shock Tube benchmark validated.\n";

	std::cout << "[RUN] Testing 1D Relativistic MHD Shock Tube (Balsara Test 1)...\n";
	test_relativistic_mhd_balsara1_shock_tube();
	std::cout << "[PASS] Relativistic MHD shock tube validated.\n";

	std::cout << "All Relativistic Hydrodynamics (GRHD/GRMHD) tests passed successfully.\n";
	return 0;
}
