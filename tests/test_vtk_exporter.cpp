#include "relativistic/io/vtk_exporter.hpp"
#include <iostream>
#include <cassert>

int main() {
	using namespace Relativistic::Core;
	using namespace Relativistic::IO;

	std::vector<VtkGeodesicPolyline> geodesics;
	VtkGeodesicPolyline ray1;
	for (size_t i = 0; i < 20; ++i) {
		const double t = static_cast<double>(i) * 0.5;
		ray1.points.push_back({t, std::sin(t), std::cos(t)});
		ray1.data.push_back({
			.affine_param = t,
			.redshift = 1.0 + 0.1 * t,
			.four_velocity = FourVector<double>(1.0, 1.0, 0.0, 0.0)
		});
	}
	geodesics.push_back(ray1);

	const std::string vtp_xml = VtkExporter::export_geodesics_vtp(geodesics);
	assert(!vtp_xml.empty());
	assert(vtp_xml.find("<VTKFile type=\"PolyData\"") != std::string::npos);
	assert(vtp_xml.find("NumberOfPoints=\"20\"") != std::string::npos);
	assert(vtp_xml.find("NumberOfLines=\"1\"") != std::string::npos);
	assert(vtp_xml.find("Name=\"Redshift\"") != std::string::npos);

	const std::string horizon_vtp = VtkExporter::export_horizon_sphere_vtp(2.0, 16, 8);
	assert(!horizon_vtp.empty());
	assert(horizon_vtp.find("<VTKFile type=\"PolyData\"") != std::string::npos);
	assert(horizon_vtp.find("NumberOfPolys=\"128\"") != std::string::npos);

	return 0;
}
