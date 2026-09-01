#pragma once

#include "relativistic/uncertainty/uncertainty_types.hpp"
#include <vector>
#include <array>
#include <cmath>
#include <numbers>
#include <algorithm>
#include <cstdint>
#include <span>

namespace Relativistic::Uncertainty {

class OrthogonalPolynomials {
public:
	[[nodiscard]] static double hermite_probabilists(size_t order, double x) noexcept {
		if (order == 0) return 1.0;
		if (order == 1) return x;
		double h0 = 1.0;
		double h1 = x;
		double hn = 0.0;
		for (size_t n = 2; n <= order; ++n) {
			hn = x * h1 - static_cast<double>(n - 1) * h0;
			h0 = h1;
			h1 = hn;
		}
		return hn;
	}

	[[nodiscard]] static double legendre(size_t order, double x) noexcept {
		if (order == 0) return 1.0;
		if (order == 1) return x;
		double p0 = 1.0;
		double p1 = x;
		double pn = 0.0;
		for (size_t n = 2; n <= order; ++n) {
			const double dn = static_cast<double>(n);
			pn = ((2.0 * dn - 1.0) * x * p1 - (dn - 1.0) * p0) / dn;
			p0 = p1;
			p1 = pn;
		}
		return pn;
	}

	[[nodiscard]] static double inner_product_norm_sq(DistributionType dist, size_t order) noexcept {
		if (dist == DistributionType::Gaussian) {
			double fact = 1.0;
			for (size_t i = 2; i <= order; ++i) fact *= static_cast<double>(i);
			return fact;
		} else {
			return 1.0 / (2.0 * static_cast<double>(order) + 1.0);
		}
	}
};

class GaussQuadrature {
public:
	struct QuadratureRule {
		std::vector<double> nodes{};
		std::vector<double> weights{};
	};

private:
	static void tridiagonal_symmetric_ql(
		std::vector<double>& d,
		std::vector<double>& e,
		std::vector<std::vector<double>>& z
	) noexcept {
		const size_t n = d.size();
		if (n <= 1) {
			if (n == 1) z[0][0] = 1.0;
			return;
		}

		for (size_t i = 0; i < n; ++i) {
			for (size_t j = 0; j < n; ++j) {
				z[i][j] = (i == j) ? 1.0 : 0.0;
			}
		}

		std::vector<double> sub(n, 0.0);
		for (size_t i = 0; i < n - 1; ++i) {
			sub[i] = e[i];
		}

		for (size_t l = 0; l < n; ++l) {
			size_t iter = 0;
			while (true) {
				size_t m = l;
				for (; m < n - 1; ++m) {
					const double dd = std::abs(d[m]) + std::abs(d[m + 1]);
					if (std::abs(sub[m]) + dd == dd) break;
				}
				if (m == l) break;
				if (++iter > 60) break;

				double g = (d[l + 1] - d[l]) / (2.0 * sub[l]);
				double r = std::hypot(g, 1.0);
				g = d[m] - d[l] + sub[l] / (g + (g >= 0.0 ? r : -r));
				double s = 1.0, c = 1.0, p = 0.0;

				for (size_t i = m; i > l; --i) {
					double f = s * sub[i - 1];
					double b = c * sub[i - 1];
					r = std::hypot(f, g);
					sub[i] = r;
					if (r == 0.0) {
						d[i] -= p;
						sub[m] = 0.0;
						break;
					}
					c = g / r;
					s = f / r;
					g = d[i] - p;
					r = (d[i - 1] - g) * s + 2.0 * c * b;
					p = s * r;
					d[i] = g + p;
					g = c * r - b;
					for (size_t k = 0; k < n; ++k) {
						f = z[k][i];
						z[k][i] = s * z[k][i - 1] + c * f;
						z[k][i - 1] = c * z[k][i - 1] - s * f;
					}
				}
				if (r == 0.0 && l > 0) continue;
				d[l] -= p;
				sub[l] = g;
				sub[m] = 0.0;
			}
		}
	}

public:
	[[nodiscard]] static QuadratureRule gauss_hermite(size_t num_points) {
		const size_t n = std::max(num_points, size_t{1});
		std::vector<double> d(n, 0.0);
		std::vector<double> e(n > 1 ? n - 1 : 1, 0.0);

		for (size_t i = 0; i < n - 1; ++i) {
			e[i] = std::sqrt(static_cast<double>(i + 1));
		}

		std::vector<std::vector<double>> z(n, std::vector<double>(n, 0.0));
		tridiagonal_symmetric_ql(d, e, z);

		QuadratureRule rule;
		rule.nodes = std::move(d);
		rule.weights.resize(n);
		for (size_t i = 0; i < n; ++i) {
			rule.weights[i] = z[0][i] * z[0][i];
		}
		return rule;
	}

