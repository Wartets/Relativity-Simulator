#pragma once

#include "relativistic/core/tensor.hpp"
#include "relativistic/core/christoffel.hpp"
#include "relativistic/core/pcg64.hpp"
#include "relativistic/metrics/spacetime_concept.hpp"
#include "relativistic/uncertainty/covariance.hpp"
#include <array>
#include <vector>
#include <span>
#include <cmath>
#include <algorithm>
#include <concepts>
#include <thread>
#include <cstdint>

namespace Relativistic::Uncertainty {

template <typename Scalar = double>
struct alignas(64) PhaseState8D {
	Core::FourVector<Scalar> x{};
	Core::FourVector<Scalar> p{};

	constexpr PhaseState8D() noexcept = default;

	constexpr PhaseState8D(const Core::FourVector<Scalar>& pos, const Core::FourVector<Scalar>& mom) noexcept
		: x(pos), p(mom) {}

	constexpr PhaseState8D(Scalar x0, Scalar x1, Scalar x2, Scalar x3,
	                       Scalar p0, Scalar p1, Scalar p2, Scalar p3) noexcept
		: x(x0, x1, x2, x3), p(p0, p1, p2, p3) {}

	[[nodiscard]] constexpr std::array<Scalar, 8> to_array() const noexcept {
		return {x(0), x(1), x(2), x(3), p(0), p(1), p(2), p(3)};
	}

	[[nodiscard]] static constexpr PhaseState8D from_array(const std::array<Scalar, 8>& arr) noexcept {
		return PhaseState8D(arr[0], arr[1], arr[2], arr[3], arr[4], arr[5], arr[6], arr[7]);
	}

	[[nodiscard]] constexpr Scalar operator[](size_t idx) const noexcept {
		return (idx < 4) ? x(idx) : p(idx - 4);
	}

	[[nodiscard]] constexpr Scalar& operator[](size_t idx) noexcept {
		return (idx < 4) ? x(idx) : p(idx - 4);
	}
};

template <typename Scalar = double>
struct alignas(64) VariationalDerivatives8D {
	Core::FourVector<Scalar> dx{};
	Core::FourVector<Scalar> dp{};
};

template <typename MetricType, typename Scalar = double>
	requires Metrics::SpacetimeMetric<MetricType, Scalar>
class GeodesicJacobianComputer {
public:
	using Matrix8x8 = std::array<std::array<Scalar, 8>, 8>;
	using GammaDerivs = std::array<std::array<std::array<std::array<Scalar, 4>, 4>, 4>, 4>;

	[[nodiscard]] static GammaDerivs compute_christoffel_derivatives_8th(
		const MetricType& metric,
		const Core::FourVector<Scalar>& x
	) noexcept {
		using Coeffs = Core::FiniteDifferenceCoefficients<Core::DerivativeOrder::EighthOrder, Scalar>;
		GammaDerivs dgamma{};

		for (size_t nu = 0; nu < 4; ++nu) {
			const Scalar h = Core::compute_adaptive_step_size<Core::DerivativeOrder::EighthOrder, Scalar>(x(nu));
			const Scalar inv_denom_h = static_cast<Scalar>(1.0) / (Coeffs::DENOMINATOR * h);

			for (size_t k = 1; k <= Coeffs::STENCIL_RADIUS; ++k) {
				const Scalar offset = static_cast<Scalar>(k) * h;
				const Scalar weight = Coeffs::WEIGHTS[k - 1];

				Core::FourVector<Scalar> x_plus = x;
				x_plus(nu) += offset;
				const auto gamma_plus = Core::compute_christoffel<Core::DerivativeOrder::EighthOrder, MetricType, Scalar>(metric, x_plus);

				Core::FourVector<Scalar> x_minus = x;
				x_minus(nu) -= offset;
				const auto gamma_minus = Core::compute_christoffel<Core::DerivativeOrder::EighthOrder, MetricType, Scalar>(metric, x_minus);

				for (size_t mu = 0; mu < 4; ++mu) {
					for (size_t alpha = 0; alpha < 4; ++alpha) {
						for (size_t beta = 0; beta < 4; ++beta) {
							dgamma[nu][mu][alpha][beta] += weight * (gamma_plus(mu, alpha, beta) - gamma_minus(mu, alpha, beta));
						}
					}
				}
			}

			for (size_t mu = 0; mu < 4; ++mu) {
				for (size_t alpha = 0; alpha < 4; ++alpha) {
					for (size_t beta = 0; beta < 4; ++beta) {
						dgamma[nu][mu][alpha][beta] *= inv_denom_h;
					}
				}
			}
		}

		return dgamma;
	}

