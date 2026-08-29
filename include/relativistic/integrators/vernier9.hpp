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
struct Vernier9Tableau {
	static constexpr size_t STAGES = 16;
	static constexpr size_t ORDER = 9;
	static constexpr size_t EMBEDDED_ORDER = 8;

	static constexpr std::array<Scalar, 16> C = {
		static_cast<Scalar>(0.0),
		static_cast<Scalar>(1.0 / 18.0),
		static_cast<Scalar>(1.0 / 12.0),
		static_cast<Scalar>(1.0 / 8.0),
		static_cast<Scalar>(5.0 / 16.0),
		static_cast<Scalar>(3.0 / 8.0),
		static_cast<Scalar>(59.0 / 128.0),
		static_cast<Scalar>(7.0 / 16.0),
		static_cast<Scalar>(17.0 / 32.0),
		static_cast<Scalar>(25.0 / 32.0),
		static_cast<Scalar>(23.0 / 32.0),
		static_cast<Scalar>(13.0 / 16.0),
		static_cast<Scalar>(7.0 / 8.0),
		static_cast<Scalar>(1.0),
		static_cast<Scalar>(1.0),
		static_cast<Scalar>(1.0)
	};

	static constexpr std::array<Scalar, 16> B9 = {
		static_cast<Scalar>(2379.0 / 58752.0),
		static_cast<Scalar>(0.0),
		static_cast<Scalar>(0.0),
		static_cast<Scalar>(0.0),
		static_cast<Scalar>(0.0),
		static_cast<Scalar>(0.0),
		static_cast<Scalar>(0.0),
		static_cast<Scalar>(279841.0 / 3110400.0),
		static_cast<Scalar>(20873.0 / 182784.0),
		static_cast<Scalar>(1552.0 / 4995.0),
		static_cast<Scalar>(13364.0 / 72225.0),
		static_cast<Scalar>(67456.0 / 465525.0),
		static_cast<Scalar>(8192.0 / 92025.0),
		static_cast<Scalar>(0.0),
		static_cast<Scalar>(11.0 / 320.0),
		static_cast<Scalar>(11.0 / 320.0)
	};

	static constexpr std::array<Scalar, 16> B8 = {
		static_cast<Scalar>(2379.0 / 58752.0),
		static_cast<Scalar>(0.0),
		static_cast<Scalar>(0.0),
		static_cast<Scalar>(0.0),
		static_cast<Scalar>(0.0),
		static_cast<Scalar>(0.0),
		static_cast<Scalar>(0.0),
		static_cast<Scalar>(279841.0 / 3110400.0),
		static_cast<Scalar>(20873.0 / 182784.0),
		static_cast<Scalar>(1552.0 / 4995.0),
		static_cast<Scalar>(13364.0 / 72225.0),
		static_cast<Scalar>(67456.0 / 465525.0),
		static_cast<Scalar>(8192.0 / 92025.0),
		static_cast<Scalar>(11.0 / 160.0),
		static_cast<Scalar>(0.0),
		static_cast<Scalar>(0.0)
	};

