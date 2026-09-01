#pragma once

#include "relativistic/core/tensor.hpp"
#include <array>
#include <vector>
#include <cmath>
#include <numbers>
#include <algorithm>
#include <concepts>

namespace Relativistic::Uncertainty {

template <typename Scalar = double, size_t Dim = 8>
class CovarianceMatrix {
	static_assert(Dim > 0, "Dimension must be positive");

public:
	static constexpr size_t DIMENSION = Dim;
	using MatrixType = std::array<std::array<Scalar, Dim>, Dim>;
	using VectorType = std::array<Scalar, Dim>;

private:
	MatrixType cov_{};

public:
	constexpr CovarianceMatrix() noexcept {
		zero();
	}

	explicit constexpr CovarianceMatrix(const MatrixType& matrix) noexcept : cov_(matrix) {}

	[[nodiscard]] static constexpr CovarianceMatrix identity() noexcept {
		CovarianceMatrix res;
		for (size_t i = 0; i < Dim; ++i) {
			res.cov_[i][i] = static_cast<Scalar>(1.0);
		}
		return res;
	}

	[[nodiscard]] static constexpr CovarianceMatrix diagonal(const VectorType& variances) noexcept {
		CovarianceMatrix res;
		for (size_t i = 0; i < Dim; ++i) {
			res.cov_[i][i] = variances[i];
		}
		return res;
	}

	constexpr void zero() noexcept {
		for (size_t i = 0; i < Dim; ++i) {
			for (size_t j = 0; j < Dim; ++j) {
				cov_[i][j] = static_cast<Scalar>(0.0);
			}
		}
	}

	[[nodiscard]] constexpr Scalar operator()(size_t i, size_t j) const noexcept {
		return cov_[i][j];
	}

	[[nodiscard]] constexpr Scalar& operator()(size_t i, size_t j) noexcept {
		return cov_[i][j];
	}

	[[nodiscard]] constexpr const MatrixType& data() const noexcept {
		return cov_;
	}

	[[nodiscard]] constexpr MatrixType& data() noexcept {
		return cov_;
	}

	[[nodiscard]] constexpr Scalar variance(size_t i) const noexcept {
		return cov_[i][i];
	}

	[[nodiscard]] Scalar standard_deviation(size_t i) const noexcept {
		return std::sqrt(std::max(cov_[i][i], static_cast<Scalar>(0.0)));
	}

	[[nodiscard]] Scalar correlation(size_t i, size_t j) const noexcept {
		const Scalar sigma_i = standard_deviation(i);
		const Scalar sigma_j = standard_deviation(j);
		const Scalar denom = sigma_i * sigma_j;
		if (denom <= static_cast<Scalar>(1e-30)) {
			return static_cast<Scalar>(0.0);
		}
		return std::clamp(cov_[i][j] / denom, static_cast<Scalar>(-1.0), static_cast<Scalar>(1.0));
	}

	[[nodiscard]] constexpr Scalar trace() const noexcept {
		Scalar sum = static_cast<Scalar>(0.0);
		for (size_t i = 0; i < Dim; ++i) {
			sum += cov_[i][i];
		}
		return sum;
	}

	constexpr void symmetrize() noexcept {
		for (size_t i = 0; i < Dim; ++i) {
			for (size_t j = i + 1; j < Dim; ++j) {
				const Scalar val = static_cast<Scalar>(0.5) * (cov_[i][j] + cov_[j][i]);
				cov_[i][j] = val;
				cov_[j][i] = val;
			}
		}
	}

	[[nodiscard]] CovarianceMatrix sandwich(const MatrixType& phi) const noexcept {
		CovarianceMatrix result;
		for (size_t i = 0; i < Dim; ++i) {
			for (size_t j = 0; j < Dim; ++j) {
				Scalar sum = static_cast<Scalar>(0.0);
				for (size_t k = 0; k < Dim; ++k) {
					for (size_t l = 0; l < Dim; ++l) {
						sum += phi[i][k] * cov_[k][l] * phi[j][l];
					}
				}
				result.cov_[i][j] = sum;
			}
		}
		result.symmetrize();
		return result;
	}