	[[nodiscard]] static Matrix8x8 compute_jacobian(
		const MetricType& metric,
		const Core::FourVector<Scalar>& x,
		const Core::FourVector<Scalar>& p
	) noexcept {
		Matrix8x8 J{};
		for (size_t i = 0; i < 8; ++i) {
			for (size_t j = 0; j < 8; ++j) {
				J[i][j] = static_cast<Scalar>(0.0);
			}
		}

		for (size_t mu = 0; mu < 4; ++mu) {
			J[mu][4 + mu] = static_cast<Scalar>(1.0);
		}

		const auto gamma = Core::compute_christoffel<Core::DerivativeOrder::EighthOrder, MetricType, Scalar>(metric, x);
		const auto dgamma = compute_christoffel_derivatives_8th(metric, x);

		for (size_t mu = 0; mu < 4; ++mu) {
			for (size_t nu = 0; nu < 4; ++nu) {
				Scalar sum_dgamma = static_cast<Scalar>(0.0);
				for (size_t alpha = 0; alpha < 4; ++alpha) {
					const Scalar p_a = p(alpha);
					if (p_a == static_cast<Scalar>(0.0)) continue;
					sum_dgamma += dgamma[nu][mu][alpha][alpha] * p_a * p_a;
					for (size_t beta = alpha + 1; beta < 4; ++beta) {
						const Scalar p_b = p(beta);
						if (p_b != static_cast<Scalar>(0.0)) {
							sum_dgamma += static_cast<Scalar>(2.0) * dgamma[nu][mu][alpha][beta] * p_a * p_b;
						}
					}
				}
				J[4 + mu][nu] = -sum_dgamma;

				Scalar sum_gamma_p = static_cast<Scalar>(0.0);
				for (size_t beta = 0; beta < 4; ++beta) {
					sum_gamma_p += gamma(mu, nu, beta) * p(beta);
				}
				J[4 + mu][4 + nu] = -static_cast<Scalar>(2.0) * sum_gamma_p;
			}
		}

		return J;
	}
};

template <typename Scalar = double>
struct alignas(64) VariationalGeodesicState {
	PhaseState8D<Scalar> phase{};
	CovarianceMatrix<Scalar, 8> covariance{};
	std::array<std::array<Scalar, 8>, 8> transition_matrix{};
	Scalar affine_parameter{static_cast<Scalar>(0.0)};

	constexpr VariationalGeodesicState() noexcept {
		for (size_t i = 0; i < 8; ++i) {
			for (size_t j = 0; j < 8; ++j) {
				transition_matrix[i][j] = (i == j) ? static_cast<Scalar>(1.0) : static_cast<Scalar>(0.0);
			}
		}
	}

