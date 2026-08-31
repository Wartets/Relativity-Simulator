#pragma once

#include "relativistic/core/constants.hpp"
#include <array>
#include <cmath>
#include <numbers>
#include <algorithm>
#include <concepts>

namespace Relativistic::ModifiedGravity {

enum class MondInterpolationFunction : uint32_t {
	Standard = 0,
	Simple = 1,
	Exponential = 2,
	Bekenstein = 3
};

template <typename Scalar = double>
class MondFramework {
public:
	static constexpr Scalar DEFAULT_A0 = static_cast<Scalar>(1.2e-10);

private:
	Scalar a0_{DEFAULT_A0};
	Scalar g_const_{static_cast<Scalar>(Core::PhysicalConstants<double>::GRAVITATIONAL_CONSTANT)};
	MondInterpolationFunction function_type_{MondInterpolationFunction::Standard};

public:
	constexpr MondFramework() noexcept = default;

	explicit constexpr MondFramework(
		Scalar critical_acceleration = DEFAULT_A0,
		MondInterpolationFunction func = MondInterpolationFunction::Standard,
		Scalar g = static_cast<Scalar>(Core::PhysicalConstants<double>::GRAVITATIONAL_CONSTANT)
	) noexcept
		: a0_(critical_acceleration), g_const_(g), function_type_(func) {}

	[[nodiscard]] constexpr Scalar critical_acceleration() const noexcept { return a0_; }
	[[nodiscard]] constexpr Scalar gravitational_constant() const noexcept { return g_const_; }
	[[nodiscard]] constexpr MondInterpolationFunction function_type() const noexcept { return function_type_; }

	void set_critical_acceleration(Scalar a0) noexcept { a0_ = a0; }
	void set_function_type(MondInterpolationFunction func) noexcept { function_type_ = func; }

	[[nodiscard]] Scalar mu(Scalar x) const noexcept {
		const Scalar x_safe = std::max(x, static_cast<Scalar>(0.0));
		switch (function_type_) {
			case MondInterpolationFunction::Simple:
				return x_safe / (static_cast<Scalar>(1.0) + x_safe);
			case MondInterpolationFunction::Exponential:
				return static_cast<Scalar>(1.0) - std::exp(-x_safe);
			case MondInterpolationFunction::Bekenstein: {
				const Scalar sqrt_x = std::sqrt(x_safe);
				const Scalar denom = static_cast<Scalar>(1.0) - std::exp(-sqrt_x);
				return (denom > static_cast<Scalar>(1e-12)) ? (x_safe / denom) : static_cast<Scalar>(1.0);
			}
			case MondInterpolationFunction::Standard:
			default:
				return x_safe / std::sqrt(static_cast<Scalar>(1.0) + x_safe * x_safe);
		}
	}

	[[nodiscard]] Scalar nu(Scalar y) const noexcept {
		const Scalar y_safe = std::max(y, static_cast<Scalar>(1e-30));
		switch (function_type_) {
			case MondInterpolationFunction::Simple:
				return static_cast<Scalar>(0.5) * (static_cast<Scalar>(1.0) + std::sqrt(static_cast<Scalar>(1.0) + static_cast<Scalar>(4.0) / y_safe));
			case MondInterpolationFunction::Standard:
			default:
				return std::sqrt(static_cast<Scalar>(0.5) * (static_cast<Scalar>(1.0) + std::sqrt(static_cast<Scalar>(1.0) + static_cast<Scalar>(4.0) / (y_safe * y_safe))));
		}
	}

