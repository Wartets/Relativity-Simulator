#pragma once

#include "relativistic/core/tensor.hpp"
#include "relativistic/core/tensor_ops.hpp"
#include "relativistic/core/christoffel.hpp"
#include "relativistic/metrics/spacetime_concept.hpp"
#include "relativistic/observer/observer_tetrad.hpp"
#include <array>
#include <cmath>
#include <numbers>
#include <algorithm>
#include <optional>
#include <cstdint>

namespace Relativistic::Observer {

template <typename Scalar = double>
struct alignas(64) WorldlineObserverState {
	Core::FourVector<Scalar> position{};
	Core::FourVector<Scalar> four_velocity{};
	std::array<Core::FourVector<Scalar>, 4> tetrad{};
	Scalar proper_time{static_cast<Scalar>(0)};
	Scalar coordinate_time{static_cast<Scalar>(0)};

	constexpr WorldlineObserverState() noexcept = default;

	constexpr WorldlineObserverState(
		const Core::FourVector<Scalar>& pos,
		const Core::FourVector<Scalar>& u,
		const std::array<Core::FourVector<Scalar>, 4>& e,
		Scalar tau = static_cast<Scalar>(0)
	) noexcept
		: position(pos),
		  four_velocity(u),
		  tetrad(e),
		  proper_time(tau),
		  coordinate_time(pos(0)) {}

	[[nodiscard]] ObserverTetrad<Scalar> to_observer_tetrad() const noexcept {
		return ObserverTetrad<Scalar>(position, tetrad[0], tetrad[1], tetrad[2], tetrad[3]);
	}
};

template <typename Scalar = double>
struct alignas(64) FermiWalkerDerivatives {
	Core::FourVector<Scalar> d_position{};
	Core::FourVector<Scalar> d_velocity{};
	std::array<Core::FourVector<Scalar>, 4> d_tetrad{};
};

template <typename Scalar = double>
struct TransportConfig {
	Scalar rtol = static_cast<Scalar>(1e-12);
	Scalar atol = static_cast<Scalar>(1e-15);
	Scalar min_step = static_cast<Scalar>(1e-10);
	Scalar max_step = static_cast<Scalar>(1e8);
	Scalar safety_factor = static_cast<Scalar>(0.9);
	bool auto_orthonormalize = true;
	size_t orthonormalize_interval = 10;
};

template <typename Scalar = double>
struct TransportStats {
	uint64_t steps_taken = 0;
	uint64_t evaluations = 0;
	Scalar max_orthonormality_residual = static_cast<Scalar>(0);
	Scalar accumulated_proper_time = static_cast<Scalar>(0);
};

template <typename MetricType, typename Scalar = double>
	requires Metrics::SpacetimeMetric<MetricType, Scalar>
class FermiWalkerTransport {
private:
	const MetricType& metric_;
	TransportConfig<Scalar> config_;
	mutable TransportStats<Scalar> stats_;

public:
	explicit constexpr FermiWalkerTransport(
		const MetricType& metric,
		const TransportConfig<Scalar>& config = {}
	) noexcept
		: metric_(metric), config_(config), stats_{} {}

	[[nodiscard]] constexpr const TransportStats<Scalar>& statistics() const noexcept {
		return stats_;
	}

	constexpr void reset_statistics() noexcept {
		stats_ = TransportStats<Scalar>{};
	}

