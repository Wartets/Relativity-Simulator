#pragma once

#include "relativistic/core/tensor.hpp"
#include "relativistic/core/tensor_ops.hpp"
#include "relativistic/core/christoffel.hpp"
#include "relativistic/metrics/spacetime_concept.hpp"
#include "relativistic/integrators/geodesic_state.hpp"
#include "relativistic/integrators/horizon_manager.hpp"
#include <cmath>
#include <algorithm>
#include <optional>
#include <cstdint>

namespace Relativistic::Integrators {

enum class GeodesicType : uint8_t {
	Timelike,
	Null
};

template <typename Scalar = double>
struct RK45Config {
	Scalar initial_step = static_cast<Scalar>(1e6);
	Scalar min_step = static_cast<Scalar>(1e-8);
	Scalar max_step = static_cast<Scalar>(1e13);
	Scalar rtol = static_cast<Scalar>(1e-10);
	Scalar atol = static_cast<Scalar>(1e-14);
	Scalar safety_factor = static_cast<Scalar>(0.9);
	Scalar invariant_tolerance = static_cast<Scalar>(1e-12);
	HorizonCrossingMode crossing_mode = HorizonCrossingMode::Continuity;
	Scalar singularity_threshold = static_cast<Scalar>(1e-4);
};

template <typename Scalar = double>
struct RK45Stats {
	uint64_t accepted_steps = 0;
	uint64_t rejected_steps = 0;
	uint64_t evaluations = 0;
	Scalar max_invariant_residual = static_cast<Scalar>(0);
	bool crossed_horizon = false;
	bool absorbed_at_horizon = false;
	bool reached_singularity = false;
	Scalar min_radius_reached = static_cast<Scalar>(1e30);
};

template <typename MetricType, typename Scalar = double>
	requires Metrics::SpacetimeMetric<MetricType, Scalar>
class RK45AdaptiveIntegrator {
private:
	const MetricType& metric_;
	RK45Config<Scalar> config_;
	GeodesicType type_;
	mutable RK45Stats<Scalar> stats_;

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

	void project_invariant(GeodesicState<Scalar>& state) const noexcept {
		const auto g = metric_.metric_tensor(state.x);
		Scalar norm_sq = static_cast<Scalar>(0);
		for (size_t mu = 0; mu < 4; ++mu) {
			const Scalar u_mu = state.u(mu);
			norm_sq += g(mu, mu) * u_mu * u_mu;
			for (size_t nu = mu + 1; nu < 4; ++nu) {
				norm_sq += static_cast<Scalar>(2) * g(mu, nu) * u_mu * state.u(nu);
			}
		}

		const Scalar c = metric_.speed_of_light();
		const Scalar target_norm = (type_ == GeodesicType::Timelike) ? -(c * c) : static_cast<Scalar>(0);
		const Scalar residual = norm_sq - target_norm;
		const Scalar abs_res = std::abs(residual);

		if (abs_res > stats_.max_invariant_residual) {
			stats_.max_invariant_residual = abs_res;
		}

		if (abs_res >= config_.invariant_tolerance) {
			const Scalar a_coeff = g(0, 0);
			Scalar b_coeff = static_cast<Scalar>(0);
			for (size_t i = 1; i < 4; ++i) {
				b_coeff += static_cast<Scalar>(2) * g(0, i) * state.u(i);
			}
			Scalar c_coeff = -target_norm;
			for (size_t i = 1; i < 4; ++i) {
				const Scalar u_i = state.u(i);
				c_coeff += g(i, i) * u_i * u_i;
				for (size_t j = i + 1; j < 4; ++j) {
					c_coeff += static_cast<Scalar>(2) * g(i, j) * u_i * state.u(j);
				}
			}

			if (std::abs(a_coeff) > static_cast<Scalar>(1e-14)) {
				const Scalar discr = b_coeff * b_coeff - static_cast<Scalar>(4) * a_coeff * c_coeff;
				if (discr >= static_cast<Scalar>(0)) {
					const Scalar sqrt_d = std::sqrt(discr);
					const Scalar r1 = (-b_coeff + sqrt_d) / (static_cast<Scalar>(2) * a_coeff);
					const Scalar r2 = (-b_coeff - sqrt_d) / (static_cast<Scalar>(2) * a_coeff);
					const Scalar cur_u0 = state.u(0);
					state.u(0) = (std::abs(r1 - cur_u0) <= std::abs(r2 - cur_u0)) ? r1 : r2;
				}
			} else if (std::abs(b_coeff) > static_cast<Scalar>(1e-14)) {
				state.u(0) = -c_coeff / b_coeff;
			}
		}
	}

public:
	explicit constexpr RK45AdaptiveIntegrator(
		const MetricType& metric,
		GeodesicType type,
		const RK45Config<Scalar>& config = {}
	) noexcept
		: metric_(metric), config_(config), type_(type), stats_{} {}

