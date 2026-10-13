#pragma once

#include "relativistic/core/math/tensor.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <numbers>

namespace Relativistic::Optics {

enum class GenericTraceFate : uint32_t {
	Escaped = 0,
	Absorbed = 1,
	Terminated = 2,
	StepBudgetExhausted = 3,
	InvalidState = 4
};

struct GenericTraceSettings {
	double escape_radius{100.0};
	uint32_t max_steps{2048};
	double min_step{1.0e-4};
	double max_step{50.0};
	double pole_guard_scale{1.0};
	double horizon_clearance{1.0001};
	std::function<double(const Core::FourVector<double>&, const Core::FourVector<double>&)> step_length{};
	std::function<double(const Core::FourVector<double>&)> horizon_radius{};
	std::function<bool(const Core::FourVector<double>&)> is_regular{};
};

struct GenericTraceOutcome {
	GenericTraceFate fate{GenericTraceFate::StepBudgetExhausted};
	uint32_t iterations{0};
	Core::FourVector<double> position{};
	Core::FourVector<double> velocity{};
	std::array<double, 3> sky_direction{1.0, 0.0, 0.0};
	double energy_ratio{1.0};
};

template <typename Metric>
class GenericMetricTracer {
public:
	using Vec4 = Core::FourVector<double>;
	using Mat4 = Core::MetricTensor<double>;

	[[nodiscard]] static double inner(const Mat4& g, const Vec4& a, const Vec4& b) noexcept {
		double sum = 0.0;
		for (size_t i = 0; i < 4; ++i) {
			for (size_t j = 0; j < 4; ++j) {
				sum += g(i, j) * a(i) * b(j);
			}
		}
		return sum;
	}

	[[nodiscard]] static std::array<Vec4, 4> static_frame(const Mat4& g) noexcept {
		std::array<Vec4, 4> e{};
		Vec4 t;
		t(0) = 1.0;
		e[0] = t * (1.0 / std::sqrt(std::max(-inner(g, t, t), 1.0e-30)));
		for (size_t k = 1; k < 4; ++k) {
			Vec4 v;
			v(k) = 1.0;
			for (size_t j = 0; j < k; ++j) {
				const double sign = (j == 0) ? -1.0 : 1.0;
				v = v - e[j] * (inner(g, v, e[j]) / sign);
			}
			e[k] = v * (1.0 / std::sqrt(std::max(inner(g, v, v), 1.0e-30)));
		}
		return e;
	}

	[[nodiscard]] static std::array<std::array<double, 3>, 3> spherical_axes(double theta, double phi) noexcept {
		const double st = std::sin(theta);
		const double ct = std::cos(theta);
		const double sp = std::sin(phi);
		const double cp = std::cos(phi);
		return {{{st * cp, st * sp, ct}, {ct * cp, ct * sp, -st}, {-sp, cp, 0.0}}};
	}

	[[nodiscard]] static Vec4 initial_velocity(const Metric& metric, const Vec4& x, const std::array<double, 3>& direction) noexcept {
		const auto frame = static_frame(metric.metric_tensor(x));
		const auto axes = spherical_axes(x(2), x(3));
		Vec4 u = -frame[0];
		for (size_t k = 0; k < 3; ++k) {
			const double component = direction[0] * axes[k][0] + direction[1] * axes[k][1] + direction[2] * axes[k][2];
			u = u + frame[k + 1] * component;
		}
		return u;
	}

	[[nodiscard]] static Vec4 acceleration(const Metric& metric, const Vec4& x, const Vec4& u) noexcept {
		const auto gamma = metric.christoffel_symbols(x);
		Vec4 result;
		for (size_t mu = 0; mu < 4; ++mu) {
			double sum = 0.0;
			for (size_t a = 0; a < 4; ++a) {
				const double ua = u(a);
				if (ua == 0.0) continue;
				sum -= gamma(mu, a, a) * ua * ua;
				for (size_t b = a + 1; b < 4; ++b) {
					const double ub = u(b);
					if (ub != 0.0) {
						sum -= 2.0 * gamma(mu, a, b) * ua * ub;
					}
				}
			}
			result(mu) = sum;
		}
		return result;
	}

	template <typename Visitor>
	[[nodiscard]] static GenericTraceOutcome trace(const Metric& metric, const GenericTraceSettings& settings, const Vec4& start, const std::array<double, 3>& direction, Visitor&& visitor) {
		constexpr double pi = std::numbers::pi_v<double>;
		Vec4 x = start;
		Vec4 u = initial_velocity(metric, x, direction);
		GenericTraceOutcome outcome;
		outcome.position = x;
		outcome.velocity = u;
		const double initial_e = -u(0);

		for (uint32_t step = 0; step < settings.max_steps; ++step) {
			outcome.iterations = step + 1;
			outcome.position = x;
			outcome.velocity = u;

			const double r = std::abs(x(1));
			if (settings.is_regular && !settings.is_regular(x)) {
				outcome.fate = GenericTraceFate::Absorbed;
				break;
			}

			const double horizon = settings.horizon_radius ? settings.horizon_radius(x) : 0.0;
			if (horizon > 0.0 && r <= horizon * settings.horizon_clearance) {
				outcome.fate = GenericTraceFate::Absorbed;
				break;
			}

			if (r >= settings.escape_radius) {
				outcome.fate = GenericTraceFate::Escaped;
				const double st = std::sin(x(2));
				const double ct = std::cos(x(2));
				const double sp = std::sin(x(3));
				const double cp = std::cos(x(3));
				outcome.sky_direction = {st * cp, st * sp, ct};
				outcome.energy_ratio = (initial_e != 0.0) ? (-u(0) / initial_e) : 1.0;
				break;
			}

			if (visitor(x, u)) {
				outcome.fate = GenericTraceFate::Terminated;
				break;
			}

			double h = settings.step_length ? settings.step_length(x, u) : 0.05 * std::max(r, 1.0);
			h = std::clamp(h, settings.min_step, settings.max_step);

			const Vec4 k1_x = u;
			const Vec4 k1_u = acceleration(metric, x, u);

			const Vec4 x2 = x + k1_x * (0.5 * h);
			const Vec4 u2 = u + k1_u * (0.5 * h);
			const Vec4 k2_x = u2;
			const Vec4 k2_u = acceleration(metric, x2, u2);

			const Vec4 x3 = x + k2_x * (0.5 * h);
			const Vec4 u3 = u + k2_u * (0.5 * h);
			const Vec4 k3_x = u3;
			const Vec4 k3_u = acceleration(metric, x3, u3);

			const Vec4 x4 = x + k3_x * h;
			const Vec4 u4 = u + k3_u * h;
			const Vec4 k4_x = u4;
			const Vec4 k4_u = acceleration(metric, x4, u4);

			x = x + (k1_x + k2_x * 2.0 + k3_x * 2.0 + k4_x) * (h / 6.0);
			u = u + (k1_u + k2_u * 2.0 + k3_u * 2.0 + k4_u) * (h / 6.0);

			x(2) = std::clamp(x(2), 1.0e-5 * settings.pole_guard_scale, pi - 1.0e-5 * settings.pole_guard_scale);
			x(3) = std::fmod(x(3), 2.0 * pi);
			if (x(3) < 0.0) {
				x(3) += 2.0 * pi;
			}

			if (!std::isfinite(x(0)) || !std::isfinite(x(1)) || !std::isfinite(u(0)) || !std::isfinite(u(1))) {
				outcome.fate = GenericTraceFate::InvalidState;
				break;
			}
		}

		return outcome;
	}
};

}
