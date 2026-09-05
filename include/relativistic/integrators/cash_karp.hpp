#pragma once

#include "relativistic/core/tensor.hpp"
#include "relativistic/core/tensor_ops.hpp"
#include "relativistic/core/christoffel.hpp"
#include "relativistic/metrics/spacetime_concept.hpp"
#include "relativistic/integrators/geodesic_state.hpp"
#include "relativistic/integrators/rk45_adaptive.hpp"
#include <array>
#include <cmath>
#include <algorithm>
#include <optional>
#include <cstdint>

namespace Relativistic::Integrators {

template <typename Scalar = double>
struct CashKarpTableau {
	static constexpr size_t STAGES = 6;
	static constexpr size_t ORDER = 5;
	static constexpr size_t EMBEDDED_ORDER = 4;

	static constexpr std::array<Scalar, 6> C = {
		static_cast<Scalar>(0),
		static_cast<Scalar>(1.0 / 5.0),
		static_cast<Scalar>(3.0 / 10.0),
		static_cast<Scalar>(3.0 / 5.0),
		static_cast<Scalar>(1.0),
		static_cast<Scalar>(7.0 / 8.0)
	};

	static constexpr std::array<Scalar, 6> B5 = {
		static_cast<Scalar>(37.0 / 378.0),
		static_cast<Scalar>(0),
		static_cast<Scalar>(250.0 / 621.0),
		static_cast<Scalar>(125.0 / 594.0),
		static_cast<Scalar>(0),
		static_cast<Scalar>(512.0 / 1771.0)
	};

	static constexpr std::array<Scalar, 6> B4 = {
		static_cast<Scalar>(2825.0 / 27648.0),
		static_cast<Scalar>(0),
		static_cast<Scalar>(18575.0 / 48384.0),
		static_cast<Scalar>(13525.0 / 55296.0),
		static_cast<Scalar>(277.0 / 14336.0),
		static_cast<Scalar>(1.0 / 4.0)
	};

	static constexpr std::array<Scalar, 6> E = {
		static_cast<Scalar>(37.0 / 378.0) - static_cast<Scalar>(2825.0 / 27648.0),
		static_cast<Scalar>(0),
		static_cast<Scalar>(250.0 / 621.0) - static_cast<Scalar>(18575.0 / 48384.0),
		static_cast<Scalar>(125.0 / 594.0) - static_cast<Scalar>(13525.0 / 55296.0),
		static_cast<Scalar>(-277.0 / 14336.0),
		static_cast<Scalar>(512.0 / 1771.0) - static_cast<Scalar>(1.0 / 4.0)
	};

	static constexpr Scalar A21 = static_cast<Scalar>(1.0 / 5.0);
	static constexpr Scalar A31 = static_cast<Scalar>(3.0 / 40.0), A32 = static_cast<Scalar>(9.0 / 40.0);
	static constexpr Scalar A41 = static_cast<Scalar>(3.0 / 10.0), A42 = static_cast<Scalar>(-9.0 / 10.0), A43 = static_cast<Scalar>(6.0 / 5.0);
	static constexpr Scalar A51 = static_cast<Scalar>(-11.0 / 54.0), A52 = static_cast<Scalar>(5.0 / 2.0), A53 = static_cast<Scalar>(-70.0 / 27.0), A54 = static_cast<Scalar>(35.0 / 27.0);
	static constexpr Scalar A61 = static_cast<Scalar>(1631.0 / 55296.0), A62 = static_cast<Scalar>(175.0 / 512.0), A63 = static_cast<Scalar>(575.0 / 13824.0), A64 = static_cast<Scalar>(44275.0 / 110592.0), A65 = static_cast<Scalar>(253.0 / 4096.0);
};

template <typename MetricType, typename Scalar = double>
	requires Metrics::SpacetimeMetric<MetricType, Scalar>
class CashKarpIntegrator {
private:
	using Tableau = CashKarpTableau<Scalar>;

