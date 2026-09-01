#pragma once

#include "relativistic/metrics/bssn_grid.hpp"
#include <cmath>
#include <algorithm>

namespace Relativistic::Metrics {

class BssnEvolution {
private:
	BssnGrid k1_, k2_, k3_, k4_, temp_;

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

	[[nodiscard]] static double dxy(const std::vector<double>& f, const BssnGrid& g, int i, int j, int k) noexcept {
		return (f[g.index(i + 1, j + 1, k)] - f[g.index(i - 1, j + 1, k)] - f[g.index(i + 1, j - 1, k)] + f[g.index(i - 1, j - 1, k)]) / (4.0 * g.dx * g.dy);
	}

	[[nodiscard]] static double dxz(const std::vector<double>& f, const BssnGrid& g, int i, int j, int k) noexcept {
		return (f[g.index(i + 1, j, k + 1)] - f[g.index(i - 1, j, k + 1)] - f[g.index(i + 1, j, k - 1)] + f[g.index(i - 1, j, k - 1)]) / (4.0 * g.dx * g.dz);
	}

	[[nodiscard]] static double dyz(const std::vector<double>& f, const BssnGrid& g, int i, int j, int k) noexcept {
		return (f[g.index(i, j + 1, k + 1)] - f[g.index(i, j - 1, k + 1)] - f[g.index(i, j + 1, k - 1)] + f[g.index(i, j - 1, k - 1)]) / (4.0 * g.dy * g.dz);
	}

public:
	BssnEvolution(size_t nx, size_t ny, size_t nz, double dx, double dy, double dz)
		: k1_(nx, ny, nz, dx, dy, dz),
		  k2_(nx, ny, nz, dx, dy, dz),
		  k3_(nx, ny, nz, dx, dy, dz),
		  k4_(nx, ny, nz, dx, dy, dz),
		  temp_(nx, ny, nz, dx, dy, dz) {}

