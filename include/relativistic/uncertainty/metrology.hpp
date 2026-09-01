#pragma once

#include "relativistic/uncertainty/uncertainty_types.hpp"
#include "relativistic/uncertainty/covariance.hpp"
#include "relativistic/uncertainty/zonotope.hpp"
#include "relativistic/uncertainty/polynomial_chaos.hpp"
#include <vector>
#include <array>
#include <span>
#include <cmath>
#include <numbers>
#include <algorithm>

namespace Relativistic::Uncertainty {

struct EllipsoidMeshVertex {
	std::array<double, 3> position{0.0, 0.0, 0.0};
	std::array<double, 3> normal{0.0, 0.0, 0.0};
	double sigma_level{1.0};
};

struct EllipsoidMesh3D {
	std::vector<EllipsoidMeshVertex> vertices{};
	std::vector<uint32_t> triangle_indices{};
};

struct ProbabilityHeatmap2D {
	size_t width{100};
	size_t height{100};
	double x_min{-10.0};
	double x_max{10.0};
	double y_min{-10.0};
	double y_max{10.0};
	std::vector<double> density_grid{};
};

class MetrologyVisualizer {
public:
	[[nodiscard]] static EllipsoidMesh3D generate_covariance_ellipsoid_mesh(
		const std::array<double, 3>& center,
		const CovarianceMatrix<double, 3>& cov,
		double sigma_scale = 1.0,
		size_t u_segments = 24,
		size_t v_segments = 12
	) {
		EllipsoidMesh3D mesh;
		const auto es = cov.compute_eigensystem();

		const double a = sigma_scale * std::sqrt(std::max(es.eigenvalues[0], 1e-15));
		const double b = sigma_scale * std::sqrt(std::max(es.eigenvalues[1], 1e-15));
		const double c = sigma_scale * std::sqrt(std::max(es.eigenvalues[2], 1e-15));

		const size_t num_vertices = (u_segments + 1) * (v_segments + 1);
		mesh.vertices.reserve(num_vertices);

		for (size_t j = 0; j <= v_segments; ++j) {
			const double theta = (static_cast<double>(j) / static_cast<double>(v_segments)) * std::numbers::pi;
			const double sin_t = std::sin(theta);
			const double cos_t = std::cos(theta);

			for (size_t i = 0; i <= u_segments; ++i) {
				const double phi = (static_cast<double>(i) / static_cast<double>(u_segments)) * 2.0 * std::numbers::pi;
				const double cos_p = std::cos(phi);
				const double sin_p = std::sin(phi);

				const double x_local = a * sin_t * cos_p;
				const double y_local = b * sin_t * sin_p;
				const double z_local = c * cos_t;

				double x_world = center[0];
				double y_world = center[1];
				double z_world = center[2];

				for (size_t d = 0; d < 3; ++d) {
					x_world += es.eigenvectors[0][d] * (d == 0 ? x_local : (d == 1 ? y_local : z_local));
					y_world += es.eigenvectors[1][d] * (d == 0 ? x_local : (d == 1 ? y_local : z_local));
					z_world += es.eigenvectors[2][d] * (d == 0 ? x_local : (d == 1 ? y_local : z_local));
				}

				const double nx_local = (a > 0.0) ? (x_local / (a * a)) : 0.0;
				const double ny_local = (b > 0.0) ? (y_local / (b * b)) : 0.0;
				const double nz_local = (c > 0.0) ? (z_local / (c * c)) : 0.0;
				const double n_len = std::sqrt(nx_local * nx_local + ny_local * ny_local + nz_local * nz_local);

				EllipsoidMeshVertex v;
				v.position = {x_world, y_world, z_world};
				v.normal = (n_len > 0.0) ? std::array<double, 3>{nx_local / n_len, ny_local / n_len, nz_local / n_len} : std::array<double, 3>{0.0, 0.0, 1.0};
				v.sigma_level = sigma_scale;

				mesh.vertices.push_back(v);
			}
		}

		mesh.triangle_indices.reserve(u_segments * v_segments * 6);
		for (size_t j = 0; j < v_segments; ++j) {
			for (size_t i = 0; i < u_segments; ++i) {
				const uint32_t p0 = static_cast<uint32_t>(j * (u_segments + 1) + i);
				const uint32_t p1 = static_cast<uint32_t>(p0 + 1);
				const uint32_t p2 = static_cast<uint32_t>((j + 1) * (u_segments + 1) + i + 1);
				const uint32_t p3 = static_cast<uint32_t>((j + 1) * (u_segments + 1) + i);

				mesh.triangle_indices.push_back(p0);
				mesh.triangle_indices.push_back(p1);
				mesh.triangle_indices.push_back(p2);

				mesh.triangle_indices.push_back(p0);
				mesh.triangle_indices.push_back(p2);
				mesh.triangle_indices.push_back(p3);
			}
		}

		return mesh;
	}

