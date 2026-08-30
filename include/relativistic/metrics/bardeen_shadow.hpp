#pragma once

#include "relativistic/core/tensor.hpp"
#include "relativistic/metrics/kerr.hpp"
#include <cmath>
#include <numbers>
#include <vector>
#include <span>
#include <array>
#include <algorithm>
#include <optional>

namespace Relativistic::Metrics {

struct ShadowPoint2D {
	double alpha{0.0};
	double beta{0.0};
};

class BardeenKerrShadow {
private:
	double mass_{1.0};
	double spin_{0.0};
	double theta_0_{std::numbers::pi_v<double> / 2.0};

public:
	explicit constexpr BardeenKerrShadow(
		double mass,
		double spin,
		double inclination_rad = std::numbers::pi_v<double> / 2.0
	) noexcept
		: mass_(mass), spin_(spin), theta_0_(inclination_rad) {}

	[[nodiscard]] constexpr double mass() const noexcept { return mass_; }
	[[nodiscard]] constexpr double spin() const noexcept { return spin_; }
	[[nodiscard]] constexpr double inclination() const noexcept { return theta_0_; }

	[[nodiscard]] std::pair<double, double> photon_orbit_radii() const noexcept {
		const double m = mass_;
		const double a = spin_;
		if (std::abs(a) < 1e-14) {
			return {3.0 * m, 3.0 * m};
		}
		const double a_m = std::clamp(a / m, -0.999999999, 0.999999999);
		const double r1 = 2.0 * m * (1.0 + std::cos(2.0 / 3.0 * std::acos(-a_m)));
		const double r2 = 2.0 * m * (1.0 + std::cos(2.0 / 3.0 * std::acos(a_m)));
		return {std::min(r1, r2), std::max(r1, r2)};
	}

