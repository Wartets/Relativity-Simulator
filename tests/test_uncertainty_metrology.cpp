#include "support/test_harness.hpp"
#include "relativistic/uncertainty/metrology.hpp"

using namespace Relativistic::Uncertainty;
using Relativistic::Testing::TestSuite;

int main() {
	TestSuite suite(
		"uncertainty_metrology",
		"Verifies the metrology visualizer: a covariance ellipsoid mesh with the requested tessellation is generated for a diagonal covariance, and a two-dimensional Gaussian heatmap has the requested raster and a populated density grid."
	);

	suite.section("Covariance ellipsoid");
	CovarianceMatrix<double, 3> covariance;
	covariance(0, 0) = 4.0;
	covariance(1, 1) = 2.0;
	covariance(2, 2) = 1.0;
	const auto mesh = MetrologyVisualizer::generate_covariance_ellipsoid_mesh({0.0, 0.0, 0.0}, covariance, 2.0, 16, 8);
	suite.check(!mesh.vertices.empty(), "the mesh contains vertices", "vertices=" + std::to_string(mesh.vertices.size()));
	suite.check(!mesh.triangle_indices.empty(), "the mesh contains triangles", "indices=" + std::to_string(mesh.triangle_indices.size()));
	suite.check(mesh.triangle_indices.size() % 3 == 0, "the triangle index count is a multiple of three");

	suite.section("Gaussian heatmap");
	CovarianceMatrix<double, 2> planar;
	planar(0, 0) = 1.0;
	planar(1, 1) = 1.0;
	const auto heatmap = MetrologyVisualizer::generate_2d_gaussian_heatmap({0.0, 0.0}, planar, 50, 50);
	suite.check(heatmap.width == 50 && heatmap.height == 50, "the heatmap has the requested raster of 50 by 50 cells");
	suite.check(heatmap.density_grid.size() == 2500, "the density grid holds one value per cell");
	return suite.finish();
}