	[[nodiscard]] FermiWalkerDerivatives<Scalar> compute_derivatives(
		const WorldlineObserverState<Scalar>& state,
		const Core::FourVector<Scalar>& proper_acceleration = {}
	) const noexcept {
		FermiWalkerDerivatives<Scalar> d;
		d.d_position = state.four_velocity;

		const auto gamma = Core::compute_christoffel<Core::DerivativeOrder::EighthOrder, MetricType, Scalar>(metric_, state.position);
		const auto g = metric_.metric_tensor(state.position);
		const Scalar c = metric_.speed_of_light();
		const Scalar c2 = c * c;
		const Scalar inv_c2 = static_cast<Scalar>(1) / c2;

		Core::FourVector<Scalar> a_geom;
		a_geom.zero();
		for (size_t mu = 0; mu < 4; ++mu) {
			Scalar sum = static_cast<Scalar>(0);
			for (size_t alpha = 0; alpha < 4; ++alpha) {
				const Scalar u_a = state.four_velocity(alpha);
				if (u_a == static_cast<Scalar>(0)) continue;
				sum -= gamma(mu, alpha, alpha) * u_a * u_a;
				for (size_t beta = alpha + 1; beta < 4; ++beta) {
					const Scalar u_b = state.four_velocity(beta);
					if (u_b != static_cast<Scalar>(0)) {
						sum -= static_cast<Scalar>(2) * gamma(mu, alpha, beta) * u_a * u_b;
					}
				}
			}
			a_geom(mu) = sum;
		}

		for (size_t mu = 0; mu < 4; ++mu) {
			d.d_velocity(mu) = a_geom(mu) + proper_acceleration(mu);
		}

		const Core::FourVector<Scalar>& a_total = proper_acceleration;

		Core::FourVector<Scalar> u_lower;
		Core::FourVector<Scalar> a_lower;
		u_lower.zero();
		a_lower.zero();
		for (size_t mu = 0; mu < 4; ++mu) {
			for (size_t nu = 0; nu < 4; ++nu) {
				u_lower(mu) += g(mu, nu) * state.four_velocity(nu);
				a_lower(mu) += g(mu, nu) * a_total(nu);
			}
		}

		for (size_t sigma = 0; sigma < 4; ++sigma) {
			d.d_tetrad[sigma].zero();

			Core::FourVector<Scalar> parallel_term;
			parallel_term.zero();
			for (size_t mu = 0; mu < 4; ++mu) {
				Scalar sum = static_cast<Scalar>(0);
				for (size_t alpha = 0; alpha < 4; ++alpha) {
					const Scalar u_a = state.four_velocity(alpha);
					if (u_a == static_cast<Scalar>(0)) continue;
					for (size_t beta = 0; beta < 4; ++beta) {
						const Scalar e_b = state.tetrad[sigma](beta);
						if (e_b != static_cast<Scalar>(0)) {
							sum -= gamma(mu, alpha, beta) * u_a * e_b;
						}
					}
				}
				parallel_term(mu) = sum;
			}

			Scalar u_dot_e = static_cast<Scalar>(0);
			Scalar a_dot_e = static_cast<Scalar>(0);
			for (size_t mu = 0; mu < 4; ++mu) {
				u_dot_e += u_lower(mu) * state.tetrad[sigma](mu);
				a_dot_e += a_lower(mu) * state.tetrad[sigma](mu);
			}

			for (size_t mu = 0; mu < 4; ++mu) {
				const Scalar fw_correction = inv_c2 * (a_total(mu) * u_dot_e - state.four_velocity(mu) * a_dot_e);
				d.d_tetrad[sigma](mu) = parallel_term(mu) - fw_correction;
			}
		}

		++stats_.evaluations;
		return d;
	}

	void orthonormalize(WorldlineObserverState<Scalar>& state) const noexcept {
		const auto g = metric_.metric_tensor(state.position);
		const Scalar c = metric_.speed_of_light();

		auto inner_product = [&](const Core::FourVector<Scalar>& v1, const Core::FourVector<Scalar>& v2) noexcept -> Scalar {
			Scalar sum = static_cast<Scalar>(0);
			for (size_t mu = 0; mu < 4; ++mu) {
				for (size_t nu = 0; nu < 4; ++nu) {
					sum += g(mu, nu) * v1(mu) * v2(nu);
				}
			}
			return sum;
		};

		const Scalar norm_u = inner_product(state.four_velocity, state.four_velocity);
		if (norm_u < static_cast<Scalar>(0)) {
			const Scalar scale_u = c / std::sqrt(-norm_u);
			for (size_t mu = 0; mu < 4; ++mu) {
				state.four_velocity(mu) *= scale_u;
				state.tetrad[0](mu) = state.four_velocity(mu) / c;
			}
		}

		for (size_t i = 1; i < 4; ++i) {
			const Scalar proj0 = inner_product(state.tetrad[i], state.tetrad[0]);
			for (size_t mu = 0; mu < 4; ++mu) {
				state.tetrad[i](mu) += proj0 * state.tetrad[0](mu);
			}

			for (size_t j = 1; j < i; ++j) {
				const Scalar proj_j = inner_product(state.tetrad[i], state.tetrad[j]);
				for (size_t mu = 0; mu < 4; ++mu) {
					state.tetrad[i](mu) -= proj_j * state.tetrad[j](mu);
				}
			}

			const Scalar norm_i = inner_product(state.tetrad[i], state.tetrad[i]);
			if (norm_i > static_cast<Scalar>(0)) {
				const Scalar scale_i = static_cast<Scalar>(1) / std::sqrt(norm_i);
				for (size_t mu = 0; mu < 4; ++mu) {
					state.tetrad[i](mu) *= scale_i;
				}
			}
		}

		static_cast<void>(check_and_record_residuals(state));
	}