	constexpr VariationalGeodesicState(
		const PhaseState8D<Scalar>& state,
		const CovarianceMatrix<Scalar, 8>& cov,
		Scalar lambda = static_cast<Scalar>(0.0)
	) noexcept
		: phase(state), covariance(cov), affine_parameter(lambda) {
		for (size_t i = 0; i < 8; ++i) {
			for (size_t j = 0; j < 8; ++j) {
				transition_matrix[i][j] = (i == j) ? static_cast<Scalar>(1.0) : static_cast<Scalar>(0.0);
			}
		}
	}
};

template <typename Scalar = double>
struct VariationalGeodesicConfig {
	Scalar initial_step{static_cast<Scalar>(0.01)};
	Scalar min_step{static_cast<Scalar>(1e-12)};
	Scalar max_step{static_cast<Scalar>(100.0)};
	Scalar rtol{static_cast<Scalar>(1e-10)};
	Scalar atol{static_cast<Scalar>(1e-14)};
	Scalar safety_factor{static_cast<Scalar>(0.9)};
};

template <typename Scalar = double>
struct VariationalGeodesicStats {
	uint64_t steps_accepted{0};
	uint64_t steps_rejected{0};
	uint64_t evaluations{0};
	Scalar final_determinant_transition_matrix{static_cast<Scalar>(1.0)};
};

template <size_t N, typename Scalar = double>
[[nodiscard]] constexpr Scalar matrix_determinant_gaussian(std::array<std::array<Scalar, N>, N> mat) noexcept {
	Scalar det = static_cast<Scalar>(1.0);
	for (size_t i = 0; i < N; ++i) {
		size_t pivot = i;
		Scalar max_val = std::abs(mat[i][i]);
		for (size_t r = i + 1; r < N; ++r) {
			const Scalar val = std::abs(mat[r][i]);
			if (val > max_val) {
				max_val = val;
				pivot = r;
			}
		}
		if (max_val < static_cast<Scalar>(1e-20)) {
			return static_cast<Scalar>(0.0);
		}
		if (pivot != i) {
			std::swap(mat[i], mat[pivot]);
			det = -det;
		}
		det *= mat[i][i];
		const Scalar inv_pivot = static_cast<Scalar>(1.0) / mat[i][i];
		for (size_t r = i + 1; r < N; ++r) {
			const Scalar factor = mat[r][i] * inv_pivot;
			for (size_t c = i + 1; c < N; ++c) {
				mat[r][c] -= factor * mat[i][c];
			}
		}
	}
	return det;
}

template <typename MetricType, typename Scalar = double>
	requires Metrics::SpacetimeMetric<MetricType, Scalar>
class VariationalGeodesicIntegrator {
private:
	const MetricType& metric_;
	VariationalGeodesicConfig<Scalar> config_{};
	mutable VariationalGeodesicStats<Scalar> stats_{};

	[[nodiscard]] VariationalDerivatives8D<Scalar> compute_phase_derivatives(
		const PhaseState8D<Scalar>& state
	) const noexcept {
		VariationalDerivatives8D<Scalar> d;
		d.dx = state.p;

		const auto gamma = Core::compute_christoffel<Core::DerivativeOrder::EighthOrder, MetricType, Scalar>(metric_, state.x);
		d.dp.zero();

		for (size_t mu = 0; mu < 4; ++mu) {
			Scalar sum = static_cast<Scalar>(0.0);
			for (size_t alpha = 0; alpha < 4; ++alpha) {
				const Scalar p_a = state.p(alpha);
				if (p_a == static_cast<Scalar>(0.0)) continue;
				sum -= gamma(mu, alpha, alpha) * p_a * p_a;
				for (size_t beta = alpha + 1; beta < 4; ++beta) {
					const Scalar p_b = state.p(beta);
					if (p_b != static_cast<Scalar>(0.0)) {
						sum -= static_cast<Scalar>(2.0) * gamma(mu, alpha, beta) * p_a * p_b;
					}
				}
			}
			d.dp(mu) = sum;
		}

		++stats_.evaluations;
		return d;
	}

public:
	explicit constexpr VariationalGeodesicIntegrator(
		const MetricType& metric,
		const VariationalGeodesicConfig<Scalar>& config = {}
	) noexcept
		: metric_(metric), config_(config), stats_{} {}

	[[nodiscard]] constexpr const VariationalGeodesicStats<Scalar>& statistics() const noexcept {
		return stats_;
	}

	constexpr void reset_statistics() noexcept {
		stats_ = VariationalGeodesicStats<Scalar>{};
	}

