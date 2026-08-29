#pragma once

#include <atomic>
#include <cstddef>
#include <concepts>
#include <new>
#include <type_traits>
#include <utility>
#include <array>

namespace Relativistic::Core {

template <typename T, size_t Capacity>
class alignas(128) SpscQueue {
	static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");
	static_assert(Capacity >= 2, "Capacity must be at least 2");
	static_assert(std::is_nothrow_destructible_v<T>, "T must be nothrow destructible");

private:
	static constexpr size_t BUFFER_MASK = Capacity - 1;

	alignas(64) std::atomic<size_t> head_{0};
	alignas(64) std::atomic<size_t> tail_{0};

	alignas(64) std::array<T, Capacity> buffer_{};

public:
	constexpr SpscQueue() noexcept = default;

	~SpscQueue() noexcept = default;

	SpscQueue(const SpscQueue&) = delete;
	SpscQueue& operator=(const SpscQueue&) = delete;
	SpscQueue(SpscQueue&&) = delete;
	SpscQueue& operator=(SpscQueue&&) = delete;

	[[nodiscard]] static constexpr size_t capacity() noexcept {
		return Capacity;
	}

	[[nodiscard]] bool empty() const noexcept {
		return head_.load(std::memory_order_relaxed) == tail_.load(std::memory_order_acquire);
	}

	[[nodiscard]] size_t size() const noexcept {
		const size_t t = tail_.load(std::memory_order_acquire);
		const size_t h = head_.load(std::memory_order_relaxed);
		return t - h;
	}

	[[nodiscard]] bool try_push(const T& value) noexcept {
		const size_t current_tail = tail_.load(std::memory_order_relaxed);
		const size_t current_head = head_.load(std::memory_order_acquire);

		if ((current_tail - current_head) >= Capacity) {
			return false;
		}

		buffer_[current_tail & BUFFER_MASK] = value;
		tail_.store(current_tail + 1, std::memory_order_release);
		return true;
	}

	[[nodiscard]] bool try_push(T&& value) noexcept {
		const size_t current_tail = tail_.load(std::memory_order_relaxed);
		const size_t current_head = head_.load(std::memory_order_acquire);

		if ((current_tail - current_head) >= Capacity) {
			return false;
		}

		buffer_[current_tail & BUFFER_MASK] = std::move(value);
		tail_.store(current_tail + 1, std::memory_order_release);
		return true;
	}

	[[nodiscard]] bool try_pop(T& value) noexcept {
		const size_t current_head = head_.load(std::memory_order_relaxed);
		const size_t current_tail = tail_.load(std::memory_order_acquire);

		if (current_head == current_tail) {
			return false;
		}

		value = std::move(buffer_[current_head & BUFFER_MASK]);
		head_.store(current_head + 1, std::memory_order_release);
		return true;
	}

	void reset() noexcept {
		head_.store(0, std::memory_order_relaxed);
		tail_.store(0, std::memory_order_relaxed);
	}
};

}
