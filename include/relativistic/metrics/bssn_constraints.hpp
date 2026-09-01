#pragma once

#include "relativistic/metrics/bssn_grid.hpp"
#include <vector>
#include <cmath>
#include <algorithm>

namespace Relativistic::Metrics {

class BssnConstraints {
private:
	[[nodiscard]] static double dx(const std::vector<double>& f, const BssnGrid& g, int i, int j, int k) noexcept {
		return (-f[g.index(i + 2, j, k)] + 8.0 * f[g.index(i + 1, j, k)] - 8.0 * f[g.index(i - 1, j, k)] + f[g.index(i - 2, j, k)]) / (12.0 * g.dx);
	}

	[[nodiscard]] static double dy(const std::vector<double>& f, const BssnGrid& g, int i, int j, int k) noexcept {
		return (-f[g.index(i, j + 2, k)] + 8.0 * f[g.index(i, j + 1, k)] - 8.0 * f[g.index(i, j - 1, k)] + f[g.index(i, j - 2, k)]) / (12.0 * g.dy);
	}

	[[nodiscard]] static double dz(const std::vector<double>& f, const BssnGrid& g, int i, int j, int k) noexcept {
		return (-f[g.index(i, j, k + 2)] + 8.0 * f[g.index(i, j, k + 1)] - 8.0 * f[g.index(i, j, k - 1)] + f[g.index(i, j, k - 2)]) / (12.0 * g.dz);
	}

	[[nodiscard]] static double dxx(const std::vector<double>& f, const BssnGrid& g, int i, int j, int k) noexcept {
		return (-f[g.index(i + 2, j, k)] + 16.0 * f[g.index(i + 1, j, k)] - 30.0 * f[g.index(i, j, k)] + 16.0 * f[g.index(i - 1, j, k)] - f[g.index(i - 2, j, k)]) / (12.0 * g.dx * g.dx);
	}

	[[nodiscard]] static double dyy(const std::vector<double>& f, const BssnGrid& g, int i, int j, int k) noexcept {
		return (-f[g.index(i, j + 2, k)] + 16.0 * f[g.index(i, j + 1, k)] - 30.0 * f[g.index(i, j, k)] + 16.0 * f[g.index(i, j - 1, k)] - f[g.index(i, j - 2, k)]) / (12.0 * g.dy * g.dy);
	}

	[[nodiscard]] static double dzz(const std::vector<double>& f, const BssnGrid& g, int i, int j, int k) noexcept {
		return (-f[g.index(i, j, k + 2)] + 16.0 * f[g.index(i, j, k + 1)] - 30.0 * f[g.index(i, j, k)] + 16.0 * f[g.index(i, j, k - 1)] - f[g.index(i, j, k - 2)]) / (12.0 * g.dz * g.dz);
	}

public:
	[[nodiscard]] static std::vector<double> compute_hamiltonian(const BssnGrid& grid) noexcept {
		std::vector<double> H(grid.nx * grid.ny * grid.nz, 0.0);

		for (int k = 0; k < static_cast<int>(grid.nz); ++k) {
			for (int j = 0; j < static_cast<int>(grid.ny); ++j) {
				for (int i = 0; i < static_cast<int>(grid.nx); ++i) {
					const size_t idx = grid.index(i, j, k);

					const double g11 = grid.gt11[idx];
					const double g12 = grid.gt12[idx];
					const double g13 = grid.gt13[idx];
					const double g22 = grid.gt22[idx];
					const double g23 = grid.gt23[idx];
					const double g33 = grid.gt33[idx];

					const double det = g11 * (g22 * g33 - g23 * g23) - g12 * (g12 * g33 - g13 * g23) + g13 * (g12 * g23 - g13 * g22);
					const double inv_det = 1.0 / std::max(det, 1e-15);
					const double ig11 = (g22 * g33 - g23 * g23) * inv_det;
					const double ig12 = (g13 * g23 - g12 * g33) * inv_det;
					const double ig13 = (g12 * g23 - g13 * g22) * inv_det;
					const double ig22 = (g11 * g33 - g13 * g13) * inv_det;
					const double ig23 = (g12 * g13 - g11 * g23) * inv_det;
					const double ig33 = (g11 * g22 - g12 * g12) * inv_det;

					const double d2phi_xx = dxx(grid.phi, grid, i, j, k);
					const double d2phi_yy = dyy(grid.phi, grid, i, j, k);
					const double d2phi_zz = dzz(grid.phi, grid, i, j, k);

					const double D2phi = ig11 * d2phi_xx + ig22 * d2phi_yy + ig33 * d2phi_zz;

					const double A11 = grid.At11[idx];
					const double A12 = grid.At12[idx];
					const double A13 = grid.At13[idx];
					const double A22 = grid.At22[idx];
					const double A23 = grid.At23[idx];
					const double A33 = grid.At33[idx];

					const double A2 = ig11 * (ig11 * A11 * A11 + 2.0 * ig12 * A11 * A12 + 2.0 * ig13 * A11 * A13 + ig22 * A12 * A12 + 2.0 * ig23 * A12 * A13 + ig33 * A13 * A13)
					                + 2.0 * ig12 * (ig11 * A11 * A12 + 2.0 * ig12 * A12 * A12 + 2.0 * ig13 * A12 * A13 + ig22 * A12 * A22 + 2.0 * ig23 * A12 * A23 + ig33 * A13 * A23)
					                + 2.0 * ig13 * (ig11 * A11 * A13 + 2.0 * ig12 * A12 * A13 + 2.0 * ig13 * A13 * A13 + ig22 * A13 * A22 + 2.0 * ig23 * A13 * A23 + ig33 * A13 * A33)
					                + ig22 * (ig11 * A12 * A12 + 2.0 * ig12 * A12 * A22 + 2.0 * ig13 * A12 * A23 + ig22 * A22 * A22 + 2.0 * ig23 * A22 * A23 + ig33 * A23 * A23)
					                + 2.0 * ig23 * (ig11 * A12 * A13 + 2.0 * ig12 * A13 * A22 + 2.0 * ig13 * A13 * A23 + ig22 * A22 * A23 + 2.0 * ig23 * A23 * A23 + ig33 * A23 * A33)
					                + ig33 * (ig11 * A13 * A13 + 2.0 * ig12 * A13 * A23 + 2.0 * ig13 * A13 * A33 + ig22 * A23 * A23 + 2.0 * ig23 * A23 * A33 + ig33 * A33 * A33);

					const double K = grid.K[idx];
					const double R_tilde = 0.0;

					H[idx] = R_tilde + 8.0 * D2phi - A2 + (2.0 / 3.0) * K * K;
				}
			}
		}

		return H;
	}
};

}
