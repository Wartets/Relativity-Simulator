#include "relativistic/core/tensor.hpp"
#include "relativistic/core/christoffel.hpp"
#include "relativistic/metrics/spacetime_concept.hpp"
#include "relativistic/metrics/flat_minkowski.hpp"
#include "relativistic/metrics/schwarzschild.hpp"
#include <cassert>
#include <cmath>
#include <numbers>
#include <atomic>
#include <cstdlib>
#include <new>

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

template <typename Scalar = double>
struct EvaluationResult {
	Scalar max_relative_error;
	Scalar max_absolute_error;
};

template <typename Scalar = double>
[[nodiscard]] EvaluationResult<Scalar> compare_christoffel(
	const Relativistic::Core::ChristoffelSymbols<Scalar>& num,
	const Relativistic::Core::ChristoffelSymbols<Scalar>& exact,
	Scalar zero_threshold = static_cast<Scalar>(1e-14)
) noexcept {
	Scalar max_rel = static_cast<Scalar>(0);
	Scalar max_abs = static_cast<Scalar>(0);

	for (size_t sigma = 0; sigma < 4; ++sigma) {
		for (size_t mu = 0; mu < 4; ++mu) {
			for (size_t nu = 0; nu < 4; ++nu) {
				const Scalar val_num = num(sigma, mu, nu);
				const Scalar val_exact = exact(sigma, mu, nu);
				const Scalar abs_err = std::abs(val_num - val_exact);

				if (abs_err > max_abs) {
					max_abs = abs_err;
				}

				if (std::abs(val_exact) > zero_threshold) {
					const Scalar rel_err = abs_err / std::abs(val_exact);
					if (rel_err > max_rel) {
						max_rel = rel_err;
					}
				}
			}
		}
	}

	return EvaluationResult<Scalar>{
		.max_relative_error = max_rel,
		.max_absolute_error = max_abs
	};
}

int main() {
	using namespace Relativistic::Core;
	using namespace Relativistic::Metrics;

	static_assert(SpacetimeMetric<FlatMinkowskiMetric<double>>);
	static_assert(SpacetimeMetric<SchwarzschildMetric<double>>);
	static_assert(SpacetimeMetric<NumericalMetricWrapper<SchwarzschildMetric<double>>>);

	g_trap_dynamic_allocations.store(true, std::memory_order_seq_cst);

	{
		FlatMinkowskiMetric<double> minkowski(1.0);
		FourVector<double> x;
		x(0) = 10.0;
		x(1) = 5.0;
		x(2) = 2.0;
		x(3) = -3.0;

		const auto gamma_exact = minkowski.christoffel_symbols(x);
		const auto gamma_num_4 = compute_christoffel_numerical<DerivativeOrder::FourthOrder>(minkowski, x);
		const auto gamma_num_6 = compute_christoffel_numerical<DerivativeOrder::SixthOrder>(minkowski, x);
		const auto gamma_num_8 = compute_christoffel_numerical<DerivativeOrder::EighthOrder>(minkowski, x);
		const auto gamma_disp = compute_christoffel(minkowski, x);

		const auto res_4 = compare_christoffel(gamma_num_4, gamma_exact);
		const auto res_6 = compare_christoffel(gamma_num_6, gamma_exact);
		const auto res_8 = compare_christoffel(gamma_num_8, gamma_exact);
		const auto res_disp = compare_christoffel(gamma_disp, gamma_exact);

		assert(res_4.max_absolute_error < 1e-15);
		assert(res_6.max_absolute_error < 1e-15);
		assert(res_8.max_absolute_error < 1e-15);
		assert(res_disp.max_absolute_error < 1e-15);
	}

	{
		constexpr double M = 1.0;
		constexpr double c = 1.0;
		constexpr double G = 1.0;
		SchwarzschildMetric<double> schwarzschild(M, c, G);

		const std::array<FourVector<double>, 4> test_points = {
			FourVector<double>(0.0, 6.0, std::numbers::pi_v<double> / 3.0, 0.0),
			FourVector<double>(10.0, 10.0, std::numbers::pi_v<double> / 2.0, 1.2),
			FourVector<double>(5.0, 50.0, std::numbers::pi_v<double> / 4.0, 2.5),
			FourVector<double>(0.0, 3.0, std::numbers::pi_v<double> / 3.0, 0.5)
		};

		for (const auto& x : test_points) {
			const auto gamma_exact = schwarzschild.christoffel_symbols(x);

			const auto gamma_num_4 = compute_christoffel_numerical<DerivativeOrder::FourthOrder>(schwarzschild, x);
			const auto gamma_num_6 = compute_christoffel_numerical<DerivativeOrder::SixthOrder>(schwarzschild, x);
			const auto gamma_num_8 = compute_christoffel_numerical<DerivativeOrder::EighthOrder>(schwarzschild, x);

			for (size_t sigma = 0; sigma < 4; ++sigma) {
				for (size_t mu = 0; mu < 4; ++mu) {
					for (size_t nu = 0; nu < 4; ++nu) {
						assert(std::abs(gamma_num_4(sigma, mu, nu) - gamma_num_4(sigma, nu, mu)) < 1e-15);
						assert(std::abs(gamma_num_6(sigma, mu, nu) - gamma_num_6(sigma, nu, mu)) < 1e-15);
						assert(std::abs(gamma_num_8(sigma, mu, nu) - gamma_num_8(sigma, nu, mu)) < 1e-15);
					}
				}
			}

			const auto res_4 = compare_christoffel(gamma_num_4, gamma_exact);
			const auto res_6 = compare_christoffel(gamma_num_6, gamma_exact);
			const auto res_8 = compare_christoffel(gamma_num_8, gamma_exact);

			assert(res_4.max_relative_error < 1e-6);
			assert(res_6.max_relative_error < 1e-9);
			assert(res_8.max_relative_error < 1e-12);
			assert(res_8.max_absolute_error < 1e-12);

			assert(res_6.max_relative_error <= res_4.max_relative_error);

			const auto gamma_disp = compute_christoffel(schwarzschild, x);
			const auto res_disp = compare_christoffel(gamma_disp, gamma_exact);
			assert(res_disp.max_absolute_error == 0.0);

			NumericalMetricWrapper<SchwarzschildMetric<double>> num_wrapper(schwarzschild);
			const auto gamma_wrapper_disp = compute_christoffel(num_wrapper, x);
			const auto res_wrapper = compare_christoffel(gamma_wrapper_disp, gamma_exact);
			assert(res_wrapper.max_relative_error < 1e-12);
		}
	}

	g_trap_dynamic_allocations.store(false, std::memory_order_seq_cst);

	assert(g_dynamic_allocations_count.load(std::memory_order_relaxed) == 0);

	return 0;
}