	[[nodiscard]] static ProbabilityHeatmap2D generate_2d_gaussian_heatmap(
		const std::array<double, 2>& mean,
		const CovarianceMatrix<double, 2>& cov,
		size_t width = 100,
		size_t height = 100,
		double n_sigma_bounds = 4.0
	) {
		ProbabilityHeatmap2D map;
		map.width = width;
		map.height = height;

		const double sigma_x = cov.standard_deviation(0);
		const double sigma_y = cov.standard_deviation(1);

		map.x_min = mean[0] - n_sigma_bounds * sigma_x;
		map.x_max = mean[0] + n_sigma_bounds * sigma_x;
		map.y_min = mean[1] - n_sigma_bounds * sigma_y;
		map.y_max = mean[1] + n_sigma_bounds * sigma_y;

		map.density_grid.assign(width * height, 0.0);

		const double det = cov(0, 0) * cov(1, 1) - cov(0, 1) * cov(1, 0);
		if (det <= 1e-30) {
			return map;
		}

		const double inv_cov00 = cov(1, 1) / det;
		const double inv_cov01 = -cov(0, 1) / det;
		const double inv_cov11 = cov(0, 0) / det;
		const double norm_const = 1.0 / (2.0 * std::numbers::pi * std::sqrt(det));

		const double dx = (map.x_max - map.x_min) / static_cast<double>(width - 1);
		const double dy = (map.y_max - map.y_min) / static_cast<double>(height - 1);

		for (size_t j = 0; j < height; ++j) {
			const double y = map.y_min + static_cast<double>(j) * dy;
			const double dy_val = y - mean[1];

			for (size_t i = 0; i < width; ++i) {
				const double x = map.x_min + static_cast<double>(i) * dx;
				const double dx_val = x - mean[0];

				const double mahalanobis_sq = dx_val * (inv_cov00 * dx_val + inv_cov01 * dy_val) + dy_val * (inv_cov01 * dx_val + inv_cov11 * dy_val);
				map.density_grid[j * width + i] = norm_const * std::exp(-0.5 * mahalanobis_sq);
			}
		}

		return map;
	}

	[[nodiscard]] static std::vector<QuantileBands> compute_waveform_quantiles(
		std::span<const PolynomialChaos1D> time_series_pce
	) {
		std::vector<QuantileBands> bands;
		bands.reserve(time_series_pce.size());
		for (const auto& pce : time_series_pce) {
			bands.push_back(pce.compute_quantiles(4000));
		}
		return bands;
	}

	[[nodiscard]] static ProbabilityHeatmap2D generate_empirical_2d_heatmap(
		std::span<const std::array<double, 2>> points,
		size_t width = 100,
		size_t height = 100,
		double x_min = -10.0,
		double x_max = 10.0,
		double y_min = -10.0,
		double y_max = 10.0
	) {
		ProbabilityHeatmap2D map;
		map.width = width;
		map.height = height;
		map.x_min = x_min;
		map.x_max = x_max;
		map.y_min = y_min;
		map.y_max = y_max;
		map.density_grid.assign(width * height, 0.0);

		if (points.empty()) return map;

		const double dx = (x_max - x_min) / static_cast<double>(width);
		const double dy = (y_max - y_min) / static_cast<double>(height);

		for (const auto& pt : points) {
			if (pt[0] >= x_min && pt[0] < x_max && pt[1] >= y_min && pt[1] < y_max) {
				const size_t i = std::clamp(static_cast<size_t>((pt[0] - x_min) / dx), size_t{0}, width - 1);
				const size_t j = std::clamp(static_cast<size_t>((pt[1] - y_min) / dy), size_t{0}, height - 1);
				map.density_grid[j * width + i] += 1.0;
			}
		}

		const double norm = 1.0 / (static_cast<double>(points.size()) * dx * dy);
		for (auto& val : map.density_grid) {
			val *= norm;
		}

		return map;
	}
};

}