	[[nodiscard]] CovarianceMatrix lyapunov_derivative(
		const MatrixType& jacobian,
		const CovarianceMatrix& process_noise_q
	) const noexcept {
		CovarianceMatrix d_sigma;
		for (size_t i = 0; i < Dim; ++i) {
			for (size_t j = 0; j < Dim; ++j) {
				Scalar sum = process_noise_q(i, j);
				for (size_t k = 0; k < Dim; ++k) {
					sum += jacobian[i][k] * cov_[k][j] + cov_[i][k] * jacobian[j][k];
				}
				d_sigma.cov_[i][j] = sum;
			}
		}
		d_sigma.symmetrize();
		return d_sigma;
	}

	void step_euler(
		const MatrixType& jacobian,
		const CovarianceMatrix& process_noise_q,
		Scalar dt
	) noexcept {
		const auto d_cov = lyapunov_derivative(jacobian, process_noise_q);
		for (size_t i = 0; i < Dim; ++i) {
			for (size_t j = 0; j < Dim; ++j) {
				cov_[i][j] += dt * d_cov(i, j);
			}
		}
		symmetrize();
	}

	void step_rk4(
		const MatrixType& jacobian,
		const CovarianceMatrix& process_noise_q,
		Scalar dt
	) noexcept {
		const auto k1 = lyapunov_derivative(jacobian, process_noise_q);

		CovarianceMatrix s2 = *this;
		for (size_t i = 0; i < Dim; ++i) {
			for (size_t j = 0; j < Dim; ++j) {
				s2.cov_[i][j] += static_cast<Scalar>(0.5) * dt * k1(i, j);
			}
		}
		const auto k2 = s2.lyapunov_derivative(jacobian, process_noise_q);

		CovarianceMatrix s3 = *this;
		for (size_t i = 0; i < Dim; ++i) {
			for (size_t j = 0; j < Dim; ++j) {
				s3.cov_[i][j] += static_cast<Scalar>(0.5) * dt * k2(i, j);
			}
		}
		const auto k3 = s3.lyapunov_derivative(jacobian, process_noise_q);

		CovarianceMatrix s4 = *this;
		for (size_t i = 0; i < Dim; ++i) {
			for (size_t j = 0; j < Dim; ++j) {
				s4.cov_[i][j] += dt * k3(i, j);
			}
		}
		const auto k4 = s4.lyapunov_derivative(jacobian, process_noise_q);

		const Scalar sixth_dt = dt / static_cast<Scalar>(6.0);
		for (size_t i = 0; i < Dim; ++i) {
			for (size_t j = 0; j < Dim; ++j) {
				cov_[i][j] += sixth_dt * (k1(i, j) + static_cast<Scalar>(2.0) * k2(i, j) + static_cast<Scalar>(2.0) * k3(i, j) + k4(i, j));
			}
		}
		symmetrize();
	}

	struct Eigensystem {
		VectorType eigenvalues{};
		MatrixType eigenvectors{};
	};