	void step_rk4(
		VariationalGeodesicState<Scalar>& state,
		Scalar d_lambda,
		const CovarianceMatrix<Scalar, 8>& process_noise_q = {}
	) const noexcept {
		using JacobianComp = GeodesicJacobianComputer<MetricType, Scalar>;

		const auto k1_y = compute_phase_derivatives(state.phase);
		const auto j1 = JacobianComp::compute_jacobian(metric_, state.phase.x, state.phase.p);
		const auto k1_cov = state.covariance.lyapunov_derivative(j1, process_noise_q);

		std::array<std::array<Scalar, 8>, 8> k1_phi{};
		for (size_t i = 0; i < 8; ++i) {
			for (size_t j = 0; j < 8; ++j) {
				Scalar sum = static_cast<Scalar>(0.0);
				for (size_t k = 0; k < 8; ++k) {
					sum += j1[i][k] * state.transition_matrix[k][j];
				}
				k1_phi[i][j] = sum;
			}
		}

		PhaseState8D<Scalar> s2_phase;
		CovarianceMatrix<Scalar, 8> s2_cov;
		std::array<std::array<Scalar, 8>, 8> s2_phi{};
		const Scalar half_h = d_lambda * static_cast<Scalar>(0.5);

		for (size_t i = 0; i < 4; ++i) {
			s2_phase.x(i) = state.phase.x(i) + half_h * k1_y.dx(i);
			s2_phase.p(i) = state.phase.p(i) + half_h * k1_y.dp(i);
		}
		for (size_t i = 0; i < 8; ++i) {
			for (size_t j = 0; j < 8; ++j) {
				s2_cov(i, j) = state.covariance(i, j) + half_h * k1_cov(i, j);
				s2_phi[i][j] = state.transition_matrix[i][j] + half_h * k1_phi[i][j];
			}
		}
		s2_cov.symmetrize();

		const auto k2_y = compute_phase_derivatives(s2_phase);
		const auto j2 = JacobianComp::compute_jacobian(metric_, s2_phase.x, s2_phase.p);
		const auto k2_cov = s2_cov.lyapunov_derivative(j2, process_noise_q);

		std::array<std::array<Scalar, 8>, 8> k2_phi{};
		for (size_t i = 0; i < 8; ++i) {
			for (size_t j = 0; j < 8; ++j) {
				Scalar sum = static_cast<Scalar>(0.0);
				for (size_t k = 0; k < 8; ++k) {
					sum += j2[i][k] * s2_phi[k][j];
				}
				k2_phi[i][j] = sum;
			}
		}

		PhaseState8D<Scalar> s3_phase;
		CovarianceMatrix<Scalar, 8> s3_cov;
		std::array<std::array<Scalar, 8>, 8> s3_phi{};

		for (size_t i = 0; i < 4; ++i) {
			s3_phase.x(i) = state.phase.x(i) + half_h * k2_y.dx(i);
			s3_phase.p(i) = state.phase.p(i) + half_h * k2_y.dp(i);
		}
		for (size_t i = 0; i < 8; ++i) {
			for (size_t j = 0; j < 8; ++j) {
				s3_cov(i, j) = state.covariance(i, j) + half_h * k2_cov(i, j);
				s3_phi[i][j] = state.transition_matrix[i][j] + half_h * k2_phi[i][j];
			}
		}
		s3_cov.symmetrize();

		const auto k3_y = compute_phase_derivatives(s3_phase);
		const auto j3 = JacobianComp::compute_jacobian(metric_, s3_phase.x, s3_phase.p);
		const auto k3_cov = s3_cov.lyapunov_derivative(j3, process_noise_q);

		std::array<std::array<Scalar, 8>, 8> k3_phi{};
		for (size_t i = 0; i < 8; ++i) {
			for (size_t j = 0; j < 8; ++j) {
				Scalar sum = static_cast<Scalar>(0.0);
				for (size_t k = 0; k < 8; ++k) {
					sum += j3[i][k] * s3_phi[k][j];
				}
				k3_phi[i][j] = sum;
			}
		}

		PhaseState8D<Scalar> s4_phase;
		CovarianceMatrix<Scalar, 8> s4_cov;
		std::array<std::array<Scalar, 8>, 8> s4_phi{};

		for (size_t i = 0; i < 4; ++i) {
			s4_phase.x(i) = state.phase.x(i) + d_lambda * k3_y.dx(i);
			s4_phase.p(i) = state.phase.p(i) + d_lambda * k3_y.dp(i);
		}
		for (size_t i = 0; i < 8; ++i) {
			for (size_t j = 0; j < 8; ++j) {
				s4_cov(i, j) = state.covariance(i, j) + d_lambda * k3_cov(i, j);
				s4_phi[i][j] = state.transition_matrix[i][j] + d_lambda * k3_phi[i][j];
			}
		}
		s4_cov.symmetrize();

		const auto k4_y = compute_phase_derivatives(s4_phase);
		const auto j4 = JacobianComp::compute_jacobian(metric_, s4_phase.x, s4_phase.p);
		const auto k4_cov = s4_cov.lyapunov_derivative(j4, process_noise_q);

		std::array<std::array<Scalar, 8>, 8> k4_phi{};
		for (size_t i = 0; i < 8; ++i) {
			for (size_t j = 0; j < 8; ++j) {
				Scalar sum = static_cast<Scalar>(0.0);
				for (size_t k = 0; k < 8; ++k) {
					sum += j4[i][k] * s4_phi[k][j];
				}
				k4_phi[i][j] = sum;
			}
		}

		const Scalar sixth_h = d_lambda * (static_cast<Scalar>(1.0) / static_cast<Scalar>(6.0));
		for (size_t i = 0; i < 4; ++i) {
			state.phase.x(i) += sixth_h * (k1_y.dx(i) + static_cast<Scalar>(2.0) * k2_y.dx(i) + static_cast<Scalar>(2.0) * k3_y.dx(i) + k4_y.dx(i));
			state.phase.p(i) += sixth_h * (k1_y.dp(i) + static_cast<Scalar>(2.0) * k2_y.dp(i) + static_cast<Scalar>(2.0) * k3_y.dp(i) + k4_y.dp(i));
		}

		for (size_t i = 0; i < 8; ++i) {
			for (size_t j = 0; j < 8; ++j) {
				state.covariance(i, j) += sixth_h * (k1_cov(i, j) + static_cast<Scalar>(2.0) * k2_cov(i, j) + static_cast<Scalar>(2.0) * k3_cov(i, j) + k4_cov(i, j));
				state.transition_matrix[i][j] += sixth_h * (k1_phi[i][j] + static_cast<Scalar>(2.0) * k2_phi[i][j] + static_cast<Scalar>(2.0) * k3_phi[i][j] + k4_phi[i][j]);
			}
		}

		state.covariance.symmetrize();
		state.affine_parameter += d_lambda;
		++stats_.steps_accepted;
	}