	void compute_rhs(const BssnGrid& in, BssnGrid& out) noexcept {
		for (int k = 0; k < static_cast<int>(in.nz); ++k) {
			for (int j = 0; j < static_cast<int>(in.ny); ++j) {
				for (int i = 0; i < static_cast<int>(in.nx); ++i) {
					const size_t idx = in.index(i, j, k);

					const double alpha = in.alpha[idx];
					const double K = in.K[idx];
					const double b1 = in.beta1[idx];
					const double b2 = in.beta2[idx];
					const double b3 = in.beta3[idx];

					const double dphi_x = dx(in.phi, in, i, j, k);
					const double dphi_y = dy(in.phi, in, i, j, k);
					const double dphi_z = dz(in.phi, in, i, j, k);

					const double db1_x = dx(in.beta1, in, i, j, k);
					const double db2_y = dy(in.beta2, in, i, j, k);
					const double db3_z = dz(in.beta3, in, i, j, k);
					const double div_beta = db1_x + db2_y + db3_z;

					out.phi[idx] = - (1.0 / 6.0) * alpha * K + b1 * dphi_x + b2 * dphi_y + b3 * dphi_z + (1.0 / 6.0) * div_beta;

					const double g11 = in.gt11[idx];
					const double g12 = in.gt12[idx];
					const double g13 = in.gt13[idx];
					const double g22 = in.gt22[idx];
					const double g23 = in.gt23[idx];
					const double g33 = in.gt33[idx];

					const double A11 = in.At11[idx];
					const double A12 = in.At12[idx];
					const double A13 = in.At13[idx];
					const double A22 = in.At22[idx];
					const double A23 = in.At23[idx];
					const double A33 = in.At33[idx];

					const double dg11_x = dx(in.gt11, in, i, j, k);
					const double dg11_y = dy(in.gt11, in, i, j, k);
					const double dg11_z = dz(in.gt11, in, i, j, k);

					out.gt11[idx] = -2.0 * alpha * A11 + b1 * dg11_x + b2 * dg11_y + b3 * dg11_z + 2.0 * (g11 * db1_x + g12 * dx(in.beta2, in, i, j, k) + g13 * dx(in.beta3, in, i, j, k)) - (2.0 / 3.0) * g11 * div_beta;
					out.gt22[idx] = -2.0 * alpha * A22 + b1 * dx(in.gt22, in, i, j, k) + b2 * dy(in.gt22, in, i, j, k) + b3 * dz(in.gt22, in, i, j, k) + 2.0 * (g12 * dy(in.beta1, in, i, j, k) + g22 * db2_y + g23 * dy(in.beta3, in, i, j, k)) - (2.0 / 3.0) * g22 * div_beta;
					out.gt33[idx] = -2.0 * alpha * A33 + b1 * dx(in.gt33, in, i, j, k) + b2 * dy(in.gt33, in, i, j, k) + b3 * dz(in.gt33, in, i, j, k) + 2.0 * (g13 * dz(in.beta1, in, i, j, k) + g23 * dz(in.beta2, in, i, j, k) + g33 * db3_z) - (2.0 / 3.0) * g33 * div_beta;

					out.gt12[idx] = -2.0 * alpha * A12 + b1 * dx(in.gt12, in, i, j, k) + b2 * dy(in.gt12, in, i, j, k) + b3 * dz(in.gt12, in, i, j, k) + g11 * dy(in.beta1, in, i, j, k) + g12 * (db1_x + db2_y) + g22 * dx(in.beta2, in, i, j, k) + g13 * dy(in.beta3, in, i, j, k) + g23 * dx(in.beta3, in, i, j, k) - (2.0 / 3.0) * g12 * div_beta;
					out.gt13[idx] = -2.0 * alpha * A13 + b1 * dx(in.gt13, in, i, j, k) + b2 * dy(in.gt13, in, i, j, k) + b3 * dz(in.gt13, in, i, j, k) + g11 * dz(in.beta1, in, i, j, k) + g13 * (db1_x + db3_z) + g33 * dx(in.beta3, in, i, j, k) + g12 * dz(in.beta2, in, i, j, k) + g23 * dx(in.beta2, in, i, j, k) - (2.0 / 3.0) * g13 * div_beta;
					out.gt23[idx] = -2.0 * alpha * A23 + b1 * dx(in.gt23, in, i, j, k) + b2 * dy(in.gt23, in, i, j, k) + b3 * dz(in.gt23, in, i, j, k) + g22 * dz(in.beta2, in, i, j, k) + g23 * (db2_y + db3_z) + g33 * dy(in.beta3, in, i, j, k) + g12 * dz(in.beta1, in, i, j, k) + g13 * dy(in.beta1, in, i, j, k) - (2.0 / 3.0) * g23 * div_beta;

					const double det = g11 * (g22 * g33 - g23 * g23) - g12 * (g12 * g33 - g13 * g23) + g13 * (g12 * g23 - g13 * g22);
					const double inv_det = 1.0 / std::max(det, 1e-15);
					const double ig11 = (g22 * g33 - g23 * g23) * inv_det;
					const double ig12 = (g13 * g23 - g12 * g33) * inv_det;
					const double ig13 = (g12 * g23 - g13 * g22) * inv_det;
					const double ig22 = (g11 * g33 - g13 * g13) * inv_det;
					const double ig23 = (g12 * g13 - g11 * g23) * inv_det;
					const double ig33 = (g11 * g22 - g12 * g12) * inv_det;

					const double A2 = ig11 * (ig11 * A11 * A11 + 2.0 * ig12 * A11 * A12 + 2.0 * ig13 * A11 * A13 + ig22 * A12 * A12 + 2.0 * ig23 * A12 * A13 + ig33 * A13 * A13)
					                + 2.0 * ig12 * (ig11 * A11 * A12 + 2.0 * ig12 * A12 * A12 + 2.0 * ig13 * A12 * A13 + ig22 * A12 * A22 + 2.0 * ig23 * A12 * A23 + ig33 * A13 * A23)
					                + 2.0 * ig13 * (ig11 * A11 * A13 + 2.0 * ig12 * A12 * A13 + 2.0 * ig13 * A13 * A13 + ig22 * A13 * A22 + 2.0 * ig23 * A13 * A23 + ig33 * A13 * A33)
					                + ig22 * (ig11 * A12 * A12 + 2.0 * ig12 * A12 * A22 + 2.0 * ig13 * A12 * A23 + ig22 * A22 * A22 + 2.0 * ig23 * A22 * A23 + ig33 * A23 * A23)
					                + 2.0 * ig23 * (ig11 * A12 * A13 + 2.0 * ig12 * A13 * A22 + 2.0 * ig13 * A13 * A23 + ig22 * A22 * A23 + 2.0 * ig23 * A23 * A23 + ig33 * A23 * A33)
					                + ig33 * (ig11 * A13 * A13 + 2.0 * ig12 * A13 * A23 + 2.0 * ig13 * A13 * A33 + ig22 * A23 * A23 + 2.0 * ig23 * A23 * A33 + ig33 * A33 * A33);

					const double dK_x = dx(in.K, in, i, j, k);
					const double dK_y = dy(in.K, in, i, j, k);
					const double dK_z = dz(in.K, in, i, j, k);

					const double d2a_xx = dxx(in.alpha, in, i, j, k);
					const double d2a_yy = dyy(in.alpha, in, i, j, k);
					const double d2a_zz = dzz(in.alpha, in, i, j, k);
					const double d2a_xy = dxy(in.alpha, in, i, j, k);
					const double d2a_xz = dxz(in.alpha, in, i, j, k);
					const double d2a_yz = dyz(in.alpha, in, i, j, k);

					const double D2a = ig11 * d2a_xx + ig22 * d2a_yy + ig33 * d2a_zz + 2.0 * (ig12 * d2a_xy + ig13 * d2a_xz + ig23 * d2a_yz);

					out.K[idx] = -std::exp(-4.0 * in.phi[idx]) * D2a + alpha * (A2 + (1.0 / 3.0) * K * K) + b1 * dK_x + b2 * dK_y + b3 * dK_z;

					out.At11[idx] = 0.0;
					out.At12[idx] = 0.0;
					out.At13[idx] = 0.0;
					out.At22[idx] = 0.0;
					out.At23[idx] = 0.0;
					out.At33[idx] = 0.0;

					out.Gt1[idx] = 0.0;
					out.Gt2[idx] = 0.0;
					out.Gt3[idx] = 0.0;

					out.alpha[idx] = -2.0 * alpha * K + b1 * dx(in.alpha, in, i, j, k) + b2 * dy(in.alpha, in, i, j, k) + b3 * dz(in.alpha, in, i, j, k);
					out.beta1[idx] = 0.0;
					out.beta2[idx] = 0.0;
					out.beta3[idx] = 0.0;
				}
			}
		}
	}

	void step_rk4(BssnGrid& grid, double dt) noexcept {
		compute_rhs(grid, k1_);

		temp_.copy_from(grid);
		temp_.add_scaled(k1_, 0.5 * dt);
		compute_rhs(temp_, k2_);

		temp_.copy_from(grid);
		temp_.add_scaled(k2_, 0.5 * dt);
		compute_rhs(temp_, k3_);

		temp_.copy_from(grid);
		temp_.add_scaled(k3_, dt);
		compute_rhs(temp_, k4_);

		const double sixth_dt = dt / 6.0;
		grid.add_scaled(k1_, sixth_dt);
		grid.add_scaled(k2_, 2.0 * sixth_dt);
		grid.add_scaled(k3_, 2.0 * sixth_dt);
		grid.add_scaled(k4_, sixth_dt);
	}
};

}