	[[nodiscard]] Scalar compute_mond_acceleration(Scalar a_newton) const noexcept {
		const Scalar a_n = std::abs(a_newton);
		if (a_n <= static_cast<Scalar>(1e-30)) {
			return static_cast<Scalar>(0.0);
		}

		const Scalar y = a_n / a0_;

		switch (function_type_) {
			case MondInterpolationFunction::Simple: {
				const Scalar a_mond = static_cast<Scalar>(0.5) * (a_n + std::sqrt(a_n * a_n + static_cast<Scalar>(4.0) * a_n * a0_));
				return (a_newton < static_cast<Scalar>(0.0)) ? -a_mond : a_mond;
			}
			case MondInterpolationFunction::Standard: {
				const Scalar term = static_cast<Scalar>(1.0) + std::sqrt(static_cast<Scalar>(1.0) + static_cast<Scalar>(4.0) / (y * y));
				const Scalar a_mond = a_n * std::sqrt(static_cast<Scalar>(0.5) * term);
				return (a_newton < static_cast<Scalar>(0.0)) ? -a_mond : a_mond;
			}
			case MondInterpolationFunction::Exponential: {
				Scalar x = std::sqrt(y);
				for (int iter = 0; iter < 12; ++iter) {
					const Scalar mu_val = static_cast<Scalar>(1.0) - std::exp(-x);
					const Scalar f = x * mu_val - y;
					const Scalar df = mu_val + x * std::exp(-x);
					const Scalar dx = f / (df + static_cast<Scalar>(1e-30));
					x -= dx;
					if (std::abs(dx) < static_cast<Scalar>(1e-14)) break;
				}
				const Scalar a_mond = x * a0_;
				return (a_newton < static_cast<Scalar>(0.0)) ? -a_mond : a_mond;
			}
			case MondInterpolationFunction::Bekenstein: {
				const Scalar a_mond = a_n / (static_cast<Scalar>(1.0) - std::exp(-std::sqrt(std::max(y, static_cast<Scalar>(1e-15)))));
				return (a_newton < static_cast<Scalar>(0.0)) ? -a_mond : a_mond;
			}
			default:
				return a_newton;
		}
	}

	[[nodiscard]] std::array<Scalar, 3> compute_mond_acceleration_3d(const std::array<Scalar, 3>& a_newton) const noexcept {
		const Scalar a_n2 = a_newton[0] * a_newton[0] + a_newton[1] * a_newton[1] + a_newton[2] * a_newton[2];
		if (a_n2 <= static_cast<Scalar>(0.0)) {
			return {static_cast<Scalar>(0.0), static_cast<Scalar>(0.0), static_cast<Scalar>(0.0)};
		}
		const Scalar a_n = std::sqrt(a_n2);
		const Scalar a_mond = std::abs(compute_mond_acceleration(a_n));
		const Scalar factor = a_mond / a_n;
		return {
			a_newton[0] * factor,
			a_newton[1] * factor,
			a_newton[2] * factor
		};
	}

	[[nodiscard]] Scalar asymptotic_flat_velocity(Scalar baryonic_mass) const noexcept {
		return std::sqrt(std::sqrt(g_const_ * baryonic_mass * a0_));
	}

	[[nodiscard]] Scalar point_mass_circular_velocity(Scalar baryonic_mass, Scalar r) const noexcept {
		const Scalar r_safe = std::max(r, static_cast<Scalar>(1e-12));
		const Scalar a_n = g_const_ * baryonic_mass / (r_safe * r_safe);
		const Scalar a_mond = std::abs(compute_mond_acceleration(a_n));
		return std::sqrt(r_safe * a_mond);
	}

	[[nodiscard]] Scalar exponential_disk_baryonic_acceleration(Scalar disk_mass, Scalar scale_length, Scalar r) const noexcept {
		const Scalar r_safe = std::max(r, static_cast<Scalar>(1e-12));
		const Scalar y = r_safe / (static_cast<Scalar>(2.0) * scale_length);
		const Scalar m_enc = disk_mass * (static_cast<Scalar>(1.0) - (static_cast<Scalar>(1.0) + r_safe / scale_length) * std::exp(-r_safe / scale_length));
		return g_const_ * m_enc / (r_safe * r_safe);
	}

	[[nodiscard]] Scalar galaxy_rotation_curve(
		Scalar disk_mass,
		Scalar disk_scale_length,
		Scalar bulge_mass,
		Scalar bulge_scale_radius,
		Scalar gas_mass,
		Scalar gas_scale_length,
		Scalar r
	) const noexcept {
		const Scalar r_safe = std::max(r, static_cast<Scalar>(1e-12));

		const Scalar a_disk = exponential_disk_baryonic_acceleration(disk_mass, disk_scale_length, r_safe);
		const Scalar a_gas = exponential_disk_baryonic_acceleration(gas_mass, gas_scale_length, r_safe);
		const Scalar a_bulge = (bulge_mass > static_cast<Scalar>(0.0)) ? (g_const_ * bulge_mass / ((r_safe + bulge_scale_radius) * (r_safe + bulge_scale_radius))) : static_cast<Scalar>(0.0);

		const Scalar a_n_total = a_disk + a_gas + a_bulge;
		const Scalar a_mond = std::abs(compute_mond_acceleration(a_n_total));
		return std::sqrt(r_safe * a_mond);
	}
};

}
