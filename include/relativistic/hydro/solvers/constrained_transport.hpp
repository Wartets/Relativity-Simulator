#pragma once

#include "relativistic/hydro/hydro_types.hpp"
#include <vector>
#include <span>
#include <cmath>
#include <algorithm>

namespace Relativistic::Hydro {

template <typename Scalar = double>
class ConstrainedTransport2D {
private:
	size_t nx_{0};
	size_t ny_{0};
	Scalar dx_{static_cast<Scalar>(1.0)};
	Scalar dy_{static_cast<Scalar>(1.0)};

	std::vector<Scalar> bx_face_{};
	std::vector<Scalar> by_face_{};
	std::vector<Scalar> ez_vertex_{};

public:
	ConstrainedTransport2D() = default;

	ConstrainedTransport2D(size_t nx, size_t ny, Scalar dx, Scalar dy)
		: nx_(nx), ny_(ny), dx_(dx), dy_(dy) {
		resize(nx, ny, dx, dy);
	}

	void resize(size_t nx, size_t ny, Scalar dx, Scalar dy) {
		nx_ = nx;
		ny_ = ny;
		dx_ = dx;
		dy_ = dy;
		bx_face_.assign((nx + 1) * ny, static_cast<Scalar>(0.0));
		by_face_.assign(nx * (ny + 1), static_cast<Scalar>(0.0));
		ez_vertex_.assign((nx + 1) * (ny + 1), static_cast<Scalar>(0.0));
	}

	[[nodiscard]] constexpr size_t nx() const noexcept { return nx_; }
	[[nodiscard]] constexpr size_t ny() const noexcept { return ny_; }

	[[nodiscard]] Scalar bx_face(size_t i, size_t j) const noexcept {
		return bx_face_[j * (nx_ + 1) + i];
	}

	Scalar& bx_face(size_t i, size_t j) noexcept {
		return bx_face_[j * (nx_ + 1) + i];
	}

	[[nodiscard]] Scalar by_face(size_t i, size_t j) const noexcept {
		return by_face_[j * nx_ + i];
	}

	Scalar& by_face(size_t i, size_t j) noexcept {
		return by_face_[j * nx_ + i];
	}

	[[nodiscard]] Scalar ez_vertex(size_t i, size_t j) const noexcept {
		return ez_vertex_[j * (nx_ + 1) + i];
	}

	Scalar& ez_vertex(size_t i, size_t j) noexcept {
		return ez_vertex_[j * (nx_ + 1) + i];
	}

	void compute_vertex_electric_fields_from_cell_centered(
		std::span<const PrimitiveVariables<Scalar>> cells
	) noexcept {
		for (size_t j = 0; j <= ny_; ++j) {
			for (size_t i = 0; i <= nx_; ++i) {
				const size_t im1 = (i > 0) ? (i - 1) : 0;
				const size_t i0 = (i < nx_) ? i : (nx_ - 1);
				const size_t jm1 = (j > 0) ? (j - 1) : 0;
				const size_t j0 = (j < ny_) ? j : (ny_ - 1);

				const auto& c00 = cells[jm1 * nx_ + im1];
				const auto& c10 = cells[jm1 * nx_ + i0];
				const auto& c01 = cells[j0 * nx_ + im1];
				const auto& c11 = cells[j0 * nx_ + i0];

				const Scalar ez00 = -(c00.vx * c00.by - c00.vy * c00.bx);
				const Scalar ez10 = -(c10.vx * c10.by - c10.vy * c10.bx);
				const Scalar ez01 = -(c01.vx * c01.by - c01.vy * c01.bx);
				const Scalar ez11 = -(c11.vx * c11.by - c11.vy * c11.bx);

				ez_vertex(i, j) = static_cast<Scalar>(0.25) * (ez00 + ez10 + ez01 + ez11);
			}
		}
	}

	void update_face_magnetic_fields(Scalar dt) noexcept {
		const Scalar dt_dy = dt / dy_;
		const Scalar dt_dx = dt / dx_;

		for (size_t j = 0; j < ny_; ++j) {
			for (size_t i = 0; i <= nx_; ++i) {
				const Scalar dez_dy = ez_vertex(i, j + 1) - ez_vertex(i, j);
				bx_face(i, j) -= dt_dy * dez_dy;
			}
		}

		for (size_t j = 0; j <= ny_; ++j) {
			for (size_t i = 0; i < nx_; ++i) {
				const Scalar dez_dx = ez_vertex(i + 1, j) - ez_vertex(i, j);
				by_face(i, j) += dt_dx * dez_dx;
			}
		}
	}

	void average_to_cell_centers(std::span<PrimitiveVariables<Scalar>> cells) const noexcept {
		for (size_t j = 0; j < ny_; ++j) {
			for (size_t i = 0; i < nx_; ++i) {
				auto& c = cells[j * nx_ + i];
				c.bx = static_cast<Scalar>(0.5) * (bx_face(i, j) + bx_face(i + 1, j));
				c.by = static_cast<Scalar>(0.5) * (by_face(i, j) + by_face(i, j + 1));
				c.recompute_derived();
			}
		}
	}

	[[nodiscard]] Scalar compute_max_divergence() const noexcept {
		Scalar max_div = static_cast<Scalar>(0.0);
		for (size_t j = 0; j < ny_; ++j) {
			for (size_t i = 0; i < nx_; ++i) {
				const Scalar dbx = (bx_face(i + 1, j) - bx_face(i, j)) / dx_;
				const Scalar dby = (by_face(i, j + 1) - by_face(i, j)) / dy_;
				const Scalar div_b = std::abs(dbx + dby);
				if (div_b > max_div) {
					max_div = div_b;
				}
			}
		}
		return max_div;
	}
};

}
