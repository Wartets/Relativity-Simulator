#include "support/test_harness.hpp"
#include "relativistic/dynamics/pn/pn_body.hpp"
#include "relativistic/dynamics/pn/pn_orders.hpp"
#include "relativistic/dynamics/pn/pn_spin_precession.hpp"
#include <array>
#include <cmath>

using namespace Relativistic::Dynamics;
using Relativistic::Testing::TestSuite;

namespace {

template <typename VectorA, typename VectorB>
[[nodiscard]] double dot3(const VectorA& a, const VectorB& b) noexcept {
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

template <typename Vector>
[[nodiscard]] double norm3(const Vector& v) noexcept {
	return std::sqrt(dot3(v, v));
}

}

int main() {
	TestSuite suite(
		"pn_spin_precession",
		"Verifies the post-Newtonian spin precession of a binary: both precession angular velocities point along the orbital angular momentum and each spin derivative is orthogonal to its spin, so the spin magnitudes are conserved."
	);

	const auto config = PNOrderConfig::make_all_enabled(1.0, 1.0);
	const std::array<double, 3> separation = {10.0, 0.0, 0.0};
	const std::array<double, 3> velocity = {0.0, 0.2, 0.0};
	const std::array<double, 3> spin_1 = {0.0, 0.0, 0.5};
	const std::array<double, 3> spin_2 = {0.1, 0.0, 0.4};
	const auto rates = PostNewtonianSpinSolver::compute_spin_precession_binary(separation, velocity, 1.0, 1.0, spin_1, spin_2, config);

	suite.section("Precession direction");
	suite.check(rates.omega_1[2] > 0.0, "the precession angular velocity of the first body has a positive component along the orbital axis");
	suite.check(rates.omega_2[2] > 0.0, "the precession angular velocity of the second body has a positive component along the orbital axis");

	suite.section("Spin magnitude conservation");
	const double alignment_1 = std::abs(dot3(spin_1, rates.d_spin_1));
	const double alignment_2 = std::abs(dot3(spin_2, rates.d_spin_2));
	const double reference_1 = norm3(spin_1) * norm3(rates.d_spin_1);
	const double reference_2 = norm3(spin_2) * norm3(rates.d_spin_2);
	suite.check_at_most(alignment_1, 1e-12 * reference_1 + 1e-300, "the first spin derivative is orthogonal to the first spin relative to their norms");
	suite.check_at_most(alignment_2, 1e-12 * reference_2 + 1e-300, "the second spin derivative is orthogonal to the second spin relative to their norms");
	return suite.finish();
}