	[[nodiscard]] constexpr const RK45Stats<Scalar>& statistics() const noexcept {
		return stats_;
	}

	constexpr void reset_statistics() noexcept {
		stats_ = RK45Stats<Scalar>{};
	}

	[[nodiscard]] std::optional<Scalar> step(GeodesicState<Scalar>& state, Scalar& current_dt) const noexcept {
		Scalar current_r = state.x(1);
		if constexpr (requires { { metric_.coordinate_radius(state.x) } -> std::convertible_to<Scalar>; }) {
			current_r = metric_.coordinate_radius(state.x);
		}

		if constexpr (requires { { metric_.schwarzschild_radius() } -> std::convertible_to<Scalar>; }) {
			const Scalar r_s = static_cast<Scalar>(metric_.schwarzschild_radius());
			if (r_s > static_cast<Scalar>(0)) {
				if (config_.crossing_mode == HorizonCrossingMode::Absorption && current_r <= r_s) {
					stats_.absorbed_at_horizon = true;
					return std::nullopt;
				}
				const Scalar sing_r = config_.singularity_threshold * r_s;
				if (current_r <= sing_r) {
					stats_.reached_singularity = true;
					return std::nullopt;
				}
			}
		}

		bool check_radial_step = true;
		if constexpr (requires { { MetricType::is_cartesian() } -> std::convertible_to<bool>; }) {
			if constexpr (MetricType::is_cartesian()) {
				check_radial_step = false;
			}
		}

		if (check_radial_step) {
			if (state.x(1) > static_cast<Scalar>(0) && state.u(1) < static_cast<Scalar>(0)) {
				const Scalar max_radial_dt = static_cast<Scalar>(0.5) * state.x(1) / std::abs(state.u(1));
				if (std::abs(current_dt) > max_radial_dt) {
					const Scalar sign = (current_dt < static_cast<Scalar>(0)) ? static_cast<Scalar>(-1) : static_cast<Scalar>(1);
					current_dt = sign * std::max(max_radial_dt, config_.min_step);
				}
			}
		}

		constexpr Scalar a21 = static_cast<Scalar>(1.0/5.0);
		constexpr Scalar a31 = static_cast<Scalar>(3.0/40.0), a32 = static_cast<Scalar>(9.0/40.0);
		constexpr Scalar a41 = static_cast<Scalar>(44.0/45.0), a42 = static_cast<Scalar>(-56.0/15.0), a43 = static_cast<Scalar>(32.0/9.0);
		constexpr Scalar a51 = static_cast<Scalar>(19372.0/6561.0), a52 = static_cast<Scalar>(-25360.0/2187.0), a53 = static_cast<Scalar>(64448.0/6561.0), a54 = static_cast<Scalar>(-212.0/729.0);
		constexpr Scalar a61 = static_cast<Scalar>(9017.0/3168.0), a62 = static_cast<Scalar>(-355.0/33.0), a63 = static_cast<Scalar>(46732.0/5247.0), a64 = static_cast<Scalar>(49.0/176.0), a65 = static_cast<Scalar>(-5103.0/18656.0);
		constexpr Scalar a71 = static_cast<Scalar>(35.0/384.0), a73 = static_cast<Scalar>(500.0/1113.0), a74 = static_cast<Scalar>(125.0/192.0), a75 = static_cast<Scalar>(-2187.0/6784.0), a76 = static_cast<Scalar>(11.0/84.0);
		
		constexpr Scalar e1 = static_cast<Scalar>(71.0/57600.0), e3 = static_cast<Scalar>(-71.0/16695.0), e4 = static_cast<Scalar>(71.0/1920.0), e5 = static_cast<Scalar>(-17253.0/339200.0), e6 = static_cast<Scalar>(22.0/525.0), e7 = static_cast<Scalar>(-1.0/40.0);

		for (;;) {
			if (std::abs(current_dt) < config_.min_step) {
				return std::nullopt;
			}

			const auto k1 = compute_derivatives(state);

			GeodesicState<Scalar> s2 = state;
			for (size_t i = 0; i < 4; ++i) {
				s2.x(i) += current_dt * (a21 * k1.dx(i));
				s2.u(i) += current_dt * (a21 * k1.du(i));
			}
			const auto k2 = compute_derivatives(s2);

			GeodesicState<Scalar> s3 = state;
			for (size_t i = 0; i < 4; ++i) {
				s3.x(i) += current_dt * (a31 * k1.dx(i) + a32 * k2.dx(i));
				s3.u(i) += current_dt * (a31 * k1.du(i) + a32 * k2.du(i));
			}
			const auto k3 = compute_derivatives(s3);

			GeodesicState<Scalar> s4 = state;
			for (size_t i = 0; i < 4; ++i) {
				s4.x(i) += current_dt * (a41 * k1.dx(i) + a42 * k2.dx(i) + a43 * k3.dx(i));
				s4.u(i) += current_dt * (a41 * k1.du(i) + a42 * k2.du(i) + a43 * k3.du(i));
			}
			const auto k4 = compute_derivatives(s4);

			GeodesicState<Scalar> s5 = state;
			for (size_t i = 0; i < 4; ++i) {
				s5.x(i) += current_dt * (a51 * k1.dx(i) + a52 * k2.dx(i) + a53 * k3.dx(i) + a54 * k4.dx(i));
				s5.u(i) += current_dt * (a51 * k1.du(i) + a52 * k2.du(i) + a53 * k3.du(i) + a54 * k4.du(i));
			}
			const auto k5 = compute_derivatives(s5);

			GeodesicState<Scalar> s6 = state;
			for (size_t i = 0; i < 4; ++i) {
				s6.x(i) += current_dt * (a61 * k1.dx(i) + a62 * k2.dx(i) + a63 * k3.dx(i) + a64 * k4.dx(i) + a65 * k5.dx(i));
				s6.u(i) += current_dt * (a61 * k1.du(i) + a62 * k2.du(i) + a63 * k3.du(i) + a64 * k4.du(i) + a65 * k5.du(i));
			}
			const auto k6 = compute_derivatives(s6);

			GeodesicState<Scalar> s_next = state;
			for (size_t i = 0; i < 4; ++i) {
				s_next.x(i) += current_dt * (a71 * k1.dx(i) + a73 * k3.dx(i) + a74 * k4.dx(i) + a75 * k5.dx(i) + a76 * k6.dx(i));
				s_next.u(i) += current_dt * (a71 * k1.du(i) + a73 * k3.du(i) + a74 * k4.du(i) + a75 * k5.du(i) + a76 * k6.du(i));
			}
			const auto k7 = compute_derivatives(s_next);

			Scalar max_error = static_cast<Scalar>(0);
			for (size_t i = 0; i < 4; ++i) {
				const Scalar err_x = current_dt * std::abs(e1 * k1.dx(i) + e3 * k3.dx(i) + e4 * k4.dx(i) + e5 * k5.dx(i) + e6 * k6.dx(i) + e7 * k7.dx(i));
				const Scalar err_u = current_dt * std::abs(e1 * k1.du(i) + e3 * k3.du(i) + e4 * k4.du(i) + e5 * k5.du(i) + e6 * k6.du(i) + e7 * k7.du(i));
				const Scalar scale_x = config_.rtol * std::max(std::abs(state.x(i)), std::abs(s_next.x(i))) + config_.atol;
				const Scalar scale_u = config_.rtol * std::max(std::abs(state.u(i)), std::abs(s_next.u(i))) + config_.atol;
				max_error = std::max({max_error, err_x / scale_x, err_u / scale_u});
			}

			if (max_error <= static_cast<Scalar>(1.0)) {
				state = s_next;
				project_invariant(state);
				++stats_.accepted_steps;

				Scalar r_accepted = state.x(1);
				if constexpr (requires { { metric_.coordinate_radius(state.x) } -> std::convertible_to<Scalar>; }) {
					r_accepted = metric_.coordinate_radius(state.x);
				}

				if (r_accepted < stats_.min_radius_reached) {
					stats_.min_radius_reached = r_accepted;
				}

				if constexpr (requires { { metric_.schwarzschild_radius() } -> std::convertible_to<Scalar>; }) {
					const Scalar r_s = static_cast<Scalar>(metric_.schwarzschild_radius());
					if (r_s > static_cast<Scalar>(0)) {
						if (r_accepted <= r_s) {
							stats_.crossed_horizon = true;
							if (config_.crossing_mode == HorizonCrossingMode::Absorption) {
								stats_.absorbed_at_horizon = true;
							}
						}
						const Scalar sing_r = config_.singularity_threshold * r_s;
						if (r_accepted <= sing_r) {
							stats_.reached_singularity = true;
						}
					}
				}
				
				const Scalar dt_actual = current_dt;
				const Scalar factor = (max_error == static_cast<Scalar>(0)) ? static_cast<Scalar>(5.0) : std::pow(max_error, static_cast<Scalar>(-0.2));
				const Scalar scale = config_.safety_factor * factor;
				const Scalar sign = (current_dt < static_cast<Scalar>(0)) ? static_cast<Scalar>(-1) : static_cast<Scalar>(1);
				Scalar abs_dt = std::abs(current_dt);
				abs_dt *= std::clamp(scale, static_cast<Scalar>(0.2), static_cast<Scalar>(5.0));
				abs_dt = std::clamp(abs_dt, config_.min_step, config_.max_step);
				current_dt = sign * abs_dt;
				
				return dt_actual;
			} else {
				++stats_.rejected_steps;
				const Scalar factor = std::pow(max_error, static_cast<Scalar>(-0.25));
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