	[[nodiscard]] std::vector<ShadowPoint2D> compute_boundary_points(size_t num_points = 2000) const {
		std::vector<ShadowPoint2D> points;
		const double m = mass_;
		const double a = spin_;
		const double sin_t = std::sin(theta_0_);
		const double cos_t = std::cos(theta_0_);

		if (std::abs(a) < 1e-12 || std::abs(sin_t) < 1e-12) {
			const double r_shadow = std::sqrt(27.0) * m;
			points.reserve(num_points);
			for (size_t i = 0; i < num_points; ++i) {
				const double phi = (static_cast<double>(i) / static_cast<double>(num_points)) * 2.0 * std::numbers::pi_v<double>;
				points.push_back(ShadowPoint2D{
					.alpha = r_shadow * std::cos(phi),
					.beta = r_shadow * std::sin(phi)
				});
			}
			return points;
		}

		const auto [r_min, r_max] = photon_orbit_radii();

		auto eval_xi_eta = [m, a](double r, double& out_xi, double& out_eta) noexcept {
			const double u = r - m;
			const double u_safe = (std::abs(u) < 1e-15) ? ((u >= 0.0) ? 1e-15 : -1e-15) : u;
			const double num_xi = -r * r * r + 3.0 * m * r * r - a * a * r - a * a * m;
			const double den_xi = a * u_safe;
			out_xi = num_xi / den_xi;

			const double r3 = r * r * r;
			const double delta_bracket = 4.0 * m * a * a - r * (r - 3.0 * m) * (r - 3.0 * m);
			const double den_eta = a * a * u_safe * u_safe;
			out_eta = (r3 * delta_bracket) / den_eta;
		};

		auto eval_beta_sq = [cos_t, sin_t, a, &eval_xi_eta](double r) noexcept -> double {
			double xi = 0.0, eta = 0.0;
			eval_xi_eta(r, xi, eta);
			const double cot_t = cos_t / sin_t;
			return eta + a * a * cos_t * cos_t - xi * xi * cot_t * cot_t;
		};

		double r_start = r_min;
		double r_end = r_max;

		if (eval_beta_sq(r_start) < 0.0 || eval_beta_sq(r_end) < 0.0) {
			const size_t scan_steps = 200;
			const double dr_scan = (r_max - r_min) / static_cast<double>(scan_steps);
			double r_peak = 0.5 * (r_min + r_max);
			double max_b = -1e30;
			for (size_t k = 0; k <= scan_steps; ++k) {
				const double r_k = r_min + static_cast<double>(k) * dr_scan;
				const double b_k = eval_beta_sq(r_k);
				if (b_k > max_b) {
					max_b = b_k;
					r_peak = r_k;
				}
			}

			if (max_b > 0.0) {
				double left = r_min, right = r_peak;
				for (int iter = 0; iter < 40; ++iter) {
					const double mid = 0.5 * (left + right);
					if (eval_beta_sq(mid) >= 0.0) {
						right = mid;
					} else {
						left = mid;
					}
				}
				r_start = right;

				left = r_peak;
				right = r_max;
				for (int iter = 0; iter < 40; ++iter) {
					const double mid = 0.5 * (left + right);
					if (eval_beta_sq(mid) >= 0.0) {
						left = mid;
					} else {
						right = mid;
					}
				}
				r_end = left;
			}
		}

		const size_t half_samples = num_points / 2;
		std::vector<ShadowPoint2D> top_branch;
		std::vector<ShadowPoint2D> bottom_branch;
		top_branch.reserve(half_samples);
		bottom_branch.reserve(half_samples);

		for (size_t i = 0; i < half_samples; ++i) {
			const double u = static_cast<double>(i) / static_cast<double>(half_samples - 1);
			const double r = r_start + u * (r_end - r_start);

			double xi = 0.0, eta = 0.0;
			eval_xi_eta(r, xi, eta);
			const double cot_t = cos_t / sin_t;
			const double b_sq = eta + a * a * cos_t * cos_t - xi * xi * cot_t * cot_t;
			const double alpha_val = -xi / sin_t;
			const double beta_val = std::sqrt(std::max(b_sq, 0.0));

			top_branch.push_back(ShadowPoint2D{.alpha = alpha_val, .beta = beta_val});
			bottom_branch.push_back(ShadowPoint2D{.alpha = alpha_val, .beta = -beta_val});
		}

		points.insert(points.end(), top_branch.begin(), top_branch.end());
		for (auto it = bottom_branch.rbegin(); it != bottom_branch.rend(); ++it) {
			points.push_back(*it);
		}

		return points;
	}

	[[nodiscard]] double compute_shadow_area(size_t num_points = 2000) const noexcept {
		const auto boundary = compute_boundary_points(num_points);
		if (boundary.size() < 3) {
			return 0.0;
		}

		double area = 0.0;
		const size_t n = boundary.size();
		for (size_t i = 0; i < n; ++i) {
			const size_t next = (i + 1) % n;
			area += (boundary[i].alpha * boundary[next].beta - boundary[next].alpha * boundary[i].beta);
		}
		return 0.5 * std::abs(area);
	}

	[[nodiscard]] static bool is_inside_polygon(
		double alpha,
		double beta,
		std::span<const ShadowPoint2D> boundary
	) noexcept {
		if (boundary.size() < 3) {
			return false;
		}

		bool inside = false;
		const size_t n = boundary.size();
		for (size_t i = 0, j = n - 1; i < n; j = i++) {
			const double xi = boundary[i].alpha, yi = boundary[i].beta;
			const double xj = boundary[j].alpha, yj = boundary[j].beta;

			const bool intersect = ((yi > beta) != (yj > beta)) &&
				(alpha < (xj - xi) * (beta - yi) / (yj - yi + 1e-30) + xi);
			if (intersect) {
				inside = !inside;
			}
		}
		return inside;
	}

	[[nodiscard]] bool is_inside_analytical_shadow(double alpha, double beta, size_t num_points = 2000) const noexcept {
		const auto boundary = compute_boundary_points(num_points);
		return is_inside_polygon(alpha, beta, boundary);
	}

