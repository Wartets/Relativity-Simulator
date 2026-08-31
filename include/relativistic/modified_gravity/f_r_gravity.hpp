#pragma once

#include "relativistic/core/constants.hpp"
#include <array>
#include <cmath>
#include <numbers>
#include <algorithm>
#include <concepts>

namespace Relativistic::ModifiedGravity {

enum class F_R_ModelType : uint32_t {
	HuSawicki = 0,
	Starobinsky = 1,
	ExponentialGravity = 2
};

template <typename Scalar = double>
struct HuSawickiParams {
	Scalar f_r0{static_cast<Scalar>(1e-6)};
	Scalar n_index{static_cast<Scalar>(1.0)};
	Scalar r_background0{static_cast<Scalar>(1e-52)};
};

template <typename Scalar = double>
struct StarobinskyFRParams {
	Scalar lambda{static_cast<Scalar>(1.0)};
	Scalar r_ch{static_cast<Scalar>(1e-52)};
	Scalar n_index{static_cast<Scalar>(1.0)};
};

template <typename Scalar = double>
class FRChameleonModel {
private:
	F_R_ModelType model_type_{F_R_ModelType::HuSawicki};
	HuSawickiParams<Scalar> hs_params_{};
	StarobinskyFRParams<Scalar> star_params_{};
	Scalar g_const_{static_cast<Scalar>(Core::PhysicalConstants<double>::GRAVITATIONAL_CONSTANT)};
	Scalar c_{static_cast<Scalar>(Core::PhysicalConstants<double>::SPEED_OF_LIGHT)};

public:
	constexpr FRChameleonModel() noexcept = default;

	explicit constexpr FRChameleonModel(
		const HuSawickiParams<Scalar>& params,
		Scalar g = static_cast<Scalar>(Core::PhysicalConstants<double>::GRAVITATIONAL_CONSTANT),
		Scalar c = static_cast<Scalar>(Core::PhysicalConstants<double>::SPEED_OF_LIGHT)
	) noexcept
		: model_type_(F_R_ModelType::HuSawicki), hs_params_(params), g_const_(g), c_(c) {}

	explicit constexpr FRChameleonModel(
		const StarobinskyFRParams<Scalar>& params,
		Scalar g = static_cast<Scalar>(Core::PhysicalConstants<double>::GRAVITATIONAL_CONSTANT),
		Scalar c = static_cast<Scalar>(Core::PhysicalConstants<double>::SPEED_OF_LIGHT)
	) noexcept
		: model_type_(F_R_ModelType::Starobinsky), star_params_(params), g_const_(g), c_(c) {}

	[[nodiscard]] constexpr F_R_ModelType model_type() const noexcept { return model_type_; }
	[[nodiscard]] constexpr Scalar gravitational_constant() const noexcept { return g_const_; }
	[[nodiscard]] constexpr Scalar speed_of_light() const noexcept { return c_; }

	[[nodiscard]] Scalar scalaron_f_R(Scalar curvature_r) const noexcept {
		const Scalar r_safe = std::max(curvature_r, static_cast<Scalar>(1e-60));
		if (model_type_ == F_R_ModelType::HuSawicki) {
			const Scalar ratio = hs_params_.r_background0 / r_safe;
			return -hs_params_.n_index * hs_params_.f_r0 * std::pow(ratio, hs_params_.n_index + static_cast<Scalar>(1.0));
		} else {
			const Scalar x = r_safe / star_params_.r_ch;
			const Scalar x2 = x * x;
			const Scalar denom = std::pow(static_cast<Scalar>(1.0) + x2, star_params_.n_index + static_cast<Scalar>(1.0));
			return -static_cast<Scalar>(2.0) * star_params_.lambda * star_params_.n_index * x / denom;
		}
	}

