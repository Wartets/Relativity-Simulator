#pragma once

#include "relativistic/interferometry/fits_table.hpp"
#include "relativistic/interferometry/visibility_synthesis.hpp"
#include "relativistic/interferometry/vlbi_array.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <numbers>
#include <string>
#include <vector>

namespace Relativistic::Interferometry {

struct OifitsTarget {
	std::string name{"TARGET"};
	std::string instrument{"VLBI"};
	std::string observer{"Observer"};
	double right_ascension_deg{0.0};
	double declination_deg{0.0};
	double equinox_year{2000.0};
	std::string spectral_type{"UNKNOWN"};
};

class OifitsWriter {
public:
	[[nodiscard]] static bool write(const VlbiDataset& dataset, const OifitsTarget& target, const std::filesystem::path& path, std::string& error) {
		if (!dataset.valid) {
			error = "The dataset holds no valid visibilities to export.";
			return false;
		}
		std::vector<size_t> rows;
		rows.reserve(dataset.samples.size());
		for (size_t i = 0; i < dataset.samples.size(); ++i) {
			if (!dataset.samples[i].flagged) {
				rows.push_back(i);
			}
		}
		if (rows.empty()) {
			error = "Every visibility sample is flagged; nothing can be exported.";
			return false;
		}

		const std::string array_name = dataset.array.name.empty() ? std::string("VLBI") : dataset.array.name;
		const std::string instrument = target.instrument.empty() ? std::string("VLBI") : target.instrument;
		const double start_mjd = dataset.settings.start_mjd + dataset.settings.start_hour_ut / 24.0;
		const std::string date_obs = iso_date_from_mjd(start_mjd);

		FitsDocument document;
		auto& primary = document.primary_keywords();
		primary.add_string("CONTENT", "OIFITS2");
		primary.add_string("ORIGIN", "Relativistic Engine");
		primary.add_string("OBJECT", target.name);
		primary.add_string("INSTRUME", instrument);
		primary.add_string("OBSERVER", target.observer);
		primary.add_string("DATE-OBS", date_obs);
		primary.add_real("MJD-OBS", start_mjd);

		append_target_table(document, target);
		append_array_table(document, dataset, array_name);
		append_wavelength_table(document, dataset, instrument);
		append_vis_table(document, dataset, rows, array_name, instrument, date_obs);
		append_vis2_table(document, dataset, rows, array_name, instrument, date_obs);
		append_t3_table(document, dataset, array_name, instrument, date_obs);

		return document.write(path, error);
	}

private:
	static constexpr double kRadiansToDegrees = 180.0 / std::numbers::pi_v<double>;

	[[nodiscard]] static std::string iso_date_from_mjd(double mjd) {
		const int64_t days = static_cast<int64_t>(std::floor(mjd)) - 40587;
		const int64_t z = days + 719468;
		const int64_t era = (z >= 0 ? z : z - 146096) / 146097;
		const int64_t day_of_era = z - era * 146097;
		const int64_t year_of_era = (day_of_era - day_of_era / 1460 + day_of_era / 36524 - day_of_era / 146096) / 365;
		const int64_t day_of_year = day_of_era - (365 * year_of_era + year_of_era / 4 - year_of_era / 100);
		const int64_t shifted_month = (5 * day_of_year + 2) / 153;
		const int64_t day = day_of_year - (153 * shifted_month + 2) / 5 + 1;
		const int64_t month = shifted_month < 10 ? shifted_month + 3 : shifted_month - 9;
		const int64_t year = year_of_era + era * 400 + (month <= 2 ? 1 : 0);
		char buffer[32];
		std::snprintf(buffer, sizeof(buffer), "%04lld-%02lld-%02lld", static_cast<long long>(year), static_cast<long long>(month), static_cast<long long>(day));
		return buffer;
	}

	static void add_observation_keywords(FitsBinaryTable& table, const std::string& array_name, const std::string& instrument, const std::string& date_obs) {
		table.keywords().add_integer("OI_REVN", 2);
		table.keywords().add_string("DATE-OBS", date_obs);
		table.keywords().add_string("ARRNAME", array_name);
		table.keywords().add_string("INSNAME", instrument);
	}

