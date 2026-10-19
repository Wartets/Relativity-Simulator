#include "support/test_harness.hpp"
#include "relativistic/core/math/tensor.hpp"
#include "relativistic/metrics/flat_minkowski.hpp"
#include "relativistic/metrics/schwarzschild.hpp"
#include "relativistic/integrators/symplectic_gauss_legendre.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>

using namespace Relativistic::Core;
using namespace Relativistic::Metrics;
using namespace Relativistic::Integrators;
using Relativistic::Testing::TestSuite;

int main() {
	TestSuite suite(
		"symplectic_gauss_legendre",
		"Verifies the implicit Gauss-Legendre integrators: exact straight-line propagation in flat spacetime with the fourth-order scheme, and conservation of the normalization, energy and angular momentum of a circular Schwarzschild orbit over 2000 steps with the sixth-order scheme."
	);

	suite.section("Flat spacetime");
	FlatMinkowskiMetric<double> flat(1.0);
	GaussLegendre4<FlatMinkowskiMetric<double>, double> fourth(flat);
	GeodesicState<double> free_particle;
	free_particle.x = FourVector<double>(0.0, 1.0, 2.0, 3.0);
	free_particle.u = FourVector<double>(std::sqrt(1.0 + 0.01 + 0.04 + 0.09), 0.1, 0.2, 0.3);
	bool steps_ok = true;
	for (size_t index = 0; index < 100; ++index) {
		steps_ok = fourth.step(free_particle, 0.1) && steps_ok;
	}
	suite.check(steps_ok, "all one hundred implicit steps converge");
	suite.check_near(free_particle.x(1), 2.0, 1e-12, "the x coordinate advances by u^x times the proper time");
	suite.check_near(free_particle.x(2), 4.0, 1e-12, "the y coordinate advances by u^y times the proper time");
	suite.check_near(free_particle.x(3), 6.0, 1e-12, "the z coordinate advances by u^z times the proper time");

	suite.section("Circular Schwarzschild orbit");
	SchwarzschildMetric<double> schwarzschild(1.0, 1.0, 1.0);
	GaussLegendre6<SchwarzschildMetric<double>, double> sixth(schwarzschild);
	const double orbit = 10.0;
	const double velocity = std::sqrt(1.0 / (orbit - 3.0));
	const double u_t = std::sqrt((1.0 + orbit * orbit * velocity * velocity) / (1.0 - 2.0 / orbit));
	GeodesicState<double> state;
	state.x = FourVector<double>(0.0, orbit, std::numbers::pi_v<double> / 2.0, 0.0);
	state.u = FourVector<double>(u_t, 0.0, 0.0, velocity);
	const double norm0 = -(1.0 - 2.0 / orbit) * u_t * u_t + orbit * orbit * velocity * velocity;
	const double energy0 = (1.0 - 2.0 / orbit) * u_t;
	const double momentum0 = orbit * orbit * velocity;
	double norm_drift = 0.0;
	double energy_drift = 0.0;
	double momentum_drift = 0.0;
	bool orbit_ok = true;
	for (size_t index = 0; index < 2000; ++index) {
		orbit_ok = sixth.step(state, 0.05) && orbit_ok;
		const double r = state.x(1);
		const double f = 1.0 - 2.0 / r;
		norm_drift = std::max(norm_drift, std::abs(-f * state.u(0) * state.u(0) + state.u(1) * state.u(1) / f + r * r * state.u(3) * state.u(3) - norm0));
		energy_drift = std::max(energy_drift, std::abs(f * state.u(0) - energy0));
		momentum_drift = std::max(momentum_drift, std::abs(r * r * state.u(3) - momentum0));
	}
	suite.check(orbit_ok, "all two thousand implicit steps converge");
	suite.measure("normalization_drift", norm_drift, "dimensionless");
	suite.measure("energy_drift", energy_drift, "dimensionless");
	suite.measure("angular_momentum_drift", momentum_drift, "dimensionless");
	suite.check_at_most(norm_drift, 1e-11, "the four-velocity normalization drifts by less than 1e-11");
	suite.check_at_most(energy_drift, 1e-11, "the conserved energy drifts by less than 1e-11");
	suite.check_at_most(momentum_drift, 1e-11, "the conserved angular momentum drifts by less than 1e-11");
	return suite.finish();
}
