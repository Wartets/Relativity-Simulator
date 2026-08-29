#include "relativistic/core/spsc_queue.hpp"
#include <cassert>
#include <cstdint>
#include <thread>
#include <vector>

int main() {
	using namespace Relativistic::Core;

	{
		SpscQueue<uint32_t, 16> queue;
		assert(queue.empty());
		assert(queue.size() == 0);
		assert(queue.capacity() == 16);

		for (uint32_t i = 0; i < 15; ++i) {
			const bool pushed = queue.try_push(i + 100);
			assert(pushed);
		}

		assert(!queue.empty());
		assert(queue.size() == 15);

		for (uint32_t i = 0; i < 15; ++i) {
			uint32_t val = 0;
			const bool popped = queue.try_pop(val);
			assert(popped);
			assert(val == i + 100);
		}

		assert(queue.empty());
		assert(queue.size() == 0);

		for (uint32_t cycle = 0; cycle < 50; ++cycle) {
			for (uint32_t i = 0; i < 8; ++i) {
				assert(queue.try_push(cycle * 100 + i));
			}
			for (uint32_t i = 0; i < 8; ++i) {
				uint32_t val = 0;
				assert(queue.try_pop(val));
				assert(val == cycle * 100 + i);
			}
		}
	}

	{
		constexpr size_t QUEUE_CAP = 1024;
		constexpr size_t TOTAL_ELEMENTS = 500000;
		SpscQueue<uint64_t, QUEUE_CAP> concurrent_queue;

		std::thread producer([&concurrent_queue]() {
			for (uint64_t i = 1; i <= TOTAL_ELEMENTS; ++i) {
				while (!concurrent_queue.try_push(i)) {
					std::this_thread::yield();
				}
			}
		});

		uint64_t last_val = 0;
		uint64_t count = 0;
		uint64_t sum = 0;

		std::thread consumer([&concurrent_queue, &last_val, &count, &sum]() {
			while (count < TOTAL_ELEMENTS) {
				uint64_t val = 0;
				if (concurrent_queue.try_pop(val)) {
					assert(val == last_val + 1);
					last_val = val;
					sum += val;
					++count;
				} else {
					std::this_thread::yield();
				}
			}
		});

		producer.join();
		consumer.join();

		assert(count == TOTAL_ELEMENTS);
		assert(last_val == TOTAL_ELEMENTS);
		const uint64_t expected_sum = (TOTAL_ELEMENTS * (TOTAL_ELEMENTS + 1)) / 2;
		assert(sum == expected_sum);
	}

	return 0;
}