	void integrate(
		VariationalGeodesicState<Scalar>& state,
		Scalar target_lambda,
		Scalar step_size,
		const CovarianceMatrix<Scalar, 8>& process_noise_q = {}
	) const noexcept {
		while (state.affine_parameter < target_lambda) {
			Scalar h = step_size;
			if (state.affine_parameter + h > target_lambda) {
				h = target_lambda - state.affine_parameter;
			}
			step_rk4(state, h, process_noise_q);
		}
		stats_.final_determinant_transition_matrix = matrix_determinant_gaussian<8, Scalar>(state.transition_matrix);
	}
};

template <typename MetricType, typename Scalar = double>
	requires Metrics::SpacetimeMetric<MetricType, Scalar>
class MonteCarloCovarianceValidator {
public:
	struct AxisComparisonResult {
		std::array<Scalar, 8> analytical_semi_axes{};
		std::array<Scalar, 8> empirical_semi_axes{};
		std::array<Scalar, 8> relative_errors{};
		Scalar max_relative_error{static_cast<Scalar>(0.0)};
		Scalar mean_relative_error{static_cast<Scalar>(0.0)};
		bool passed_tolerance{true};
	};

	[[nodiscard]] static std::vector<PhaseState8D<Scalar>> generate_ensemble(
		const PhaseState8D<Scalar>& nominal_state,
		const CovarianceMatrix<Scalar, 8>& initial_cov,
		size_t sample_count,
		uint64_t seed = 0x853C49E6748FEA9BULL
	) {
		std::vector<PhaseState8D<Scalar>> ensemble;
		ensemble.resize(sample_count);

		Core::PCG64Engine rng(seed, 1ULL);
		const auto nominal_array = nominal_state.to_array();
		const auto es = initial_cov.compute_eigensystem();

		std::array<Scalar, 8> std_k{};
		for (size_t k = 0; k < 8; ++k) {
			std_k[k] = std::sqrt(std::max(es.eigenvalues[k], static_cast<Scalar>(0.0)));
		}

		const size_t pairs = sample_count / 2;
		std::vector<std::array<Scalar, 8>> z_samples(pairs);

		for (size_t p = 0; p < pairs; ++p) {
			for (size_t d = 0; d < 8; d += 2) {
				const auto [g1, g2] = rng.next_gaussian_pair();
				z_samples[p][d] = static_cast<Scalar>(g1);
				z_samples[p][d + 1] = static_cast<Scalar>(g2);
			}
		}

		std::array<std::array<Scalar, 8>, 8> z_cov{};
		for (size_t i = 0; i < 8; ++i) {
			for (size_t j = i; j < 8; ++j) {
				Scalar sum = static_cast<Scalar>(0.0);
				for (size_t p = 0; p < pairs; ++p) {
					sum += z_samples[p][i] * z_samples[p][j];
				}
				z_cov[i][j] = sum * static_cast<Scalar>(2.0);
				z_cov[j][i] = z_cov[i][j];
			}
		}

		const Scalar inv_n_minus_1 = static_cast<Scalar>(1.0) / static_cast<Scalar>(sample_count - 1);
		for (size_t i = 0; i < 8; ++i) {
			for (size_t j = 0; j < 8; ++j) {
				z_cov[i][j] *= inv_n_minus_1;
			}
		}

		CovarianceMatrix<Scalar, 8> z_cov_mat;
		for (size_t i = 0; i < 8; ++i) {
			for (size_t j = 0; j < 8; ++j) {
				z_cov_mat(i, j) = z_cov[i][j];
			}
		}
		const auto z_es = z_cov_mat.compute_eigensystem();

		std::array<std::array<Scalar, 8>, 8> whiten_matrix{};
		for (size_t i = 0; i < 8; ++i) {
			for (size_t j = 0; j < 8; ++j) {
				Scalar sum = static_cast<Scalar>(0.0);
				for (size_t k = 0; k < 8; ++k) {
					const Scalar inv_sqrt_eig = static_cast<Scalar>(1.0) / std::sqrt(std::max(z_es.eigenvalues[k], static_cast<Scalar>(1e-15)));
					sum += z_es.eigenvectors[i][k] * inv_sqrt_eig * z_es.eigenvectors[j][k];
				}
				whiten_matrix[i][j] = sum;
			}
		}

		for (size_t p = 0; p < pairs; ++p) {
			std::array<Scalar, 8> z_whitened{};
			for (size_t i = 0; i < 8; ++i) {
				for (size_t j = 0; j < 8; ++j) {
					z_whitened[i] += whiten_matrix[i][j] * z_samples[p][j];
				}
			}

			std::array<Scalar, 8> dev{};
			for (size_t k = 0; k < 8; ++k) {
				const Scalar z_scaled = z_whitened[k] * std_k[k];
				for (size_t d = 0; d < 8; ++d) {
					dev[d] += es.eigenvectors[d][k] * z_scaled;
				}
			}

			std::array<Scalar, 8> plus_arr{};
			std::array<Scalar, 8> minus_arr{};
			for (size_t d = 0; d < 8; ++d) {
				plus_arr[d] = nominal_array[d] + dev[d];
				minus_arr[d] = nominal_array[d] - dev[d];
			}

			ensemble[2 * p] = PhaseState8D<Scalar>::from_array(plus_arr);
			ensemble[2 * p + 1] = PhaseState8D<Scalar>::from_array(minus_arr);
		}

		if (sample_count % 2 == 1) {
			ensemble[sample_count - 1] = nominal_state;
		}

		return ensemble;
	}

