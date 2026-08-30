#pragma once

#include "relativistic/core/tensor.hpp"
#include "relativistic/core/tensor_ops.hpp"
#include "relativistic/core/christoffel.hpp"
#include "relativistic/metrics/spacetime_concept.hpp"
#include "relativistic/observer/observer_tetrad.hpp"
#include "relativistic/observer/tetrad_transport.hpp"
#include <array>
#include <cmath>
#include <numbers>
#include <algorithm>
#include <cstdint>

namespace Relativistic::Observer {

enum class TimeFlowMode : uint8_t {
	ProperTime = 0,
	CoordinateTime = 1
};

struct RocketControlInput {
	std::array<double, 3> proper_thrust{0.0, 0.0, 0.0};
	std::array<double, 3> angular_torque{0.0, 0.0, 0.0};
	double main_engine_throttle{0.0};
	double max_proper_acceleration{100.0};
	double max_angular_rate{2.0};
};

struct RocketTelemetry {
	double proper_time{0.0};
	double coordinate_time{0.0};
	double coordinate_speed{0.0};
	double lorentz_factor{1.0};
	double proper_acceleration_norm{0.0};
	double rest_mass{1000.0};
	double fuel_mass{9000.0};
	double total_mass{10000.0};
	uint64_t total_steps{0};
};

template <typename MetricType, typename Scalar = double>
	requires Metrics::SpacetimeMetric<MetricType, Scalar>
class RelativisticRocket {
private:
	const MetricType& metric_;
	WorldlineObserverState<Scalar> state_;
	RocketControlInput control_{};
	TimeFlowMode time_flow_mode_{TimeFlowMode::ProperTime};
	RocketTelemetry telemetry_{};
	FermiWalkerTransport<MetricType, Scalar> transport_;

	Scalar dry_mass_{static_cast<Scalar>(1000.0)};
	Scalar fuel_mass_{static_cast<Scalar>(9000.0)};
	Scalar exhaust_velocity_{static_cast<Scalar>(30000.0)};

public:
	explicit RelativisticRocket(
		const MetricType& metric,
		const WorldlineObserverState<Scalar>& initial_state,
		TimeFlowMode flow_mode = TimeFlowMode::ProperTime,
		Scalar dry_mass = static_cast<Scalar>(1000.0),
		Scalar fuel_mass = static_cast<Scalar>(9000.0),
		Scalar exhaust_velocity = static_cast<Scalar>(30000.0)
	) noexcept
		: metric_(metric),
		  state_(initial_state),
		  time_flow_mode_(flow_mode),
		  transport_(metric),
		  dry_mass_(dry_mass),
		  fuel_mass_(fuel_mass),
		  exhaust_velocity_(exhaust_velocity) {
		transport_.orthonormalize(state_);
		update_telemetry();
	}

	void set_control_input(const RocketControlInput& ctrl) noexcept {
		control_ = ctrl;
	}

	void set_thrust(Scalar fx, Scalar fy, Scalar fz) noexcept {
		control_.proper_thrust[0] = static_cast<double>(fx);
		control_.proper_thrust[1] = static_cast<double>(fy);
		control_.proper_thrust[2] = static_cast<double>(fz);
	}

	void set_angular_rates(Scalar wx, Scalar wy, Scalar wz) noexcept {
		control_.angular_torque[0] = static_cast<double>(wx);
		control_.angular_torque[1] = static_cast<double>(wy);
		control_.angular_torque[2] = static_cast<double>(wz);
	}

	void set_throttle(Scalar throttle) noexcept {
		control_.main_engine_throttle = std::clamp(static_cast<double>(throttle), 0.0, 1.0);
	}

	void set_time_flow_mode(TimeFlowMode mode) noexcept {
		time_flow_mode_ = mode;
	}

	[[nodiscard]] constexpr TimeFlowMode time_flow_mode() const noexcept {
		return time_flow_mode_;
	}

	[[nodiscard]] constexpr const WorldlineObserverState<Scalar>& state() const noexcept {
		return state_;
	}

	[[nodiscard]] constexpr const RocketTelemetry& telemetry() const noexcept {
		return telemetry_;
	}

