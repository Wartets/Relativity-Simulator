#pragma once

#include "relativistic/io/screenshot_exporter.hpp"
#include <cctype>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

namespace Relativistic::IO {

struct ScreenshotCaptureSettings {
	std::string output_directory{"./screenshots"};
	std::string filename_pattern{"relativistic_%metric%_%Y%m%d_%H%M%S"};
	ScreenshotFormat format{ScreenshotFormat::PPM};
	float resolution_scale{2.0f};
};

struct ScreenshotCaptureContext {
	std::string metric_name{"Unknown"};
	double mass{1.0};
	double spin{0.0};
	uint32_t width{0};
	uint32_t height{0};
	uint64_t tick_index{0};
};

class ScreenshotFilenameBuilder {
public:
	[[nodiscard]] static std::string build(const std::string& pattern, const ScreenshotCaptureContext& ctx) {
		std::string sanitized_metric;
		sanitized_metric.reserve(ctx.metric_name.size());
		for (const char c : ctx.metric_name) {
			if (std::isalnum(static_cast<unsigned char>(c)) != 0) {
				sanitized_metric.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
			} else if (sanitized_metric.empty() || sanitized_metric.back() != '_') {
				sanitized_metric.push_back('_');
			}
		}
		while (!sanitized_metric.empty() && sanitized_metric.back() == '_') {
			sanitized_metric.pop_back();
		}
		if (sanitized_metric.empty()) {
			sanitized_metric = "metric";
		}

		std::string expanded = pattern;
		auto replace_all = [&](std::string_view token, const std::string& value) {
			size_t pos = 0;
			while ((pos = expanded.find(token, pos)) != std::string::npos) {
				expanded.replace(pos, token.size(), value);
				pos += value.size();
			}
		};

		std::ostringstream mass_ss;
		mass_ss << std::fixed << std::setprecision(2) << ctx.mass;
		std::ostringstream spin_ss;
		spin_ss << std::fixed << std::setprecision(2) << ctx.spin;

		replace_all("%metric%", sanitized_metric);
		replace_all("%mass%", mass_ss.str());
		replace_all("%spin%", spin_ss.str());
		replace_all("%width%", std::to_string(ctx.width));
		replace_all("%height%", std::to_string(ctx.height));
		replace_all("%tick%", std::to_string(ctx.tick_index));

		return ScreenshotExporter::expand_filename_pattern(expanded);
	}
};

}
