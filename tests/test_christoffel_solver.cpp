#include "support/test_harness.hpp"
#include "relativistic/core/math/tensor.hpp"
#include "relativistic/core/math/christoffel.hpp"
#include "relativistic/metrics/spacetime_concept.hpp"
#include "relativistic/metrics/flat_minkowski.hpp"
#include "relativistic/metrics/schwarzschild.hpp"
#include <algorithm>
#include <array>
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

void* operator new[](size_t size) {
	g_dynamic_allocations_count.fetch_add(1, std::memory_order_relaxed);
	if (g_trap_dynamic_allocations.load(std::memory_order_relaxed)) {
		std::abort();
	}
	return std::malloc(size);
}

void operator delete[](void* ptr) noexcept {
	std::free(ptr);
}

void operator delete[](void* ptr, size_t) noexcept {
	std::free(ptr);
}

struct Deviation {
	double relative{0.0};
	double absolute{0.0};
};

[[nodiscard]] Deviation compare_christoffel(const Relativistic::Core::ChristoffelSymbols<double>& numerical, const Relativistic::Core::ChristoffelSymbols<double>& exact, double zero_threshold = 1e-14) noexcept {
	Deviation worst;
	for (size_t sigma = 0; sigma < 4; ++sigma) {
		for (size_t mu = 0; mu < 4; ++mu) {
			for (size_t nu = 0; nu < 4; ++nu) {
				const double reference = exact(sigma, mu, nu);
				const double error = std::abs(numerical(sigma, mu, nu) - reference);
				worst.absolute = std::max(worst.absolute, error);
				if (std::abs(reference) > zero_threshold) {
					worst.relative = std::max(worst.relative, error / std::abs(reference));
				}
			}
		}
	}
	return worst;
}

