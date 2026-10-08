#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <string>
#include <vector>

namespace Relativistic::Interferometry {

enum class StationKind : uint8_t {
	Ground = 0,
	SpaceOrbit = 1
};

inline constexpr double kEarthEquatorialRadiusMeters = 6378137.0;
inline constexpr double kEarthGravitationalParameter = 3.986004418e14;

struct VlbiStation {
	std::string telescope{};
	std::string code{};
	StationKind kind{StationKind::Ground};
	std::array<double, 3> ecef_meters{0.0, 0.0, 0.0};
	double diameter_m{10.0};
	double sefd_jy{5000.0};
	bool enabled{true};
	double orbit_altitude_km{20000.0};
	double orbit_inclination_deg{60.0};
	double orbit_raan_deg{0.0};
	double orbit_phase_deg{0.0};

	[[nodiscard]] std::array<double, 3> inertial_position(double earth_rotation_rad, double elapsed_seconds) const noexcept {
		if (kind == StationKind::Ground) {
			const double c = std::cos(earth_rotation_rad);
			const double s = std::sin(earth_rotation_rad);
			return {ecef_meters[0] * c - ecef_meters[1] * s, ecef_meters[0] * s + ecef_meters[1] * c, ecef_meters[2]};
		}
		constexpr double degrees = std::numbers::pi_v<double> / 180.0;
		const double semi_major = kEarthEquatorialRadiusMeters + orbit_altitude_km * 1000.0;
		const double mean_motion = std::sqrt(kEarthGravitationalParameter / (semi_major * semi_major * semi_major));
		const double anomaly = orbit_phase_deg * degrees + mean_motion * elapsed_seconds;
		const double px = semi_major * std::cos(anomaly);
		const double py = semi_major * std::sin(anomaly);
		const double inclination = orbit_inclination_deg * degrees;
		const double raan = orbit_raan_deg * degrees;
		const double y_inclined = py * std::cos(inclination);
		const double z_inclined = py * std::sin(inclination);
		return {px * std::cos(raan) - y_inclined * std::sin(raan), px * std::sin(raan) + y_inclined * std::cos(raan), z_inclined};
	}
};

struct VlbiArray {
	std::string name{};
	std::vector<VlbiStation> stations{};
};

class VlbiArrayCatalog {
public:
	static constexpr size_t kPresetCount = 4;

	[[nodiscard]] static const char* preset_name(size_t index) noexcept {
		static constexpr const char* kNames[kPresetCount] = {"EHT 2017", "EHT 2018", "EHT 2021 Extended", "EHT 2021 + Space Orbiter"};
		return kNames[std::min(index, kPresetCount - 1)];
	}

	[[nodiscard]] static VlbiArray preset(size_t index) {
		switch (index) {
			case 1: return eht_2018();
			case 2: return eht_extended();
			case 3: return eht_with_space();
			case 0:
			default: return eht_2017();
		}
	}

	[[nodiscard]] static VlbiArray eht_2017() {
		VlbiArray array;
		array.name = "EHT";
		array.stations = {
			ground("ALMA", "AA", {2225061.164, -5440057.370, -2481681.151}, 73.0, 110.0),
			ground("APEX", "AP", {2225039.530, -5441197.630, -2479303.360}, 12.0, 4700.0),
			ground("LMT", "LM", {-768713.9637, -5988541.7982, 2063275.9472}, 50.0, 560.0),
			ground("IRAM-30m", "PV", {5088967.9, -301681.6, 3825012.2}, 30.0, 1900.0),
			ground("SMA", "SM", {-5464523.4, -2493147.08, 2150611.75}, 14.7, 4900.0),
			ground("JCMT", "JC", {-5464584.676, -2493001.170, 2150654.30}, 15.0, 5000.0),
			ground("SMT", "AZ", {-1828796.2, -5054406.8, 3427865.2}, 10.0, 15000.0),
			ground("SPT", "SP", {0.0, 0.0, -6359609.7}, 10.0, 5000.0)
		};
		return array;
	}

	[[nodiscard]] static VlbiArray eht_2018() {
		VlbiArray array = eht_2017();
		array.stations.push_back(ground("GLT", "GL", {1500692.0, -1191735.0, 6066409.0}, 12.0, 8000.0));
		return array;
	}

	[[nodiscard]] static VlbiArray eht_extended() {
		VlbiArray array = eht_2018();
		array.stations.push_back(ground("NOEMA", "NN", {4524000.4, 468042.14, 4460309.76}, 52.0, 700.0));
		array.stations.push_back(ground("Kitt Peak", "KP", {-1995678.84, -5037317.82, 3357328.03}, 12.0, 10000.0));
		return array;
	}

	[[nodiscard]] static VlbiArray eht_with_space() {
		VlbiArray array = eht_extended();
		VlbiStation orbiter;
		orbiter.telescope = "Space Orbiter";
		orbiter.code = "SO";
		orbiter.kind = StationKind::SpaceOrbit;
		orbiter.diameter_m = 5.0;
		orbiter.sefd_jy = 20000.0;
		orbiter.orbit_altitude_km = 20000.0;
		orbiter.orbit_inclination_deg = 55.0;
		orbiter.orbit_raan_deg = 40.0;
		orbiter.orbit_phase_deg = 0.0;
		array.name = "EHT-SPACE";
		array.stations.push_back(orbiter);
		return array;
	}

private:
	[[nodiscard]] static VlbiStation ground(const char* telescope, const char* code, const std::array<double, 3>& ecef, double diameter, double sefd) {
		VlbiStation station;
		station.telescope = telescope;
		station.code = code;
		station.kind = StationKind::Ground;
		station.ecef_meters = ecef;
		station.diameter_m = diameter;
		station.sefd_jy = sefd;
		return station;
	}
};

}
