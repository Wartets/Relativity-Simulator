#pragma once

#include <vector>
#include <cstddef>
#include <cstdint>
#include <cmath>

namespace Relativistic::Metrics {

struct BssnGrid {
	size_t nx{0};
	size_t ny{0};
	size_t nz{0};
	double dx{1.0};
	double dy{1.0};
	double dz{1.0};

	std::vector<double> phi;
	std::vector<double> K;
	std::vector<double> alpha;

	std::vector<double> gt11, gt12, gt13, gt22, gt23, gt33;
	std::vector<double> At11, At12, At13, At22, At23, At33;
	std::vector<double> Gt1, Gt2, Gt3;
	std::vector<double> beta1, beta2, beta3;

	BssnGrid() = default;

	BssnGrid(size_t dim_x, size_t dim_y, size_t dim_z, double delta_x, double delta_y, double delta_z)
		: nx(dim_x), ny(dim_y), nz(dim_z), dx(delta_x), dy(delta_y), dz(delta_z) {
		const size_t total_size = nx * ny * nz;
		phi.assign(total_size, 0.0);
		K.assign(total_size, 0.0);
		alpha.assign(total_size, 1.0);

		gt11.assign(total_size, 1.0);
		gt12.assign(total_size, 0.0);
		gt13.assign(total_size, 0.0);
		gt22.assign(total_size, 1.0);
		gt23.assign(total_size, 0.0);
		gt33.assign(total_size, 1.0);

		At11.assign(total_size, 0.0);
		At12.assign(total_size, 0.0);
		At13.assign(total_size, 0.0);
		At22.assign(total_size, 0.0);
		At23.assign(total_size, 0.0);
		At33.assign(total_size, 0.0);

		Gt1.assign(total_size, 0.0);
		Gt2.assign(total_size, 0.0);
		Gt3.assign(total_size, 0.0);

		beta1.assign(total_size, 0.0);
		beta2.assign(total_size, 0.0);
		beta3.assign(total_size, 0.0);
	}

	[[nodiscard]] constexpr size_t index(int i, int j, int k) const noexcept {
		const size_t wrap_i = static_cast<size_t>((i % static_cast<int>(nx) + static_cast<int>(nx)) % static_cast<int>(nx));
		const size_t wrap_j = static_cast<size_t>((j % static_cast<int>(ny) + static_cast<int>(ny)) % static_cast<int>(ny));
		const size_t wrap_k = static_cast<size_t>((k % static_cast<int>(nz) + static_cast<int>(nz)) % static_cast<int>(nz));
		return wrap_i + nx * (wrap_j + ny * wrap_k);
	}

	void add_scaled(const BssnGrid& other, double scale) noexcept {
		const size_t total_size = nx * ny * nz;
		for (size_t i = 0; i < total_size; ++i) {
			phi[i] += scale * other.phi[i];
			K[i] += scale * other.K[i];
			alpha[i] += scale * other.alpha[i];

			gt11[i] += scale * other.gt11[i];
			gt12[i] += scale * other.gt12[i];
			gt13[i] += scale * other.gt13[i];
			gt22[i] += scale * other.gt22[i];
			gt23[i] += scale * other.gt23[i];
			gt33[i] += scale * other.gt33[i];

			At11[i] += scale * other.At11[i];
			At12[i] += scale * other.At12[i];
			At13[i] += scale * other.At13[i];
			At22[i] += scale * other.At22[i];
			At23[i] += scale * other.At23[i];
			At33[i] += scale * other.At33[i];

			Gt1[i] += scale * other.Gt1[i];
			Gt2[i] += scale * other.Gt2[i];
			Gt3[i] += scale * other.Gt3[i];

			beta1[i] += scale * other.beta1[i];
			beta2[i] += scale * other.beta2[i];
			beta3[i] += scale * other.beta3[i];
		}
	}

	void copy_from(const BssnGrid& other) noexcept {
		const size_t total_size = nx * ny * nz;
		for (size_t i = 0; i < total_size; ++i) {
			phi[i] = other.phi[i];
			K[i] = other.K[i];
			alpha[i] = other.alpha[i];

			gt11[i] = other.gt11[i];
			gt12[i] = other.gt12[i];
			gt13[i] = other.gt13[i];
			gt22[i] = other.gt22[i];
			gt23[i] = other.gt23[i];
			gt33[i] = other.gt33[i];

			At11[i] = other.At11[i];
			At12[i] = other.At12[i];
			At13[i] = other.At13[i];
			At22[i] = other.At22[i];
			At23[i] = other.At23[i];
			At33[i] = other.At33[i];

			Gt1[i] = other.Gt1[i];
			Gt2[i] = other.Gt2[i];
			Gt3[i] = other.Gt3[i];

			beta1[i] = other.beta1[i];
			beta2[i] = other.beta2[i];
			beta3[i] = other.beta3[i];
		}
	}
};

}
