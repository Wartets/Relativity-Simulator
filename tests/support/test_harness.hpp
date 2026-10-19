#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace Relativistic::Testing {

struct CheckRecord {
	std::string section;
	std::string label;
	std::string detail;
	bool passed{false};
};

struct MeasurementRecord {
	std::string section;
	std::string name;
	double value{0.0};
	std::string unit;
};

[[nodiscard]] inline std::string format_number(double value, int precision = 6) {
	std::ostringstream stream;
	stream << std::setprecision(precision) << std::defaultfloat << value;
	return stream.str();
}

[[nodiscard]] inline std::string json_escape(std::string_view text) {
	std::string escaped;
	escaped.reserve(text.size() + 8);
	for (const char ch : text) {
		switch (ch) {
			case '"': escaped += "\\\""; break;
			case '\\': escaped += "\\\\"; break;
			case '\n': escaped += "\\n"; break;
			case '\r': escaped += "\\r"; break;
			case '\t': escaped += "\\t"; break;
			default:
				if (static_cast<unsigned char>(ch) < 0x20U) {
					char buffer[8];
					std::snprintf(buffer, sizeof(buffer), "\\u%04x", static_cast<unsigned>(static_cast<unsigned char>(ch)));
					escaped += buffer;
				} else {
					escaped += ch;
				}
				break;
		}
	}
	return escaped;
}

[[nodiscard]] inline std::string markdown_escape(std::string_view text) {
	std::string escaped;
	escaped.reserve(text.size() + 8);
	for (const char ch : text) {
		if (ch == '|') {
			escaped += "\\|";
		} else if (ch == '\n' || ch == '\r') {
			escaped += ' ';
		} else {
			escaped += ch;
		}
	}
	return escaped;
}

[[nodiscard]] inline std::string utc_timestamp() {
	const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::tm broken{};
#if defined(_WIN32)
	gmtime_s(&broken, &now);
#else
	gmtime_r(&now, &broken);
#endif
	std::ostringstream stream;
	stream << std::put_time(&broken, "%Y-%m-%d %H:%M:%S UTC");
	return stream.str();
}

[[nodiscard]] inline std::filesystem::path report_directory() {
	const char* override_path = std::getenv("RELATIVISTIC_TEST_REPORT_DIR");
	return std::filesystem::path((override_path != nullptr && override_path[0] != '\0') ? override_path : "test_reports");
}

class TestSuite {
private:
	std::string name_;
	std::string description_;
	std::string current_section_{"General"};
	std::vector<CheckRecord> checks_{};
	std::vector<MeasurementRecord> measurements_{};
	std::vector<std::string> notes_{};
	std::chrono::steady_clock::time_point start_{std::chrono::steady_clock::now()};

	void write_markdown(const std::filesystem::path& path, double duration_seconds) const {
		std::ofstream out(path, std::ios::trunc);
		if (!out.is_open()) {
			return;
		}
		const size_t failed = failed_count();
		out << "# " << name_ << "\n\n" << description_ << "\n\n";
		out << "| Property | Value |\n| --- | --- |\n";
		out << "| Status | " << (failed == 0 ? "PASSED" : "FAILED") << " |\n";
		out << "| Checks | " << checks_.size() << " |\n";
		out << "| Failed checks | " << failed << " |\n";
		out << "| Duration | " << format_number(duration_seconds, 4) << " s |\n";
		out << "| Generated | " << utc_timestamp() << " |\n\n";
		if (!notes_.empty()) {
			out << "## Notes\n\n";
			for (const auto& note : notes_) {
				out << "- " << markdown_escape(note) << "\n";
			}
			out << "\n";
		}
		std::vector<std::string> sections;
		for (const auto& record : checks_) {
			if (std::find(sections.begin(), sections.end(), record.section) == sections.end()) {
				sections.push_back(record.section);
			}
		}
		for (const auto& section : sections) {
			out << "## " << section << "\n\n| Result | Check | Detail |\n| --- | --- | --- |\n";
			for (const auto& record : checks_) {
				if (record.section != section) continue;
				out << "| " << (record.passed ? "PASS" : "FAIL") << " | " << markdown_escape(record.label) << " | " << markdown_escape(record.detail) << " |\n";
			}
			out << "\n";
		}
		if (!measurements_.empty()) {
			out << "## Measurements\n\n| Section | Name | Value | Unit |\n| --- | --- | --- | --- |\n";
			for (const auto& measurement : measurements_) {
				out << "| " << markdown_escape(measurement.section) << " | " << markdown_escape(measurement.name) << " | " << format_number(measurement.value, 9) << " | " << markdown_escape(measurement.unit) << " |\n";
			}
			out << "\n";
		}
	}