	[[nodiscard]] static QuadratureRule gauss_legendre(size_t num_points) {
		const size_t n = std::max(num_points, size_t{1});
		std::vector<double> d(n, 0.0);
		std::vector<double> e(n > 1 ? n - 1 : 1, 0.0);

		for (size_t i = 0; i < n - 1; ++i) {
			const double di = static_cast<double>(i + 1);
			e[i] = di / std::sqrt(4.0 * di * di - 1.0);
		}

		std::vector<std::vector<double>> z(n, std::vector<double>(n, 0.0));
		tridiagonal_symmetric_ql(d, e, z);

		QuadratureRule rule;
		rule.nodes = std::move(d);
		rule.weights.resize(n);
		for (size_t i = 0; i < n; ++i) {
			rule.weights[i] = z[0][i] * z[0][i];
		}
		return rule;
	}
};

template <size_t NumDims = 1, size_t MaxDegree = 4>
class PolynomialChaosExpansion {
public:
	using MultiIndex = std::array<uint8_t, NumDims>;

private:
	std::vector<MultiIndex> basis_indices_{};
	std::vector<double> coefficients_{};
	std::vector<double> norm_sq_{};
	std::array<DistributionType, NumDims> distributions_{};

	void build_total_degree_basis(size_t dim, size_t current_degree, MultiIndex& current_idx, std::vector<MultiIndex>& basis) {
		if (dim == NumDims) {
			basis.push_back(current_idx);
			return;
		}
		for (size_t d = 0; d <= MaxDegree - current_degree; ++d) {
			current_idx[dim] = static_cast<uint8_t>(d);
			build_total_degree_basis(dim + 1, current_degree + d, current_idx, basis);
		}
	}

public:
	explicit PolynomialChaosExpansion(DistributionType dist = DistributionType::Gaussian) {
		distributions_.fill(dist);
		MultiIndex current_idx{};
		build_total_degree_basis(0, 0, current_idx, basis_indices_);
		coefficients_.assign(basis_indices_.size(), 0.0);
		norm_sq_.resize(basis_indices_.size());

		for (size_t k = 0; k < basis_indices_.size(); ++k) {
			double nsq = 1.0;
			for (size_t d = 0; d < NumDims; ++d) {
				nsq *= OrthogonalPolynomials::inner_product_norm_sq(distributions_[d], basis_indices_[k][d]);
			}
			norm_sq_[k] = nsq;
		}
	}

	explicit PolynomialChaosExpansion(const std::array<DistributionType, NumDims>& dists)
		: distributions_(dists) {
		MultiIndex current_idx{};
		build_total_degree_basis(0, 0, current_idx, basis_indices_);
		coefficients_.assign(basis_indices_.size(), 0.0);
		norm_sq_.resize(basis_indices_.size());

		for (size_t k = 0; k < basis_indices_.size(); ++k) {
			double nsq = 1.0;
			for (size_t d = 0; d < NumDims; ++d) {
				nsq *= OrthogonalPolynomials::inner_product_norm_sq(distributions_[d], basis_indices_[k][d]);
			}
			norm_sq_[k] = nsq;
		}
	}

	[[nodiscard]] size_t basis_size() const noexcept {
		return basis_indices_.size();
	}

	[[nodiscard]] const std::vector<MultiIndex>& basis_indices() const noexcept {
		return basis_indices_;
	}

	[[nodiscard]] const std::vector<double>& coefficients() const noexcept {
		return coefficients_;
	}

	[[nodiscard]] std::vector<double>& coefficients() noexcept {
		return coefficients_;
	}

