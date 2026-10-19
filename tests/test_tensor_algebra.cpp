#include "support/test_harness.hpp"
#include "relativistic/core/math/tensor.hpp"
#include "relativistic/core/math/tensor_ops.hpp"
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
[[nodiscard]] double max_metric_inversion_residual(const Relativistic::Core::Tensor<T, 2, 4>& g, const Relativistic::Core::Tensor<T, 2, 4>& inv_g) noexcept {
	using namespace Relativistic::Core;
	const auto product = contract_metric_inverse(inv_g, g);
	const auto delta = kronecker_delta<T, 4>();
	double worst = 0.0;
	for (size_t mu = 0; mu < 4; ++mu) {
		for (size_t nu = 0; nu < 4; ++nu) {
			worst = std::max(worst, std::abs(static_cast<double>(product(mu, nu) - delta(mu, nu))));
		}
	}
	return worst;
}

int main() {
	using namespace Relativistic::Core;

	TestSuite suite(
		"tensor_algebra",
		"Verifies the static tensor algebra: closed-form 4x4 metric inversion for Minkowski, Schwarzschild, Kerr and a dense indefinite metric, the outer product, index contractions of rank-two and rank-three tensors, and symmetrization, all without a single dynamic allocation."
	);

	double residual_minkowski = 0.0;
	double residual_schwarzschild = 0.0;
	double residual_kerr = 0.0;
	double residual_dense = 0.0;
	double product_error = 0.0;
	double contraction_error = 0.0;
	double connection_trace_error = 0.0;
	double symmetric_error = 0.0;
	double antisymmetric_error = 0.0;
	size_t product_rank = 0;
	size_t contraction_rank = 99;
	size_t connection_rank = 99;

	g_trap_dynamic_allocations.store(true, std::memory_order_seq_cst);
	{
		MetricTensor<double> eta;
		eta.zero();
		eta(0, 0) = -1.0;
		eta(1, 1) = 1.0;
		eta(2, 2) = 1.0;
		eta(3, 3) = 1.0;
		residual_minkowski = max_metric_inversion_residual(eta, inverse_metric_4x4(eta));
	}
	{
		constexpr double mass = 1.0;
		constexpr double radius = 6.0 * mass;
		constexpr double theta = 0.5 * std::numbers::pi_v<double>;
		const double f = 1.0 - 2.0 * mass / radius;
		MetricTensor<double> g;
		g.zero();
		g(0, 0) = -f;
		g(1, 1) = 1.0 / f;
		g(2, 2) = radius * radius;
		g(3, 3) = radius * radius * std::sin(theta) * std::sin(theta);
		residual_schwarzschild = max_metric_inversion_residual(g, inverse_metric_4x4(g));
	}
	{
		constexpr double mass = 1.0;
		constexpr double a = 0.9 * mass;
		constexpr double radius = 4.0 * mass;
		constexpr double theta = std::numbers::pi_v<double> / 3.0;
		const double cos_t = std::cos(theta);
		const double sin2_t = std::sin(theta) * std::sin(theta);
		const double rho2 = radius * radius + a * a * cos_t * cos_t;
		const double delta = radius * radius - 2.0 * mass * radius + a * a;
		MetricTensor<double> g;
		g.zero();
		g(0, 0) = -(1.0 - (2.0 * mass * radius) / rho2);
		g(0, 3) = -(2.0 * mass * a * radius * sin2_t) / rho2;
		g(3, 0) = g(0, 3);
		g(1, 1) = rho2 / delta;
		g(2, 2) = rho2;
		g(3, 3) = (radius * radius + a * a + (2.0 * mass * a * a * radius * sin2_t) / rho2) * sin2_t;
		residual_kerr = max_metric_inversion_residual(g, inverse_metric_4x4(g));
	}
	{
		MetricTensor<double> g;
		g(0, 0) = -1.2;  g(0, 1) = 0.1;   g(0, 2) = 0.05;  g(0, 3) = -0.2;
		g(1, 0) = 0.1;   g(1, 1) = 1.5;   g(1, 2) = 0.3;   g(1, 3) = 0.08;
		g(2, 0) = 0.05;  g(2, 1) = 0.3;   g(2, 2) = 2.1;   g(2, 3) = -0.15;
		g(3, 0) = -0.2;  g(3, 1) = 0.08;  g(3, 2) = -0.15; g(3, 3) = 3.4;
		residual_dense = max_metric_inversion_residual(g, inverse_metric_4x4(g));
	}
	{
		Vector<double, 4> u;
		u(0) = 1.0; u(1) = 2.0; u(2) = -3.0; u(3) = 0.5;
		Vector<double, 4> v;
		v(0) = -0.5; v(1) = 1.5; v(2) = 2.0; v(3) = -1.0;
		const auto outer = tensor_product(u, v);
		product_rank = outer.rank();
		for (size_t i = 0; i < 4; ++i) {
			for (size_t j = 0; j < 4; ++j) {
				product_error = std::max(product_error, std::abs(outer(i, j) - u(i) * v(j)));
			}
		}
		const auto contracted = contract<0, 1>(outer);
		contraction_rank = contracted.rank();
		double dot = 0.0;
		for (size_t i = 0; i < 4; ++i) {
			dot += u(i) * v(i);
		}
		contraction_error = std::abs(contracted[0] - dot);
	}
	{
		Tensor<double, 3, 4> connection;
		connection.fill(0.0);
		connection(1, 0, 0) = 0.25;
		connection(1, 1, 1) = -0.25;
		connection(1, 2, 2) = -3.0;
		connection(1, 3, 3) = -2.5;
		const auto contracted = contract<0, 1>(connection);
		connection_rank = contracted.rank();
		connection_trace_error = std::max(std::abs(contracted(0)), std::abs(contracted(1) - (-0.25)));
	}
	{
		Matrix<double, 4> m;
		m(0, 1) = 2.0; m(1, 0) = 4.0;
		m(2, 3) = -1.0; m(3, 2) = 5.0;
		const auto s = symmetrize(m);
		const auto a = antisymmetrize(m);
		symmetric_error = std::max({std::abs(s(0, 1) - 3.0), std::abs(s(1, 0) - 3.0), std::abs(s(2, 3) - 2.0), std::abs(s(3, 2) - 2.0)});
		antisymmetric_error = std::max({std::abs(a(0, 1) + 1.0), std::abs(a(1, 0) - 1.0), std::abs(a(2, 3) + 3.0), std::abs(a(3, 2) - 3.0)});
	}
	g_trap_dynamic_allocations.store(false, std::memory_order_seq_cst);
	const size_t allocations = g_dynamic_allocations_count.load(std::memory_order_relaxed);

	suite.section("Metric inversion");
	suite.check_at_most(residual_minkowski, 1e-15, "the Minkowski inverse metric times the metric equals the identity");
	suite.check_at_most(residual_schwarzschild, 1e-15, "the Schwarzschild inverse metric times the metric equals the identity");
	suite.check_at_most(residual_kerr, 1e-15, "the Kerr inverse metric times the metric equals the identity");
	suite.check_at_most(residual_dense, 1e-15, "the dense indefinite inverse metric times the metric equals the identity");

	suite.section("Products and contractions");
	suite.check(product_rank == 2, "the outer product of two vectors has rank 2");
	suite.check_at_most(product_error, 1e-16, "every outer product component equals the product of the vector components");
	suite.check(contraction_rank == 0, "contracting both indices of a rank-two tensor yields a scalar");
	suite.check_at_most(contraction_error, 1e-16, "the full contraction of an outer product equals the dot product");
	suite.check(connection_rank == 1, "contracting two indices of a rank-three tensor yields a vector");
	suite.check_at_most(connection_trace_error, 1e-16, "the partial trace of the connection-like tensor matches the hand computed vector");

	suite.section("Symmetrization");
	suite.check_at_most(symmetric_error, 1e-16, "the symmetric part equals the mean of the matrix and its transpose");
	suite.check_at_most(antisymmetric_error, 1e-16, "the antisymmetric part equals half the difference of the matrix and its transpose");

	suite.section("Allocation discipline");
	suite.check(allocations == 0, "no dynamic allocation occurs during tensor algebra", "allocations=" + std::to_string(allocations));
	return suite.finish();
}