int main() {
	using namespace Relativistic::Core;
	using namespace Relativistic::Metrics;

	static_assert(SpacetimeMetric<FlatMinkowskiMetric<double>>);
	static_assert(SpacetimeMetric<SchwarzschildMetric<double>>);
	static_assert(SpacetimeMetric<NumericalMetricWrapper<SchwarzschildMetric<double>>>);

	TestSuite suite(
		"christoffel_solver",
		"Verifies the finite-difference connection solver at fourth, sixth and eighth order against the analytic connection of Minkowski and Schwarzschild spacetimes: exactness on flat space, lower-index symmetry, error ordering across stencil orders, dispatch to the analytic connection and the numerical wrapper, without dynamic allocation."
	);

	std::array<double, 4> flat_error{};
	double worst_relative_4 = 0.0;
	double worst_relative_6 = 0.0;
	double worst_relative_8 = 0.0;
	double worst_absolute_8 = 0.0;
	double worst_asymmetry = 0.0;
	double worst_dispatch = 0.0;
	double worst_wrapper = 0.0;
	bool ordering_holds = true;

	g_trap_dynamic_allocations.store(true, std::memory_order_seq_cst);
	{
		FlatMinkowskiMetric<double> minkowski(1.0);
		FourVector<double> x;
		x(0) = 10.0;
		x(1) = 5.0;
		x(2) = 2.0;
		x(3) = -3.0;
		const auto exact = minkowski.christoffel_symbols(x);
		flat_error[0] = compare_christoffel(compute_christoffel_numerical<DerivativeOrder::FourthOrder>(minkowski, x), exact).absolute;
		flat_error[1] = compare_christoffel(compute_christoffel_numerical<DerivativeOrder::SixthOrder>(minkowski, x), exact).absolute;
		flat_error[2] = compare_christoffel(compute_christoffel_numerical<DerivativeOrder::EighthOrder>(minkowski, x), exact).absolute;
		flat_error[3] = compare_christoffel(compute_christoffel(minkowski, x), exact).absolute;
	}
	{
		SchwarzschildMetric<double> schwarzschild(1.0, 1.0, 1.0);
		const std::array<FourVector<double>, 4> points = {
			FourVector<double>(0.0, 6.0, std::numbers::pi_v<double> / 3.0, 0.0),
			FourVector<double>(10.0, 10.0, std::numbers::pi_v<double> / 2.0, 1.2),
			FourVector<double>(5.0, 50.0, std::numbers::pi_v<double> / 4.0, 2.5),
			FourVector<double>(0.0, 3.0, std::numbers::pi_v<double> / 3.0, 0.5)
		};
		for (const auto& x : points) {
			const auto exact = schwarzschild.christoffel_symbols(x);
			const auto gamma_4 = compute_christoffel_numerical<DerivativeOrder::FourthOrder>(schwarzschild, x);
			const auto gamma_6 = compute_christoffel_numerical<DerivativeOrder::SixthOrder>(schwarzschild, x);
			const auto gamma_8 = compute_christoffel_numerical<DerivativeOrder::EighthOrder>(schwarzschild, x);
			for (size_t sigma = 0; sigma < 4; ++sigma) {
				for (size_t mu = 0; mu < 4; ++mu) {
					for (size_t nu = 0; nu < 4; ++nu) {
						worst_asymmetry = std::max({worst_asymmetry, std::abs(gamma_4(sigma, mu, nu) - gamma_4(sigma, nu, mu)), std::abs(gamma_6(sigma, mu, nu) - gamma_6(sigma, nu, mu)), std::abs(gamma_8(sigma, mu, nu) - gamma_8(sigma, nu, mu))});
					}
				}
			}
			const auto d4 = compare_christoffel(gamma_4, exact);
			const auto d6 = compare_christoffel(gamma_6, exact);
			const auto d8 = compare_christoffel(gamma_8, exact);
			worst_relative_4 = std::max(worst_relative_4, d4.relative);
			worst_relative_6 = std::max(worst_relative_6, d6.relative);
			worst_relative_8 = std::max(worst_relative_8, d8.relative);
			worst_absolute_8 = std::max(worst_absolute_8, d8.absolute);
			ordering_holds = ordering_holds && d6.relative <= d4.relative;
			worst_dispatch = std::max(worst_dispatch, compare_christoffel(compute_christoffel(schwarzschild, x), exact).absolute);
			NumericalMetricWrapper<SchwarzschildMetric<double>> wrapper(schwarzschild);
			worst_wrapper = std::max(worst_wrapper, compare_christoffel(compute_christoffel(wrapper, x), exact).relative);
		}
	}
	g_trap_dynamic_allocations.store(false, std::memory_order_seq_cst);
	const size_t allocations = g_dynamic_allocations_count.load(std::memory_order_relaxed);

	suite.section("Flat spacetime");
	suite.check_at_most(flat_error[0], 1e-15, "the fourth-order connection of Minkowski vanishes");
	suite.check_at_most(flat_error[1], 1e-15, "the sixth-order connection of Minkowski vanishes");
	suite.check_at_most(flat_error[2], 1e-15, "the eighth-order connection of Minkowski vanishes");
	suite.check_at_most(flat_error[3], 1e-15, "the dispatching solver returns the vanishing analytic connection of Minkowski");

	suite.section("Schwarzschild spacetime");
	suite.check_at_most(worst_asymmetry, 1e-15, "the numerical connection is symmetric in its lower indices for every stencil order");
	suite.check_at_most(worst_relative_4, 1e-6, "the fourth-order connection agrees with the analytic one within a relative 1e-6");
	suite.check_at_most(worst_relative_6, 1e-9, "the sixth-order connection agrees with the analytic one within a relative 1e-9");
	suite.check_at_most(worst_relative_8, 1e-12, "the eighth-order connection agrees with the analytic one within a relative 1e-12");
	suite.check_at_most(worst_absolute_8, 1e-12, "the eighth-order connection agrees with the analytic one within an absolute 1e-12");
	suite.check(ordering_holds, "the sixth-order relative error never exceeds the fourth-order relative error");
	suite.check_at_most(worst_dispatch, 0.0, "the dispatching solver returns exactly the analytic connection when the metric provides one");
	suite.check_at_most(worst_wrapper, 1e-12, "the numerical metric wrapper reproduces the analytic connection within a relative 1e-12");

	suite.section("Allocation discipline");
	suite.check(allocations == 0, "no dynamic allocation occurs in the connection solver", "allocations=" + std::to_string(allocations));
	return suite.finish();
}
