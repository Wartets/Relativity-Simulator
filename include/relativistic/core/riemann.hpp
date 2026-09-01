#pragma once

#include "relativistic/core/tensor.hpp"
#include "relativistic/core/christoffel.hpp"
#include "relativistic/metrics/spacetime_concept.hpp"
#include <array>

namespace Relativistic::Core {

template <DerivativeOrder Order = DerivativeOrder::EighthOrder, typename MetricType = void, typename Scalar = double>
class RiemannComputer {
public:
	[[nodiscard]] static RiemannTensor<Scalar> compute_riemann(
		const MetricType& metric,
		const FourVector<Scalar>& x
	) noexcept {
		using Coeffs = FiniteDifferenceCoefficients<Order, Scalar>;
		RiemannTensor<Scalar> R;
		R.zero();

		const auto gamma = compute_christoffel<Order, MetricType, Scalar>(metric, x);

		std::array<ChristoffelSymbols<Scalar>, 4> dgamma;
		for (size_t alpha = 0; alpha < 4; ++alpha) {
			const Scalar h = compute_adaptive_step_size<Order, Scalar>(x(alpha));
			const Scalar inv_denom_h = static_cast<Scalar>(1.0) / (Coeffs::DENOMINATOR * h);

			dgamma[alpha].zero();
			for (size_t k = 1; k <= Coeffs::STENCIL_RADIUS; ++k) {
				const Scalar offset = static_cast<Scalar>(k) * h;
				const Scalar weight = Coeffs::WEIGHTS[k - 1];

				FourVector<Scalar> x_plus = x;
				x_plus(alpha) += offset;
				const auto gamma_plus = compute_christoffel<Order, MetricType, Scalar>(metric, x_plus);

				FourVector<Scalar> x_minus = x;
				x_minus(alpha) -= offset;
				const auto gamma_minus = compute_christoffel<Order, MetricType, Scalar>(metric, x_minus);

				for (size_t rho = 0; rho < 4; ++rho) {
					for (size_t mu = 0; mu < 4; ++mu) {
						for (size_t nu = 0; nu < 4; ++nu) {
							dgamma[alpha](rho, mu, nu) += weight * (gamma_plus(rho, mu, nu) - gamma_minus(rho, mu, nu));
						}
					}
				}
			}
			for (size_t rho = 0; rho < 4; ++rho) {
				for (size_t mu = 0; mu < 4; ++mu) {
					for (size_t nu = 0; nu < 4; ++nu) {
						dgamma[alpha](rho, mu, nu) *= inv_denom_h;
					}
				}
			}
		}

		for (size_t rho = 0; rho < 4; ++rho) {
			for (size_t sigma = 0; sigma < 4; ++sigma) {
				for (size_t mu = 0; mu < 4; ++mu) {
					for (size_t nu = 0; nu < 4; ++nu) {
						Scalar term1 = dgamma[mu](rho, nu, sigma) - dgamma[nu](rho, mu, sigma);
						Scalar term2 = static_cast<Scalar>(0.0);
						for (size_t lambda = 0; lambda < 4; ++lambda) {
							term2 += gamma(rho, mu, lambda) * gamma(lambda, nu, sigma) - gamma(rho, nu, lambda) * gamma(lambda, mu, sigma);
						}
						R(rho, sigma, mu, nu) = term1 + term2;
					}
				}
			}
		}
		return R;
	}

	[[nodiscard]] static MetricTensor<Scalar> compute_ricci_tensor(
		const RiemannTensor<Scalar>& R
	) noexcept {
		MetricTensor<Scalar> ricci;
		ricci.zero();
		for (size_t mu = 0; mu < 4; ++mu) {
			for (size_t nu = 0; nu < 4; ++nu) {
				Scalar sum = static_cast<Scalar>(0.0);
				for (size_t rho = 0; rho < 4; ++rho) {
					sum += R(rho, mu, rho, nu);
				}
				ricci(mu, nu) = sum;
			}
		}
		return ricci;
	}

	[[nodiscard]] static Scalar compute_ricci_scalar(
		const MetricTensor<Scalar>& ricci,
		const MetricTensor<Scalar>& inv_g
	) noexcept {
		Scalar R_scalar = static_cast<Scalar>(0.0);
		for (size_t mu = 0; mu < 4; ++mu) {
			for (size_t nu = 0; nu < 4; ++nu) {
				R_scalar += inv_g(mu, nu) * ricci(mu, nu);
			}
		}
		return R_scalar;
	}

	[[nodiscard]] static Scalar compute_kretschmann_invariant(
		const RiemannTensor<Scalar>& R,
		const MetricTensor<Scalar>& g,
		const MetricTensor<Scalar>& inv_g
	) noexcept {
		Tensor<Scalar, 4, 4> R_lower;
		R_lower.zero();
		for (size_t a = 0; a < 4; ++a) {
			for (size_t b = 0; b < 4; ++b) {
				for (size_t c = 0; c < 4; ++c) {
					for (size_t d = 0; d < 4; ++d) {
						Scalar sum = static_cast<Scalar>(0.0);
						for (size_t e = 0; e < 4; ++e) {
							const Scalar g_ae = g(a, e);
							if (g_ae != static_cast<Scalar>(0.0)) {
								sum += g_ae * R(e, b, c, d);
							}
						}
						R_lower(a, b, c, d) = sum;
					}
				}
			}
		}

		Scalar K1 = static_cast<Scalar>(0.0);
		for (size_t a = 0; a < 4; ++a) {
			for (size_t b = 0; b < 4; ++b) {
				for (size_t c = 0; c < 4; ++c) {
					for (size_t d = 0; d < 4; ++d) {
						const Scalar R_low = R_lower(a, b, c, d);
						if (R_low == static_cast<Scalar>(0.0)) continue;

						for (size_t e = 0; e < 4; ++e) {
							const Scalar ig_ae = inv_g(a, e);
							if (ig_ae == static_cast<Scalar>(0.0)) continue;
							for (size_t f = 0; f < 4; ++f) {
								const Scalar ig_bf = inv_g(b, f);
								if (ig_bf == static_cast<Scalar>(0.0)) continue;
								for (size_t g_idx = 0; g_idx < 4; ++g_idx) {
									const Scalar ig_cg = inv_g(c, g_idx);
									if (ig_cg == static_cast<Scalar>(0.0)) continue;
									for (size_t h = 0; h < 4; ++h) {
										const Scalar ig_dh = inv_g(d, h);
										if (ig_dh != static_cast<Scalar>(0.0)) {
											K1 += R_low * (ig_ae * ig_bf * ig_cg * ig_dh * R_lower(e, f, g_idx, h));
										}
									}
								}
							}
						}
					}
				}
			}
		}
		return K1;
	}
};

}
