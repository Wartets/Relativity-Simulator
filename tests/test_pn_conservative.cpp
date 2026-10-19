#include "support/test_harness.hpp"
#include "relativistic/dynamics/pn/pn_body.hpp"
#include "relativistic/dynamics/pn/pn_orders.hpp"
#include "relativistic/dynamics/pn/pn_acceleration.hpp"
#include "relativistic/dynamics/pn/pn_nbody_system.hpp"
#include "relativistic/dynamics/pn/pn_integrator.hpp"
#include <cmath>
#include <cstddef>

using namespace Relativistic::Dynamics;
using Relativistic::Testing::TestSuite;

namespace {

constexpr double kMassEach = 1.0;
constexpr double kSeparation = 20.0;
constexpr double kSpeed = 0.15;
constexpr double kStep = 0.01;
constexpr size_t kStepCount = 2000;

void add_equal_mass_binary(PostNewtonianSystem& system) {
	const PostNewtonianBody first(1, kMassEach, 1.0, {-kSeparation * 0.5, 0.0, 0.0}, {0.0, -kSpeed, 0.0});
	const PostNewtonianBody second(2, kMassEach, 1.0, {kSeparation * 0.5, 0.0, 0.0}, {0.0, kSpeed, 0.0});
	system.add_body(first);
	system.add_body(second);
	system.update_accelerations();
}

void check_first_order_conservation(TestSuite& suite) {
	suite.section("1PN binary with the Runge-Kutta integrator");
	PostNewtonianSystem system(PNOrderConfig::make_1pn_only(10.0, 1.0));
	add_equal_mass_binary(system);
	const double initial_energy = system.compute_total_energy();
	const auto initial_momentum = system.total_angular_momentum();
	for (size_t step = 0; step < kStepCount; ++step) {
		RungeKutta4PNIntegrator::step(system, kStep);
	}
	const double energy_drift = std::abs((system.compute_total_energy() - initial_energy) / initial_energy);
	const double momentum_drift = std::abs((system.total_angular_momentum()[2] - initial_momentum[2]) / initial_momentum[2]);
	suite.measure("relative_energy_drift_1pn_rk4", energy_drift, "dimensionless");
	suite.measure("relative_angular_momentum_drift_1pn_rk4", momentum_drift, "dimensionless");
	suite.check_finite(initial_energy, "the initial total energy is finite");
	suite.check_at_most(energy_drift, 1e-4, "the relative energy drift over 2000 steps stays below 1e-4");
	suite.check_at_most(momentum_drift, 1e-4, "the relative drift of the orbital angular momentum stays below 1e-4");
}

void check_newtonian_symplectic_conservation(TestSuite& suite) {
	suite.section("Newtonian binary with the symplectic integrator");
	PostNewtonianSystem system(PNOrderConfig::make_newtonian_only(10.0, 1.0));
	add_equal_mass_binary(system);
	const double initial_energy = system.compute_total_energy();
	for (size_t step = 0; step < kStepCount; ++step) {
		SymplecticForestRuthPNIntegrator::step(system, kStep);
	}
	const double energy_drift = std::abs((system.compute_total_energy() - initial_energy) / initial_energy);
	suite.measure("relative_energy_drift_newtonian_symplectic", energy_drift, "dimensionless");
	suite.check_at_most(energy_drift, 1e-5, "the relative energy drift of the symplectic scheme stays below 1e-5");
}

}

int main() {
	TestSuite suite(
		"pn_conservative",
		"Verifies the conservative post-Newtonian dynamics: energy and orbital angular momentum conservation of an equal-mass 1PN binary integrated with the fourth-order Runge-Kutta scheme, and energy conservation of a Newtonian binary integrated with the symplectic Forest-Ruth scheme."
	);
	check_first_order_conservation(suite);
	check_newtonian_symplectic_conservation(suite);
	return suite.finish();
}
