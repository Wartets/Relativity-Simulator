#pragma once

#include "relativistic/metrics/bssn_grid.hpp"
#include <array>
#include <cmath>

namespace Relativistic::Metrics {

struct BssnPointData {
	double phi{0.0}, K{0.0}, alpha{1.0};
	double gt11{1.0}, gt12{0.0}, gt13{0.0}, gt22{1.0}, gt23{0.0}, gt33{1.0};
	double At11{0.0}, At12{0.0}, At13{0.0}, At22{0.0}, At23{0.0}, At33{0.0};
	double Gt1{0.0}, Gt2{0.0}, Gt3{0.0};
	double beta1{0.0}, beta2{0.0}, beta3{0.0};
};

class TricubicInterpolator {
private:
	[[nodiscard]] static constexpr double bspline_weight(int i, double t) noexcept {
		const double t2 = t * t;
		const double t3 = t2 * t;
		if (i == 0) return (1.0 - 3.0 * t + 3.0 * t2 - t3) / 6.0;
		if (i == 1) return (4.0 - 6.0 * t2 + 3.0 * t3) / 6.0;
		if (i == 2) return (1.0 + 3.0 * t + 3.0 * t2 - 3.0 * t3) / 6.0;
		if (i == 3) return t3 / 6.0;
		return 0.0;
	}