	[[nodiscard]] Scalar check_and_record_residuals(const WorldlineObserverState<Scalar>& state) const noexcept {
		const auto g = metric_.metric_tensor(state.position);
		Scalar max_res = static_cast<Scalar>(0);

		for (size_t a = 0; a < 4; ++a) {
			for (size_t b = 0; b < 4; ++b) {
				Scalar prod = static_cast<Scalar>(0);
				for (size_t mu = 0; mu < 4; ++mu) {
					for (size_t nu = 0; nu < 4; ++nu) {
						prod += g(mu, nu) * state.tetrad[a](mu) * state.tetrad[b](nu);
					}
				}
				const Scalar target = (a == b) ? ((a == 0) ? static_cast<Scalar>(-1) : static_cast<Scalar>(1)) : static_cast<Scalar>(0);
				const Scalar res = std::abs(prod - target);
				if (res > max_res) {
					max_res = res;
				}
			}
		}

		if (max_res > stats_.max_orthonormality_residual) {
			stats_.max_orthonormality_residual = max_res;
		}

		return max_res;
	}

	void step_rk4(
		WorldlineObserverState<Scalar>& state,
		Scalar dt_or_dtau,
		const Core::FourVector<Scalar>& proper_acceleration = {},
		bool is_coordinate_time = false
	) const noexcept {
		step_rk4(state, dt_or_dtau, [&proper_acceleration](const WorldlineObserverState<Scalar>&) noexcept -> Core::FourVector<Scalar> {
			return proper_acceleration;
		}, is_coordinate_time);
	}