	const MetricType& metric_;
	RK45Config<Scalar> config_;
	GeodesicType type_;
	mutable RK45Stats<Scalar> stats_;
	mutable AdaptiveStepController<Scalar> step_controller_;

	struct Derivatives {
		Core::FourVector<Scalar> dx;
		Core::FourVector<Scalar> du;
	};

	[[nodiscard]] Derivatives compute_derivatives(const GeodesicState<Scalar>& state) const noexcept {
		Derivatives d;
		d.dx = state.u;

		const auto gamma = Core::compute_christoffel<Core::DerivativeOrder::EighthOrder, MetricType, Scalar>(metric_, state.x);
		d.du.zero();

		for (size_t mu = 0; mu < 4; ++mu) {
			Scalar sum = static_cast<Scalar>(0);
			for (size_t alpha = 0; alpha < 4; ++alpha) {
				const Scalar u_alpha = state.u(alpha);
				if (u_alpha == static_cast<Scalar>(0)) {
					continue;
				}
				sum -= gamma(mu, alpha, alpha) * u_alpha * u_alpha;
				for (size_t beta = alpha + 1; beta < 4; ++beta) {
					const Scalar u_beta = state.u(beta);
					if (u_beta != static_cast<Scalar>(0)) {
						sum -= static_cast<Scalar>(2) * gamma(mu, alpha, beta) * u_alpha * u_beta;
					}
				}
			}
			d.du(mu) = sum;
		}
		++stats_.evaluations;
		return d;
	}

public:
	explicit constexpr CashKarpIntegrator(
		const MetricType& metric,
		GeodesicType type = GeodesicType::Timelike,
		const RK45Config<Scalar>& config = {}
	) noexcept
		: metric_(metric), config_(config), type_(type), stats_{}, step_controller_(config.step_controller_mode) {}

	[[nodiscard]] constexpr const RK45Stats<Scalar>& statistics() const noexcept {
		return stats_;
	}

	constexpr void reset_statistics() noexcept {
		stats_ = RK45Stats<Scalar>{};
		step_controller_.reset_history();
	}

