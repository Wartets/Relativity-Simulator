#pragma once

#include "relativistic/core/sha256.hpp"
#include "relativistic/render/gpu_types.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace Relativistic::Core {

struct EngineSignature {
	static constexpr uint32_t STRUCTURAL_REVISION = 1;
	static constexpr std::string_view BUILD_TAG = "relativistic-engine-perf-abi";

	[[nodiscard]] static std::string compute() {
		SHA256 hasher;
		hasher.update(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(BUILD_TAG.data()), BUILD_TAG.size()));
		hasher.update_value(STRUCTURAL_REVISION);

		const std::array<size_t, 2> struct_sizes{
			sizeof(Render::GpuCameraPushConstants),
			sizeof(Render::GpuPixelOutput)
		};
		hasher.update(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(struct_sizes.data()), struct_sizes.size() * sizeof(size_t)));

		const auto digest = hasher.finalize();
		std::string hex;
		hex.reserve(16);
		static constexpr char kHexChars[] = "0123456789abcdef";
		for (size_t i = 0; i < 8; ++i) {
			hex.push_back(kHexChars[digest[i] >> 4]);
			hex.push_back(kHexChars[digest[i] & 0x0F]);
		}
		return hex;
	}
};

}
