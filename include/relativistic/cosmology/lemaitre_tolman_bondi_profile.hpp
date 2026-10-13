#pragma once

#include <algorithm>
#include <cmath>
#include <numbers>

namespace Relativistic::Cosmology {

struct LtbParameters {
	double hubble0{0.005};
	double void_radius{60.0};
	double void_width{25.0};
	double omega_inner{0.2};
	double omega_outer{1.0};

	[[nodiscard]] bool operator==(const LtbParameters&) const noexcept = default;

	void sanitize() noexcept {
		const auto finite_or = [](double value, double fallback) noexcept { return std::isfinite(value) ? value : fallback; };
		hubble0 = std::clamp(finite_or(hubble0, 0.005), 1.0e-6, 10.0);
		void_radius = std::clamp(finite_or(void_radius, 60.0), 1.0e-3, 1.0e9);
		void_width = std::clamp(finite_or(void_width, 25.0), 1.0e-3, 1.0e9);
		omega_inner = std::clamp(finite_or(omega_inner, 0.2), 0.01, 3.0);
		omega_outer = std::clamp(finite_or(omega_outer, 1.0), 0.01, 3.0);
	}
};

class LtbProfile {
public:
	struct Shell {
		double radius{1.0e-9};
		double velocity{0.0};
		double curvature{0.0};
		bool regular{false};
	};

	struct Derivatives {
		double radius{1.0e-9};
		double velocity{0.0};
		double radius_r{1.0};
		double velocity_r{0.0};
		double radius_rr{0.0};
		double curvature{0.0};
		double curvature_r{0.0};
		bool regular{false};
	};

	explicit LtbProfile(const LtbParameters& parameters) noexcept
		: parameters_(parameters),
		  present_time_(std::max(age(parameters.omega_inner), age(parameters.omega_outer))) {}

	[[nodiscard]] const LtbParameters& parameters() const noexcept { return parameters_; }
	[[nodiscard]] double present_time() const noexcept { return present_time_; }

	[[nodiscard]] double omega(double r) const noexcept {
		const double width = std::max(parameters_.void_width, 1.0e-9);
		const double step = (1.0 - std::tanh((r - parameters_.void_radius) / (2.0 * width))) / (1.0 + std::tanh(parameters_.void_radius / (2.0 * width)));
		return parameters_.omega_outer + (parameters_.omega_inner - parameters_.omega_outer) * step;
	}

	[[nodiscard]] double omega_derivative(double r) const noexcept {
		const double width = std::max(parameters_.void_width, 1.0e-9);
		const double t = std::tanh((r - parameters_.void_radius) / (2.0 * width));
		return (parameters_.omega_inner - parameters_.omega_outer) * (-(1.0 - t * t) / (2.0 * width)) / (1.0 + std::tanh(parameters_.void_radius / (2.0 * width)));
	}

	[[nodiscard]] double age(double omega_value) const noexcept {
		const double h0 = parameters_.hubble0;
		const double d = 1.0 - omega_value;
		if (std::abs(d) < 1.0e-9) {
			return 2.0 / (3.0 * h0);
		}
		const double c = (2.0 - omega_value) / omega_value;
		if (d > 0.0) {
			const double eta = std::acosh(std::max(c, 1.0));
			return omega_value / (2.0 * h0 * d * std::sqrt(d)) * (std::sinh(eta) - eta);
		}
		const double eta = std::acos(std::clamp(c, -1.0, 1.0));
		return omega_value / (2.0 * h0 * (-d) * std::sqrt(-d)) * (eta - std::sin(eta));
	}

	[[nodiscard]] double bang_time(double r) const noexcept {
		return present_time_ - age(omega(r));
	}