	template <typename AccelerationFunc>
		requires std::invocable<AccelerationFunc, const WorldlineObserverState<Scalar>&>
	void step_rk4(
		WorldlineObserverState<Scalar>& state,
		Scalar dt_or_dtau,
		AccelerationFunc&& get_proper_acc,
		bool is_coordinate_time = false
	) const noexcept {
		const Scalar c = metric_.speed_of_light();

		auto eval_stage_derivatives = [&](const WorldlineObserverState<Scalar>& st, Scalar& out_dtau_rate) noexcept -> FermiWalkerDerivatives<Scalar> {
			auto d = compute_derivatives(st, get_proper_acc(st));
			const Scalar u0 = st.four_velocity(0);
			if (is_coordinate_time) {
				const Scalar inv_u0 = (u0 > static_cast<Scalar>(0.0)) ? (c / u0) : static_cast<Scalar>(1.0);
				for (size_t mu = 0; mu < 4; ++mu) {
					d.d_position(mu) *= inv_u0;
					d.d_velocity(mu) *= inv_u0;
					for (size_t sigma = 0; sigma < 4; ++sigma) {
						d.d_tetrad[sigma](mu) *= inv_u0;
					}
				}
				out_dtau_rate = inv_u0;
			} else {
				out_dtau_rate = static_cast<Scalar>(1.0);
			}
			return d;
		};

		Scalar dtau_k1 = static_cast<Scalar>(0.0);
		const auto k1 = eval_stage_derivatives(state, dtau_k1);

		WorldlineObserverState<Scalar> s2 = state;
		const Scalar half_h = dt_or_dtau * static_cast<Scalar>(0.5);
		for (size_t mu = 0; mu < 4; ++mu) {
			s2.position(mu) += half_h * k1.d_position(mu);
			s2.four_velocity(mu) += half_h * k1.d_velocity(mu);
			for (size_t sigma = 0; sigma < 4; ++sigma) {
				s2.tetrad[sigma](mu) += half_h * k1.d_tetrad[sigma](mu);
			}
		}
		Scalar dtau_k2 = static_cast<Scalar>(0.0);
		const auto k2 = eval_stage_derivatives(s2, dtau_k2);

		WorldlineObserverState<Scalar> s3 = state;
		for (size_t mu = 0; mu < 4; ++mu) {
			s3.position(mu) += half_h * k2.d_position(mu);
			s3.four_velocity(mu) += half_h * k2.d_velocity(mu);
			for (size_t sigma = 0; sigma < 4; ++sigma) {
				s3.tetrad[sigma](mu) += half_h * k2.d_tetrad[sigma](mu);
			}
		}
		Scalar dtau_k3 = static_cast<Scalar>(0.0);
		const auto k3 = eval_stage_derivatives(s3, dtau_k3);

		WorldlineObserverState<Scalar> s4 = state;
		for (size_t mu = 0; mu < 4; ++mu) {
			s4.position(mu) += dt_or_dtau * k3.d_position(mu);
			s4.four_velocity(mu) += dt_or_dtau * k3.d_velocity(mu);
			for (size_t sigma = 0; sigma < 4; ++sigma) {
				s4.tetrad[sigma](mu) += dt_or_dtau * k3.d_tetrad[sigma](mu);
			}
		}
		Scalar dtau_k4 = static_cast<Scalar>(0.0);
		const auto k4 = eval_stage_derivatives(s4, dtau_k4);

		const Scalar sixth_h = dt_or_dtau * (static_cast<Scalar>(1) / static_cast<Scalar>(6));
		for (size_t mu = 0; mu < 4; ++mu) {
			state.position(mu) += sixth_h * (k1.d_position(mu) + static_cast<Scalar>(2) * k2.d_position(mu) + static_cast<Scalar>(2) * k3.d_position(mu) + k4.d_position(mu));
			state.four_velocity(mu) += sixth_h * (k1.d_velocity(mu) + static_cast<Scalar>(2) * k2.d_velocity(mu) + static_cast<Scalar>(2) * k3.d_velocity(mu) + k4.d_velocity(mu));
			for (size_t sigma = 0; sigma < 4; ++sigma) {
				state.tetrad[sigma](mu) += sixth_h * (k1.d_tetrad[sigma](mu) + static_cast<Scalar>(2) * k2.d_tetrad[sigma](mu) + static_cast<Scalar>(2) * k3.d_tetrad[sigma](mu) + k4.d_tetrad[sigma](mu));
			}
		}

		const Scalar delta_tau = sixth_h * (dtau_k1 + static_cast<Scalar>(2) * dtau_k2 + static_cast<Scalar>(2) * dtau_k3 + dtau_k4);
		state.proper_time += delta_tau;
		state.coordinate_time = state.position(0) / c;
		stats_.accumulated_proper_time += delta_tau;
		++stats_.steps_taken;

		if (config_.auto_orthonormalize && (stats_.steps_taken % config_.orthonormalize_interval == 0)) {
			orthonormalize(state);
		}
	}

