#pragma once

#include "relativistic/core/constants.hpp"
#include "relativistic/core/pcg64.hpp"
#include <cmath>
#include <numbers>
#include <algorithm>
#include <concepts>
#include <array>
#include <cstdint>

namespace Relativistic::Optics {

class RelativisticBessel {
public:
	[[nodiscard]] static double k0(double x) noexcept {
		if (x <= 0.0) return 1e30;
		if (x <= 2.0) {
			const double y = x * x / 4.0;
			const double i0 = 1.0 + y * (1.0 + y * (0.25 + y * (1.0 / 36.0 + y * (1.0 / 576.0 + y * (1.0 / 14400.0)))));
			const double p = -0.5772156649015329 + y * (0.42278420 + y * (0.23069756 + y * (0.03488590 + y * (0.00262698 + y * 0.00010750))));
			return -std::log(x / 2.0) * i0 + p;
		}
		const double y = 2.0 / x;
		const double p = 1.25331414 + y * (-0.07832358 + y * (0.02189568 + y * (-0.01062446 + y * (0.00587872 - y * 0.00251540))));
		return (std::exp(-x) / std::sqrt(x)) * p;
	}

	[[nodiscard]] static double k1(double x) noexcept {
		if (x <= 0.0) return 1e30;
		if (x <= 2.0) {
			const double y = x * x / 4.0;
			const double i1 = (x / 2.0) * (1.0 + y * (0.5 + y * (1.0 / 12.0 + y * (1.0 / 144.0 + y * (1.0 / 2880.0)))));
			const double p = 1.0 + y * (0.15443144 + y * (-0.67278579 + y * (-0.18156897 + y * (-0.01919402 + y * -0.00110404))));
			return std::log(x / 2.0) * i1 + (1.0 / x) * p;
		}
		const double y = 2.0 / x;
		const double p = 1.25331414 + y * (0.23498619 + y * (-0.03655620 + y * (0.01504268 + y * (-0.00780353 + y * 0.00325614))));
		return (std::exp(-x) / std::sqrt(x)) * p;
	}

	[[nodiscard]] static double k2(double x) noexcept {
		if (x <= 0.0) return 1e30;
		return (2.0 / x) * k1(x) + k0(x);
	}

	[[nodiscard]] static double k3(double x) noexcept {
		if (x <= 0.0) return 1e30;
		return (4.0 / x) * k2(x) + k1(x);
	}
};

template <typename Scalar = double>
class MaxwellJuttnerDistribution {
private:
	Scalar theta_e_{static_cast<Scalar>(1.0)};
	Scalar k2_val_{static_cast<Scalar>(1.0)};
	Scalar k1_val_{static_cast<Scalar>(1.0)};
	Scalar mean_gamma_{static_cast<Scalar>(1.0)};

	void update_cache() noexcept {
		const double z = 1.0 / static_cast<double>(theta_e_);
		k2_val_ = static_cast<Scalar>(RelativisticBessel::k2(z));
		k1_val_ = static_cast<Scalar>(RelativisticBessel::k1(z));
		mean_gamma_ = (k1_val_ / k2_val_) + static_cast<Scalar>(3.0) * theta_e_;
	}

public:
	constexpr MaxwellJuttnerDistribution() noexcept {
		update_cache();
	}

	explicit MaxwellJuttnerDistribution(Scalar dimensionless_temperature) noexcept
		: theta_e_(std::max(dimensionless_temperature, static_cast<Scalar>(1e-6))) {
		update_cache();
	}

	[[nodiscard]] static MaxwellJuttnerDistribution from_temperature_kelvin(Scalar temperature_k) noexcept {
		constexpr double kb = Core::PhysicalConstants<double>::BOLTZMANN_CONSTANT;
		constexpr double me = Core::PhysicalConstants<double>::ELECTRON_MASS;
		constexpr double c = Core::PhysicalConstants<double>::SPEED_OF_LIGHT;
		const Scalar theta = static_cast<Scalar>((kb * static_cast<double>(temperature_k)) / (me * c * c));
		return MaxwellJuttnerDistribution(theta);
	}

	[[nodiscard]] constexpr Scalar dimensionless_temperature() const noexcept {
		return theta_e_;
	}

	[[nodiscard]] constexpr Scalar mean_lorentz_factor() const noexcept {
		return mean_gamma_;
	}

