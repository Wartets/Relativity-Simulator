#include "relativistic/core/thread_pool.hpp"
#include <cassert>
#include <iostream>
#include <vector>
#include <atomic>
#include <numeric>

int main() {
	using namespace Relativistic::Core;

	ThreadPool pool(4);
	assert(pool.thread_count() == 4);

	std::atomic<int> counter{0};
	for (int i = 0; i < 100; ++i) {
		pool.enqueue([&counter]() {
			counter.fetch_add(1, std::memory_order_relaxed);
		});
	}
	pool.wait_idle();
	assert(counter.load() == 100);

	constexpr size_t array_size = 10000;
	std::vector<int> data(array_size, 1);
	pool.parallel_for(array_size, [&data](size_t start, size_t end) {
		for (size_t i = start; i < end; ++i) {
			data[i] *= 2;
		}
	}, 128);

	for (size_t i = 0; i < array_size; ++i) {
		assert(data[i] == 2);
	}

	std::cout << "test_thread_pool: ALL ASSERTIONS PASSED" << std::endl;
	return 0;
}