	[[nodiscard]] Eigensystem compute_eigensystem(size_t max_sweeps = 50) const noexcept {
		Eigensystem es;
		MatrixType a = cov_;

		for (size_t i = 0; i < Dim; ++i) {
			for (size_t j = 0; j < Dim; ++j) {
				es.eigenvectors[i][j] = (i == j) ? static_cast<Scalar>(1.0) : static_cast<Scalar>(0.0);
			}
		}

		for (size_t sweep = 0; sweep < max_sweeps; ++sweep) {
			Scalar max_off_diag = static_cast<Scalar>(0.0);
			for (size_t i = 0; i < Dim; ++i) {
				for (size_t j = i + 1; j < Dim; ++j) {
					max_off_diag = std::max(max_off_diag, std::abs(a[i][j]));
				}
			}

			if (max_off_diag < static_cast<Scalar>(1e-15)) {
				break;
			}

			for (size_t p = 0; p < Dim; ++p) {
				for (size_t q = p + 1; q < Dim; ++q) {
					if (std::abs(a[p][q]) < static_cast<Scalar>(1e-18)) {
						continue;
					}

					const Scalar h = a[q][q] - a[p][p];
					Scalar t = static_cast<Scalar>(0.0);
					if (std::abs(a[p][q]) > static_cast<Scalar>(1e-15)) {
						const Scalar theta = static_cast<Scalar>(0.5) * h / a[p][q];
						t = static_cast<Scalar>(1.0) / (std::abs(theta) + std::sqrt(static_cast<Scalar>(1.0) + theta * theta));
						if (theta < static_cast<Scalar>(0.0)) t = -t;
					}

					const Scalar c = static_cast<Scalar>(1.0) / std::sqrt(static_cast<Scalar>(1.0) + t * t);
					const Scalar s = t * c;
					const Scalar tau_rot = s / (static_cast<Scalar>(1.0) + c);

					const Scalar a_pq = a[p][q];
					a[p][q] = static_cast<Scalar>(0.0);
					a[q][p] = static_cast<Scalar>(0.0);
					a[p][p] -= t * a_pq;
					a[q][q] += t * a_pq;

					for (size_t j = 0; j < p; ++j) {
						const Scalar g_val = a[j][p];
						const Scalar h_val = a[j][q];
						a[j][p] = g_val - s * (h_val + g_val * tau_rot);
						a[p][j] = a[j][p];
						a[j][q] = h_val + s * (g_val - h_val * tau_rot);
						a[q][j] = a[j][q];
					}

					for (size_t j = p + 1; j < q; ++j) {
						const Scalar g_val = a[p][j];
						const Scalar h_val = a[j][q];
						a[p][j] = g_val - s * (h_val + g_val * tau_rot);
						a[j][p] = a[p][j];
						a[j][q] = h_val + s * (g_val - h_val * tau_rot);
						a[q][j] = a[j][q];
					}

					for (size_t j = q + 1; j < Dim; ++j) {
						const Scalar g_val = a[p][j];
						const Scalar h_val = a[q][j];
						a[p][j] = g_val - s * (h_val + g_val * tau_rot);
						a[j][p] = a[p][j];
						a[q][j] = h_val + s * (g_val - h_val * tau_rot);
						a[j][q] = a[q][j];
					}

					for (size_t j = 0; j < Dim; ++j) {
						const Scalar g_val = es.eigenvectors[j][p];
						const Scalar h_val = es.eigenvectors[j][q];
						es.eigenvectors[j][p] = g_val - s * (h_val + g_val * tau_rot);
						es.eigenvectors[j][q] = h_val + s * (g_val - h_val * tau_rot);
					}
				}
			}
		}

		for (size_t i = 0; i < Dim; ++i) {
			es.eigenvalues[i] = std::max(static_cast<Scalar>(0.0), a[i][i]);
		}

		return es;
	}

	[[nodiscard]] Scalar confidence_hypervolume(Scalar n_sigma = static_cast<Scalar>(1.0)) const noexcept {
		const auto es = compute_eigensystem();
		Scalar prod = static_cast<Scalar>(1.0);
		for (size_t i = 0; i < Dim; ++i) {
			prod *= (n_sigma * std::sqrt(es.eigenvalues[i]));
		}
		const double half_d = static_cast<double>(Dim) * 0.5;
		const double vol_unit_sphere = std::pow(std::numbers::pi, half_d) / std::tgamma(half_d + 1.0);
		return static_cast<Scalar>(vol_unit_sphere) * prod;
	}
};

using CovarianceMatrix4d = CovarianceMatrix<double, 4>;
using CovarianceMatrix6d = CovarianceMatrix<double, 6>;
using CovarianceMatrix8d = CovarianceMatrix<double, 8>;

}
