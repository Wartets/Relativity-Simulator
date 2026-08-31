#pragma once

#include "relativistic/gravimetry/legendre_table.hpp"
#include "relativistic/core/constants.hpp"
#include <array>
#include <vector>
#include <span>
#include <cmath>
#include <numbers>
#include <algorithm>
#include <string_view>

namespace Relativistic::Gravimetry {

template <size_t MaxDegree = 32>
class SphericalHarmonicsGravityModel {
	static_assert(MaxDegree >= 2, "MaxDegree must be at least 2");

public:
	static constexpr size_t MAX_DEGREE = MaxDegree;
	static constexpr size_t TOTAL_COEFFICIENTS = ((MaxDegree + 1) * (MaxDegree + 2)) / 2;

private:
	double gm_{3.986004418e14};
	double reference_radius_{6378137.0};
	size_t active_degree_{MaxDegree};

	std::array<double, TOTAL_COEFFICIENTS> c_bar_{};
	std::array<double, TOTAL_COEFFICIENTS> s_bar_{};
	AssociatedLegendreTable<MaxDegree> legendre_table_{};

	[[nodiscard]] static constexpr size_t flat_index(size_t n, size_t m) noexcept {
		return (n * (n + 1)) / 2 + m;
	}

public:
	constexpr SphericalHarmonicsGravityModel() noexcept {
		c_bar_[flat_index(0, 0)] = 1.0;
	}

	constexpr SphericalHarmonicsGravityModel(double gm, double r_ref, size_t degree = MaxDegree) noexcept
		: gm_(gm), reference_radius_(r_ref), active_degree_(std::min(degree, MaxDegree)) {
		c_bar_[flat_index(0, 0)] = 1.0;
	}

	void set_gravitational_parameter(double gm) noexcept {
		gm_ = gm;
	}

	[[nodiscard]] constexpr double gravitational_parameter() const noexcept {
		return gm_;
	}

	void set_reference_radius(double r_ref) noexcept {
		reference_radius_ = r_ref;
	}

	[[nodiscard]] constexpr double reference_radius() const noexcept {
		return reference_radius_;
	}

	void set_active_degree(size_t degree) noexcept {
		active_degree_ = std::min(degree, MaxDegree);
	}

	[[nodiscard]] constexpr size_t active_degree() const noexcept {
		return active_degree_;
	}

	void set_normalized_coefficient(size_t n, size_t m, double c_val, double s_val) noexcept {
		if (n > MaxDegree || m > n) return;
		const size_t idx = flat_index(n, m);
		c_bar_[idx] = c_val;
		s_bar_[idx] = s_val;
	}

	void set_unnormalized_coefficient(size_t n, size_t m, double c_unnorm, double s_unnorm) noexcept {
		if (n > MaxDegree || m > n) return;
		const double norm = legendre_table_.normalization_factor(n, m);
		const size_t idx = flat_index(n, m);
		c_bar_[idx] = (norm > 0.0) ? (c_unnorm / norm) : 0.0;
		s_bar_[idx] = (norm > 0.0) ? (s_unnorm / norm) : 0.0;
	}

	void set_zonal_coefficient(size_t n, double j_n) noexcept {
		if (n > MaxDegree) return;
		const double norm = legendre_table_.normalization_factor(n, 0);
		const size_t idx = flat_index(n, 0);
		c_bar_[idx] = (norm > 0.0) ? (-j_n / norm) : 0.0;
		s_bar_[idx] = 0.0;
	}

	[[nodiscard]] constexpr double normalized_c(size_t n, size_t m) const noexcept {
		if (n > MaxDegree || m > n) return 0.0;
		return c_bar_[flat_index(n, m)];
	}

	[[nodiscard]] constexpr double normalized_s(size_t n, size_t m) const noexcept {
		if (n > MaxDegree || m > n) return 0.0;
		return s_bar_[flat_index(n, m)];
	}

