#pragma once

#include "relativistic/core/engine_log.hpp"
#include "relativistic/optics/earth_texture_catalog.hpp"
#include "relativistic/optics/sky_panorama_image.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace Relativistic::Optics {

namespace EarthTextureDetail {

[[nodiscard]] inline const std::array<uint8_t, 4096>& srgb_encode_table() noexcept {
	static const std::array<uint8_t, 4096> table = [] {
		std::array<uint8_t, 4096> t{};
		for (size_t i = 0; i < t.size(); ++i) {
			const double linear = static_cast<double>(i) / 4095.0;
			const double encoded = (linear <= 0.0031308) ? (12.92 * linear) : (1.055 * std::pow(linear, 1.0 / 2.4) - 0.055);
			t[i] = static_cast<uint8_t>(std::clamp(encoded, 0.0, 1.0) * 255.0 + 0.5);
		}
		return t;
	}();
	return table;
}

[[nodiscard]] inline DecodedPanorama downsample_half(const DecodedPanorama& source) {
	const auto& decode = PanoramaColor::kSrgbToLinear;
	const auto& encode = srgb_encode_table();

	DecodedPanorama result;
	result.width = std::max<uint32_t>(1U, source.width / 2U);
	result.height = std::max<uint32_t>(1U, source.height / 2U);
	result.texels.assign(static_cast<size_t>(result.width) * result.height, 0U);

	const auto encode_channel = [&encode](float linear) noexcept -> uint8_t {
		return encode[static_cast<size_t>(std::clamp(linear, 0.0f, 1.0f) * 4095.0f + 0.5f)];
	};

	for (uint32_t y = 0; y < result.height; ++y) {
		const uint32_t y0 = std::min(y * 2U, source.height - 1U);
		const uint32_t y1 = std::min(y0 + 1U, source.height - 1U);
		for (uint32_t x = 0; x < result.width; ++x) {
			const uint32_t x0 = std::min(x * 2U, source.width - 1U);
			const uint32_t x1 = std::min(x0 + 1U, source.width - 1U);
			const std::array<uint32_t, 4> taps{
				source.texels[static_cast<size_t>(y0) * source.width + x0],
				source.texels[static_cast<size_t>(y0) * source.width + x1],
				source.texels[static_cast<size_t>(y1) * source.width + x0],
				source.texels[static_cast<size_t>(y1) * source.width + x1]
			};
			float sum_r = 0.0f;
			float sum_g = 0.0f;
			float sum_b = 0.0f;
			for (const uint32_t texel : taps) {
				sum_r += decode[texel & 0xFFU];
				sum_g += decode[(texel >> 8) & 0xFFU];
				sum_b += decode[(texel >> 16) & 0xFFU];
			}
			result.texels[static_cast<size_t>(y) * result.width + x] = DecodedPanorama::pack_texel(
				encode_channel(sum_r * 0.25f),
				encode_channel(sum_g * 0.25f),
				encode_channel(sum_b * 0.25f)
			);
		}
	}
	return result;
}

}

struct EarthTextureImage {
	std::vector<DecodedPanorama> levels{};

	[[nodiscard]] static EarthTextureImage from_base(DecodedPanorama base) {
		EarthTextureImage image;
		image.levels.reserve(16);
		image.levels.push_back(std::move(base));
		while (image.levels.back().width > 1U || image.levels.back().height > 1U) {
			DecodedPanorama next = EarthTextureDetail::downsample_half(image.levels.back());
			image.levels.push_back(std::move(next));
		}
		return image;
	}

	[[nodiscard]] static EarthTextureImage make_placeholder() {
		EarthTextureImage image;
		DecodedPanorama texel;
		texel.width = 1;
		texel.height = 1;
		texel.texels.assign(1, DecodedPanorama::pack_texel(0, 0, 0));
		image.levels.push_back(std::move(texel));
		return image;
	}

	[[nodiscard]] bool is_valid() const noexcept {
		return !levels.empty() && levels.front().is_valid();
	}
};

class EarthTextureLoader {
public:
	using ImageHandle = std::shared_ptr<const EarthTextureImage>;

private:
	static constexpr size_t kSlotCount = kEarthMapKindCount * kEarthMapQualityCount;

	enum class SlotState : uint8_t {
		Empty,
		Decoding,
		Ready,
		Failed
	};

	struct Slot {
		SlotState state{SlotState::Empty};
		uint64_t generation{0};
		ImageHandle image{};
	};

	struct Worker {
		std::jthread thread{};
		std::shared_ptr<std::atomic<bool>> finished{};
	};

	std::mutex mutex_{};
	std::array<Slot, kSlotCount> slots_{};
	std::vector<Worker> workers_{};
	EarthTextureIdleGate idle_gate_{};
	std::atomic<uint64_t> epoch_{1};
	std::atomic<uint64_t> revision_{1};

	EarthTextureLoader() = default;

	~EarthTextureLoader() {
		std::vector<Worker> workers_to_join;
		{
			std::lock_guard<std::mutex> lock(mutex_);
			workers_to_join = std::move(workers_);
		}
	}

	EarthTextureLoader(const EarthTextureLoader&) = delete;
	EarthTextureLoader& operator=(const EarthTextureLoader&) = delete;

	[[nodiscard]] static constexpr size_t slot_index(EarthMapKind kind, EarthMapQuality quality) noexcept {
		return static_cast<size_t>(kind) * kEarthMapQualityCount + static_cast<size_t>(quality);
	}

	static void reset_slot(Slot& slot) noexcept {
		slot.state = SlotState::Empty;
		slot.image.reset();
		++slot.generation;
	}