	[[nodiscard]] std::optional<Scalar> step(GeodesicState<Scalar>& state, Scalar& current_dt) const noexcept {
		for (;;) {
			if (std::abs(current_dt) < config_.min_step) {
				return std::nullopt;
			}

			const auto k1 = compute_derivatives(state);

			GeodesicState<Scalar> s2 = state;
			for (size_t i = 0; i < 4; ++i) {
				s2.x(i) += current_dt * (Tableau::A21 * k1.dx(i));
				s2.u(i) += current_dt * (Tableau::A21 * k1.du(i));
			}
			const auto k2 = compute_derivatives(s2);

			GeodesicState<Scalar> s3 = state;
			for (size_t i = 0; i < 4; ++i) {
				s3.x(i) += current_dt * (Tableau::A31 * k1.dx(i) + Tableau::A32 * k2.dx(i));
				s3.u(i) += current_dt * (Tableau::A31 * k1.du(i) + Tableau::A32 * k2.du(i));
			}
			const auto k3 = compute_derivatives(s3);

			GeodesicState<Scalar> s4 = state;
			for (size_t i = 0; i < 4; ++i) {
				s4.x(i) += current_dt * (Tableau::A41 * k1.dx(i) + Tableau::A42 * k2.dx(i) + Tableau::A43 * k3.dx(i));
				s4.u(i) += current_dt * (Tableau::A41 * k1.du(i) + Tableau::A42 * k2.du(i) + Tableau::A43 * k3.du(i));
			}
			const auto k4 = compute_derivatives(s4);

			GeodesicState<Scalar> s5 = state;
			for (size_t i = 0; i < 4; ++i) {
				s5.x(i) += current_dt * (Tableau::A51 * k1.dx(i) + Tableau::A52 * k2.dx(i) + Tableau::A53 * k3.dx(i) + Tableau::A54 * k4.dx(i));
				s5.u(i) += current_dt * (Tableau::A51 * k1.du(i) + Tableau::A52 * k2.du(i) + Tableau::A53 * k3.du(i) + Tableau::A54 * k4.du(i));
			}
			const auto k5 = compute_derivatives(s5);

			GeodesicState<Scalar> s6 = state;
			for (size_t i = 0; i < 4; ++i) {
				s6.x(i) += current_dt * (Tableau::A61 * k1.dx(i) + Tableau::A62 * k2.dx(i) + Tableau::A63 * k3.dx(i) + Tableau::A64 * k4.dx(i) + Tableau::A65 * k5.dx(i));
				s6.u(i) += current_dt * (Tableau::A61 * k1.du(i) + Tableau::A62 * k2.du(i) + Tableau::A63 * k3.du(i) + Tableau::A64 * k4.du(i) + Tableau::A65 * k5.du(i));
			}
			const auto k6 = compute_derivatives(s6);

			GeodesicState<Scalar> s_next = state;
			for (size_t i = 0; i < 4; ++i) {
				s_next.x(i) += current_dt * (Tableau::B5[0] * k1.dx(i) + Tableau::B5[2] * k3.dx(i) + Tableau::B5[3] * k4.dx(i) + Tableau::B5[5] * k6.dx(i));
				s_next.u(i) += current_dt * (Tableau::B5[0] * k1.du(i) + Tableau::B5[2] * k3.du(i) + Tableau::B5[3] * k4.du(i) + Tableau::B5[5] * k6.du(i));
			}

			Scalar max_error = static_cast<Scalar>(0);
			for (size_t i = 0; i < 4; ++i) {
				const Scalar err_x = current_dt * std::abs(Tableau::E[0] * k1.dx(i) + Tableau::E[2] * k3.dx(i) + Tableau::E[3] * k4.dx(i) + Tableau::E[4] * k5.dx(i) + Tableau::E[5] * k6.dx(i));
				const Scalar err_u = current_dt * std::abs(Tableau::E[0] * k1.du(i) + Tableau::E[2] * k3.du(i) + Tableau::E[3] * k4.du(i) + Tableau::E[4] * k5.du(i) + Tableau::E[5] * k6.du(i));
				const Scalar scale_x = config_.rtol * std::max(std::abs(state.x(i)), std::abs(s_next.x(i))) + config_.atol;
				const Scalar scale_u = config_.rtol * std::max(std::abs(state.u(i)), std::abs(s_next.u(i))) + config_.atol;
				max_error = std::max({max_error, err_x / scale_x, err_u / scale_u});
			}

			if (max_error <= static_cast<Scalar>(1.0)) {
				state = s_next;
				++stats_.accepted_steps;

				const Scalar dt_actual = current_dt;
				const Scalar factor = (max_error == static_cast<Scalar>(0)) ? static_cast<Scalar>(5.0) : step_controller_.next_factor(max_error, static_cast<Scalar>(5.0));
				const Scalar scale = config_.safety_factor * factor;
				const Scalar sign = (current_dt < static_cast<Scalar>(0)) ? static_cast<Scalar>(-1) : static_cast<Scalar>(1);
				Scalar abs_dt = std::abs(current_dt);
				abs_dt *= std::clamp(scale, static_cast<Scalar>(0.2), static_cast<Scalar>(5.0));
				abs_dt = std::clamp(abs_dt, config_.min_step, config_.max_step);
				current_dt = sign * abs_dt;

				return dt_actual;
			} else {
				++stats_.rejected_steps;
				const Scalar factor = step_controller_.next_factor(max_error, static_cast<Scalar>(4.0));
				const Scalar scale = config_.safety_factor * factor;
				const Scalar sign = (current_dt < static_cast<Scalar>(0)) ? static_cast<Scalar>(-1) : static_cast<Scalar>(1);
				Scalar abs_dt = std::abs(current_dt);
				abs_dt *= std::clamp(scale, static_cast<Scalar>(0.1), static_cast<Scalar>(0.9));
				current_dt = sign * abs_dt;
			}
		}
	}
};

}