	[[nodiscard]] double unnormalized_c(size_t n, size_t m) const noexcept {
		if (n > MaxDegree || m > n) return 0.0;
		const double norm = legendre_table_.normalization_factor(n, m);
		return c_bar_[flat_index(n, m)] * norm;
	}

	[[nodiscard]] double unnormalized_s(size_t n, size_t m) const noexcept {
		if (n > MaxDegree || m > n) return 0.0;
		const double norm = legendre_table_.normalization_factor(n, m);
		return s_bar_[flat_index(n, m)] * norm;
	}

	[[nodiscard]] double zonal_j(size_t n) const noexcept {
		if (n > MaxDegree) return 0.0;
		const double norm = legendre_table_.normalization_factor(n, 0);
		return -c_bar_[flat_index(n, 0)] * norm;
	}

	[[nodiscard]] double evaluate_potential(const std::array<double, 3>& position) noexcept {
		const double x = position[0];
		const double y = position[1];
		const double z = position[2];

		const double r_xy_sq = x * x + y * y;
		const double r_sq = r_xy_sq + z * z;
		if (r_sq <= 0.0) return 0.0;

		const double r = std::sqrt(r_sq);
		const double r_xy = std::sqrt(r_xy_sq);
		const double sin_phi = z / r;
		const double cos_phi = (r > 0.0) ? (r_xy / r) : 1.0;

		double lambda = (r_xy > 0.0) ? std::atan2(y, x) : 0.0;
		if (lambda < 0.0) lambda += 2.0 * std::numbers::pi_v<double>;

		legendre_table_.compute(sin_phi, cos_phi, active_degree_);

		std::array<double, MaxDegree + 1> cos_m_lambda{};
		std::array<double, MaxDegree + 1> sin_m_lambda{};
		cos_m_lambda[0] = 1.0;
		sin_m_lambda[0] = 0.0;

		const double cos_l = (r_xy > 0.0) ? (x / r_xy) : 1.0;
		const double sin_l = (r_xy > 0.0) ? (y / r_xy) : 0.0;
		cos_m_lambda[1] = cos_l;
		sin_m_lambda[1] = sin_l;

		for (size_t m = 2; m <= active_degree_; ++m) {
			cos_m_lambda[m] = cos_m_lambda[1] * cos_m_lambda[m - 1] - sin_m_lambda[1] * sin_m_lambda[m - 1];
			sin_m_lambda[m] = sin_m_lambda[1] * cos_m_lambda[m - 1] + cos_m_lambda[1] * sin_m_lambda[m - 1];
		}

		std::array<double, MaxDegree + 1> r_ref_over_r{};
		r_ref_over_r[0] = 1.0;
		const double base_ratio = reference_radius_ / r;
		for (size_t n = 1; n <= active_degree_; ++n) {
			r_ref_over_r[n] = r_ref_over_r[n - 1] * base_ratio;
		}

		double harmonic_sum = 0.0;
		for (size_t n = 2; n <= active_degree_; ++n) {
			double degree_sum = 0.0;
			for (size_t m = 0; m <= n; ++m) {
				const size_t idx = flat_index(n, m);
				const double p_val = legendre_table_.p_bar(n, m);
				degree_sum += p_val * (c_bar_[idx] * cos_m_lambda[m] + s_bar_[idx] * sin_m_lambda[m]);
			}
			harmonic_sum += r_ref_over_r[n] * degree_sum;
		}

		return -(gm_ / r) * (1.0 + harmonic_sum);
	}

