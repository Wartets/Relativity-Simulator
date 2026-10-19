#include "support/test_harness.hpp"
#include "relativistic/io/vtk_exporter.hpp"
#include <cmath>
#include <string>
#include <vector>

using namespace Relativistic::Core;
using namespace Relativistic::IO;
using Relativistic::Testing::TestSuite;

namespace {

[[nodiscard]] bool contains(const std::string& text, const char* token) {
	return text.find(token) != std::string::npos;
}

}

int main() {
	TestSuite suite(
		"vtk_exporter",
		"Verifies the VTK PolyData exporter: the polyline document of a geodesic carries its point and line counts and the redshift array, and the horizon sphere document carries the expected number of quadrilaterals."
	);

	suite.section("Geodesic polylines");
	VtkGeodesicPolyline ray;
	for (size_t index = 0; index < 20; ++index) {
		const double t = static_cast<double>(index) * 0.5;
		ray.points.push_back({t, std::sin(t), std::cos(t)});
		ray.data.push_back({.affine_param = t, .redshift = 1.0 + 0.1 * t, .four_velocity = FourVector<double>(1.0, 1.0, 0.0, 0.0)});
	}
	const std::string polyline = VtkExporter::export_geodesics_vtp({ray});
	suite.check(!polyline.empty(), "the geodesic export is not empty");
	suite.check(contains(polyline, "<VTKFile type=\"PolyData\""), "the document is a VTK PolyData file");
	suite.check(contains(polyline, "NumberOfPoints=\"20\""), "the piece declares 20 points");
	suite.check(contains(polyline, "NumberOfLines=\"1\""), "the piece declares one line");
	suite.check(contains(polyline, "Name=\"Redshift\""), "the redshift point data array is present");

	suite.section("Horizon sphere");
	const std::string sphere = VtkExporter::export_horizon_sphere_vtp(2.0, 16, 8);
	suite.check(!sphere.empty(), "the horizon export is not empty");
	suite.check(contains(sphere, "<VTKFile type=\"PolyData\""), "the horizon document is a VTK PolyData file");
	suite.check(contains(sphere, "NumberOfPolys=\"128\""), "the 16 by 8 sphere declares 128 quadrilaterals");
	return suite.finish();
}