	[[nodiscard]] Scalar pdf(Scalar gamma) const noexcept {
		if (gamma < static_cast<Scalar>(1.0)) return static_cast<Scalar>(0.0);
		const Scalar p = std::sqrt(std::max(gamma * gamma - static_cast<Scalar>(1.0), static_cast<Scalar>(0.0)));
		const Scalar num = gamma * p * std::exp(-gamma / theta_e_);
		const Scalar den = theta_e_ * k2_val_;
		return (den > static_cast<Scalar>(1e-30)) ? (num / den) : static_cast<Scalar>(0.0);
	}

	[[nodiscard]] Scalar sample_lorentz_factor(Core::PCG64Engine& rng) const noexcept {
		const double th = static_cast<double>(theta_e_);
		const double sqrt_th = std::sqrt(th);
		const double th15 = th * sqrt_th;

		const double sqrt_pi_2 = std::sqrt(std::numbers::pi / 2.0);
		const double w1 = sqrt_pi_2;
		const double w2 = sqrt_th;
		const double w3 = 1.5 * sqrt_pi_2 * th;
		const double w4 = 2.0 * th15;
		const double w_tot = w1 + w2 + w3 + w4;

		for (;;) {
			const double r = rng.next_uniform_double() * w_tot;
			double x = 0.0;

			if (r < w1) {
				const auto [z, z2] = rng.next_gaussian_pair();
				static_cast<void>(z2);
				x = -std::log(std::max(rng.next_uniform_double(), 1e-15)) + 0.5 * z * z;
			} else if (r < w1 + w2) {
				const double u1 = rng.next_uniform_double();
				const double u2 = rng.next_uniform_double();
				x = -std::log(std::max(u1 * u2, 1e-15));
			} else if (r < w1 + w2 + w3) {
				const double u1 = rng.next_uniform_double();
				const double u2 = rng.next_uniform_double();
				const auto [z, z2] = rng.next_gaussian_pair();
				static_cast<void>(z2);
				x = -std::log(std::max(u1 * u2, 1e-15)) + 0.5 * z * z;
			} else {
				const double u1 = rng.next_uniform_double();
				const double u2 = rng.next_uniform_double();
				const double u3 = rng.next_uniform_double();
				x = -std::log(std::max(u1 * u2 * u3, 1e-15));
			}

			const double sqrt_x = std::sqrt(std::max(x, 0.0));
			const double num = (1.0 + th * x) * std::sqrt(std::max(x * (2.0 + th * x), 0.0));
			const double den = std::numbers::sqrt2_v<double> * sqrt_x + sqrt_th * x + std::numbers::sqrt2_v<double> * th * x * sqrt_x + th15 * x * x;

			if (den > 0.0) {
				const double accept_prob = num / den;
				if (rng.next_uniform_double() <= accept_prob) {
					return static_cast<Scalar>(1.0 + th * x);
				}
			}
		}
	}

	[[nodiscard]] std::array<Scalar, 3> sample_velocity_vector(Core::PCG64Engine& rng) const noexcept {
		const Scalar gamma = sample_lorentz_factor(rng);
		const Scalar beta = std::sqrt(std::max(static_cast<Scalar>(1.0) - static_cast<Scalar>(1.0) / (gamma * gamma), static_cast<Scalar>(0.0)));

		const double cos_t = 2.0 * rng.next_uniform_double() - 1.0;
		const double sin_t = std::sqrt(std::max(1.0 - cos_t * cos_t, 0.0));
		const double phi = 2.0 * std::numbers::pi_v<double> * rng.next_uniform_double();

		return {
			beta * static_cast<Scalar>(sin_t * std::cos(phi)),
			beta * static_cast<Scalar>(sin_t * std::sin(phi)),
			beta * static_cast<Scalar>(cos_t)
		};
	}

	[[nodiscard]] Scalar line_profile_kernel(Scalar nu, Scalar nu_0) const noexcept {
		if (nu <= static_cast<Scalar>(0.0) || nu_0 <= static_cast<Scalar>(0.0)) return static_cast<Scalar>(0.0);
		const Scalar y = (nu - nu_0) / nu_0;
		const Scalar sigma_th = std::sqrt(theta_e_);
		const Scalar denom = std::sqrt(static_cast<Scalar>(2.0) * std::numbers::pi_v<Scalar>) * sigma_th * nu_0;
		return std::exp(-(y * y) / (static_cast<Scalar>(2.0) * theta_e_)) / denom;
	}
};

}