	[[nodiscard]] constexpr const RocketControlInput& control_input() const noexcept {
		return control_;
	}

	[[nodiscard]] Scalar total_mass() const noexcept {
		return dry_mass_ + fuel_mass_;
	}

	[[nodiscard]] Scalar dry_mass() const noexcept {
		return dry_mass_;
	}

	[[nodiscard]] Scalar fuel_mass() const noexcept {
		return fuel_mass_;
	}

	[[nodiscard]] Core::FourVector<Scalar> compute_proper_acceleration_four_vector(const WorldlineObserverState<Scalar>& st) const noexcept {
		const Scalar a1 = static_cast<Scalar>(control_.proper_thrust[0] + control_.main_engine_throttle * control_.max_proper_acceleration);
		const Scalar a2 = static_cast<Scalar>(control_.proper_thrust[1]);
		const Scalar a3 = static_cast<Scalar>(control_.proper_thrust[2]);

		Core::FourVector<Scalar> a;
		a.zero();

		for (size_t mu = 0; mu < 4; ++mu) {
			a(mu) = a1 * st.tetrad[1](mu) + a2 * st.tetrad[2](mu) + a3 * st.tetrad[3](mu);
		}
		return a;
	}

	[[nodiscard]] Core::FourVector<Scalar> compute_proper_acceleration_four_vector() const noexcept {
		return compute_proper_acceleration_four_vector(state_);
	}

	void step(Scalar dt_input) noexcept {
		if (dt_input <= static_cast<Scalar>(0.0)) {
			return;
		}

		const bool is_coord_time = (time_flow_mode_ == TimeFlowMode::CoordinateTime);
		const Scalar tau_before = state_.proper_time;

		transport_.step_rk4(state_, dt_input, [this](const WorldlineObserverState<Scalar>& st) noexcept -> Core::FourVector<Scalar> {
			return compute_proper_acceleration_four_vector(st);
		}, is_coord_time);

		const Scalar dtau_actual = state_.proper_time - tau_before;
		const Scalar a_norm = std::sqrt(std::max(control_.proper_thrust[0] * control_.proper_thrust[0] + control_.proper_thrust[1] * control_.proper_thrust[1] + control_.proper_thrust[2] * control_.proper_thrust[2], 0.0));

		if (fuel_mass_ > static_cast<Scalar>(0.0) && a_norm > static_cast<Scalar>(0.0)) {
			const Scalar mass_flow = (total_mass() * a_norm) / exhaust_velocity_;
			const Scalar consumed = mass_flow * dtau_actual;
			fuel_mass_ = std::max(fuel_mass_ - consumed, static_cast<Scalar>(0.0));
		}

		apply_spatial_rotation(dtau_actual);
		transport_.orthonormalize(state_);
		update_telemetry();
	}

	[[nodiscard]] static constexpr Scalar hyperbolic_velocity(Scalar proper_accel, Scalar coord_time, Scalar speed_of_light = static_cast<Scalar>(1.0)) noexcept {
		const Scalar at = proper_accel * coord_time;
		const Scalar at_c = at / speed_of_light;
		const Scalar denom = std::sqrt(static_cast<Scalar>(1.0) + at_c * at_c);
		return at / denom;
	}

	[[nodiscard]] static constexpr Scalar hyperbolic_proper_time(Scalar proper_accel, Scalar coord_time, Scalar speed_of_light = static_cast<Scalar>(1.0)) noexcept {
		const Scalar at_c = (proper_accel * coord_time) / speed_of_light;
		return (speed_of_light / proper_accel) * std::asinh(at_c);
	}

	[[nodiscard]] static constexpr Scalar hyperbolic_position(Scalar proper_accel, Scalar coord_time, Scalar speed_of_light = static_cast<Scalar>(1.0)) noexcept {
		const Scalar c2 = speed_of_light * speed_of_light;
		const Scalar at_c = (proper_accel * coord_time) / speed_of_light;
		const Scalar sqrt_term = std::sqrt(static_cast<Scalar>(1.0) + at_c * at_c);
		return (c2 / proper_accel) * (sqrt_term - static_cast<Scalar>(1.0));
	}

