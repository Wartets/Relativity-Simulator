#include "support/test_harness.hpp"
#include "relativistic/optics/terrell_penrose.hpp"
#include <algorithm>
#include <array>
#include <cmath>

using namespace Relativistic::Optics;
using Relativistic::Testing::TestSuite;

namespace {

[[nodiscard]] double largest_difference(const std::array<double, 3>& a, const std::array<double, 3>& b) noexcept {
	return std::max({std::abs(a[0] - b[0]), std::abs(a[1] - b[1]), std::abs(a[2] - b[2])});
}

}

int main() {
	TestSuite suite(
		"terrell_penrose",
		"Verifies the Terrell-Penrose optics: the backward aberration inverts the forward aberration for axis-aligned and oblique velocities, a ray intersects a moving sphere with a positive Doppler factor, and the apparent rotation angle satisfies sin(angle) = beta."
	);

	suite.section("Aberration inversion");
	const std::array<double, 3> beta_axis = {0.5, 0.0, 0.0};
	const std::array<double, 3> normal_axis = {0.0, 1.0, 0.0};
	const auto restored_axis = TerrellPenroseOptics<double>::aberrate_backward(TerrellPenroseOptics<double>::aberrate_forward(normal_axis, beta_axis), beta_axis);
	suite.check_at_most(largest_difference(restored_axis, normal_axis), 1e-12, "the backward aberration restores a transverse direction for a boost along x");
	const std::array<double, 3> beta_oblique = {0.2, 0.3, 0.4};
	const double inverse_root = 1.0 / std::sqrt(3.0);
	const std::array<double, 3> normal_oblique = {inverse_root, inverse_root, inverse_root};
	const auto restored_oblique = TerrellPenroseOptics<double>::aberrate_backward(TerrellPenroseOptics<double>::aberrate_forward(normal_oblique, beta_oblique), beta_oblique);
	suite.check_at_most(largest_difference(restored_oblique, normal_oblique), 1e-12, "the backward aberration restores a diagonal direction for an oblique boost");

	suite.section("Moving sphere");
	const auto hit = TerrellPenroseOptics<double>::intersect_moving_sphere({0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, 10.0, {0.0, 10.0, 0.0}, {0.8, 0.0, 0.0}, 1.0);
	suite.check(hit.has_value(), "a ray aimed at the retarded position of the sphere intersects it");
	suite.check(hit.has_value() && hit->doppler_shift > 0.0, "the Doppler shift at the hit is positive");
	suite.check_near(std::sin(TerrellPenroseOptics<double>::rotation_angle(0.8)), 0.8, 1e-12, "the apparent rotation angle satisfies sin(angle) = beta");
	return suite.finish();
}