	static void propagate_ensemble_multithreaded(
		std::span<PhaseState8D<Scalar>> ensemble,
		const MetricType& metric,
		Scalar target_lambda,
		Scalar step_size,
		size_t num_threads = 0
	) noexcept {
		const size_t total_samples = ensemble.size();
		if (total_samples == 0) return;

		const size_t threads_to_use = (num_threads > 0)
			? num_threads
			: std::max(size_t{1}, static_cast<size_t>(std::thread::hardware_concurrency()));

		const size_t chunk_size = (total_samples + threads_to_use - 1) / threads_to_use;
		std::vector<std::jthread> workers;
		workers.reserve(threads_to_use);

		auto worker_func = [&](size_t start_idx, size_t end_idx) noexcept {
			for (size_t i = start_idx; i < end_idx; ++i) {
				auto& pstate = ensemble[i];
				Scalar lambda = static_cast<Scalar>(0.0);

				while (lambda < target_lambda) {
					Scalar h = step_size;
					if (lambda + h > target_lambda) {
						h = target_lambda - lambda;
					}

					auto eval_derivs = [&](const Core::FourVector<Scalar>& rx, const Core::FourVector<Scalar>& rp) noexcept -> Core::FourVector<Scalar> {
						const auto gamma = Core::compute_christoffel<Core::DerivativeOrder::EighthOrder, MetricType, Scalar>(metric, rx);
						Core::FourVector<Scalar> dp;
						dp.zero();
						for (size_t mu = 0; mu < 4; ++mu) {
							Scalar sum = static_cast<Scalar>(0.0);
							for (size_t alpha = 0; alpha < 4; ++alpha) {
								const Scalar p_a = rp(alpha);
								if (p_a == static_cast<Scalar>(0.0)) continue;
								sum -= gamma(mu, alpha, alpha) * p_a * p_a;
								for (size_t beta = alpha + 1; beta < 4; ++beta) {
									const Scalar p_b = rp(beta);
									if (p_b != static_cast<Scalar>(0.0)) {
										sum -= static_cast<Scalar>(2.0) * gamma(mu, alpha, beta) * p_a * p_b;
									}
								}
							}
							dp(mu) = sum;
						}
						return dp;
					};

					const auto k1_dx = pstate.p;
					const auto k1_dp = eval_derivs(pstate.x, pstate.p);

					Core::FourVector<Scalar> s2_x, s2_p;
					const Scalar half_h = h * static_cast<Scalar>(0.5);
					for (size_t c = 0; c < 4; ++c) {
						s2_x(c) = pstate.x(c) + half_h * k1_dx(c);
						s2_p(c) = pstate.p(c) + half_h * k1_dp(c);
					}
					const auto k2_dx = s2_p;
					const auto k2_dp = eval_derivs(s2_x, s2_p);

					Core::FourVector<Scalar> s3_x, s3_p;
					for (size_t c = 0; c < 4; ++c) {
						s3_x(c) = pstate.x(c) + half_h * k2_dx(c);
						s3_p(c) = pstate.p(c) + half_h * k2_dp(c);
					}
					const auto k3_dx = s3_p;
					const auto k3_dp = eval_derivs(s3_x, s3_p);

					Core::FourVector<Scalar> s4_x, s4_p;
					for (size_t c = 0; c < 4; ++c) {
						s4_x(c) = pstate.x(c) + h * k3_dx(c);
						s4_p(c) = pstate.p(c) + h * k3_dp(c);
					}
					const auto k4_dx = s4_p;
					const auto k4_dp = eval_derivs(s4_x, s4_p);

					const Scalar sixth_h = h / static_cast<Scalar>(6.0);
					for (size_t c = 0; c < 4; ++c) {
						pstate.x(c) += sixth_h * (k1_dx(c) + static_cast<Scalar>(2.0) * k2_dx(c) + static_cast<Scalar>(2.0) * k3_dx(c) + k4_dx(c));
						pstate.p(c) += sixth_h * (k1_dp(c) + static_cast<Scalar>(2.0) * k2_dp(c) + static_cast<Scalar>(2.0) * k3_dp(c) + k4_dp(c));
					}

					lambda += h;
				}
			}
		};

		for (size_t t = 0; t < threads_to_use; ++t) {
			const size_t start_idx = t * chunk_size;
			const size_t end_idx = std::min(start_idx + chunk_size, total_samples);
			if (start_idx < end_idx) {
				workers.emplace_back(worker_func, start_idx, end_idx);
			}
		}
	}

