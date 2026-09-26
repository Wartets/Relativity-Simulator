#pragma once

#include <cmath>
#include <numbers>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstddef>

namespace Relativistic::Optics {

enum class GeodesicPhaseClass : uint32_t {
	CertainAbsorption = 0,
	CertainEscape = 1,
	RequiresIntegration = 2
};

template <typename Scalar = double>
struct CarterPhaseClassifier {
	struct ClassificationResult {
		GeodesicPhaseClass phase{GeodesicPhaseClass::RequiresIntegration};
		Scalar outer_turning_point{0};
	};

	[[nodiscard]] static std::array<Scalar, 3> solve_real_cubic(Scalar a2, Scalar a1, Scalar a0) noexcept {
		const Scalar third = static_cast<Scalar>(1.0 / 3.0);
		const Scalar q = (static_cast<Scalar>(3) * a1 - a2 * a2) / static_cast<Scalar>(9);
		const Scalar r = (static_cast<Scalar>(9) * a2 * a1 - static_cast<Scalar>(27) * a0 - static_cast<Scalar>(2) * a2 * a2 * a2) / static_cast<Scalar>(54);
		const Scalar disc = q * q * q + r * r;

		std::array<Scalar, 3> roots{};
		if (disc > static_cast<Scalar>(0)) {
			const Scalar sqrt_disc = std::sqrt(disc);
			const Scalar s = std::cbrt(r + sqrt_disc);
			const Scalar t = std::cbrt(r - sqrt_disc);
			const Scalar real_root = s + t - a2 * third;
			roots = {real_root, real_root, real_root};
		} else {
			const Scalar neg_q3 = -q * q * q;
			const Scalar safe_sqrt = std::sqrt(std::max(neg_q3, static_cast<Scalar>(1e-300)));
			const Scalar theta = std::acos(std::clamp(r / safe_sqrt, static_cast<Scalar>(-1), static_cast<Scalar>(1)));
			const Scalar sqrt_neg_q = std::sqrt(std::max(-q, static_cast<Scalar>(0)));
			const Scalar two_pi = static_cast<Scalar>(2) * std::numbers::pi_v<Scalar>;
			roots[0] = static_cast<Scalar>(2) * sqrt_neg_q * std::cos(theta * third) - a2 * third;
			roots[1] = static_cast<Scalar>(2) * sqrt_neg_q * std::cos((theta + two_pi) * third) - a2 * third;
			roots[2] = static_cast<Scalar>(2) * sqrt_neg_q * std::cos((theta + static_cast<Scalar>(2) * two_pi) * third) - a2 * third;
		}
		return roots;
	}

	[[nodiscard]] static size_t solve_depressed_quartic_real_roots(Scalar p, Scalar q, Scalar r0, std::array<Scalar, 4>& roots) noexcept {
		constexpr Scalar eps = static_cast<Scalar>(1e-13);

		if (std::abs(q) < eps) {
			const Scalar disc = p * p - static_cast<Scalar>(4) * r0;
			size_t count = 0;
			if (disc >= static_cast<Scalar>(0)) {
				const Scalar sqrt_disc = std::sqrt(disc);
				const Scalar z1 = (-p + sqrt_disc) * static_cast<Scalar>(0.5);
				const Scalar z2 = (-p - sqrt_disc) * static_cast<Scalar>(0.5);
				if (z1 >= static_cast<Scalar>(0)) {
					const Scalar rt = std::sqrt(z1);
					roots[count++] = rt;
					roots[count++] = -rt;
				}
				if (z2 >= static_cast<Scalar>(0) && std::abs(z2 - z1) > eps) {
					const Scalar rt = std::sqrt(z2);
					roots[count++] = rt;
					roots[count++] = -rt;
				}
			}
			return count;
		}

		const Scalar cubic_a2 = p;
		const Scalar cubic_a1 = p * p * static_cast<Scalar>(0.25) - r0;
		const Scalar cubic_a0 = -q * q * static_cast<Scalar>(0.125);
		const auto cubic_roots = solve_real_cubic(cubic_a2, cubic_a1, cubic_a0);

		Scalar m = static_cast<Scalar>(-1);
		for (const Scalar candidate : cubic_roots) {
			if (candidate > eps && (m < static_cast<Scalar>(0) || candidate > m)) {
				m = candidate;
			}
		}
		if (m <= eps) {
			return 0;
		}

		const Scalar s = std::sqrt(static_cast<Scalar>(2) * m);
		const Scalar half_p_plus_m = p * static_cast<Scalar>(0.5) + m;
		const Scalar term_b = q / (static_cast<Scalar>(2) * s);

		size_t count = 0;
		{
			const Scalar c1 = half_p_plus_m + term_b;
			const Scalar disc1 = s * s - static_cast<Scalar>(4) * c1;
			if (disc1 >= -eps) {
				const Scalar sqrt_disc1 = std::sqrt(std::max(disc1, static_cast<Scalar>(0)));
				roots[count++] = (s + sqrt_disc1) * static_cast<Scalar>(0.5);
				roots[count++] = (s - sqrt_disc1) * static_cast<Scalar>(0.5);
			}
		}
		{
			const Scalar c2 = half_p_plus_m - term_b;
			const Scalar disc2 = s * s - static_cast<Scalar>(4) * c2;
			if (disc2 >= -eps) {
				const Scalar sqrt_disc2 = std::sqrt(std::max(disc2, static_cast<Scalar>(0)));
				roots[count++] = (-s + sqrt_disc2) * static_cast<Scalar>(0.5);
				roots[count++] = (-s - sqrt_disc2) * static_cast<Scalar>(0.5);
			}
		}
		return count;
	}

	[[nodiscard]] static ClassificationResult classify(
		Scalar mass,
		Scalar spin,
		Scalar r_obs,
		Scalar r_horizon,
		Scalar disk_outer_radius,
		Scalar xi,
		Scalar eta,
		bool traveling_inward
	) noexcept {
		ClassificationResult result{};

		if (eta < static_cast<Scalar>(-1e-6)) {
			return result;
		}
		const Scalar safe_eta = std::max(eta, static_cast<Scalar>(0));

		const Scalar a2 = spin * spin;
		const Scalar coeff_p = a2 - xi * xi - safe_eta;
		const Scalar coeff_q = static_cast<Scalar>(2) * mass * (safe_eta + (xi - spin) * (xi - spin));
		const Scalar coeff_r0 = -a2 * safe_eta;

		std::array<Scalar, 4> roots{};
		const size_t root_count = solve_depressed_quartic_real_roots(coeff_p, coeff_q, coeff_r0, roots);

		bool has_exterior_root = false;
		Scalar largest_exterior_root = static_cast<Scalar>(-1);
		for (size_t i = 0; i < root_count; ++i) {
			if (roots[i] > r_horizon) {
				has_exterior_root = true;
				largest_exterior_root = std::max(largest_exterior_root, roots[i]);
			}
		}

		static_cast<void>(r_obs);

		if (!has_exterior_root) {
			if (traveling_inward) {
				result.phase = GeodesicPhaseClass::CertainAbsorption;
			}
			return result;
		}

		result.outer_turning_point = largest_exterior_root;
		if (largest_exterior_root > static_cast<Scalar>(3) * disk_outer_radius) {
			result.phase = GeodesicPhaseClass::CertainEscape;
		}
		return result;
	}
};

}