	[[nodiscard]] double coefficient(size_t k) const noexcept {
		return coefficients_[k];
	}

	void set_coefficient(size_t k, double val) noexcept {
		coefficients_[k] = val;
	}

	[[nodiscard]] double evaluate_basis_function(size_t k, const std::array<double, NumDims>& xi) const noexcept {
		double val = 1.0;
		for (size_t d = 0; d < NumDims; ++d) {
			const size_t order = basis_indices_[k][d];
			if (distributions_[d] == DistributionType::Gaussian) {
				val *= OrthogonalPolynomials::hermite_probabilists(order, xi[d]);
			} else {
				val *= OrthogonalPolynomials::legendre(order, xi[d]);
			}
		}
		return val;
	}

	[[nodiscard]] double evaluate(const std::array<double, NumDims>& xi) const noexcept {
		double sum = 0.0;
		for (size_t k = 0; k < basis_indices_.size(); ++k) {
			sum += coefficients_[k] * evaluate_basis_function(k, xi);
		}
		return sum;
	}

	template <typename EvaluatorFunc>
	void project(EvaluatorFunc&& func, size_t quad_points_per_dim = 6) {
		std::array<GaussQuadrature::QuadratureRule, NumDims> rules;
		for (size_t d = 0; d < NumDims; ++d) {
			rules[d] = (distributions_[d] == DistributionType::Gaussian)
				? GaussQuadrature::gauss_hermite(quad_points_per_dim)
				: GaussQuadrature::gauss_legendre(quad_points_per_dim);
		}

		coefficients_.assign(basis_indices_.size(), 0.0);

		auto traverse_quad = [&](auto& self, size_t dim, std::array<double, NumDims>& xi, double weight_acc) -> void {
			if (dim == NumDims) {
				const double f_val = func(xi);
				for (size_t k = 0; k < basis_indices_.size(); ++k) {
					const double psi_k = evaluate_basis_function(k, xi);
					coefficients_[k] += f_val * psi_k * weight_acc;
				}
				return;
			}
			for (size_t q = 0; q < rules[dim].nodes.size(); ++q) {
				xi[dim] = rules[dim].nodes[q];
				self(self, dim + 1, xi, weight_acc * rules[dim].weights[q]);
			}
		};

		std::array<double, NumDims> xi_temp{};
		traverse_quad(traverse_quad, 0, xi_temp, 1.0);

		for (size_t k = 0; k < basis_indices_.size(); ++k) {
			coefficients_[k] /= norm_sq_[k];
		}
	}

	[[nodiscard]] double mean() const noexcept {
		return coefficients_.empty() ? 0.0 : coefficients_[0];
	}

	[[nodiscard]] double variance() const noexcept {
		double sum = 0.0;
		for (size_t k = 1; k < basis_indices_.size(); ++k) {
			sum += coefficients_[k] * coefficients_[k] * norm_sq_[k];
		}
		return sum;
	}

	[[nodiscard]] double standard_deviation() const noexcept {
		return std::sqrt(std::max(variance(), 0.0));
	}

