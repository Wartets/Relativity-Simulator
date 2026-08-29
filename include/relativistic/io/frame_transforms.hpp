#pragma once

#include "relativistic/core/tensor.hpp"
#include "relativistic/core/tensor_ops.hpp"
#include "relativistic/io/ephemeris_types.hpp"
#include <cmath>
#include <array>

namespace Relativistic::IO {

enum class ReferenceFrame : uint32_t {
	ICRF_J2000_Barycentric = 0,
	Heliocentric_Ecliptic_J2000 = 1,
	Geocentric_Equatorial_J2000 = 2,
	BodyFixed_Planetocentric = 3,
	LocalSpacetime_Simulation = 4
};

template <typename Scalar = double>
class FrameTransformer {
public:
	static constexpr Scalar J2000_OBLIQUITY_RAD = static_cast<Scalar>(23.43929111 * std::numbers::pi_v<double> / 180.0);

	[[nodiscard]] static Core::MetricTensor<Scalar> icrf_to_ecliptic_matrix() noexcept {
		const Scalar eps = J2000_OBLIQUITY_RAD;
		const Scalar cos_e = std::cos(eps);
		const Scalar sin_e = std::sin(eps);

		Core::MetricTensor<Scalar> r;
		r.zero();
		r(0, 0) = static_cast<Scalar>(1);
		r(1, 1) = static_cast<Scalar>(1);
		r(2, 2) = cos_e;
		r(2, 3) = sin_e;
		r(3, 2) = -sin_e;
		r(3, 3) = cos_e;
		return r;
	}

	[[nodiscard]] static Core::MetricTensor<Scalar> ecliptic_to_icrf_matrix() noexcept {
		const Scalar eps = J2000_OBLIQUITY_RAD;
		const Scalar cos_e = std::cos(eps);
		const Scalar sin_e = std::sin(eps);

		Core::MetricTensor<Scalar> r;
		r.zero();
		r(0, 0) = static_cast<Scalar>(1);
		r(1, 1) = static_cast<Scalar>(1);
		r(2, 2) = cos_e;
		r(2, 3) = -sin_e;
		r(3, 2) = sin_e;
		r(3, 3) = cos_e;
		return r;
	}

	[[nodiscard]] static Core::FourVector<Scalar> transform_vector(
		const Core::FourVector<Scalar>& v,
		const Core::MetricTensor<Scalar>& mat
	) noexcept {
		Core::FourVector<Scalar> res;
		res.zero();
		for (size_t i = 0; i < 4; ++i) {
			for (size_t j = 0; j < 4; ++j) {
				res(i) += mat(i, j) * v(j);
			}
		}
		return res;
	}

	[[nodiscard]] static EphemerisStateVector transform_icrf_to_ecliptic(const EphemerisStateVector& state) noexcept {
		const auto r_mat = icrf_to_ecliptic_matrix();
		const auto p_ecl = transform_vector(state.position, r_mat);
		const auto v_ecl = transform_vector(state.velocity, r_mat);

		EphemerisStateVector result;
		result.epoch = state.epoch;
		result.position = p_ecl;
		result.velocity = v_ecl;
		result.body_id = state.body_id;
		result.center_id = state.center_id;
		return result;
	}

	[[nodiscard]] static EphemerisStateVector transform_ecliptic_to_icrf(const EphemerisStateVector& state) noexcept {
		const auto r_mat = ecliptic_to_icrf_matrix();
		const auto p_icrf = transform_vector(state.position, r_mat);
		const auto v_icrf = transform_vector(state.velocity, r_mat);

		EphemerisStateVector result;
		result.epoch = state.epoch;
		result.position = p_icrf;
		result.velocity = v_icrf;
		result.body_id = state.body_id;
		result.center_id = state.center_id;
		return result;
	}

	[[nodiscard]] static Core::MetricTensor<Scalar> lorentz_boost_matrix(
		Scalar beta_x,
		Scalar beta_y,
		Scalar beta_z
	) noexcept {
		const Scalar b2 = beta_x * beta_x + beta_y * beta_y + beta_z * beta_z;
		Core::MetricTensor<Scalar> lambda;
		lambda.zero();

		if (b2 <= static_cast<Scalar>(0.0)) {
			for (size_t i = 0; i < 4; ++i) lambda(i, i) = static_cast<Scalar>(1);
			return lambda;
		}

		const Scalar gamma = static_cast<Scalar>(1.0) / std::sqrt(std::max(static_cast<Scalar>(1.0) - b2, static_cast<Scalar>(1e-15)));
		const Scalar gamma_factor = (gamma - static_cast<Scalar>(1.0)) / b2;

		lambda(0, 0) = gamma;
		lambda(0, 1) = -gamma * beta_x;
		lambda(0, 2) = -gamma * beta_y;
		lambda(0, 3) = -gamma * beta_z;

		lambda(1, 0) = -gamma * beta_x;
		lambda(1, 1) = static_cast<Scalar>(1.0) + gamma_factor * beta_x * beta_x;
		lambda(1, 2) = gamma_factor * beta_x * beta_y;
		lambda(1, 3) = gamma_factor * beta_x * beta_z;

		lambda(2, 0) = -gamma * beta_y;
		lambda(2, 1) = gamma_factor * beta_y * beta_x;
		lambda(2, 2) = static_cast<Scalar>(1.0) + gamma_factor * beta_y * beta_y;
		lambda(2, 3) = gamma_factor * beta_y * beta_z;

		lambda(3, 0) = -gamma * beta_z;
		lambda(3, 1) = gamma_factor * beta_z * beta_x;
		lambda(3, 2) = gamma_factor * beta_z * beta_y;
		lambda(3, 3) = static_cast<Scalar>(1.0) + gamma_factor * beta_z * beta_z;

		return lambda;
	}
};

}