	void reap_finished_workers() {
		std::erase_if(workers_, [](Worker& worker) {
			if (!worker.finished->load(std::memory_order_acquire)) {
				return false;
			}
			worker.thread.join();
			return true;
		});
	}

	[[nodiscard]] static ImageHandle decode(EarthMapKind kind, EarthMapQuality quality) noexcept {
		const std::string_view path = earth_map_relative_path(kind, quality);
		try {
			auto decoded = PanoramaDecodeDetail::decode_panorama_file(path);
			if (decoded.has_value() && decoded->is_valid()) {
				return std::make_shared<const EarthTextureImage>(EarthTextureImage::from_base(std::move(*decoded)));
			}
		} catch (...) {
		}
		try {
			Core::log_error("Earth texture could not be decoded, the body keeps its base color: " + std::string(path));
		} catch (...) {
		}
		return nullptr;
	}

	void launch_decode(EarthMapKind kind, EarthMapQuality quality, size_t index, uint64_t generation) noexcept {
		auto finished = std::make_shared<std::atomic<bool>>(false);
		try {
			std::jthread thread([this, kind, quality, index, generation, finished]() {
				ImageHandle image = decode(kind, quality);
				{
					std::lock_guard<std::mutex> lock(mutex_);
					Slot& slot = slots_[index];
					if (slot.generation == generation && slot.state == SlotState::Decoding) {
						slot.image = std::move(image);
						slot.state = (slot.image != nullptr) ? SlotState::Ready : SlotState::Failed;
						revision_.fetch_add(1, std::memory_order_release);
					}
				}
				finished->store(true, std::memory_order_release);
			});
			std::lock_guard<std::mutex> lock(mutex_);
			reap_finished_workers();
			workers_.push_back(Worker{std::move(thread), std::move(finished)});
		} catch (...) {
			std::lock_guard<std::mutex> lock(mutex_);
			Slot& slot = slots_[index];
			if (slot.generation == generation && slot.state == SlotState::Decoding) {
				slot.state = SlotState::Failed;
			}
		}
	}

public:
	static EarthTextureLoader& instance() noexcept {
		static EarthTextureLoader loader;
		return loader;
	}

	[[nodiscard]] ImageHandle try_acquire(EarthMapKind kind, EarthMapQuality quality) noexcept {
		const size_t index = slot_index(kind, quality);
		uint64_t generation = 0;
		{
			std::lock_guard<std::mutex> lock(mutex_);
			Slot& slot = slots_[index];
			switch (slot.state) {
				case SlotState::Ready:
					return slot.image;
				case SlotState::Decoding:
				case SlotState::Failed:
					return nullptr;
				case SlotState::Empty:
					break;
			}
			slot.state = SlotState::Decoding;
			generation = slot.generation;
		}
		launch_decode(kind, quality, index, generation);
		return nullptr;
	}

	[[nodiscard]] bool is_decoding(EarthMapKind kind, EarthMapQuality quality) noexcept {
		std::lock_guard<std::mutex> lock(mutex_);
		return slots_[slot_index(kind, quality)].state == SlotState::Decoding;
	}

	[[nodiscard]] bool await(EarthMapKind kind, EarthMapQuality quality, std::chrono::milliseconds timeout) noexcept {
		const auto deadline = std::chrono::steady_clock::now() + timeout;
		while (true) {
			if (try_acquire(kind, quality) != nullptr) {
				return true;
			}
			if (!is_decoding(kind, quality) || std::chrono::steady_clock::now() >= deadline) {
				return false;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(2));
		}
	}

	void release(EarthMapKind kind, EarthMapQuality quality) noexcept {
		{
			std::lock_guard<std::mutex> lock(mutex_);
			reset_slot(slots_[slot_index(kind, quality)]);
		}
		epoch_.fetch_add(1, std::memory_order_release);
	}

	void release_kind(EarthMapKind kind) noexcept {
		{
			std::lock_guard<std::mutex> lock(mutex_);
			for (size_t q = 0; q < kEarthMapQualityCount; ++q) {
				reset_slot(slots_[static_cast<size_t>(kind) * kEarthMapQualityCount + q]);
			}
		}
		epoch_.fetch_add(1, std::memory_order_release);
	}

	void release_all() noexcept {
		{
			std::lock_guard<std::mutex> lock(mutex_);
			for (Slot& slot : slots_) {
				reset_slot(slot);
			}
		}
		epoch_.fetch_add(1, std::memory_order_release);
	}

	void trim_when_idle(bool demanded) noexcept {
		bool expired = false;
		{
			std::lock_guard<std::mutex> lock(mutex_);
			expired = idle_gate_.should_release(demanded);
		}
		if (expired) {
			release_all();
		}
	}

	[[nodiscard]] uint64_t revision() const noexcept {
		return revision_.load(std::memory_order_acquire);
	}

	[[nodiscard]] std::optional<std::array<float, 3>> sample(EarthMapKind kind, EarthMapQuality quality, double u, double v) noexcept {
		struct CachedImage {
			uint64_t epoch{0};
			ImageHandle image{};
		};
		thread_local std::array<CachedImage, kSlotCount> cache{};

		CachedImage& entry = cache[slot_index(kind, quality)];
		const uint64_t current_epoch = epoch_.load(std::memory_order_acquire);
		if (entry.epoch != current_epoch) {
			entry.image.reset();
			entry.epoch = current_epoch;
		}
		if (!entry.image) {
			entry.image = try_acquire(kind, quality);
			if (!entry.image) {
				return std::nullopt;
			}
		}
		return entry.image->levels.front().sample_bilinear(u, v);
	}
};

}