	private:
	[[nodiscard]] static double standard_normal_quantile(double p) noexcept {
		if (p <= 0.0) return -8.0;
		if (p >= 1.0) return 8.0;

		const double a1 = -3.969683028665376e+01;
		const double a2 =  2.209460984245205e+02;
		const double a3 = -2.759285104469687e+02;
		const double a4 =  1.383577518672690e+02;
		const double a5 = -3.066479806614716e+01;
		const double a6 =  2.506628277459239e+00;

		const double b1 = -5.447609879822406e+01;
		const double b2 =  1.615858368580409e+02;
		const double b3 = -1.556989798598866e+02;
		const double b4 =  6.680131188771972e+01;
		const double b5 = -1.328068155288572e+01;

		const double c1 = -7.784894002430293e-03;
		const double c2 = -3.223964580411365e-01;
		const double c3 = -2.400758277161838e+00;
		const double c4 = -2.549732539343734e+00;
		const double c5 =  4.374664141464968e+00;
		const double c6 =  2.938163982698783e+00;

		const double d1 =  7.784695709041462e-03;
		const double d2 =  3.224671290700398e-01;
		const double d3 =  2.445134137142996e+00;
		const double d4 =  3.754408661907416e+00;

		const double p_low = 0.02425;
		const double p_high = 1.0 - p_low;

		if (p < p_low) {
			const double q = std::sqrt(-2.0 * std::log(p));
			return (((((c1 * q + c2) * q + c3) * q + c4) * q + c5) * q + c6) /
			       ((((d1 * q + d2) * q + d3) * q + d4) * q + 1.0);
		}
		if (p <= p_high) {
			const double q = p - 0.5;
			const double r = q * q;
			return (((((a1 * r + a2) * r + a3) * r + a4) * r + a5) * r + a6) * q /
			       (((((b1 * r + b2) * r + b3) * r + b4) * r + b5) * r + 1.0);
		}
		const double q = std::sqrt(-2.0 * std::log(1.0 - p));
		return -(((((c1 * q + c2) * q + c3) * q + c4) * q + c5) * q + c6) /
		        ((((d1 * q + d2) * q + d3) * q + d4) * q + 1.0);
	}

public:
	[[nodiscard]] StatisticalMoments compute_moments(size_t sample_count = 5000) const {
		StatisticalMoments m;
		m.mean = mean();
		m.variance = variance();
		m.standard_deviation = standard_deviation();

		if (m.standard_deviation <= 1e-15 || sample_count < 10) {
			m.skewness = 0.0;
			m.kurtosis = 3.0;
			return m;
		}

		const double sigma3 = m.standard_deviation * m.standard_deviation * m.standard_deviation;
		const double sigma4 = sigma3 * m.standard_deviation;

		double m3_sum = 0.0;
		double m4_sum = 0.0;

		for (size_t i = 0; i < sample_count; ++i) {
			std::array<double, NumDims> xi;
			for (size_t d = 0; d < NumDims; ++d) {
				const double u = (static_cast<double>(i) + 0.5) / static_cast<double>(sample_count);
				xi[d] = (distributions_[d] == DistributionType::Gaussian)
					? standard_normal_quantile(u)
					: (2.0 * u - 1.0);
			}
			const double dev = evaluate(xi) - m.mean;
			m3_sum += dev * dev * dev;
			m4_sum += dev * dev * dev * dev;
		}

		m.skewness = (m3_sum / static_cast<double>(sample_count)) / sigma3;
		m.kurtosis = (m4_sum / static_cast<double>(sample_count)) / sigma4;
		return m;
	}

	[[nodiscard]] QuantileBands compute_quantiles(size_t sample_count = 10000) const {
		std::vector<double> samples;
		samples.reserve(sample_count);

		for (size_t i = 0; i < sample_count; ++i) {
			std::array<double, NumDims> xi;
			for (size_t d = 0; d < NumDims; ++d) {
				const double u = (static_cast<double>(i) + 0.5) / static_cast<double>(sample_count);
				xi[d] = (distributions_[d] == DistributionType::Gaussian)
					? standard_normal_quantile(u)
					: (2.0 * u - 1.0);
			}
			samples.push_back(evaluate(xi));
		}

		std::sort(samples.begin(), samples.end());

		auto get_percentile = [&](double p) noexcept -> double {
			const double idx = p * static_cast<double>(samples.size() - 1);
			const size_t i0 = static_cast<size_t>(idx);
			const size_t i1 = std::min(i0 + 1, samples.size() - 1);
			const double frac = idx - static_cast<double>(i0);
			return (1.0 - frac) * samples[i0] + frac * samples[i1];
		};

		QuantileBands qb;
		qb.median = get_percentile(0.50);
		qb.sigma_1_lower = get_percentile(0.158655253931457);
		qb.sigma_1_upper = get_percentile(0.841344746068543);
		qb.sigma_2_lower = get_percentile(0.022750131948179);
		qb.sigma_2_upper = get_percentile(0.977249868051821);
		qb.sigma_3_lower = get_percentile(0.001349898031630);
		qb.sigma_3_upper = get_percentile(0.998650101968370);
		return qb;
	}
};

using PolynomialChaos1D = PolynomialChaosExpansion<1, 4>;
using PolynomialChaos2D = PolynomialChaosExpansion<2, 3>;
using PolynomialChaos4D = PolynomialChaosExpansion<4, 2>;

}
