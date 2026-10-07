#pragma once

#include "relativistic/io/hdf5_serializer.hpp"
#include "relativistic/io/capture/recording_settings.hpp"
#include "relativistic/io/vtk_exporter.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Relativistic::IO {

struct TelemetryMetadata {
	std::string session{};
	std::string script{};
	std::string unit_system{"geometric"};
	double frames_per_second{30.0};
	uint32_t decimation{1};
};

class TelemetryTable {
private:
	std::vector<std::string> names_{};
	std::vector<std::string> units_{};
	std::vector<std::vector<double>> columns_{};

public:
	size_t add_column(std::string name, std::string unit) {
		names_.push_back(std::move(name));
		units_.push_back(std::move(unit));
		columns_.emplace_back();
		return columns_.size() - 1;
	}

	void append_row(std::span<const double> row) {
		for (size_t i = 0; i < columns_.size(); ++i) {
			columns_[i].push_back(i < row.size() ? row[i] : std::numeric_limits<double>::quiet_NaN());
		}
	}

	void clear() noexcept {
		names_.clear();
		units_.clear();
		columns_.clear();
	}

	void clear_rows() noexcept {
		for (auto& column : columns_) column.clear();
	}

	[[nodiscard]] size_t column_count() const noexcept { return columns_.size(); }
	[[nodiscard]] size_t row_count() const noexcept { return columns_.empty() ? 0 : columns_.front().size(); }
	[[nodiscard]] const std::string& name(size_t index) const noexcept { return names_[index]; }
	[[nodiscard]] const std::string& unit(size_t index) const noexcept { return units_[index]; }
	[[nodiscard]] const std::vector<double>& column(size_t index) const noexcept { return columns_[index]; }

	[[nodiscard]] std::optional<size_t> find(std::string_view column_name) const noexcept {
		for (size_t i = 0; i < names_.size(); ++i) {
			if (names_[i] == column_name) return i;
		}
		return std::nullopt;
	}
};

