#include "relativistic/core/tensor.hpp"
#include "relativistic/core/tensor_ops.hpp"
#include <cassert>
#include <cmath>
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

template <typename T>
[[nodiscard]] double max_metric_inversion_residual(
	const Relativistic::Core::Tensor<T, 2, 4>& g,
	const Relativistic::Core::Tensor<T, 2, 4>& inv_g
) noexcept {
	using namespace Relativistic::Core;
	const auto product = contract_metric_inverse(inv_g, g);
	const auto delta = kronecker_delta<T, 4>();
	double max_err = 0.0;
	for (size_t mu = 0; mu < 4; ++mu) {
		for (size_t nu = 0; nu < 4; ++nu) {
			const double diff = std::abs(static_cast<double>(product(mu, nu) - delta(mu, nu)));
			if (diff > max_err) {
				max_err = diff;
			}
		}
	}
	return max_err;
}

int main() {
	using namespace Relativistic::Core;

	g_trap_dynamic_allocations.store(true, std::memory_order_seq_cst);

	{
		MetricTensor<double> eta;
		eta.zero();
		eta(0, 0) = -1.0;
		eta(1, 1) = 1.0;
		eta(2, 2) = 1.0;
		eta(3, 3) = 1.0;

		const auto inv_eta = inverse_metric_4x4(eta);
		const double res_eta = max_metric_inversion_residual(eta, inv_eta);
		assert(res_eta < 1e-15);
	}

	{
		constexpr double M = 1.0;
		constexpr double r = 6.0 * M;
		constexpr double theta = 0.5 * std::numbers::pi_v<double>;
		const double f = 1.0 - 2.0 * M / r;

		MetricTensor<double> g_schwarzschild;
		g_schwarzschild.zero();
		g_schwarzschild(0, 0) = -f;
		g_schwarzschild(1, 1) = 1.0 / f;
		g_schwarzschild(2, 2) = r * r;
		g_schwarzschild(3, 3) = r * r * std::sin(theta) * std::sin(theta);

		const auto inv_g_schwarzschild = inverse_metric_4x4(g_schwarzschild);
		const double res_schwarzschild = max_metric_inversion_residual(g_schwarzschild, inv_g_schwarzschild);
		assert(res_schwarzschild < 1e-15);
	}

	{
		constexpr double M = 1.0;
		constexpr double a = 0.9 * M;
		constexpr double r = 4.0 * M;
		constexpr double theta = std::numbers::pi_v<double> / 3.0;

		const double cos_t = std::cos(theta);
		const double sin_t = std::sin(theta);
		const double sin2_t = sin_t * sin_t;
		const double rho2 = r * r + a * a * cos_t * cos_t;
		const double delta = r * r - 2.0 * M * r + a * a;

		MetricTensor<double> g_kerr;
		g_kerr.zero();
		g_kerr(0, 0) = -(1.0 - (2.0 * M * r) / rho2);
		g_kerr(0, 3) = -(2.0 * M * a * r * sin2_t) / rho2;
		g_kerr(3, 0) = g_kerr(0, 3);
		g_kerr(1, 1) = rho2 / delta;
		g_kerr(2, 2) = rho2;
		g_kerr(3, 3) = (r * r + a * a + (2.0 * M * a * a * r * sin2_t) / rho2) * sin2_t;

		const auto inv_g_kerr = inverse_metric_4x4(g_kerr);
		const double res_kerr = max_metric_inversion_residual(g_kerr, inv_g_kerr);
		assert(res_kerr < 1e-15);
	}

	{
		MetricTensor<double> g_dense;
		g_dense(0, 0) = -1.2;  g_dense(0, 1) = 0.1;   g_dense(0, 2) = 0.05;  g_dense(0, 3) = -0.2;
		g_dense(1, 0) = 0.1;   g_dense(1, 1) = 1.5;   g_dense(1, 2) = 0.3;   g_dense(1, 3) = 0.08;
		g_dense(2, 0) = 0.05;  g_dense(2, 1) = 0.3;   g_dense(2, 2) = 2.1;   g_dense(2, 3) = -0.15;
		g_dense(3, 0) = -0.2;  g_dense(3, 1) = 0.08;  g_dense(3, 2) = -0.15; g_dense(3, 3) = 3.4;

		const auto inv_g_dense = inverse_metric_4x4(g_dense);
		const double res_dense = max_metric_inversion_residual(g_dense, inv_g_dense);
		assert(res_dense < 1e-15);
	}

	{
		Vector<double, 4> u;
		u(0) = 1.0; u(1) = 2.0; u(2) = -3.0; u(3) = 0.5;

		Vector<double, 4> v;
		v(0) = -0.5; v(1) = 1.5; v(2) = 2.0; v(3) = -1.0;

		const auto t_prod = tensor_product(u, v);
		assert(t_prod.rank() == 2);
		for (size_t i = 0; i < 4; ++i) {
			for (size_t j = 0; j < 4; ++j) {
				assert(std::abs(t_prod(i, j) - u(i) * v(j)) < 1e-16);
			}
		}

		const auto contracted = contract<0, 1>(t_prod);
		assert(contracted.rank() == 0);
		double manual_dot = 0.0;
		for (size_t i = 0; i < 4; ++i) {
			manual_dot += u(i) * v(i);
		}
		assert(std::abs(contracted[0] - manual_dot) < 1e-16);
	}

	{
		Tensor<double, 3, 4> christoffel;
		christoffel.fill(0.0);
		christoffel(1, 0, 0) = 0.25;
		christoffel(1, 1, 1) = -0.25;
		christoffel(1, 2, 2) = -3.0;
		christoffel(1, 3, 3) = -2.5;

		const auto contracted = contract<0, 1>(christoffel);
		assert(contracted.rank() == 1);
		assert(std::abs(contracted(0) - 0.0) < 1e-16);
		assert(std::abs(contracted(1) - (-0.25)) < 1e-16);
	}

	{
		Matrix<double, 4> m;
		m(0, 1) = 2.0; m(1, 0) = 4.0;
		m(2, 3) = -1.0; m(3, 2) = 5.0;

		const auto s = symmetrize(m);
		assert(s(0, 1) == 3.0 && s(1, 0) == 3.0);
		assert(s(2, 3) == 2.0 && s(3, 2) == 2.0);

		const auto a = antisymmetrize(m);
		assert(a(0, 1) == -1.0 && a(1, 0) == 1.0);
		assert(a(2, 3) == -3.0 && a(3, 2) == 3.0);
	}

	g_trap_dynamic_allocations.store(false, std::memory_order_seq_cst);

	assert(g_dynamic_allocations_count.load(std::memory_order_relaxed) == 0);

	return 0;
}
