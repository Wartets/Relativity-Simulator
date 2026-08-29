#pragma once

#include "relativistic/io/ephemeris_types.hpp"
#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <string>
#include <vector>
#include <optional>
#include <algorithm>

namespace Relativistic::IO {

struct HorizonsQueryConfig {
	uint32_t target_body_id{399};
	uint32_t center_body_id{0};
	Epoch start_epoch{Epoch::J2000_JD};
	Epoch stop_epoch{Epoch::J2000_JD + 1.0};
	double step_days{1.0};
	bool request_vectors{true};
	bool request_elements{false};
};

class HorizonsInterface {
public:
	[[nodiscard]] static std::string build_query_url(const HorizonsQueryConfig& config) {
		std::string url = "https://ssd.jpl.nasa.gov/api/horizons.api?format=text";
		url += "&COMMAND='" + std::to_string(config.target_body_id) + "'";
		url += "&OBJ_DATA='NO'&MAKE_EPHEM='YES'";
		if (config.request_vectors) {
			url += "&EPHEM_TYPE='VECTORS'";
		} else {
			url += "&EPHEM_TYPE='ELEMENTS'";
		}
		url += "&CENTER='@" + std::to_string(config.center_body_id) + "'";
		url += "&START_TIME='JD" + std::to_string(config.start_epoch.jd) + "'";
		url += "&STOP_TIME='JD" + std::to_string(config.stop_epoch.jd) + "'";
		url += "&STEP_SIZE='" + std::to_string(config.step_days) + "d'";
		url += "&REF_PLANE='FRAME'&REF_SYSTEM='ICRF'&VEC_TABLE='3'&CSV_FORMAT='YES'";
		return url;
	}

	[[nodiscard]] static std::vector<EphemerisStateVector> parse_horizons_response(
		std::string_view text,
		uint32_t target_id = 0,
		uint32_t center_id = 0
	) noexcept {
		std::vector<EphemerisStateVector> results;

		const std::string_view soe = "$$SOE";
		const std::string_view eoe = "$$EOE";

		const size_t soe_pos = text.find(soe);
		if (soe_pos == std::string_view::npos) {
			return results;
		}

		const size_t start_data = soe_pos + soe.size();
		const size_t eoe_pos = text.find(eoe, start_data);
		if (eoe_pos == std::string_view::npos) {
			return results;
		}

		std::string_view block = text.substr(start_data, eoe_pos - start_data);

		while (!block.empty()) {
			size_t line_end = block.find('\n');
			std::string_view line = (line_end == std::string_view::npos) ? block : block.substr(0, line_end);
			if (line_end == std::string_view::npos) {
				block = {};
			} else {
				block = block.substr(line_end + 1);
			}

			while (!line.empty() && (line.front() == ' ' || line.front() == '\t' || line.front() == '\r')) {
				line.remove_prefix(1);
			}
			while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r')) {
				line.remove_suffix(1);
			}

			if (line.empty()) {
				continue;
			}

			std::vector<std::string_view> tokens;
			std::string_view rem = line;
			while (!rem.empty()) {
				const size_t comma = rem.find(',');
				if (comma == std::string_view::npos) {
					tokens.push_back(rem);
					rem = {};
				} else {
					tokens.push_back(rem.substr(0, comma));
					rem = rem.substr(comma + 1);
				}
			}

			if (tokens.size() >= 8) {
				auto parse_token_double = [](std::string_view t_sv) noexcept -> std::optional<double> {
					while (!t_sv.empty() && (t_sv.front() == ' ' || t_sv.front() == '\t')) t_sv.remove_prefix(1);
					while (!t_sv.empty() && (t_sv.back() == ' ' || t_sv.back() == '\t')) t_sv.remove_suffix(1);
					if (t_sv.empty()) return std::nullopt;
					char buf[64];
					if (t_sv.size() >= sizeof(buf)) return std::nullopt;
					std::memcpy(buf, t_sv.data(), t_sv.size());
					buf[t_sv.size()] = '\0';
					char* end_ptr = nullptr;
					const double v = std::strtod(buf, &end_ptr);
					if (end_ptr != buf + t_sv.size()) return std::nullopt;
					return v;
				};

				const auto jd_val = parse_token_double(tokens[0]);
				const auto x_km = parse_token_double(tokens[2]);
				const auto y_km = parse_token_double(tokens[3]);
				const auto z_km = parse_token_double(tokens[4]);
				const auto vx_kms = parse_token_double(tokens[5]);
				const auto vy_kms = parse_token_double(tokens[6]);
				const auto vz_kms = parse_token_double(tokens[7]);

				if (jd_val && x_km && y_km && z_km && vx_kms && vy_kms && vz_kms) {
					const Epoch ep(*jd_val);
					const double x_m = *x_km * 1000.0;
					const double y_m = *y_km * 1000.0;
					const double z_m = *z_km * 1000.0;
					const double vx_ms = *vx_kms * 1000.0;
					const double vy_ms = *vy_kms * 1000.0;
					const double vz_ms = *vz_kms * 1000.0;

					results.emplace_back(ep, x_m, y_m, z_m, vx_ms, vy_ms, vz_ms, target_id, center_id);
				}
			}
		}

		return results;
	}
};

}
