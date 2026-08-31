#pragma once

#include "relativistic/gravimetry/spherical_harmonics.hpp"
#include <array>
#include <cmath>
#include <numbers>
#include <algorithm>

namespace Relativistic::Gravimetry {

struct LoveNumbers {
	double k2{0.301};
	double k3{0.093};
	double k22{0.301};
	double h2{0.609};
	double l2{0.085};
	double phase_lag_rad{0.0};
};

class TidalPerturbationModel {
private:
	LoveNumbers love_numbers_{};

	static constexpr double dot3(const std::array<double, 3>& a, const std::array<double, 3>& b) noexcept {
		return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
	}

public:
	constexpr TidalPerturbationModel() noexcept = default;

	explicit constexpr TidalPerturbationModel(const LoveNumbers& love) noexcept
		: love_numbers_(love) {}

	[[nodiscard]] constexpr const LoveNumbers& love_numbers() const noexcept {
		return love_numbers_;
	}

	void set_love_numbers(const LoveNumbers& love) noexcept {
		love_numbers_ = love;
	}

	template <size_t MaxDegree>
	void apply_third_body_tide(
		SphericalHarmonicsGravityModel<MaxDegree>& model,
		const std::array<double, 3>& third_body_pos,
		double third_body_gm
	) const noexcept {
		const double r3_sq = dot3(third_body_pos, third_body_pos);
		if (r3_sq <= 0.0) return;

		const double r3 = std::sqrt(r3_sq);
		const double r_ref = model.reference_radius();
		const double body_gm = model.gravitational_parameter();

		const double ratio_gm = third_body_gm / body_gm;
		const double r_ratio = r_ref / r3;
		const double r_ratio3 = r_ratio * r_ratio * r_ratio;

		const double x3 = third_body_pos[0];
		const double y3 = third_body_pos[1];
		const double z3 = third_body_pos[2];
		const double r3_xy = std::sqrt(x3 * x3 + y3 * y3);

		const double sin_phi3 = z3 / r3;
		const double cos_phi3 = (r3 > 0.0) ? (r3_xy / r3) : 1.0;
		const double lambda3 = (r3_xy > 0.0) ? std::atan2(y3, x3) : 0.0;

		const double factor2 = (love_numbers_.k2 / 5.0) * ratio_gm * r_ratio3;

		const double c20_tide = factor2 * (3.0 * sin_phi3 * sin_phi3 - 1.0) * 0.5 * std::sqrt(5.0);
		const double c21_tide = factor2 * std::sqrt(15.0) * sin_phi3 * cos_phi3 * std::cos(lambda3);
		const double s21_tide = factor2 * std::sqrt(15.0) * sin_phi3 * cos_phi3 * std::sin(lambda3);
		const double c22_tide = factor2 * std::sqrt(15.0 / 4.0) * cos_phi3 * cos_phi3 * std::cos(2.0 * lambda3);
		const double s22_tide = factor2 * std::sqrt(15.0 / 4.0) * cos_phi3 * cos_phi3 * std::sin(2.0 * lambda3);

		model.set_normalized_coefficient(2, 0, model.normalized_c(2, 0) + c20_tide, 0.0);
		model.set_normalized_coefficient(2, 1, model.normalized_c(2, 1) + c21_tide, model.normalized_s(2, 1) + s21_tide);
		model.set_normalized_coefficient(2, 2, model.normalized_c(2, 2) + c22_tide, model.normalized_s(2, 2) + s22_tide);
	}

	[[nodiscard]] static std::array<double, 3> direct_tidal_acceleration(
		const std::array<double, 3>& sat_pos,
		const std::array<double, 3>& third_body_pos,
		double third_body_gm
	) noexcept {
		const double r3_sq = dot3(third_body_pos, third_body_pos);
		if (r3_sq <= 0.0) return {0.0, 0.0, 0.0};
		const double r3 = std::sqrt(r3_sq);
		const double r3_cube = r3_sq * r3;

		const std::array<double, 3> d_vec = {
			third_body_pos[0] - sat_pos[0],
			third_body_pos[1] - sat_pos[1],
			third_body_pos[2] - sat_pos[2]
		};
		const double d_sq = dot3(d_vec, d_vec);
		if (d_sq <= 0.0) return {0.0, 0.0, 0.0};
		const double d = std::sqrt(d_sq);
		const double d_cube = d_sq * d;

		const double factor_d = third_body_gm / d_cube;
		const double factor_r3 = third_body_gm / r3_cube;

		return {
			factor_d * d_vec[0] - factor_r3 * third_body_pos[0],
			factor_d * d_vec[1] - factor_r3 * third_body_pos[1],
			factor_d * d_vec[2] - factor_r3 * third_body_pos[2]
		};
	}

	[[nodiscard]] static double rotational_flattening_j2(
		double angular_velocity_rad_s,
		double equatorial_radius,
		double gm,
		double k2_fluid = 0.938
	) noexcept {
		const double omega2 = angular_velocity_rad_s * angular_velocity_rad_s;
		const double r3 = equatorial_radius * equatorial_radius * equatorial_radius;
		const double m_rot = (omega2 * r3) / gm;
		return (1.0 / 3.0) * k2_fluid * m_rot;
	}
};

}
