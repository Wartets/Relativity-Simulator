#include "relativistic/uncertainty/metrology.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

int main() {
	using namespace Relativistic::Uncertainty;

	CovarianceMatrix<double, 3> cov3;
	cov3(0, 0) = 4.0;
	cov3(1, 1) = 2.0;
	cov3(2, 2) = 1.0;

	const auto mesh = MetrologyVisualizer::generate_covariance_ellipsoid_mesh({0.0, 0.0, 0.0}, cov3, 2.0, 16, 8);
	assert(!mesh.vertices.empty());
	assert(!mesh.triangle_indices.empty());

	CovarianceMatrix<double, 2> cov2;
	cov2(0, 0) = 1.0;
	cov2(1, 1) = 1.0;

	const auto heatmap = MetrologyVisualizer::generate_2d_gaussian_heatmap({0.0, 0.0}, cov2, 50, 50);
	assert(heatmap.width == 50);
	assert(heatmap.height == 50);
	assert(!heatmap.density_grid.empty());

	std::cout << "All metrology visualizer tests passed successfully.\n";
	return 0;
}
