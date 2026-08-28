#pragma once

#include <cstddef>
#include <cstdint>
#include <new>
#include <concepts>
#include <type_traits>
#include <bit>
#include <array>

namespace Relativistic::Core {

template <size_t CapacityBytes, size_t BaseAlignment = 64>
class LinearMemoryArena {
	static_assert(BaseAlignment >= 64, "BaseAlignment must be at least 64 bytes for cache-line alignment");
	static_assert((BaseAlignment & (BaseAlignment - 1)) == 0, "BaseAlignment must be a power of two");
	static_assert(CapacityBytes > 0, "CapacityBytes must be greater than zero");

private:
	static constexpr uint64_t SENTINEL_HEAD_MAGIC = 0xDEADBEEFCAFE0001ULL;
	static constexpr uint64_t SENTINEL_TAIL_MAGIC = 0xDEADBEEFCAFE0002ULL;

	struct alignas(BaseAlignment) StorageBlock {
		uint64_t head_sentinel;
		alignas(BaseAlignment) std::byte memory[CapacityBytes];
		uint64_t tail_sentinel;
	};

	StorageBlock storage_;
	size_t offset_;
	size_t high_water_mark_;
	size_t allocation_count_;

public:
	using Marker = size_t;

	constexpr LinearMemoryArena() noexcept
		: storage_{.head_sentinel = SENTINEL_HEAD_MAGIC, .memory{}, .tail_sentinel = SENTINEL_TAIL_MAGIC},
		  offset_(0),
		  high_water_mark_(0),
		  allocation_count_(0) {}

	LinearMemoryArena(const LinearMemoryArena&) = delete;
	LinearMemoryArena& operator=(const LinearMemoryArena&) = delete;
	LinearMemoryArena(LinearMemoryArena&&) noexcept = delete;
	LinearMemoryArena& operator=(LinearMemoryArena&&) noexcept = delete;

	~LinearMemoryArena() noexcept {
		static_cast<void>(validate_sentinels());
	}

	[[nodiscard]] constexpr size_t capacity() const noexcept {
		return CapacityBytes;
	}

	[[nodiscard]] constexpr size_t allocated_bytes() const noexcept {
		return offset_;
	}

	[[nodiscard]] constexpr size_t remaining_bytes() const noexcept {
		return CapacityBytes - offset_;
	}

	[[nodiscard]] constexpr size_t high_water_mark() const noexcept {
		return high_water_mark_;
	}

	[[nodiscard]] constexpr size_t allocation_count() const noexcept {
		return allocation_count_;
	}

	[[nodiscard]] constexpr bool validate_sentinels() const noexcept {
		return (storage_.head_sentinel == SENTINEL_HEAD_MAGIC) && (storage_.tail_sentinel == SENTINEL_TAIL_MAGIC);
	}

	template <typename T, size_t ExplicitAlignment = alignof(T)>
	[[nodiscard]] T* allocate(size_t count = 1) noexcept {
		static_assert((ExplicitAlignment & (ExplicitAlignment - 1)) == 0, "Alignment must be a power of two");
		static_assert(ExplicitAlignment <= BaseAlignment || (ExplicitAlignment % BaseAlignment == 0), "Alignment exceeds base alignment capabilities");

		if (count == 0) [[unlikely]] {
			return nullptr;
		}

		const size_t bytes_needed = sizeof(T) * count;
		const uintptr_t current_ptr = reinterpret_cast<uintptr_t>(storage_.memory + offset_);
		const uintptr_t aligned_ptr = (current_ptr + (ExplicitAlignment - 1)) & ~(static_cast<uintptr_t>(ExplicitAlignment) - 1);
		const size_t padding = static_cast<size_t>(aligned_ptr - current_ptr);
		const size_t total_advance = padding + bytes_needed;

		if (offset_ + total_advance > CapacityBytes) [[unlikely]] {
			return nullptr;
		}

		offset_ += total_advance;
		if (offset_ > high_water_mark_) {
			high_water_mark_ = offset_;
		}
		++allocation_count_;

		return reinterpret_cast<T*>(aligned_ptr);
	}

	template <typename T, typename... Args>
	[[nodiscard]] T* create(Args&&... args) noexcept {
		T* memory = allocate<T, alignof(T)>(1);
		if (memory == nullptr) [[unlikely]] {
			return nullptr;
		}
		return ::new (static_cast<void*>(memory)) T(std::forward<Args>(args)...);
	}

	template <typename T, size_t ExplicitAlignment, typename... Args>
	[[nodiscard]] T* create_aligned(Args&&... args) noexcept {
		T* memory = allocate<T, ExplicitAlignment>(1);
		if (memory == nullptr) [[unlikely]] {
			return nullptr;
		}
		return ::new (static_cast<void*>(memory)) T(std::forward<Args>(args)...);
	}

	[[nodiscard]] constexpr Marker get_marker() const noexcept {
		return offset_;
	}

	constexpr void rewind_to_marker(Marker marker) noexcept {
		if (marker <= offset_) {
			offset_ = marker;
		}
	}

	constexpr void reset() noexcept {
		offset_ = 0;
		allocation_count_ = 0;
	}
};

template <size_t CapacityBytes>
using CacheAlignedArena64 = LinearMemoryArena<CapacityBytes, 64>;

template <size_t CapacityBytes>
using CacheAlignedArena128 = LinearMemoryArena<CapacityBytes, 128>;

template <typename ArenaType>
class ScopedArenaMarker {
private:
	ArenaType& arena_;
	typename ArenaType::Marker marker_;

public:
	explicit constexpr ScopedArenaMarker(ArenaType& arena) noexcept
		: arena_(arena), marker_(arena.get_marker()) {}

	ScopedArenaMarker(const ScopedArenaMarker&) = delete;
	ScopedArenaMarker& operator=(const ScopedArenaMarker&) = delete;

	~ScopedArenaMarker() noexcept {
		arena_.rewind_to_marker(marker_);
	}
};

}