	[[nodiscard]] Shell shell(double r, double t) const noexcept {
		const double h0 = parameters_.hubble0;
		const double omega_value = omega(r);
		const double d = 1.0 - omega_value;
		Shell result;
		result.curvature = h0 * h0 * d * r * r;
		const double tau = t - (present_time_ - age(omega_value));
		if (!(tau > 1.0e-9)) {
			return result;
		}
		double areal = 0.0;
		if (std::abs(d) < 1.0e-9) {
			areal = r * std::cbrt(2.25 * h0 * h0 * tau * tau);
		} else if (d > 0.0) {
			const double scale = omega_value / (2.0 * h0 * d * std::sqrt(d));
			const double eta = solve_eta(tau / scale, 1.0);
			areal = r * omega_value / (2.0 * d) * one_minus_trig(eta, 1.0);
		} else {
			const double scale = omega_value / (2.0 * h0 * (-d) * std::sqrt(-d));
			const double eta = solve_eta(std::min(tau / scale, std::numbers::pi_v<double>), -1.0);
			areal = r * omega_value / (2.0 * (-d)) * one_minus_trig(eta, -1.0);
		}
		result.radius = std::max(areal, 1.0e-12);
		result.velocity = std::sqrt(std::max(h0 * h0 * omega_value * r * r * r / result.radius + result.curvature, 0.0));
		result.regular = true;
		return result;
	}

	[[nodiscard]] Derivatives derivatives(double r, double t) const noexcept {
		const double h = 2.0e-3 * std::max(r, 1.0);
		const double rc = std::max(r, 4.0 * h);
		const Shell at_point = shell(std::max(r, 1.0e-9), t);
		const Shell c = shell(rc, t);
		const Shell p1 = shell(rc + h, t);
		const Shell m1 = shell(rc - h, t);
		const Shell p2 = shell(rc + 2.0 * h, t);
		const Shell m2 = shell(rc - 2.0 * h, t);
		Derivatives d;
		d.regular = at_point.regular && c.regular && p1.regular && m1.regular && p2.regular && m2.regular;
		d.radius = at_point.radius;
		d.velocity = at_point.velocity;
		d.curvature = at_point.curvature;
		const double h0 = parameters_.hubble0;
		d.curvature_r = h0 * h0 * (-omega_derivative(std::max(r, 1.0e-9)) * r * r + 2.0 * (1.0 - omega(std::max(r, 1.0e-9))) * r);
		if (!d.regular) {
			return d;
		}
		d.radius_r = (8.0 * (p1.radius - m1.radius) - (p2.radius - m2.radius)) / (12.0 * h);
		d.velocity_r = (8.0 * (p1.velocity - m1.velocity) - (p2.velocity - m2.velocity)) / (12.0 * h);
		d.radius_rr = (-p2.radius + 16.0 * p1.radius - 30.0 * c.radius + 16.0 * m1.radius - m2.radius) / (12.0 * h * h);
		return d;
	}

private:
	LtbParameters parameters_;
	double present_time_;

	[[nodiscard]] static double eta_minus_trig(double eta, double s) noexcept {
		const double e2 = eta * eta;
		if (e2 < 0.04) {
			return eta * e2 / 6.0 * (1.0 + s * e2 / 20.0 * (1.0 + s * e2 / 42.0 * (1.0 + s * e2 / 72.0)));
		}
		return (s > 0.0) ? (std::sinh(eta) - eta) : (eta - std::sin(eta));
	}

	[[nodiscard]] static double one_minus_trig(double eta, double s) noexcept {
		const double e2 = eta * eta;
		if (e2 < 0.04) {
			return 0.5 * e2 * (1.0 + s * e2 / 12.0 * (1.0 + s * e2 / 30.0 * (1.0 + s * e2 / 56.0)));
		}
		return (s > 0.0) ? (std::cosh(eta) - 1.0) : (1.0 - std::cos(eta));
	}

	[[nodiscard]] static double solve_eta(double y, double s) noexcept {
		if (!(y > 0.0)) {
			return 0.0;
		}
		const double bound = std::cbrt(6.0 * y);
		double eta = (s > 0.0) ? std::min(bound, std::log(2.0 * y + 2.0 * bound + 1.0)) : std::numbers::pi_v<double>;
		for (int iteration = 0; iteration < 60; ++iteration) {
			const double slope = one_minus_trig(eta, s);
			if (slope < 1.0e-18) {
				break;
			}
			const double step = (eta_minus_trig(eta, s) - y) / slope;
			eta = std::max(eta - step, 0.0);
			if (std::abs(step) < 1.0e-14 * std::max(eta, 1.0e-3)) {
				break;
			}
		}
		return eta;
	}
};

}