	[[nodiscard]] static constexpr Scalar hyperbolic_gamma(Scalar proper_accel, Scalar coord_time, Scalar speed_of_light = static_cast<Scalar>(1.0)) noexcept {
		const Scalar at_c = (proper_accel * coord_time) / speed_of_light;
		return std::sqrt(static_cast<Scalar>(1.0) + at_c * at_c);
	}

private:
	void apply_spatial_rotation(Scalar dtau) noexcept {
		const Scalar wx = static_cast<Scalar>(control_.angular_torque[0]);
		const Scalar wy = static_cast<Scalar>(control_.angular_torque[1]);
		const Scalar wz = static_cast<Scalar>(control_.angular_torque[2]);

		const Scalar w_mag = std::sqrt(wx * wx + wy * wy + wz * wz);
		if (w_mag < static_cast<Scalar>(1e-14)) {
			return;
		}

		const Scalar angle = w_mag * dtau;
		const Scalar kx = wx / w_mag;
		const Scalar ky = wy / w_mag;
		const Scalar kz = wz / w_mag;

		const Scalar cos_a = std::cos(angle);
		const Scalar sin_a = std::sin(angle);
		const Scalar one_minus_cos = static_cast<Scalar>(1.0) - cos_a;

		std::array<std::array<Scalar, 3>, 3> rot{{
			{cos_a + kx * kx * one_minus_cos, kx * ky * one_minus_cos - kz * sin_a, kx * kz * one_minus_cos + ky * sin_a},
			{ky * kx * one_minus_cos + kz * sin_a, cos_a + ky * ky * one_minus_cos, ky * kz * one_minus_cos - kx * sin_a},
			{kz * kx * one_minus_cos - ky * sin_a, kz * ky * one_minus_cos + kx * sin_a, cos_a + kz * kz * one_minus_cos}
		}};

		std::array<Core::FourVector<Scalar>, 3> old_e = {state_.tetrad[1], state_.tetrad[2], state_.tetrad[3]};

		for (size_t i = 0; i < 3; ++i) {
			state_.tetrad[i + 1].zero();
			for (size_t j = 0; j < 3; ++j) {
				for (size_t mu = 0; mu < 4; ++mu) {
					state_.tetrad[i + 1](mu) += rot[i][j] * old_e[j](mu);
				}
			}
		}
	}

	void update_telemetry() noexcept {
		const auto g = metric_.metric_tensor(state_.position);
		const Scalar c = metric_.speed_of_light();
		const Scalar u0 = state_.four_velocity(0);

		Scalar spatial_v2 = static_cast<Scalar>(0.0);
		for (size_t i = 1; i < 4; ++i) {
			for (size_t j = 1; j < 4; ++j) {
				spatial_v2 += g(i, j) * (state_.four_velocity(i) / u0) * (state_.four_velocity(j) / u0);
			}
		}
		const Scalar speed = std::sqrt(std::max(spatial_v2, static_cast<Scalar>(0.0)));
		const Scalar gamma = (std::abs(g(0, 0)) > static_cast<Scalar>(0.0)) ? (u0 / c) : static_cast<Scalar>(1.0);

		const auto a_vec = compute_proper_acceleration_four_vector();
		Scalar a_norm_sq = static_cast<Scalar>(0.0);
		for (size_t mu = 0; mu < 4; ++mu) {
			for (size_t nu = 0; nu < 4; ++nu) {
				a_norm_sq += g(mu, nu) * a_vec(mu) * a_vec(nu);
			}
		}

		telemetry_.proper_time = static_cast<double>(state_.proper_time);
		telemetry_.coordinate_time = static_cast<double>(state_.coordinate_time);
		telemetry_.coordinate_speed = static_cast<double>(speed);
		telemetry_.lorentz_factor = static_cast<double>(gamma);
		telemetry_.proper_acceleration_norm = static_cast<double>(std::sqrt(std::max(a_norm_sq, static_cast<Scalar>(0.0))));
		telemetry_.rest_mass = static_cast<double>(dry_mass_);
		telemetry_.fuel_mass = static_cast<double>(fuel_mass_);
		telemetry_.total_mass = static_cast<double>(total_mass());
		++telemetry_.total_steps;
	}
};

}