	[[nodiscard]] bool is_inside_carter_numerical(double alpha, double beta) const noexcept {
		const double m = mass_;
		const double a = spin_;
		const double sin_t = std::sin(theta_0_);
		const double cos_t = std::cos(theta_0_);

		if (std::abs(a) < 1e-12 || std::abs(sin_t) < 1e-12) {
			const double r_shadow_sq = 27.0 * m * m;
			return (alpha * alpha + beta * beta) <= r_shadow_sq;
		}

		const double xi = -alpha * sin_t;
		const double cot_t = cos_t / sin_t;
		const double eta = beta * beta + xi * xi * (cot_t * cot_t) - a * a * cos_t * cos_t;

		const double cap_a = a * a - xi * xi - eta;
		const double cap_b = 2.0 * m * (eta + (xi - a) * (xi - a));
		const double cap_c = -a * a * eta;

		auto eval_poly = [cap_a, cap_b, cap_c](double r) noexcept -> double {
			const double r2 = r * r;
			return r2 * r2 + cap_a * r2 + cap_b * r + cap_c;
		};

		const double r_plus = m + std::sqrt(std::max(m * m - a * a, 0.0));

		const double p = 0.5 * cap_a;
		const double q = 0.25 * cap_b;
		const double discr = (0.25 * q * q) + (p * p * p / 27.0);

		if (discr > 0.0) {
			const double sqrt_d = std::sqrt(discr);
			const double term1 = -0.5 * q + sqrt_d;
			const double term2 = -0.5 * q - sqrt_d;
			const double u = (term1 >= 0.0) ? std::cbrt(term1) : -std::cbrt(-term1);
			const double v = (term2 >= 0.0) ? std::cbrt(term2) : -std::cbrt(-term2);
			const double r_crit = u + v;
			if (r_crit > r_plus && eval_poly(r_crit) <= 0.0) {
				return false;
			}
		} else {
			const double r_term = std::sqrt(std::max(-p / 3.0, 0.0));
			if (r_term > 0.0) {
				const double phi = std::acos(std::clamp(-0.5 * q / (r_term * r_term * r_term), -1.0, 1.0));
				const double rc0 = 2.0 * r_term * std::cos(phi / 3.0);
				const double rc1 = 2.0 * r_term * std::cos((phi + 2.0 * std::numbers::pi_v<double>) / 3.0);
				const double rc2 = 2.0 * r_term * std::cos((phi + 4.0 * std::numbers::pi_v<double>) / 3.0);

				if (rc0 > r_plus && eval_poly(rc0) <= 0.0) return false;
				if (rc1 > r_plus && eval_poly(rc1) <= 0.0) return false;
				if (rc2 > r_plus && eval_poly(rc2) <= 0.0) return false;
			}
		}

		return true;
	}

	[[nodiscard]] double compute_overlap_ratio(size_t num_slices = 1000) const noexcept {
		const auto boundary = compute_boundary_points(num_slices * 2);
		if (boundary.size() < 4) {
			return 1.0;
		}

		const size_t half_count = boundary.size() / 2;
		double intersection_integral = 0.0;
		double union_integral = 0.0;

		for (size_t i = 0; i < half_count; ++i) {
			const double alpha = boundary[i].alpha;
			const double beta_ana = std::abs(boundary[i].beta);
			const double next_alpha = (i + 1 < half_count) ? boundary[i + 1].alpha : alpha;
			const double d_alpha = std::abs(next_alpha - alpha);

			double low = 0.0;
			double high = std::max(beta_ana * 1.5, 10.0);
			for (int iter = 0; iter < 45; ++iter) {
				const double mid = 0.5 * (low + high);
				if (is_inside_carter_numerical(alpha, mid)) {
					low = mid;
				} else {
					high = mid;
				}
			}
			const double beta_num = 0.5 * (low + high);

			const double beta_inter = std::min(beta_ana, beta_num);
			const double beta_un = std::max(beta_ana, beta_num);

			intersection_integral += beta_inter * d_alpha;
			union_integral += beta_un * d_alpha;
		}

		if (union_integral <= 0.0) {
			return 1.0;
		}
		return intersection_integral / union_integral;
	}
};

}
