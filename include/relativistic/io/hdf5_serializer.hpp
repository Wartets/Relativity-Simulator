#pragma once

#include "relativistic/core/tensor.hpp"
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <vector>
#include <span>
#include <string>
#include <string_view>
#include <optional>
#include <array>
#include <algorithm>

namespace Relativistic::IO {

enum class Hdf5DataType : uint32_t {
	Float64 = 1,
	Float32 = 2,
	Int64 = 3,
	Int32 = 4,
	Byte = 5
};

struct Hdf5Dataset {
	std::string path{};
	Hdf5DataType type{Hdf5DataType::Float64};
	std::vector<size_t> dimensions{};
	std::vector<uint8_t> raw_data{};

	[[nodiscard]] size_t total_elements() const noexcept {
		if (dimensions.empty()) return 0;
		size_t total = 1;
		for (size_t d : dimensions) total *= d;
		return total;
	}

	template <typename T>
	[[nodiscard]] std::span<const T> as_span() const noexcept {
		const size_t count = raw_data.size() / sizeof(T);
		return std::span<const T>(reinterpret_cast<const T*>(raw_data.data()), count);
	}
};

class Hdf5Container {
private:
	static constexpr uint64_t HDF5_SIGNATURE = 0x894844460D0A1A0AULL;
	static constexpr uint32_t CONTAINER_VERSION = 1;

	std::vector<Hdf5Dataset> datasets_;

public:
	Hdf5Container() = default;

	void add_dataset(Hdf5Dataset&& dataset) {
		datasets_.push_back(std::move(dataset));
	}

	void write_tensor_series(
		std::string_view path,
		const std::vector<Core::MetricTensor<double>>& series
	) {
		Hdf5Dataset ds;
		ds.path = std::string(path);
		ds.type = Hdf5DataType::Float64;
		ds.dimensions = {series.size(), 4, 4};
		ds.raw_data.resize(series.size() * 16 * sizeof(double));
		std::memcpy(ds.raw_data.data(), series.data(), ds.raw_data.size());
		datasets_.push_back(std::move(ds));
	}

	void write_worldline(
		std::string_view path,
		const std::vector<Core::FourVector<double>>& positions,
		const std::vector<Core::FourVector<double>>& velocities
	) {
		Hdf5Dataset ds_pos;
		ds_pos.path = std::string(path) + "/position";
		ds_pos.type = Hdf5DataType::Float64;
		ds_pos.dimensions = {positions.size(), 4};
		ds_pos.raw_data.resize(positions.size() * sizeof(Core::FourVector<double>));
		std::memcpy(ds_pos.raw_data.data(), positions.data(), ds_pos.raw_data.size());
		datasets_.push_back(std::move(ds_pos));

		Hdf5Dataset ds_vel;
		ds_vel.path = std::string(path) + "/velocity";
		ds_vel.type = Hdf5DataType::Float64;
		ds_vel.dimensions = {velocities.size(), 4};
		ds_vel.raw_data.resize(velocities.size() * sizeof(Core::FourVector<double>));
		std::memcpy(ds_vel.raw_data.data(), velocities.data(), ds_vel.raw_data.size());
		datasets_.push_back(std::move(ds_vel));
	}

	[[nodiscard]] const std::vector<Hdf5Dataset>& datasets() const noexcept {
		return datasets_;
	}

	[[nodiscard]] std::optional<Hdf5Dataset> get_dataset(std::string_view path) const noexcept {
		for (const auto& ds : datasets_) {
			if (ds.path == path) {
				return ds;
			}
		}
		return std::nullopt;
	}

	[[nodiscard]] std::vector<uint8_t> serialize() const {
		std::vector<uint8_t> buffer;
		const uint64_t sig = HDF5_SIGNATURE;
		const uint32_t ver = CONTAINER_VERSION;
		const uint32_t count = static_cast<uint32_t>(datasets_.size());

		auto append_bytes = [&](const void* ptr, size_t sz) {
			const auto* byte_ptr = reinterpret_cast<const uint8_t*>(ptr);
			buffer.insert(buffer.end(), byte_ptr, byte_ptr + sz);
		};

		append_bytes(&sig, sizeof(sig));
		append_bytes(&ver, sizeof(ver));
		append_bytes(&count, sizeof(count));

		for (const auto& ds : datasets_) {
			const uint32_t path_len = static_cast<uint32_t>(ds.path.size());
			append_bytes(&path_len, sizeof(path_len));
			append_bytes(ds.path.data(), path_len);

			const uint32_t type_val = static_cast<uint32_t>(ds.type);
			append_bytes(&type_val, sizeof(type_val));

			const uint32_t rank = static_cast<uint32_t>(ds.dimensions.size());
			append_bytes(&rank, sizeof(rank));
			for (size_t d : ds.dimensions) {
				const uint64_t dim_val = static_cast<uint64_t>(d);
				append_bytes(&dim_val, sizeof(dim_val));
			}

			const uint64_t data_sz = static_cast<uint64_t>(ds.raw_data.size());
			append_bytes(&data_sz, sizeof(data_sz));
			append_bytes(ds.raw_data.data(), ds.raw_data.size());
		}

		return buffer;
	}

	[[nodiscard]] static std::optional<Hdf5Container> deserialize(std::span<const uint8_t> bytes) noexcept {
		if (bytes.size() < sizeof(uint64_t) + sizeof(uint32_t) * 2) {
			return std::nullopt;
		}

		size_t offset = 0;
		auto read_bytes = [&](void* dst, size_t sz) -> bool {
			if (offset + sz > bytes.size()) return false;
			std::memcpy(dst, bytes.data() + offset, sz);
			offset += sz;
			return true;
		};

		uint64_t sig = 0;
		uint32_t ver = 0;
		uint32_t count = 0;

		if (!read_bytes(&sig, sizeof(sig)) || sig != HDF5_SIGNATURE) return std::nullopt;
		if (!read_bytes(&ver, sizeof(ver)) || ver != CONTAINER_VERSION) return std::nullopt;
		if (!read_bytes(&count, sizeof(count))) return std::nullopt;

		Hdf5Container container;
		for (uint32_t i = 0; i < count; ++i) {
			uint32_t path_len = 0;
			if (!read_bytes(&path_len, sizeof(path_len))) return std::nullopt;
			if (offset + path_len > bytes.size()) return std::nullopt;

			std::string path(reinterpret_cast<const char*>(bytes.data() + offset), path_len);
			offset += path_len;

			uint32_t type_val = 0;
			if (!read_bytes(&type_val, sizeof(type_val))) return std::nullopt;

			uint32_t rank = 0;
			if (!read_bytes(&rank, sizeof(rank))) return std::nullopt;

			std::vector<size_t> dims;
			dims.reserve(rank);
			for (uint32_t r = 0; r < rank; ++r) {
				uint64_t dim_val = 0;
				if (!read_bytes(&dim_val, sizeof(dim_val))) return std::nullopt;
				dims.push_back(static_cast<size_t>(dim_val));
			}

			uint64_t data_sz = 0;
			if (!read_bytes(&data_sz, sizeof(data_sz))) return std::nullopt;
			if (offset + data_sz > bytes.size()) return std::nullopt;

			std::vector<uint8_t> raw(bytes.data() + offset, bytes.data() + offset + data_sz);
			offset += static_cast<size_t>(data_sz);

			Hdf5Dataset ds;
			ds.path = std::move(path);
			ds.type = static_cast<Hdf5DataType>(type_val);
			ds.dimensions = std::move(dims);
			ds.raw_data = std::move(raw);

			container.add_dataset(std::move(ds));
		}

		return container;
	}
};

}