	[[nodiscard]] static WorldlineObserverState<Scalar> initialize_circular_geodesic_equatorial(
		const MetricType& metric,
		Scalar radius
	) noexcept {
		const Scalar m = metric.mass();
		const Scalar c = metric.speed_of_light();
		const Scalar g_const = metric.gravitational_constant();
		const Scalar r0 = radius;
		const Scalar gm = g_const * m;

		const Scalar omega = std::sqrt(gm / (r0 * r0 * r0));
		const Scalar factor = static_cast<Scalar>(1) - (static_cast<Scalar>(3) * gm) / (c * c * r0);
		const Scalar u_t = static_cast<Scalar>(1) / std::sqrt(std::max(factor, static_cast<Scalar>(1e-30)));
		const Scalar u_phi = omega * u_t;

		Core::FourVector<Scalar> pos(static_cast<Scalar>(0), r0, std::numbers::pi_v<Scalar> * static_cast<Scalar>(0.5), static_cast<Scalar>(0));
		Core::FourVector<Scalar> vel(u_t, static_cast<Scalar>(0), static_cast<Scalar>(0), u_phi);

		const auto g = metric.metric_tensor(pos);
		const Core::FourVector<Scalar> e0 = vel / c;

		const Core::FourVector<Scalar> e1(static_cast<Scalar>(0), std::sqrt(static_cast<Scalar>(1) / g(1, 1)), static_cast<Scalar>(0), static_cast<Scalar>(0));
		const Core::FourVector<Scalar> e2(static_cast<Scalar>(0), static_cast<Scalar>(0), std::sqrt(static_cast<Scalar>(1) / g(2, 2)), static_cast<Scalar>(0));

		const Scalar factor_2gm = static_cast<Scalar>(1) - (static_cast<Scalar>(2) * gm) / (c * c * r0);
		const Scalar sqrt_factor_2gm = std::sqrt(std::max(factor_2gm, static_cast<Scalar>(1e-30)));
		const Scalar e3_t = (r0 * u_phi) / (c * c * sqrt_factor_2gm);
		const Scalar e3_phi = (u_t * sqrt_factor_2gm) / r0;
		const Core::FourVector<Scalar> e3(e3_t, static_cast<Scalar>(0), static_cast<Scalar>(0), e3_phi);

		WorldlineObserverState<Scalar> state(pos, vel, {e0, e1, e2, e3}, static_cast<Scalar>(0));
		return state;
	}

	[[nodiscard]] static Scalar compute_geodetic_precession_angle(
		const WorldlineObserverState<Scalar>& initial_state,
		const WorldlineObserverState<Scalar>& current_state,
		const MetricType& metric
	) noexcept {
		static_cast<void>(initial_state);
		const Scalar r0 = current_state.position(1);
		const Scalar gm = metric.gravitational_constant() * metric.mass();
		const Scalar c = metric.speed_of_light();
		const Scalar c2 = c * c;

		const Scalar factor_2gm = static_cast<Scalar>(1) - (static_cast<Scalar>(2) * gm) / (c2 * r0);
		const Scalar factor_3gm = static_cast<Scalar>(1) - (static_cast<Scalar>(3) * gm) / (c2 * r0);
		const Scalar f = std::sqrt(std::max(factor_3gm / factor_2gm, static_cast<Scalar>(1e-30)));

		const Scalar cos_psi = current_state.tetrad[1](1) / std::sqrt(factor_2gm);
		const Scalar sin_psi = -current_state.tetrad[1](3) * r0 * f;

		const Scalar two_pi = static_cast<Scalar>(2) * std::numbers::pi_v<Scalar>;
		Scalar psi = std::atan2(sin_psi, cos_psi);
		if (psi < static_cast<Scalar>(0)) {
			psi += two_pi;
		}

		Scalar delta = two_pi - psi;
		while (delta < static_cast<Scalar>(0)) delta += two_pi;
		while (delta >= two_pi) delta -= two_pi;

		return delta;
	}

	[[nodiscard]] static Scalar theoretical_geodetic_precession_rate_rad_s(
		Scalar mass,
		Scalar orbit_radius,
		Scalar speed_of_light = static_cast<Scalar>(299792458.0),
		Scalar gravitational_constant = static_cast<Scalar>(6.67430e-11)
	) noexcept {
		const Scalar gm = gravitational_constant * mass;
		const Scalar c = speed_of_light;
		const Scalar r = orbit_radius;
		const Scalar omega_orb = std::sqrt(gm / (r * r * r));
		return static_cast<Scalar>(1.5) * (gm / (c * c * r)) * omega_orb;
	}

	[[nodiscard]] static Scalar theoretical_thomas_precession_angle(
		Scalar velocity,
		Scalar speed_of_light = static_cast<Scalar>(1)
	) noexcept {
		const Scalar beta = velocity / speed_of_light;
		const Scalar gamma = static_cast<Scalar>(1) / std::sqrt(static_cast<Scalar>(1) - beta * beta);
		return static_cast<Scalar>(2.0) * std::numbers::pi_v<Scalar> * (gamma - static_cast<Scalar>(1));
	}
};

}
