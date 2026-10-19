#include "support/test_harness.hpp"
#include "relativistic/core/math/geodesic_bundle.hpp"
#include "relativistic/core/math/four_vector_bundle.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <new>
#include <numbers>

using Relativistic::Testing::TestSuite;

static std::atomic<size_t> g_dynamic_allocations_count{0};
static std::atomic<bool> g_trap_dynamic_allocations{false};

void* operator new(size_t size) {
	g_dynamic_allocations_count.fetch_add(1, std::memory_order_relaxed);
	if (g_trap_dynamic_allocations.load(std::memory_order_relaxed)) {
		std::abort();
	}
	return std::malloc(size);
}

void operator delete(void* ptr) noexcept {
	std::free(ptr);
}

void operator delete(void* ptr, size_t) noexcept {
	std::free(ptr);
}

int main() {
	using namespace Relativistic::Core;

	TestSuite suite(
		"geodesic_bundle",
		"Verifies the structure-of-arrays ray bundles and their four-vector algebra: the lane-wise Minkowski product, the Lorentz factor, the relativistic velocity addition, and the evolution of an eight-lane Schwarzschild null bundle, without any dynamic allocation."
	);

	double dot_error = 0.0;
	double gamma_error = 0.0;
	double addition_error = 0.0;
	bool bundle_initially_active = false;
	size_t bundle_initial_count = 0;
	bool bundle_positions_positive = true;

	g_trap_dynamic_allocations.store(true, std::memory_order_seq_cst);
	{
		Double4 u0(1.0, 2.0, 3.0, 4.0);
		Double4 u1(0.0, 1.0, 0.0, 0.0);
		Double4 u2(0.0, 0.0, 1.0, 0.0);
		Double4 u3(0.0, 0.0, 0.0, 1.0);
		Double4 v0(1.0, 1.0, 1.0, 1.0);
		Double4 v1(0.0, 0.5, 0.0, 0.0);
		Double4 v2(0.0, 0.0, 0.5, 0.0);
		Double4 v3(0.0, 0.0, 0.0, 0.5);
		const auto dot = minkowski_dot(u0, u1, u2, u3, v0, v1, v2, v3, 1.0);
		dot_error = std::max({std::abs(dot[0] + 1.0), std::abs(dot[1] - (-2.0 + 0.5)), std::abs(dot[2] - (-3.0 + 0.5)), std::abs(dot[3] - (-4.0 + 0.5))});
	}
	{
		Double4 v1(0.6, 0.0, 0.8, 0.0);
		Double4 v2(0.0, 0.6, 0.0, 0.0);
		Double4 v3(0.0, 0.0, 0.0, 0.0);
		const auto gamma = lorentz_factor(v1, v2, v3, 1.0);
		gamma_error = std::max({std::abs(gamma[0] - 1.25), std::abs(gamma[1] - 1.25), std::abs(gamma[2] - 1.0 / 0.6), std::abs(gamma[3] - 1.0)});
	}
	{
		Double4 u1(0.5, 0.5, 0.0, 0.0);
		Double4 u2(0.0, 0.0, 0.0, 0.0);
		Double4 u3(0.0, 0.0, 0.0, 0.0);
		Double4 v1(0.5, -0.5, 0.0, 0.0);
		Double4 v2(0.0, 0.0, 0.0, 0.0);
		Double4 v3(0.0, 0.0, 0.0, 0.0);
		const auto added = relativistic_velocity_add(u1, u2, u3, v1, v2, v3, 1.0);
		addition_error = std::max(std::abs(added.v1[0] - 1.0 / 1.25), std::abs(added.v1[1]));
	}
	{
		GeodesicBundle8d bundle;
		bundle_initially_active = bundle.is_all_active();
		bundle_initial_count = bundle.active_count();
		constexpr double mass = 1.0;
		constexpr double r_start = 20.0;
		constexpr double theta = 0.5 * std::numbers::pi_v<double>;
		for (size_t i = 0; i < 8; ++i) {
			const double impact = 4.0 + static_cast<double>(i) * 0.5;
			const double f = 1.0 - 2.0 * mass / r_start;
			const double pr_sq = 1.0 - f * (impact * impact) / (r_start * r_start);
			bundle.set_ray(i, {0.0, r_start, theta, 0.0}, {1.0 / f, -std::sqrt(std::max(0.0, pr_sq)), 0.0, impact / (r_start * r_start)}, 0.01);
		}
		for (int step = 0; step < 500 && bundle.is_any_active(); ++step) {
			bundle.step_rk4_schwarzschild(mass, 1.0, 1.0);
		}
		for (size_t i = 0; i < 8; ++i) {
			bundle_positions_positive = bundle_positions_positive && bundle.get_position(i)[1] > 0.0;
		}
	}
	g_trap_dynamic_allocations.store(false, std::memory_order_seq_cst);
	const size_t allocations = g_dynamic_allocations_count.load(std::memory_order_relaxed);

	suite.section("Four-vector algebra");
	suite.check_at_most(dot_error, 1e-15, "the lane-wise Minkowski product follows -u0 v0 + u.v");
	suite.check_at_most(gamma_error, 1e-14, "the Lorentz factor equals 1 / sqrt(1 - beta^2) in every lane");
	suite.check_at_most(addition_error, 1e-14, "the relativistic addition of 0.5 c and -0.5 c along x yields 0.8 c in the first lane and zero in the second");

	suite.section("Ray bundle");
	suite.check(bundle_initially_active && bundle_initial_count == 8, "a new eight-lane bundle has all lanes active");
	suite.check(bundle_positions_positive, "every lane keeps a positive radius after the Schwarzschild evolution");

	suite.section("Allocation discipline");
	suite.check(allocations == 0, "no dynamic allocation occurs in the bundle algebra", "allocations=" + std::to_string(allocations));
	return suite.finish();
}