	static void append_target_table(FitsDocument& document, const OifitsTarget& target) {
		auto& table = document.add_table("OI_TARGET");
		table.keywords().add_integer("OI_REVN", 2);
		table.add_column("TARGET_ID", "1I");
		table.add_column("TARGET", "16A");
		table.add_column("RAEP0", "1D", "deg");
		table.add_column("DEC0", "1D", "deg");
		table.add_column("EQUINOX", "1E", "yr");
		table.add_column("RA_ERR", "1D", "deg");
		table.add_column("DEC_ERR", "1D", "deg");
		table.add_column("SYSVEL", "1D", "m/s");
		table.add_column("VELTYP", "8A");
		table.add_column("VELDEF", "8A");
		table.add_column("PMRA", "1D", "deg/yr");
		table.add_column("PMDEC", "1D", "deg/yr");
		table.add_column("PMRA_ERR", "1D", "deg/yr");
		table.add_column("PMDEC_ERR", "1D", "deg/yr");
		table.add_column("PARALLAX", "1E", "deg");
		table.add_column("PARA_ERR", "1E", "deg");
		table.add_column("SPECTYP", "16A");
		table.add_column("CATEGORY", "3A");
		table.put_int16(1);
		table.put_text(target.name, 16);
		table.put_float64(target.right_ascension_deg);
		table.put_float64(target.declination_deg);
		table.put_float32(static_cast<float>(target.equinox_year));
		table.put_float64(0.0);
		table.put_float64(0.0);
		table.put_float64(0.0);
		table.put_text("LSR", 8);
		table.put_text("OPTICAL", 8);
		table.put_float64(0.0);
		table.put_float64(0.0);
		table.put_float64(0.0);
		table.put_float64(0.0);
		table.put_float32(0.0f);
		table.put_float32(0.0f);
		table.put_text(target.spectral_type, 16);
		table.put_text("SCI", 3);
		table.end_row();
	}

	static void append_array_table(FitsDocument& document, const VlbiDataset& dataset, const std::string& array_name) {
		auto& table = document.add_table("OI_ARRAY");
		table.keywords().add_integer("OI_REVN", 2);
		table.keywords().add_string("ARRNAME", array_name);
		table.keywords().add_string("FRAME", "GEOCENTRIC");
		table.keywords().add_real("ARRAYX", 0.0);
		table.keywords().add_real("ARRAYY", 0.0);
		table.keywords().add_real("ARRAYZ", 0.0);
		table.add_column("TEL_NAME", "16A");
		table.add_column("STA_NAME", "16A");
		table.add_column("STA_INDEX", "1I");
		table.add_column("DIAMETER", "1E", "m");
		table.add_column("STAXYZ", "3D", "m");
		table.add_column("FOV", "1D", "arcsec");
		table.add_column("FOVTYPE", "6A");
		for (size_t i = 0; i < dataset.array.stations.size(); ++i) {
			const VlbiStation& station = dataset.array.stations[i];
			const std::array<double, 3> position = (station.kind == StationKind::Ground) ? station.ecef_meters : station.inertial_position(0.0, 0.0);
			table.put_text(station.telescope, 16);
			table.put_text(station.code, 16);
			table.put_int16(static_cast<int16_t>(i + 1));
			table.put_float32(static_cast<float>(station.diameter_m));
			table.put_float64(position[0]);
			table.put_float64(position[1]);
			table.put_float64(position[2]);
			table.put_float64(0.0);
			table.put_text("RADIUS", 6);
			table.end_row();
		}
	}

	static void append_wavelength_table(FitsDocument& document, const VlbiDataset& dataset, const std::string& instrument) {
		auto& table = document.add_table("OI_WAVELENGTH");
		table.keywords().add_integer("OI_REVN", 2);
		table.keywords().add_string("INSNAME", instrument);
		table.add_column("EFF_WAVE", "1E", "m");
		table.add_column("EFF_BAND", "1E", "m");
		const double frequency = std::max(dataset.settings.frequency_hz, 1.0);
		const double bandwidth_m = dataset.wavelength_m * dataset.settings.bandwidth_hz / frequency;
		table.put_float32(static_cast<float>(dataset.wavelength_m));
		table.put_float32(static_cast<float>(bandwidth_m));
		table.end_row();
	}

	static void append_vis_table(FitsDocument& document, const VlbiDataset& dataset, const std::vector<size_t>& rows, const std::string& array_name, const std::string& instrument, const std::string& date_obs) {
		auto& table = document.add_table("OI_VIS");
		add_observation_keywords(table, array_name, instrument, date_obs);
		table.keywords().add_string("AMPTYP", "correlated flux");
		table.keywords().add_string("PHITYP", "absolute");
		table.add_column("TARGET_ID", "1I");
		table.add_column("TIME", "1D", "s");
		table.add_column("MJD", "1D", "day");
		table.add_column("INT_TIME", "1D", "s");
		table.add_column("VISAMP", "1D", "Jy");
		table.add_column("VISAMPERR", "1D", "Jy");
		table.add_column("VISPHI", "1D", "deg");
		table.add_column("VISPHIERR", "1D", "deg");
		table.add_column("UCOORD", "1D", "m");
		table.add_column("VCOORD", "1D", "m");
		table.add_column("STA_INDEX", "2I");
		table.add_column("FLAG", "1L");
		const double flux = dataset.settings.total_flux_jy;
		for (const size_t index : rows) {
			const VisibilitySample& sample = dataset.samples[index];
			const double amplitude = std::abs(sample.value);
			const double sigma = std::max(sample.sigma_jy, 1.0e-6 * flux);
			const double phase_error = (amplitude > 1.0e-30) ? std::min(sigma / amplitude, std::numbers::pi_v<double>) * kRadiansToDegrees : 180.0;
			table.put_int16(1);
			table.put_float64(sample.time_seconds);
			table.put_float64(sample.mjd);
			table.put_float64(dataset.settings.integration_time_s);
			table.put_float64(amplitude);
			table.put_float64(sigma);
			table.put_float64(std::arg(sample.value) * kRadiansToDegrees);
			table.put_float64(std::max(phase_error, 1.0e-4));
			table.put_float64(sample.u_meters);
			table.put_float64(sample.v_meters);
			table.put_int16(static_cast<int16_t>(sample.station_a + 1));
			table.put_int16(static_cast<int16_t>(sample.station_b + 1));
			table.put_logical(false);
			table.end_row();
		}
	}