	[[nodiscard]] Scalar d2f_dR2(Scalar curvature_r) const noexcept {
		const Scalar r_safe = std::max(curvature_r, static_cast<Scalar>(1e-60));
		if (model_type_ == F_R_ModelType::HuSawicki) {
			const Scalar n = hs_params_.n_index;
			const Scalar ratio = hs_params_.r_background0 / r_safe;
			return (n * (n + static_cast<Scalar>(1.0)) * hs_params_.f_r0 / hs_params_.r_background0) * std::pow(ratio, n + static_cast<Scalar>(2.0));
		} else {
			const Scalar x = r_safe / star_params_.r_ch;
			const Scalar n = star_params_.n_index;
			const Scalar denom = std::pow(static_cast<Scalar>(1.0) + x * x, n + static_cast<Scalar>(2.0));
			const Scalar num = static_cast<Scalar>(2.0) * star_params_.lambda * n * (static_cast<Scalar>(2.0) * (n + static_cast<Scalar>(1.0)) * x * x - (static_cast<Scalar>(1.0) + x * x));
			return (num / (star_params_.r_ch * star_params_.r_ch)) / denom;
		}
	}

	[[nodiscard]] Scalar scalaron_effective_mass(Scalar matter_density) const noexcept {
		const Scalar c2 = c_ * c_;
		const Scalar curvature_r = (static_cast<Scalar>(8.0) * std::numbers::pi_v<Scalar> * g_const_ * matter_density) / c2;
		const Scalar f_rr = std::abs(d2f_dR2(curvature_r));
		if (f_rr <= static_cast<Scalar>(1e-60)) {
			return static_cast<Scalar>(1e30);
		}
		return std::sqrt(static_cast<Scalar>(1.0) / (static_cast<Scalar>(3.0) * f_rr));
	}

	[[nodiscard]] Scalar thin_shell_factor(
		Scalar body_mass,
		Scalar body_radius,
		Scalar env_density,
		Scalar core_density
	) const noexcept {
		const Scalar c2 = c_ * c_;
		const Scalar r_core = (static_cast<Scalar>(8.0) * std::numbers::pi_v<Scalar> * g_const_ * core_density) / c2;
		const Scalar r_env = (static_cast<Scalar>(8.0) * std::numbers::pi_v<Scalar> * g_const_ * env_density) / c2;

		const Scalar f_r_core = scalaron_f_R(r_core);
		const Scalar f_r_env = scalaron_f_R(r_env);

		const Scalar delta_f_r = std::abs(f_r_env - f_r_core);
		const Scalar phi_n = (g_const_ * body_mass) / (body_radius * c2);

		if (phi_n <= static_cast<Scalar>(1e-30)) {
			return static_cast<Scalar>(1.0);
		}

		const Scalar delta_r_over_r = delta_f_r / (static_cast<Scalar>(2.0) * phi_n);
		return std::clamp(delta_r_over_r, static_cast<Scalar>(0.0), static_cast<Scalar>(1.0));
	}

	[[nodiscard]] Scalar effective_gravitational_coupling(
		Scalar body_mass,
		Scalar body_radius,
		Scalar env_density,
		Scalar core_density
	) const noexcept {
		const Scalar delta_r_over_r = thin_shell_factor(body_mass, body_radius, env_density, core_density);
		const Scalar screening = std::min(static_cast<Scalar>(1.0), static_cast<Scalar>(3.0) * delta_r_over_r);
		return g_const_ * (static_cast<Scalar>(1.0) + (static_cast<Scalar>(1.0) / static_cast<Scalar>(3.0)) * screening);
	}

	[[nodiscard]] Scalar modified_radial_acceleration(
		Scalar body_mass,
		Scalar body_radius,
		Scalar r,
		Scalar env_density,
		Scalar core_density
	) const noexcept {
		const Scalar r_safe = std::max(r, static_cast<Scalar>(1e-12));
		const Scalar g_eff = effective_gravitational_coupling(body_mass, body_radius, env_density, core_density);
		const Scalar m_eff = (r_safe < body_radius) ? (body_mass * (r_safe * r_safe * r_safe) / (body_radius * body_radius * body_radius)) : body_mass;
		return -g_eff * m_eff / (r_safe * r_safe);
	}
};

}