	[[nodiscard]] static double interpolate_field(const std::vector<double>& field, const BssnGrid& grid, double x, double y, double z) noexcept {
		const double fx = x / grid.dx;
		const double fy = y / grid.dy;
		const double fz = z / grid.dz;

		const int ix = static_cast<int>(std::floor(fx));
		const int iy = static_cast<int>(std::floor(fy));
		const int iz = static_cast<int>(std::floor(fz));

		const double tx = fx - static_cast<double>(ix);
		const double ty = fy - static_cast<double>(iy);
		const double tz = fz - static_cast<double>(iz);

		double wx[4], wy[4], wz[4];
		for (int i = 0; i < 4; ++i) {
			wx[i] = bspline_weight(i, tx);
			wy[i] = bspline_weight(i, ty);
			wz[i] = bspline_weight(i, tz);
		}

		double sum = 0.0;
		for (int k = 0; k < 4; ++k) {
			for (int j = 0; j < 4; ++j) {
				for (int i = 0; i < 4; ++i) {
					const size_t idx = grid.index(ix - 1 + i, iy - 1 + j, iz - 1 + k);
					sum += wx[i] * wy[j] * wz[k] * field[idx];
				}
			}
		}
		return sum;
	}

public:
	[[nodiscard]] static BssnPointData evaluate(const BssnGrid& grid, double x, double y, double z) noexcept {
		BssnPointData pt;
		pt.phi = interpolate_field(grid.phi, grid, x, y, z);
		pt.K = interpolate_field(grid.K, grid, x, y, z);
		pt.alpha = interpolate_field(grid.alpha, grid, x, y, z);

		pt.gt11 = interpolate_field(grid.gt11, grid, x, y, z);
		pt.gt12 = interpolate_field(grid.gt12, grid, x, y, z);
		pt.gt13 = interpolate_field(grid.gt13, grid, x, y, z);
		pt.gt22 = interpolate_field(grid.gt22, grid, x, y, z);
		pt.gt23 = interpolate_field(grid.gt23, grid, x, y, z);
		pt.gt33 = interpolate_field(grid.gt33, grid, x, y, z);

		pt.At11 = interpolate_field(grid.At11, grid, x, y, z);
		pt.At12 = interpolate_field(grid.At12, grid, x, y, z);
		pt.At13 = interpolate_field(grid.At13, grid, x, y, z);
		pt.At22 = interpolate_field(grid.At22, grid, x, y, z);
		pt.At23 = interpolate_field(grid.At23, grid, x, y, z);
		pt.At33 = interpolate_field(grid.At33, grid, x, y, z);

		pt.Gt1 = interpolate_field(grid.Gt1, grid, x, y, z);
		pt.Gt2 = interpolate_field(grid.Gt2, grid, x, y, z);
		pt.Gt3 = interpolate_field(grid.Gt3, grid, x, y, z);

		pt.beta1 = interpolate_field(grid.beta1, grid, x, y, z);
		pt.beta2 = interpolate_field(grid.beta2, grid, x, y, z);
		pt.beta3 = interpolate_field(grid.beta3, grid, x, y, z);

		return pt;
	}
};

class QuinticHermiteTimeInterpolator {
public:
	[[nodiscard]] static BssnPointData evaluate(
		const BssnPointData& p0, const BssnPointData& dp0, const BssnPointData& ddp0,
		const BssnPointData& p1, const BssnPointData& dp1, const BssnPointData& ddp1,
		double t
	) noexcept {
		const double t2 = t * t;
		const double t3 = t2 * t;
		const double t4 = t3 * t;
		const double t5 = t4 * t;

		const double h0 = 1.0 - 10.0 * t3 + 15.0 * t4 - 6.0 * t5;
		const double h1 = t - 6.0 * t3 + 8.0 * t4 - 3.0 * t5;
		const double h2 = 0.5 * t2 - 1.5 * t3 + 1.5 * t4 - 0.5 * t5;
		const double h3 = 10.0 * t3 - 15.0 * t4 + 6.0 * t5;
		const double h4 = -4.0 * t3 + 7.0 * t4 - 3.0 * t5;
		const double h5 = 0.5 * t3 - t4 + 0.5 * t5;

		auto interp = [&](double v0, double v1, double dv0, double dv1, double ddv0, double ddv1) {
			return v0 * h0 + dv0 * h1 + ddv0 * h2 + v1 * h3 + dv1 * h4 + ddv1 * h5;
		};

		BssnPointData pt;
		pt.phi = interp(p0.phi, p1.phi, dp0.phi, dp1.phi, ddp0.phi, ddp1.phi);
		pt.K = interp(p0.K, p1.K, dp0.K, dp1.K, ddp0.K, ddp1.K);
		pt.alpha = interp(p0.alpha, p1.alpha, dp0.alpha, dp1.alpha, ddp0.alpha, ddp1.alpha);

		pt.gt11 = interp(p0.gt11, p1.gt11, dp0.gt11, dp1.gt11, ddp0.gt11, ddp1.gt11);
		pt.gt12 = interp(p0.gt12, p1.gt12, dp0.gt12, dp1.gt12, ddp0.gt12, ddp1.gt12);
		pt.gt13 = interp(p0.gt13, p1.gt13, dp0.gt13, dp1.gt13, ddp0.gt13, ddp1.gt13);
		pt.gt22 = interp(p0.gt22, p1.gt22, dp0.gt22, dp1.gt22, ddp0.gt22, ddp1.gt22);
		pt.gt23 = interp(p0.gt23, p1.gt23, dp0.gt23, dp1.gt23, ddp0.gt23, ddp1.gt23);
		pt.gt33 = interp(p0.gt33, p1.gt33, dp0.gt33, dp1.gt33, ddp0.gt33, ddp1.gt33);

		pt.At11 = interp(p0.At11, p1.At11, dp0.At11, dp1.At11, ddp0.At11, ddp1.At11);
		pt.At12 = interp(p0.At12, p1.At12, dp0.At12, dp1.At12, ddp0.At12, ddp1.At12);
		pt.At13 = interp(p0.At13, p1.At13, dp0.At13, dp1.At13, ddp0.At13, ddp1.At13);
		pt.At22 = interp(p0.At22, p1.At22, dp0.At22, dp1.At22, ddp0.At22, ddp1.At22);
		pt.At23 = interp(p0.At23, p1.At23, dp0.At23, dp1.At23, ddp0.At23, ddp1.At23);
		pt.At33 = interp(p0.At33, p1.At33, dp0.At33, dp1.At33, ddp0.At33, ddp1.At33);

		pt.Gt1 = interp(p0.Gt1, p1.Gt1, dp0.Gt1, dp1.Gt1, ddp0.Gt1, ddp1.Gt1);
		pt.Gt2 = interp(p0.Gt2, p1.Gt2, dp0.Gt2, dp1.Gt2, ddp0.Gt2, ddp1.Gt2);
		pt.Gt3 = interp(p0.Gt3, p1.Gt3, dp0.Gt3, dp1.Gt3, ddp0.Gt3, ddp1.Gt3);

		pt.beta1 = interp(p0.beta1, p1.beta1, dp0.beta1, dp1.beta1, ddp0.beta1, ddp1.beta1);
		pt.beta2 = interp(p0.beta2, p1.beta2, dp0.beta2, dp1.beta2, ddp0.beta2, ddp1.beta2);
		pt.beta3 = interp(p0.beta3, p1.beta3, dp0.beta3, dp1.beta3, ddp0.beta3, ddp1.beta3);

		return pt;
	}
};

}
