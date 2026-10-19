#include "support/test_harness.hpp"
#include "relativistic/hydro/eos.hpp"
#include "relativistic/hydro/hydro_types.hpp"
#include "relativistic/hydro/solvers/con2prim.hpp"
#include "relativistic/hydro/solvers/reconstruction.hpp"
#include "relativistic/hydro/solvers/riemann_solvers.hpp"
#include "relativistic/hydro/solvers/constrained_transport.hpp"
#include "relativistic/hydro/solvers/grhd_solver.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <string>
#include <vector>

using namespace Relativistic::Hydro;
using Relativistic::Testing::TestSuite;

namespace {

void check_equation_of_state(TestSuite& suite) {
	suite.section("Equation of state");
	IdealGasEOS<double> eos(5.0 / 3.0);
	const double rho = 2.0;
	const double eps = 1.5;
	const double p = eos.pressure(rho, eps);
	suite.check_near(p, (2.0 / 3.0) * rho * eps, 1e-12, "the ideal gas pressure equals (Gamma - 1) rho eps");
	suite.check_near(eos.specific_internal_energy(rho, p), eps, 1e-12, "the specific internal energy is recovered from the pressure");
	const double h = eos.specific_enthalpy(rho, p);
	suite.check_near(h, 1.0 + eps + p / rho, 1e-12, "the specific enthalpy equals 1 + eps + p / rho");
	const double cs2 = eos.sound_speed_squared(rho, p);
	suite.check(cs2 > 0.0 && cs2 < 5.0 / 3.0 - 1.0, "the sound speed squared is positive and below Gamma - 1");
	suite.check_near(eos.pressure_from_enthalpy(rho, h), p, 1e-12, "the pressure is recovered from the enthalpy");
	SyngeEOS<double> synge;
	suite.check(synge.specific_enthalpy(rho, p) > 1.0, "the Synge enthalpy exceeds 1");
	const double cs2_synge = synge.sound_speed_squared(rho, p);
	suite.check(cs2_synge > 0.0 && cs2_synge <= 1.0 / 3.0, "the Synge sound speed squared lies in (0, 1/3]");
}

void check_con2prim(TestSuite& suite) {
	suite.section("Conserved to primitive inversion");
	IdealGasEOS<double> eos(5.0 / 3.0);
	Con2PrimSolver<IdealGasEOS<double>, double> solver(eos);
	{
		PrimitiveVariables<double> original(1.2, 0.8, 0.4, 0.1, -0.2);
		const auto recovered = solver.solve(prim_to_con_flat(original));
		const double worst = std::max({std::abs(recovered.rho - original.rho), std::abs(recovered.p - original.p), std::abs(recovered.vx - original.vx), std::abs(recovered.vy - original.vy), std::abs(recovered.vz - original.vz)});
		suite.check_at_most(worst, 1e-10, "a hydrodynamic state round trips within 1e-10");
	}
	{
		PrimitiveVariables<double> ultra(0.1, 0.05, 0.999, 0.0, 0.0);
		const auto recovered = solver.solve(prim_to_con_flat(ultra));
		const double worst = std::max({std::abs(recovered.rho - ultra.rho), std::abs(recovered.p - ultra.p), std::abs(recovered.vx - ultra.vx)});
		suite.check_at_most(worst, 1e-8, "an ultra-relativistic state with v = 0.999 round trips within 1e-8");
	}
	{
		PrimitiveVariables<double> magnetized(1.5, 2.0, 0.3, -0.2, 0.1, 0.5, 1.0, -0.4);
		const auto recovered = solver.solve(prim_to_con_flat(magnetized));
		const double worst = std::max({std::abs(recovered.rho - magnetized.rho), std::abs(recovered.p - magnetized.p), std::abs(recovered.vx - magnetized.vx), std::abs(recovered.vy - magnetized.vy), std::abs(recovered.vz - magnetized.vz)});
		suite.check_at_most(worst, 1e-6, "a magnetized state round trips within 1e-6");
		const double field = std::max({std::abs(recovered.bx - magnetized.bx), std::abs(recovered.by - magnetized.by), std::abs(recovered.bz - magnetized.bz)});
		suite.check_at_most(field, 1e-12, "the magnetic field is recovered within 1e-12");
	}
}

void check_reconstruction(TestSuite& suite) {
	suite.section("Spatial reconstruction");
	const auto cell_average = [](double left, double right) noexcept {
		const double two_pi = 2.0 * std::numbers::pi_v<double>;
		return (std::cos(two_pi * left) - std::cos(two_pi * right)) / (two_pi * (right - left));
	};
	constexpr size_t n = 64;
	const double dx = 1.0 / static_cast<double>(n);
	std::vector<double> values(n);
	for (size_t i = 0; i < n; ++i) {
		values[i] = cell_average(static_cast<double>(i) * dx, static_cast<double>(i + 1) * dx);
	}
	double weno_error = 0.0;
	double mp5_error = 0.0;
	for (size_t i = 2; i < n - 2; ++i) {
		const double exact = std::sin(2.0 * std::numbers::pi_v<double> * static_cast<double>(i + 1) * dx);
		weno_error = std::max(weno_error, std::abs(WENO5Reconstructor<double>::reconstruct_left_face(values[i - 2], values[i - 1], values[i], values[i + 1], values[i + 2], true) - exact));
		mp5_error = std::max(mp5_error, std::abs(MP5Reconstructor<double>::reconstruct_left_face(values[i - 2], values[i - 1], values[i], values[i + 1], values[i + 2]) - exact));
	}
	suite.check_at_most(weno_error, 1e-6, "WENO5 reconstructs the left face of a smooth profile within 1e-6");
	suite.check_at_most(mp5_error, 1e-6, "MP5 reconstructs the left face of a smooth profile within 1e-6");
	const double step = WENO5Reconstructor<double>::reconstruct_left_face(1.0, 1.0, 1.0, 0.0, 0.0, true);
	suite.check(step >= 0.0 && step <= 1.0, "WENO5 stays within the data bounds across a discontinuity");
}

void check_riemann_solvers(TestSuite& suite) {
	suite.section("Riemann solvers");
	IdealGasEOS<double> eos(1.4);
	HLLRiemannSolver<IdealGasEOS<double>, double> hll(eos);
	HLLCRiemannSolver<IdealGasEOS<double>, double> hllc(eos);
	HLLDRiemannSolver<IdealGasEOS<double>, double> hlld(eos);
	PrimitiveVariables<double> left(1.0, 1.0, 0.0);
	PrimitiveVariables<double> right(1.0, 1.0, 0.0);
	const auto f_hll = hll.solve_1d_x(left, right);
	const auto f_hllc = hllc.solve_1d_x(left, right);
	const auto f_hlld = hlld.solve_1d_x(left, right);
	suite.check_at_most(std::max({std::abs(f_hll.fd), std::abs(f_hllc.fd), std::abs(f_hlld.fd)}), 1e-12, "a uniform static state has no mass flux for HLL, HLLC and HLLD");
	suite.check_at_most(std::max({std::abs(f_hll.fsx - 1.0), std::abs(f_hllc.fsx - 1.0), std::abs(f_hlld.fsx - 1.0)}), 1e-12, "a uniform static state has a momentum flux equal to the pressure for HLL, HLLC and HLLD");
	PrimitiveVariables<double> sod_left(10.0, 40.0 / 3.0, 0.0);
	PrimitiveVariables<double> sod_right(1.0, 1e-6, 0.0);
	suite.check(hll.solve_1d_x(sod_left, sod_right).fsx > 0.0, "the HLL momentum flux of the Sod interface is positive");
	suite.check(hllc.solve_1d_x(sod_left, sod_right).fsx > 0.0, "the HLLC momentum flux of the Sod interface is positive");
}

void check_constrained_transport(TestSuite& suite) {
	suite.section("Constrained transport");
	constexpr size_t nx = 32;
	constexpr size_t ny = 32;
	const double dx = 1.0 / static_cast<double>(nx);
	const double dy = 1.0 / static_cast<double>(ny);
	ConstrainedTransport2D<double> ct(nx, ny, dx, dy);
	std::vector<PrimitiveVariables<double>> cells(nx * ny);
	const auto potential = [](double x, double y) noexcept {
		const double r = std::sqrt(x * x + y * y);
		return (r < 0.3) ? ((0.3 - r) * (0.3 - r)) : 0.0;
	};
	std::vector<double> az((nx + 1) * (ny + 1), 0.0);
	for (size_t j = 0; j <= ny; ++j) {
		for (size_t i = 0; i <= nx; ++i) {
			az[j * (nx + 1) + i] = potential(static_cast<double>(i) * dx - 0.5, static_cast<double>(j) * dy - 0.5);
		}
	}
	for (size_t j = 0; j < ny; ++j) {
		for (size_t i = 0; i <= nx; ++i) {
			ct.bx_face(i, j) = (az[(j + 1) * (nx + 1) + i] - az[j * (nx + 1) + i]) / dy;
		}
	}
	for (size_t j = 0; j <= ny; ++j) {
		for (size_t i = 0; i < nx; ++i) {
			ct.by_face(i, j) = -(az[j * (nx + 1) + i + 1] - az[j * (nx + 1) + i]) / dx;
		}
	}
	for (size_t j = 0; j < ny; ++j) {
		for (size_t i = 0; i < nx; ++i) {
			auto& cell = cells[j * nx + i];
			cell.rho = 1.0;
			cell.p = 1.0;
			cell.vx = 0.1;
			cell.vy = -0.1;
			cell.bx = 0.5 * (ct.bx_face(i, j) + ct.bx_face(i + 1, j));
			cell.by = 0.5 * (ct.by_face(i, j) + ct.by_face(i, j + 1));
			cell.recompute_derived();
		}
	}
	suite.check_at_most(ct.compute_max_divergence(), 1e-12, "the initial field derived from a vector potential is divergence free");
	double worst = 0.0;
	for (int step = 0; step < 50; ++step) {
		ct.compute_vertex_electric_fields_from_cell_centered(cells);
		ct.update_face_magnetic_fields(0.001);
		ct.average_to_cell_centers(cells);
		worst = std::max(worst, ct.compute_max_divergence());
	}
	suite.measure("max_divergence", worst, "dimensionless");
	suite.check_at_most(worst, 1e-12, "the divergence stays below 1e-12 over fifty updates");
}

void check_shock_tubes(TestSuite& suite) {
	suite.section("Relativistic Sod shock tube");
	constexpr size_t nx = 200;
	{
		IdealGasEOS<double> eos(5.0 / 3.0);
		RelativisticHydroSolver1D<IdealGasEOS<double>, double> solver(nx, 0.0, 1.0, eos, ReconstructionMethod::WENO5_Z, RiemannSolverType::HLLC, 0.4);
		solver.initialize_sod_shock_tube(10.0, 40.0 / 3.0, 1.0, 1e-6, 0.5);
		solver.evolve(0.4);
		suite.check(std::abs(solver.primitive(0).rho - 10.0) < 0.1 && std::abs(solver.primitive(0).p - 40.0 / 3.0) < 0.1, "the left state is undisturbed at t = 0.4");
		suite.check(std::abs(solver.primitive(nx - 1).rho - 1.0) < 0.1, "the right density is undisturbed at t = 0.4");
		bool intermediate = false;
		bool intermediate_pressure_valid = true;
		bool physical = true;
		for (size_t i = 0; i < nx; ++i) {
			const auto& p = solver.primitive(i);
			if (p.vx > 0.60 && p.vx < 0.80) {
				intermediate = true;
				intermediate_pressure_valid = intermediate_pressure_valid && p.p > 0.01 && p.p < 5.0;
			}
			physical = physical && std::isfinite(p.rho) && p.rho > 0.0 && std::isfinite(p.p) && p.p > 0.0 && std::isfinite(p.vx) && std::abs(p.vx) < 1.0;
		}
		suite.check(intermediate, "the shocked region has a velocity between 0.60 and 0.80");
		suite.check(intermediate_pressure_valid, "the pressure of the shocked region lies between 0.01 and 5");
		suite.check(physical, "every cell keeps a finite positive density and pressure and a subluminal velocity");
	}
	suite.section("Relativistic Balsara 1 magnetized shock tube");
	{
		IdealGasEOS<double> eos(2.0);
		RelativisticHydroSolver1D<IdealGasEOS<double>, double> solver(nx, 0.0, 1.0, eos, ReconstructionMethod::WENO5_Z, RiemannSolverType::HLLD, 0.4);
		solver.initialize_balsara1_mhd(0.5);
		solver.evolve(0.4);
		bool physical = true;
		for (size_t i = 0; i < nx; ++i) {
			const auto& p = solver.primitive(i);
			physical = physical && std::isfinite(p.rho) && p.rho > 0.0 && std::isfinite(p.p) && p.p > 0.0 && std::isfinite(p.vx) && std::abs(p.vx) < 1.0 && std::isfinite(p.by);
		}
		suite.check(physical, "every cell keeps a finite positive density and pressure, a subluminal velocity and a finite transverse field");
	}
}

}

int main() {
	TestSuite suite(
		"relativistic_hydro",
		"Verifies the relativistic hydrodynamics stack: ideal gas and Synge equations of state, the conserved to primitive inversion for hydrodynamic, ultra-relativistic and magnetized states, WENO5 and MP5 reconstruction, the HLL, HLLC and HLLD solvers, constrained transport and the relativistic Sod and Balsara 1 shock tubes."
	);
	check_equation_of_state(suite);
	check_con2prim(suite);
	check_reconstruction(suite);
	check_riemann_solvers(suite);
	check_constrained_transport(suite);
	check_shock_tubes(suite);
	return suite.finish();
}