	void write_json(const std::filesystem::path& path, double duration_seconds) const {
		std::ofstream out(path, std::ios::trunc);
		if (!out.is_open()) {
			return;
		}
		out << "{\n  \"suite\": \"" << json_escape(name_) << "\",\n";
		out << "  \"description\": \"" << json_escape(description_) << "\",\n";
		out << "  \"generated\": \"" << json_escape(utc_timestamp()) << "\",\n";
		out << "  \"duration_seconds\": " << format_number(duration_seconds, 9) << ",\n";
		out << "  \"passed\": " << (failed_count() == 0 ? "true" : "false") << ",\n";
		out << "  \"checks\": [\n";
		for (size_t i = 0; i < checks_.size(); ++i) {
			const auto& record = checks_[i];
			out << "    {\"section\": \"" << json_escape(record.section) << "\", \"label\": \"" << json_escape(record.label) << "\", \"detail\": \"" << json_escape(record.detail) << "\", \"passed\": " << (record.passed ? "true" : "false") << "}" << (i + 1 < checks_.size() ? "," : "") << "\n";
		}
		out << "  ],\n  \"measurements\": [\n";
		for (size_t i = 0; i < measurements_.size(); ++i) {
			const auto& measurement = measurements_[i];
			out << "    {\"section\": \"" << json_escape(measurement.section) << "\", \"name\": \"" << json_escape(measurement.name) << "\", \"value\": " << format_number(measurement.value, 12) << ", \"unit\": \"" << json_escape(measurement.unit) << "\"}" << (i + 1 < measurements_.size() ? "," : "") << "\n";
		}
		out << "  ]\n}\n";
	}

public:
	TestSuite(std::string name, std::string description)
		: name_(std::move(name)), description_(std::move(description)) {
		std::cout << "================================================================================\n"
		          << name_ << "\n"
		          << "================================================================================\n";
	}

	void section(std::string title) {
		current_section_ = std::move(title);
		std::cout << "[" << current_section_ << "]\n";
	}

	bool check(bool condition, std::string label, std::string detail = {}) {
		std::cout << "  " << (condition ? "PASS  " : "FAIL  ") << label;
		if (!detail.empty()) {
			std::cout << "  (" << detail << ")";
		}
		std::cout << std::endl;
		checks_.push_back(CheckRecord{current_section_, std::move(label), std::move(detail), condition});
		return condition;
	}

	bool check_near(double actual, double expected, double tolerance, std::string label) {
		const double error = std::abs(actual - expected);
		const bool passed = std::isfinite(error) && error <= tolerance;
		return check(passed, std::move(label), "actual=" + format_number(actual, 12) + " expected=" + format_number(expected, 12) + " abs_error=" + format_number(error, 4) + " tolerance=" + format_number(tolerance, 4));
	}

	bool check_relative(double actual, double expected, double relative_tolerance, std::string label) {
		const double error = std::abs(actual - expected) / std::max(std::abs(expected), 1e-300);
		const bool passed = std::isfinite(error) && error <= relative_tolerance;
		return check(passed, std::move(label), "actual=" + format_number(actual, 12) + " expected=" + format_number(expected, 12) + " rel_error=" + format_number(error, 4) + " tolerance=" + format_number(relative_tolerance, 4));
	}

	bool check_at_most(double actual, double limit, std::string label) {
		const bool passed = std::isfinite(actual) && actual <= limit;
		return check(passed, std::move(label), "value=" + format_number(actual, 6) + " limit=" + format_number(limit, 6));
	}

	bool check_finite(double value, std::string label) {
		return check(std::isfinite(value), std::move(label), "value=" + format_number(value, 12));
	}

	void measure(std::string name, double value, std::string unit) {
		std::cout << "  MEASURE " << name << " = " << format_number(value, 9) << " " << unit << std::endl;
		measurements_.push_back(MeasurementRecord{current_section_, std::move(name), value, std::move(unit)});
	}

	void note(std::string text) {
		std::cout << "  NOTE  " << text << std::endl;
		notes_.push_back(std::move(text));
	}

	[[nodiscard]] size_t failed_count() const noexcept {
		return static_cast<size_t>(std::count_if(checks_.begin(), checks_.end(), [](const CheckRecord& record) noexcept { return !record.passed; }));
	}

	[[nodiscard]] int finish() {
		const double duration = std::chrono::duration<double>(std::chrono::steady_clock::now() - start_).count();
		std::error_code error;
		const auto directory = report_directory();
		std::filesystem::create_directories(directory, error);
		write_markdown(directory / (name_ + ".md"), duration);
		write_json(directory / (name_ + ".json"), duration);
		const size_t failed = failed_count();
		std::cout << "--------------------------------------------------------------------------------\n"
		          << name_ << ": " << (failed == 0 ? "PASSED" : "FAILED") << " (" << checks_.size() << " checks, " << failed << " failed, " << format_number(duration, 4) << " s)\n";
		return failed == 0 ? 0 : 1;
	}
};

}