	static constexpr std::array<std::array<Scalar, 16>, 16> A = {{
		{0},
		{static_cast<Scalar>(1.0 / 18.0)},
		{static_cast<Scalar>(1.0 / 48.0), static_cast<Scalar>(1.0 / 16.0)},
		{static_cast<Scalar>(1.0 / 32.0), static_cast<Scalar>(0.0), static_cast<Scalar>(3.0 / 32.0)},
		{static_cast<Scalar>(5.0 / 16.0), static_cast<Scalar>(0.0), static_cast<Scalar>(-75.0 / 64.0), static_cast<Scalar>(75.0 / 64.0)},
		{static_cast<Scalar>(3.0 / 80.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(3.0 / 16.0), static_cast<Scalar>(3.0 / 20.0)},
		{static_cast<Scalar>(2301.0 / 32768.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(-10503.0 / 32768.0), static_cast<Scalar>(10005.0 / 16384.0), static_cast<Scalar>(345.0 / 4096.0)},
		{static_cast<Scalar>(287.0 / 4096.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(-1225.0 / 4096.0), static_cast<Scalar>(3283.0 / 8192.0), static_cast<Scalar>(0.0), static_cast<Scalar>(128.0 / 1001.0)},
		{static_cast<Scalar>(2261.0 / 32768.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(-5985.0 / 32768.0), static_cast<Scalar>(10965.0 / 32768.0), static_cast<Scalar>(0.0), static_cast<Scalar>(320.0 / 3003.0), static_cast<Scalar>(208.0 / 1001.0)},
		{static_cast<Scalar>(15325.0 / 32768.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(-71325.0 / 32768.0), static_cast<Scalar>(100775.0 / 32768.0), static_cast<Scalar>(0.0), static_cast<Scalar>(6400.0 / 9009.0), static_cast<Scalar>(-20800.0 / 9009.0), static_cast<Scalar>(4096.0 / 3003.0)},
		{static_cast<Scalar>(15479.0 / 81920.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(-43631.0 / 81920.0), static_cast<Scalar>(29693.0 / 40960.0), static_cast<Scalar>(0.0), static_cast<Scalar>(2048.0 / 9009.0), static_cast<Scalar>(-208.0 / 1001.0), static_cast<Scalar>(1024.0 / 9009.0), static_cast<Scalar>(2.0 / 25.0)},
		{static_cast<Scalar>(12519.0 / 40960.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(-42263.0 / 40960.0), static_cast<Scalar>(49203.0 / 40960.0), static_cast<Scalar>(0.0), static_cast<Scalar>(1024.0 / 3003.0), static_cast<Scalar>(-104.0 / 1001.0), static_cast<Scalar>(1024.0 / 3003.0), static_cast<Scalar>(-6.0 / 25.0), static_cast<Scalar>(16.0 / 25.0)},
		{static_cast<Scalar>(1379.0 / 8192.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(-4165.0 / 8192.0), static_cast<Scalar>(4459.0 / 8192.0), static_cast<Scalar>(0.0), static_cast<Scalar>(128.0 / 1001.0), static_cast<Scalar>(0.0), static_cast<Scalar>(1024.0 / 3003.0), static_cast<Scalar>(-3.0 / 25.0), static_cast<Scalar>(12.0 / 25.0), static_cast<Scalar>(1.0 / 4.0)},
		{static_cast<Scalar>(2379.0 / 58752.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(279841.0 / 3110400.0), static_cast<Scalar>(20873.0 / 182784.0), static_cast<Scalar>(1552.0 / 4995.0), static_cast<Scalar>(13364.0 / 72225.0), static_cast<Scalar>(67456.0 / 465525.0), static_cast<Scalar>(8192.0 / 92025.0)},
		{static_cast<Scalar>(2379.0 / 58752.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(279841.0 / 3110400.0), static_cast<Scalar>(20873.0 / 182784.0), static_cast<Scalar>(1552.0 / 4995.0), static_cast<Scalar>(13364.0 / 72225.0), static_cast<Scalar>(67456.0 / 465525.0), static_cast<Scalar>(8192.0 / 92025.0), static_cast<Scalar>(0.0)},
		{static_cast<Scalar>(2379.0 / 58752.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(279841.0 / 3110400.0), static_cast<Scalar>(20873.0 / 182784.0), static_cast<Scalar>(1552.0 / 4995.0), static_cast<Scalar>(13364.0 / 72225.0), static_cast<Scalar>(67456.0 / 465525.0), static_cast<Scalar>(8192.0 / 92025.0), static_cast<Scalar>(11.0 / 320.0), static_cast<Scalar>(0.0)}
	}};
};

template <typename MetricType, typename Scalar = double>
	requires Metrics::SpacetimeMetric<MetricType, Scalar>
class Vernier9Integrator {
private:
	using Tableau = Vernier9Tableau<Scalar>;

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

public:
	explicit constexpr Vernier9Integrator(
		const MetricType& metric,
		GeodesicType type = GeodesicType::Timelike,
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
		for (;;) {
			if (std::abs(current_dt) < config_.min_step) {
				return std::nullopt;
			}

			std::array<Derivatives, 16> k;
			k[0] = compute_derivatives(state);

			for (size_t stage = 1; stage < 16; ++stage) {
				GeodesicState<Scalar> s = state;
				for (size_t j = 0; j < stage; ++j) {
					const Scalar a_ij = Tableau::A[stage][j];
					if (a_ij != static_cast<Scalar>(0.0)) {
						const Scalar w = current_dt * a_ij;
						for (size_t i = 0; i < 4; ++i) {
							s.x(i) += w * k[j].dx(i);
							s.u(i) += w * k[j].du(i);
						}
					}
				}
				k[stage] = compute_derivatives(s);
			}

			GeodesicState<Scalar> s9 = state;
			GeodesicState<Scalar> s8 = state;

			for (size_t stage = 0; stage < 16; ++stage) {
				const Scalar w9 = current_dt * Tableau::B9[stage];
				const Scalar w8 = current_dt * Tableau::B8[stage];
				for (size_t i = 0; i < 4; ++i) {
					s9.x(i) += w9 * k[stage].dx(i);
					s9.u(i) += w9 * k[stage].du(i);
					s8.x(i) += w8 * k[stage].dx(i);
					s8.u(i) += w8 * k[stage].du(i);
				}
			}

			Scalar max_error = static_cast<Scalar>(0);
			for (size_t i = 0; i < 4; ++i) {
				const Scalar err_x = std::abs(s9.x(i) - s8.x(i));
				const Scalar err_u = std::abs(s9.u(i) - s8.u(i));
				const Scalar scale_x = config_.rtol * std::max(std::abs(state.x(i)), std::abs(s9.x(i))) + config_.atol;
				const Scalar scale_u = config_.rtol * std::max(std::abs(state.u(i)), std::abs(s9.u(i))) + config_.atol;
				max_error = std::max({max_error, err_x / scale_x, err_u / scale_u});
			}

			if (max_error <= static_cast<Scalar>(1.0)) {
				state = s9;
				++stats_.accepted_steps;

				const Scalar dt_actual = current_dt;
				const Scalar factor = (max_error == static_cast<Scalar>(0)) ? static_cast<Scalar>(5.0) : std::pow(max_error, static_cast<Scalar>(-1.0 / 9.0));
				const Scalar scale = config_.safety_factor * factor;
				const Scalar sign = (current_dt < static_cast<Scalar>(0)) ? static_cast<Scalar>(-1) : static_cast<Scalar>(1);
				Scalar abs_dt = std::abs(current_dt);
				abs_dt *= std::clamp(scale, static_cast<Scalar>(0.2), static_cast<Scalar>(5.0));
				abs_dt = std::clamp(abs_dt, config_.min_step, config_.max_step);
				current_dt = sign * abs_dt;

				return dt_actual;
			} else {
				++stats_.rejected_steps;
				const Scalar factor = std::pow(max_error, static_cast<Scalar>(-1.0 / 8.0));
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