	[[nodiscard]] std::array<double, 3> evaluate_acceleration(const std::array<double, 3>& position) noexcept {
		const double x = position[0];
		const double y = position[1];
		const double z = position[2];

		const double r_xy_sq = x * x + y * y;
		const double r_sq = r_xy_sq + z * z;
		if (r_sq <= 0.0) return {0.0, 0.0, 0.0};

		const double r = std::sqrt(r_sq);
		const double r_xy = std::sqrt(r_xy_sq);
		const double sin_phi = z / r;
		const double cos_phi = (r > 0.0) ? (r_xy / r) : 1.0;
		const double safe_cos_phi = std::max(cos_phi, 1e-15);

		double lambda = (r_xy > 0.0) ? std::atan2(y, x) : 0.0;
		if (lambda < 0.0) lambda += 2.0 * std::numbers::pi_v<double>;

		legendre_table_.compute(sin_phi, cos_phi, active_degree_);

		std::array<double, MaxDegree + 1> cos_m_lambda{};
		std::array<double, MaxDegree + 1> sin_m_lambda{};
		cos_m_lambda[0] = 1.0;
		sin_m_lambda[0] = 0.0;

		const double cos_l = (r_xy > 0.0) ? (x / r_xy) : 1.0;
		const double sin_l = (r_xy > 0.0) ? (y / r_xy) : 0.0;
		cos_m_lambda[1] = cos_l;
		sin_m_lambda[1] = sin_l;

		for (size_t m = 2; m <= active_degree_; ++m) {
			cos_m_lambda[m] = cos_m_lambda[1] * cos_m_lambda[m - 1] - sin_m_lambda[1] * sin_m_lambda[m - 1];
			sin_m_lambda[m] = sin_m_lambda[1] * cos_m_lambda[m - 1] + cos_m_lambda[1] * sin_m_lambda[m - 1];
		}

		std::array<double, MaxDegree + 1> r_ref_over_r{};
		r_ref_over_r[0] = 1.0;
		const double base_ratio = reference_radius_ / r;
		for (size_t n = 1; n <= active_degree_; ++n) {
			r_ref_over_r[n] = r_ref_over_r[n - 1] * base_ratio;
		}

		double dV_dr_sum = 0.0;
		double dV_dphi_sum = 0.0;
		double dV_dlambda_sum = 0.0;

		for (size_t n = 2; n <= active_degree_; ++n) {
			double sum_r = 0.0;
			double sum_phi = 0.0;
			double sum_lambda = 0.0;

			for (size_t m = 0; m <= n; ++m) {
				const size_t idx = flat_index(n, m);
				const double c = c_bar_[idx];
				const double s = s_bar_[idx];
				const double cos_ml = cos_m_lambda[m];
				const double sin_ml = sin_m_lambda[m];

				const double trig_factor = c * cos_ml + s * sin_ml;
				const double d_trig_dlambda = static_cast<double>(m) * (-c * sin_ml + s * cos_ml);

				const double p_val = legendre_table_.p_bar(n, m);
				const double dp_val = legendre_table_.dp_bar_dphi(n, m);

				sum_r += static_cast<double>(n + 1) * p_val * trig_factor;
				sum_phi += dp_val * trig_factor;
				sum_lambda += p_val * d_trig_dlambda;
			}

			dV_dr_sum += r_ref_over_r[n] * sum_r;
			dV_dphi_sum += r_ref_over_r[n] * sum_phi;
			dV_dlambda_sum += r_ref_over_r[n] * sum_lambda;
		}

		const double gm_over_r2 = gm_ / (r * r);
		const double a_r = -gm_over_r2 * (1.0 + dV_dr_sum);
		const double a_phi = (gm_over_r2)*dV_dphi_sum;
		const double a_lambda = (gm_over_r2 / safe_cos_phi) * dV_dlambda_sum;

		const double a_x = (a_r * cos_phi - a_phi * sin_phi) * cos_l - a_lambda * sin_l;
		const double a_y = (a_r * cos_phi - a_phi * sin_phi) * sin_l + a_lambda * cos_l;
		const double a_z = a_r * sin_phi + a_phi * cos_phi;

		return {a_x, a_y, a_z};
	}