	static void append_vis2_table(FitsDocument& document, const VlbiDataset& dataset, const std::vector<size_t>& rows, const std::string& array_name, const std::string& instrument, const std::string& date_obs) {
		auto& table = document.add_table("OI_VIS2");
		add_observation_keywords(table, array_name, instrument, date_obs);
		table.add_column("TARGET_ID", "1I");
		table.add_column("TIME", "1D", "s");
		table.add_column("MJD", "1D", "day");
		table.add_column("INT_TIME", "1D", "s");
		table.add_column("VIS2DATA", "1D");
		table.add_column("VIS2ERR", "1D");
		table.add_column("UCOORD", "1D", "m");
		table.add_column("VCOORD", "1D", "m");
		table.add_column("STA_INDEX", "2I");
		table.add_column("FLAG", "1L");
		const double flux = std::max(dataset.settings.total_flux_jy, 1.0e-30);
		for (const size_t index : rows) {
			const VisibilitySample& sample = dataset.samples[index];
			const double amplitude = std::abs(sample.value);
			const double sigma = std::max(sample.sigma_jy, 1.0e-6 * flux);
			const double squared = (amplitude / flux) * (amplitude / flux);
			const double squared_error = 2.0 * std::max(amplitude, sigma) * sigma / (flux * flux);
			table.put_int16(1);
			table.put_float64(sample.time_seconds);
			table.put_float64(sample.mjd);
			table.put_float64(dataset.settings.integration_time_s);
			table.put_float64(squared);
			table.put_float64(std::max(squared_error, 1.0e-12));
			table.put_float64(sample.u_meters);
			table.put_float64(sample.v_meters);
			table.put_int16(static_cast<int16_t>(sample.station_a + 1));
			table.put_int16(static_cast<int16_t>(sample.station_b + 1));
			table.put_logical(false);
			table.end_row();
		}
	}

	static void append_t3_table(FitsDocument& document, const VlbiDataset& dataset, const std::string& array_name, const std::string& instrument, const std::string& date_obs) {
		auto& table = document.add_table("OI_T3");
		add_observation_keywords(table, array_name, instrument, date_obs);
		table.add_column("TARGET_ID", "1I");
		table.add_column("TIME", "1D", "s");
		table.add_column("MJD", "1D", "day");
		table.add_column("INT_TIME", "1D", "s");
		table.add_column("T3AMP", "1D");
		table.add_column("T3AMPERR", "1D");
		table.add_column("T3PHI", "1D", "deg");
		table.add_column("T3PHIERR", "1D", "deg");
		table.add_column("U1COORD", "1D", "m");
		table.add_column("V1COORD", "1D", "m");
		table.add_column("U2COORD", "1D", "m");
		table.add_column("V2COORD", "1D", "m");
		table.add_column("STA_INDEX", "3I");
		table.add_column("FLAG", "1L");
		const double flux = std::max(dataset.settings.total_flux_jy, 1.0e-30);
		const double flux_cubed = flux * flux * flux;
		for (const ClosurePhaseSample& closure : dataset.closures) {
			const double amplitude = closure.amplitude / flux_cubed;
			const double amplitude_error = std::max(closure.amplitude_error / flux_cubed, 1.0e-12);
			table.put_int16(1);
			table.put_float64(closure.time_seconds);
			table.put_float64(closure.mjd);
			table.put_float64(dataset.settings.integration_time_s);
			table.put_float64(amplitude);
			table.put_float64(amplitude_error);
			table.put_float64(closure.phase_rad * kRadiansToDegrees);
			table.put_float64(std::max(closure.phase_error_rad * kRadiansToDegrees, 1.0e-4));
			table.put_float64(closure.u1_meters);
			table.put_float64(closure.v1_meters);
			table.put_float64(closure.u2_meters);
			table.put_float64(closure.v2_meters);
			table.put_int16(static_cast<int16_t>(closure.station_a + 1));
			table.put_int16(static_cast<int16_t>(closure.station_b + 1));
			table.put_int16(static_cast<int16_t>(closure.station_c + 1));
			table.put_logical(false);
			table.end_row();
		}
	}
};

}