	[[nodiscard]] static std::array<Scalar, 8> compute_sample_mean(
		std::span<const PhaseState8D<Scalar>> ensemble
	) noexcept {
		std::array<Scalar, 8> mean{};
		const size_t n = ensemble.size();
		if (n == 0) return mean;

		for (const auto& p : ensemble) {
			const auto arr = p.to_array();
			for (size_t c = 0; c < 8; ++c) {
				mean[c] += arr[c];
			}
		}

		const Scalar inv_n = static_cast<Scalar>(1.0) / static_cast<Scalar>(n);
		for (size_t c = 0; c < 8; ++c) {
			mean[c] *= inv_n;
		}

		return mean;
	}

	[[nodiscard]] static CovarianceMatrix<Scalar, 8> compute_sample_covariance(
		std::span<const PhaseState8D<Scalar>> ensemble
	) noexcept {
		CovarianceMatrix<Scalar, 8> cov;
		const size_t n = ensemble.size();
		if (n < 2) return cov;

		const auto mean = compute_sample_mean(ensemble);

		for (const auto& p : ensemble) {
			const auto arr = p.to_array();
			std::array<Scalar, 8> dev{};
			for (size_t c = 0; c < 8; ++c) {
				dev[c] = arr[c] - mean[c];
			}

			for (size_t i = 0; i < 8; ++i) {
				for (size_t j = i; j < 8; ++j) {
					cov(i, j) += dev[i] * dev[j];
				}
			}
		}

		const Scalar inv_n_minus_1 = static_cast<Scalar>(1.0) / static_cast<Scalar>(n - 1);
		for (size_t i = 0; i < 8; ++i) {
			for (size_t j = i; j < 8; ++j) {
				cov(i, j) *= inv_n_minus_1;
				cov(j, i) = cov(i, j);
			}
		}

		return cov;
	}