namespace TelemetryDetail {

inline void append_number(std::string& out, double value, int precision, const char* non_finite) {
	if (!std::isfinite(value)) {
		out += non_finite;
		return;
	}
	char buffer[48];
	const int written = std::snprintf(buffer, sizeof(buffer), "%.*g", precision, value);
	if (written > 0) out.append(buffer, static_cast<size_t>(written));
}

[[nodiscard]] inline std::string json_escape(std::string_view text) {
	std::string out;
	out.reserve(text.size() + 2);
	for (const char c : text) {
		switch (c) {
			case '"': out += "\\\""; break;
			case '\\': out += "\\\\"; break;
			case '\n': out += "\\n"; break;
			case '\r': out += "\\r"; break;
			case '\t': out += "\\t"; break;
			default:
				if (static_cast<unsigned char>(c) < 0x20U) {
					char buffer[8];
					std::snprintf(buffer, sizeof(buffer), "\\u%04x", static_cast<unsigned int>(static_cast<unsigned char>(c)));
					out += buffer;
				} else {
					out.push_back(c);
				}
				break;
		}
	}
	return out;
}

inline bool write_bytes(const std::filesystem::path& path, const void* data, size_t size) {
	std::ofstream out(path, std::ios::binary | std::ios::trunc);
	if (!out.is_open()) return false;
	out.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
	return out.good();
}

inline void append_le(std::string& out, uint64_t value, size_t bytes) {
	for (size_t i = 0; i < bytes; ++i) {
		out.push_back(static_cast<char>((value >> (8U * i)) & 0xFFU));
	}
}

[[nodiscard]] inline std::string build_metadata(const TelemetryTable& table, const TelemetryMetadata& meta, RecordingFormat format) {
	std::string out = "{\"format\":\"relativistic-telemetry\",\"version\":1,\"storage\":\"";
	out += recording_format_extension(format);
	out += "\",\"session\":\"" + json_escape(meta.session) + "\",\"script\":\"" + json_escape(meta.script) + "\",\"unit_system\":\"" + json_escape(meta.unit_system) + "\",\"frames_per_second\":";
	append_number(out, meta.frames_per_second, 9, "null");
	out += ",\"decimation\":" + std::to_string(meta.decimation) + ",\"rows\":" + std::to_string(table.row_count()) + ",\"endianness\":\"little\",\"channels\":[";
	for (size_t c = 0; c < table.column_count(); ++c) {
		if (c > 0) out.push_back(',');
		out += "{\"name\":\"" + json_escape(table.name(c)) + "\",\"unit\":\"" + json_escape(table.unit(c)) + "\"}";
	}
	out += "]";
	return out;
}

[[nodiscard]] inline std::string build_delimited(const TelemetryTable& table, char delimiter, int precision) {
	std::string out;
	out.reserve(table.row_count() * table.column_count() * 12U + 256U);
	for (size_t c = 0; c < table.column_count(); ++c) {
		if (c > 0) out.push_back(delimiter);
		out += table.name(c);
	}
	out.push_back('\n');
	for (size_t r = 0; r < table.row_count(); ++r) {
		for (size_t c = 0; c < table.column_count(); ++c) {
			if (c > 0) out.push_back(delimiter);
			append_number(out, table.column(c)[r], precision, "nan");
		}
		out.push_back('\n');
	}
	return out;
}

[[nodiscard]] inline std::string build_json(const TelemetryTable& table, const TelemetryMetadata& meta, int precision) {
	std::string out = build_metadata(table, meta, RecordingFormat::Json);
	out += ",\"data\":{";
	for (size_t c = 0; c < table.column_count(); ++c) {
		if (c > 0) out.push_back(',');
		out += "\"" + json_escape(table.name(c)) + "\":[";
		const auto& values = table.column(c);
		for (size_t r = 0; r < values.size(); ++r) {
			if (r > 0) out.push_back(',');
			append_number(out, values[r], precision, "null");
		}
		out.push_back(']');
	}
	out += "}}\n";
	return out;
}

[[nodiscard]] inline std::string build_json_lines(const TelemetryTable& table, int precision) {
	std::string out;
	out.reserve(table.row_count() * table.column_count() * 24U + 64U);
	for (size_t r = 0; r < table.row_count(); ++r) {
		out.push_back('{');
		for (size_t c = 0; c < table.column_count(); ++c) {
			if (c > 0) out.push_back(',');
			out += "\"" + json_escape(table.name(c)) + "\":";
			append_number(out, table.column(c)[r], precision, "null");
		}
		out += "}\n";
	}
	return out;
}

[[nodiscard]] inline std::string build_binary(const TelemetryTable& table) {
	std::string out;
	out.reserve(16U + table.row_count() * table.column_count() * 8U);
	out += "RCAP";
	append_le(out, 1U, 4);
	append_le(out, table.column_count(), 4);
	append_le(out, table.row_count(), 8);
	for (size_t c = 0; c < table.column_count(); ++c) {
		for (const double value : table.column(c)) {
			append_le(out, std::bit_cast<uint64_t>(value), 8);
		}
	}
	return out;
}

[[nodiscard]] inline std::vector<uint8_t> build_container(const TelemetryTable& table, const std::string& schema) {
	Hdf5Container container;
	for (size_t c = 0; c < table.column_count(); ++c) {
		Hdf5Dataset dataset;
		dataset.path = "channels/" + table.name(c);
		dataset.type = Hdf5DataType::Float64;
		dataset.dimensions = {table.column(c).size()};
		dataset.raw_data.resize(table.column(c).size() * sizeof(double));
		if (!table.column(c).empty()) {
			std::memcpy(dataset.raw_data.data(), table.column(c).data(), dataset.raw_data.size());
		}
		container.add_dataset(std::move(dataset));
	}
	Hdf5Dataset info;
	info.path = "meta/info";
	info.type = Hdf5DataType::Byte;
	info.dimensions = {schema.size()};
	info.raw_data.assign(schema.begin(), schema.end());
	container.add_dataset(std::move(info));
	return container.serialize();
}

[[nodiscard]] inline std::optional<std::string> build_vtk(const TelemetryTable& table, std::string& error) {
	const auto x = table.find("camera_x");
	const auto y = table.find("camera_y");
	const auto z = table.find("camera_z");
	if (!x || !y || !z) {
		error = "The VTK trajectory format requires the camera position channels.";
		return std::nullopt;
	}
	const auto time = table.find("session_time");
	const auto redshift = table.find("grav_redshift");
	const auto vx = table.find("camera_vx");
	const auto vy = table.find("camera_vy");
	const auto vz = table.find("camera_vz");

	std::vector<VtkGeodesicPolyline> lines(1);
	auto& line = lines.front();
	line.points.reserve(table.row_count());
	line.data.reserve(table.row_count());
	for (size_t r = 0; r < table.row_count(); ++r) {
		line.points.push_back({table.column(*x)[r], table.column(*y)[r], table.column(*z)[r]});
		VtkPointData data;
		data.affine_param = time ? table.column(*time)[r] : static_cast<double>(r);
		data.redshift = redshift ? table.column(*redshift)[r] : 1.0;
		data.four_velocity = Core::FourVector<double>(
			1.0,
			vx ? table.column(*vx)[r] : 0.0,
			vy ? table.column(*vy)[r] : 0.0,
			vz ? table.column(*vz)[r] : 0.0
		);
		line.data.push_back(data);
	}
	return VtkExporter::export_geodesics_vtp(lines);
}

}