	[[nodiscard]] static SphericalHarmonicsGravityModel make_earth_egm96(size_t degree = 20) noexcept {
		SphericalHarmonicsGravityModel model(3.986004418e14, 6378137.0, degree);

		model.set_zonal_coefficient(2, 1.08262668355e-3);
		model.set_zonal_coefficient(3, -2.53265648533e-6);
		model.set_zonal_coefficient(4, -1.61962159137e-6);
		model.set_zonal_coefficient(5, -2.27296082869e-7);
		model.set_zonal_coefficient(6, 5.40681239107e-7);
		model.set_zonal_coefficient(7, -3.54500000000e-7);
		model.set_zonal_coefficient(8, -2.04300000000e-7);
		model.set_zonal_coefficient(9, -1.18000000000e-7);
		model.set_zonal_coefficient(10, 3.52000000000e-7);
		model.set_zonal_coefficient(11, -8.70000000000e-8);
		model.set_zonal_coefficient(12, -4.70000000000e-8);
		model.set_zonal_coefficient(13, 2.30000000000e-7);
		model.set_zonal_coefficient(14, -1.00000000000e-8);
		model.set_zonal_coefficient(15, -2.40000000000e-8);
		model.set_zonal_coefficient(16, 2.70000000000e-8);
		model.set_zonal_coefficient(17, -8.00000000000e-8);
		model.set_zonal_coefficient(18, -4.00000000000e-8);
		model.set_zonal_coefficient(19, 1.00000000000e-8);
		model.set_zonal_coefficient(20, -1.80000000000e-7);

		model.set_normalized_coefficient(2, 2, 2.43938357328e-6, -1.40027370386e-6);
		model.set_normalized_coefficient(3, 1, 2.03046201047e-6, 2.48177804477e-7);
		model.set_normalized_coefficient(3, 2, 9.04786016920e-7, -6.19005475177e-7);
		model.set_normalized_coefficient(3, 3, 7.21321757122e-7, 1.41434926127e-6);

		return model;
	}

	[[nodiscard]] static SphericalHarmonicsGravityModel make_moon_lp165(size_t degree = 16) noexcept {
		SphericalHarmonicsGravityModel model(4.902800066e12, 1738000.0, degree);
		model.set_zonal_coefficient(2, 2.0335e-4);
		model.set_zonal_coefficient(3, 8.47e-6);
		model.set_zonal_coefficient(4, -9.60e-6);
		model.set_normalized_coefficient(2, 2, 3.47e-5, 0.0);
		return model;
	}

	[[nodiscard]] static SphericalHarmonicsGravityModel make_mars_mro110(size_t degree = 16) noexcept {
		SphericalHarmonicsGravityModel model(4.2828375214e13, 3396190.0, degree);
		model.set_zonal_coefficient(2, 1.96045e-3);
		model.set_zonal_coefficient(3, 3.15e-5);
		model.set_zonal_coefficient(4, -1.54e-5);
		model.set_normalized_coefficient(2, 2, -5.55e-5, 3.10e-5);
		return model;
	}

	[[nodiscard]] static SphericalHarmonicsGravityModel make_jupiter(size_t degree = 10) noexcept {
		SphericalHarmonicsGravityModel model(1.266865349e17, 71492000.0, degree);
		model.set_zonal_coefficient(2, 1.4696572e-2);
		model.set_zonal_coefficient(3, -4.2e-8);
		model.set_zonal_coefficient(4, -5.86609e-4);
		model.set_zonal_coefficient(6, 3.41988e-5);
		model.set_zonal_coefficient(8, -2.426e-6);
		return model;
	}

	[[nodiscard]] static SphericalHarmonicsGravityModel make_sun(size_t degree = 6) noexcept {
		SphericalHarmonicsGravityModel model(1.32712440018e20, 696340000.0, degree);
		model.set_zonal_coefficient(2, 2.20e-7);
		model.set_zonal_coefficient(4, -4.0e-9);
		return model;
	}
};

}