	[[nodiscard]] static AxisComparisonResult compare_semi_axes(
		const CovarianceMatrix<Scalar, 8>& analytical_cov,
		const CovarianceMatrix<Scalar, 8>& empirical_cov,
		Scalar tolerance = static_cast<Scalar>(0.001)
	) noexcept {
		AxisComparisonResult res;
		res.analytical_semi_axes = analytical_cov.principal_semi_axes();
		res.empirical_semi_axes = empirical_cov.principal_semi_axes();

		Scalar max_err = static_cast<Scalar>(0.0);
		Scalar sum_err = static_cast<Scalar>(0.0);
		size_t active_count = 0;

		for (size_t i = 0; i < 8; ++i) {
			const Scalar a = res.analytical_semi_axes[i];
			const Scalar e = res.empirical_semi_axes[i];
			const Scalar denom = std::max(a, static_cast<Scalar>(1e-12));
			const Scalar rel_err = std::abs(a - e) / denom;
			res.relative_errors[i] = rel_err;

			if (a > static_cast<Scalar>(1e-12)) {
				max_err = std::max(max_err, rel_err);
				sum_err += rel_err;
				++active_count;
			}
		}

		res.max_relative_error = max_err;
		res.mean_relative_error = (active_count > 0) ? (sum_err / static_cast<Scalar>(active_count)) : static_cast<Scalar>(0.0);
		res.passed_tolerance = (max_err <= tolerance);

		return res;
	}
};

}