[[nodiscard]] inline std::optional<std::filesystem::path> write_telemetry_table(
	const TelemetryTable& table,
	const RecordingSettings& settings,
	const std::filesystem::path& directory,
	const TelemetryMetadata& meta,
	std::string& error
) {
	using namespace TelemetryDetail;
	std::error_code ec;
	std::filesystem::create_directories(directory, ec);
	const std::filesystem::path path = directory / (settings.file_stem + "." + recording_format_extension(settings.format));
	const int precision = static_cast<int>(settings.precision);
	const std::string schema = build_metadata(table, meta, settings.format) + "}\n";

	bool written = false;
	switch (settings.format) {
		case RecordingFormat::TSV: {
			const std::string content = build_delimited(table, '\t', precision);
			written = write_bytes(path, content.data(), content.size());
			break;
		}
		case RecordingFormat::Json: {
			const std::string content = build_json(table, meta, precision);
			written = write_bytes(path, content.data(), content.size());
			break;
		}
		case RecordingFormat::JsonLines: {
			const std::string content = build_json_lines(table, precision);
			written = write_bytes(path, content.data(), content.size());
			break;
		}
		case RecordingFormat::BinaryColumns: {
			const std::string content = build_binary(table);
			written = write_bytes(path, content.data(), content.size());
			break;
		}
		case RecordingFormat::Container: {
			const std::vector<uint8_t> content = build_container(table, schema);
			written = write_bytes(path, content.data(), content.size());
			break;
		}
		case RecordingFormat::VtkPolyline: {
			const auto content = build_vtk(table, error);
			if (!content.has_value()) return std::nullopt;
			written = write_bytes(path, content->data(), content->size());
			break;
		}
		case RecordingFormat::CSV:
		default: {
			const std::string content = build_delimited(table, ',', precision);
			written = write_bytes(path, content.data(), content.size());
			break;
		}
	}
	if (!written) {
		error = "The telemetry file could not be written: " + path.string();
		return std::nullopt;
	}
	const std::filesystem::path schema_path = directory / (settings.file_stem + ".schema.json");
	static_cast<void>(write_bytes(schema_path, schema.data(), schema.size()));
	return path;
}

}
