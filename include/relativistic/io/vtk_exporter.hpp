#pragma once

#include "relativistic/core/tensor.hpp"
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cstdint>
#include <numbers>

namespace Relativistic::IO {

struct VtkPointData {
	double affine_param{0.0};
	double redshift{1.0};
	Core::FourVector<double> four_velocity{1.0, 0.0, 0.0, 0.0};
};

struct VtkGeodesicPolyline {
	std::vector<std::array<double, 3>> points{};
	std::vector<VtkPointData> data{};
};

class VtkExporter {
public:
	[[nodiscard]] static std::string export_geodesics_vtp(const std::vector<VtkGeodesicPolyline>& geodesics) {
		std::ostringstream ss;
		ss << std::setprecision(12);

		size_t total_points = 0;
		for (const auto& g : geodesics) {
			total_points += g.points.size();
		}

		ss << "<?xml version=\"1.0\"?>\n";
		ss << "<VTKFile type=\"PolyData\" version=\"1.0\" byte_order=\"LittleEndian\">\n";
		ss << "  <PolyData>\n";
		ss << "    <Piece NumberOfPoints=\"" << total_points << "\" NumberOfLines=\"" << geodesics.size() << "\">\n";

		ss << "      <Points>\n";
		ss << "        <DataArray type=\"Float64\" Name=\"Points\" NumberOfComponents=\"3\" format=\"ascii\">\n          ";
		for (const auto& g : geodesics) {
			for (const auto& pt : g.points) {
				ss << pt[0] << " " << pt[1] << " " << pt[2] << " ";
			}
		}
		ss << "\n        </DataArray>\n";
		ss << "      </Points>\n";

		ss << "      <PointData Scalars=\"Redshift\" Vectors=\"FourVelocity\">\n";
		ss << "        <DataArray type=\"Float64\" Name=\"AffineParameter\" format=\"ascii\">\n          ";
		for (const auto& g : geodesics) {
			for (const auto& d : g.data) {
				ss << d.affine_param << " ";
			}
		}
		ss << "\n        </DataArray>\n";

		ss << "        <DataArray type=\"Float64\" Name=\"Redshift\" format=\"ascii\">\n          ";
		for (const auto& g : geodesics) {
			for (const auto& d : g.data) {
				ss << d.redshift << " ";
			}
		}
		ss << "\n        </DataArray>\n";

		ss << "        <DataArray type=\"Float64\" Name=\"FourVelocity\" NumberOfComponents=\"4\" format=\"ascii\">\n          ";
		for (const auto& g : geodesics) {
			for (const auto& d : g.data) {
				ss << d.four_velocity(0) << " " << d.four_velocity(1) << " " << d.four_velocity(2) << " " << d.four_velocity(3) << " ";
			}
		}
		ss << "\n        </DataArray>\n";
		ss << "      </PointData>\n";

		ss << "      <Lines>\n";
		ss << "        <DataArray type=\"Int64\" Name=\"connectivity\" format=\"ascii\">\n          ";
		int64_t current_idx = 0;
		for (const auto& g : geodesics) {
			for (size_t i = 0; i < g.points.size(); ++i) {
				ss << current_idx++ << " ";
			}
		}
		ss << "\n        </DataArray>\n";

		ss << "        <DataArray type=\"Int64\" Name=\"offsets\" format=\"ascii\">\n          ";
		int64_t current_offset = 0;
		for (const auto& g : geodesics) {
			current_offset += static_cast<int64_t>(g.points.size());
			ss << current_offset << " ";
		}
		ss << "\n        </DataArray>\n";
		ss << "      </Lines>\n";

		ss << "    </Piece>\n";
		ss << "  </PolyData>\n";
		ss << "</VTKFile>\n";

		return ss.str();
	}

	[[nodiscard]] static std::string export_horizon_sphere_vtp(double radius, size_t u_res = 32, size_t v_res = 16) {
		std::ostringstream ss;
		ss << std::setprecision(12);

		const size_t num_points = (u_res + 1) * (v_res + 1);
		const size_t num_polys = u_res * v_res;

		ss << "<?xml version=\"1.0\"?>\n";
		ss << "<VTKFile type=\"PolyData\" version=\"1.0\" byte_order=\"LittleEndian\">\n";
		ss << "  <PolyData>\n";
		ss << "    <Piece NumberOfPoints=\"" << num_points << "\" NumberOfPolys=\"" << num_polys << "\">\n";

		ss << "      <Points>\n";
		ss << "        <DataArray type=\"Float64\" Name=\"Points\" NumberOfComponents=\"3\" format=\"ascii\">\n          ";
		for (size_t j = 0; j <= v_res; ++j) {
			const double theta = (static_cast<double>(j) / static_cast<double>(v_res)) * std::numbers::pi_v<double>;
			const double sin_t = std::sin(theta);
			const double cos_t = std::cos(theta);
			for (size_t i = 0; i <= u_res; ++i) {
				const double phi = (static_cast<double>(i) / static_cast<double>(u_res)) * 2.0 * std::numbers::pi_v<double>;
				const double x = radius * sin_t * std::cos(phi);
				const double y = radius * sin_t * std::sin(phi);
				const double z = radius * cos_t;
				ss << x << " " << y << " " << z << " ";
			}
		}
		ss << "\n        </DataArray>\n";
		ss << "      </Points>\n";

		ss << "      <Polys>\n";
		ss << "        <DataArray type=\"Int64\" Name=\"connectivity\" format=\"ascii\">\n          ";
		for (size_t j = 0; j < v_res; ++j) {
			for (size_t i = 0; i < u_res; ++i) {
				const int64_t p0 = static_cast<int64_t>(j * (u_res + 1) + i);
				const int64_t p1 = static_cast<int64_t>(p0 + 1);
				const int64_t p2 = static_cast<int64_t>((j + 1) * (u_res + 1) + i + 1);
				const int64_t p3 = static_cast<int64_t>((j + 1) * (u_res + 1) + i);
				ss << p0 << " " << p1 << " " << p2 << " " << p3 << " ";
			}
		}
		ss << "\n        </DataArray>\n";

		ss << "        <DataArray type=\"Int64\" Name=\"offsets\" format=\"ascii\">\n          ";
		int64_t offset = 0;
		for (size_t k = 0; k < num_polys; ++k) {
			offset += 4;
			ss << offset << " ";
		}
		ss << "\n        </DataArray>\n";
		ss << "      </Polys>\n";

		ss << "    </Piece>\n";
		ss << "  </PolyData>\n";
		ss << "</VTKFile>\n";

		return ss.str();
	}
};

}
