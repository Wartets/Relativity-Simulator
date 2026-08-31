#pragma once

#include <cstddef>
#include <cstdint>
#include <array>
#include <vector>
#include <cmath>
#include <numbers>
#include <algorithm>

namespace Relativistic::Gravimetry {

template <size_t MaxDegree = 32>
class AssociatedLegendreTable {
	static_assert(MaxDegree >= 2, "MaxDegree must be at least 2");

public:
	static constexpr size_t MAX_DEGREE = MaxDegree;
	static constexpr size_t TOTAL_COEFFICIENTS = ((MaxDegree + 1) * (MaxDegree + 2)) / 2;

private:
	std::array<double, TOTAL_COEFFICIENTS> p_bar_{};
	std::array<double, TOTAL_COEFFICIENTS> dp_bar_dphi_{};
	std::array<double, TOTAL_COEFFICIENTS> normalization_factors_{};
	size_t current_degree_{MaxDegree};

	[[nodiscard]] static constexpr size_t flat_index(size_t n, size_t m) noexcept {
		return (n * (n + 1)) / 2 + m;
	}

	void precompute_normalization_factors() noexcept {
		normalization_factors_[flat_index(0, 0)] = 1.0;

		for (size_t n = 1; n <= MaxDegree; ++n) {
			for (size_t m = 0; m <= n; ++m) {
				const double delta_0m = (m == 0) ? 1.0 : 0.0;
				double log_fact_diff = 0.0;
				for (size_t i = n - m + 1; i <= n + m; ++i) {
					log_fact_diff += std::log(static_cast<double>(i));
				}
				const double log_norm = 0.5 * (std::log(2.0 * static_cast<double>(n) + 1.0) + std::log(2.0 - delta_0m) - log_fact_diff);
				normalization_factors_[flat_index(n, m)] = std::exp(log_norm);
			}
		}
	}

public:
	constexpr AssociatedLegendreTable() noexcept {
		precompute_normalization_factors();
	}

	void compute(double sin_phi, double cos_phi, size_t degree_limit = MaxDegree) noexcept {
		current_degree_ = std::min(degree_limit, MaxDegree);
		const double u = sin_phi;
		const double s = std::max(cos_phi, 1e-15);

		p_bar_.fill(0.0);
		dp_bar_dphi_.fill(0.0);

		p_bar_[flat_index(0, 0)] = 1.0;
		dp_bar_dphi_[flat_index(0, 0)] = 0.0;

		p_bar_[flat_index(1, 0)] = std::sqrt(3.0) * u;
		dp_bar_dphi_[flat_index(1, 0)] = std::sqrt(3.0) * s;

		p_bar_[flat_index(1, 1)] = std::sqrt(3.0) * s;
		dp_bar_dphi_[flat_index(1, 1)] = -std::sqrt(3.0) * u;

		for (size_t m = 2; m <= current_degree_; ++m) {
			const double factor = std::sqrt(static_cast<double>(2 * m + 1) / static_cast<double>(2 * m));
			const size_t idx_mm = flat_index(m, m);
			const size_t idx_prev = flat_index(m - 1, m - 1);
			p_bar_[idx_mm] = factor * s * p_bar_[idx_prev];
			dp_bar_dphi_[idx_mm] = factor * (s * dp_bar_dphi_[idx_prev] - u * p_bar_[idx_prev]);
		}

		for (size_t m = 0; m <= current_degree_ - 1; ++m) {
			const size_t n = m + 1;
			const size_t idx_nm = flat_index(n, m);
			const size_t idx_mm = flat_index(m, m);
			const double factor = std::sqrt(static_cast<double>(2 * m + 3));
			p_bar_[idx_nm] = factor * u * p_bar_[idx_mm];
			dp_bar_dphi_[idx_nm] = factor * (u * dp_bar_dphi_[idx_mm] + s * p_bar_[idx_mm]);
		}

		for (size_t m = 0; m <= current_degree_; ++m) {
			for (size_t n = m + 2; n <= current_degree_; ++n) {
				const double dn = static_cast<double>(n);
				const double dm = static_cast<double>(m);

				const double a_nm = std::sqrt(((2.0 * dn - 1.0) * (2.0 * dn + 1.0)) / ((dn - dm) * (dn + dm)));
				const double b_nm = std::sqrt(((2.0 * dn + 1.0) * (dn - dm - 1.0) * (dn + dm - 1.0)) / ((2.0 * dn - 3.0) * (dn - dm) * (dn + dm)));

				const size_t idx = flat_index(n, m);
				const size_t idx_n1 = flat_index(n - 1, m);
				const size_t idx_n2 = flat_index(n - 2, m);

				p_bar_[idx] = a_nm * u * p_bar_[idx_n1] - b_nm * p_bar_[idx_n2];
				dp_bar_dphi_[idx] = a_nm * (u * dp_bar_dphi_[idx_n1] + s * p_bar_[idx_n1]) - b_nm * dp_bar_dphi_[idx_n2];
			}
		}
	}

	[[nodiscard]] constexpr double p_bar(size_t n, size_t m) const noexcept {
		if (n > current_degree_ || m > n) return 0.0;
		return p_bar_[flat_index(n, m)];
	}

	[[nodiscard]] constexpr double dp_bar_dphi(size_t n, size_t m) const noexcept {
		if (n > current_degree_ || m > n) return 0.0;
		return dp_bar_dphi_[flat_index(n, m)];
	}

	[[nodiscard]] double p_unnormalized(size_t n, size_t m) const noexcept {
		if (n > current_degree_ || m > n) return 0.0;
		const double norm = normalization_factors_[flat_index(n, m)];
		return (norm > 0.0) ? (p_bar_[flat_index(n, m)] / norm) : 0.0;
	}

	[[nodiscard]] double dp_unnormalized_dphi(size_t n, size_t m) const noexcept {
		if (n > current_degree_ || m > n) return 0.0;
		const double norm = normalization_factors_[flat_index(n, m)];
		return (norm > 0.0) ? (dp_bar_dphi_[flat_index(n, m)] / norm) : 0.0;
	}

	[[nodiscard]] constexpr double normalization_factor(size_t n, size_t m) const noexcept {
		if (n > MaxDegree || m > n) return 0.0;
		return normalization_factors_[flat_index(n, m)];
	}

	[[nodiscard]] constexpr size_t max_degree() const noexcept {
		return current_degree_;
	}
};

}
