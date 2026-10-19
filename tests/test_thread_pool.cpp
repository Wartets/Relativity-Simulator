#include "support/test_harness.hpp"
#include "relativistic/core/thread_pool.hpp"
#include <atomic>
#include <cstddef>
#include <vector>

using Relativistic::Core::ThreadPool;
using Relativistic::Testing::TestSuite;

int main() {
	TestSuite suite(
		"thread_pool",
		"Verifies the persistent worker pool: worker count, execution of every enqueued task before wait_idle returns, and exact coverage of the index range by chunked parallel loops."
	);

	suite.section("Task execution");
	ThreadPool pool(4);
	suite.check(pool.thread_count() == 4, "the pool creates the requested number of workers");
	std::atomic<int> counter{0};
	for (int i = 0; i < 100; ++i) {
		pool.enqueue([&counter]() { counter.fetch_add(1, std::memory_order_relaxed); });
	}
	pool.wait_idle();
	suite.check(counter.load() == 100, "wait_idle returns only after all hundred tasks completed");

	suite.section("Chunked parallel loop");
	constexpr size_t kSize = 10000;
	std::vector<int> data(kSize, 1);
	pool.parallel_for(kSize, [&data](size_t start, size_t end) {
		for (size_t i = start; i < end; ++i) {
			data[i] *= 2;
		}
	}, 128);
	size_t wrong = 0;
	for (const int value : data) {
		if (value != 2) {
			++wrong;
		}
	}
	suite.check(wrong == 0, "every index is processed exactly once", "wrong_elements=" + std::to_string(wrong));

	std::atomic<size_t> covered{0};
	pool.parallel_for(1, [&covered](size_t start, size_t end) { covered.fetch_add(end - start, std::memory_order_relaxed); });
	suite.check(covered.load() == 1, "a single-element range is processed exactly once");

	return suite.finish();
}
