#include "support/test_harness.hpp"
#include "relativistic/core/spsc_queue.hpp"
#include <atomic>
#include <cstdint>
#include <thread>

using Relativistic::Core::SpscQueue;
using Relativistic::Testing::TestSuite;

namespace {

void check_sequential(TestSuite& suite) {
	suite.section("Sequential behaviour");
	SpscQueue<uint32_t, 16> queue;
	suite.check(queue.empty() && queue.size() == 0, "a new queue is empty");
	suite.check(queue.capacity() == 16, "the capacity equals the template parameter");

	bool all_pushed = true;
	for (uint32_t i = 0; i < 15; ++i) {
		all_pushed = queue.try_push(i + 100) && all_pushed;
	}
	suite.check(all_pushed, "fifteen elements fit in a queue of capacity sixteen");
	suite.check(!queue.empty() && queue.size() == 15, "the size reflects the pushed elements");

	bool in_order = true;
	for (uint32_t i = 0; i < 15; ++i) {
		uint32_t value = 0;
		in_order = queue.try_pop(value) && value == i + 100 && in_order;
	}
	suite.check(in_order, "elements are popped in first-in first-out order");
	suite.check(queue.empty() && queue.size() == 0, "the queue is empty after popping every element");
	uint32_t unused = 0;
	suite.check(!queue.try_pop(unused), "popping an empty queue fails");

	bool wrap_ok = true;
	for (uint32_t cycle = 0; cycle < 50; ++cycle) {
		for (uint32_t i = 0; i < 8; ++i) {
			wrap_ok = queue.try_push(cycle * 100 + i) && wrap_ok;
		}
		for (uint32_t i = 0; i < 8; ++i) {
			uint32_t value = 0;
			wrap_ok = queue.try_pop(value) && value == cycle * 100 + i && wrap_ok;
		}
	}
	suite.check(wrap_ok, "repeated fill and drain cycles preserve order across index wrap-around");
}

void check_concurrent(TestSuite& suite) {
	suite.section("Concurrent producer and consumer");
	constexpr size_t kTotal = 500000;
	SpscQueue<uint64_t, 1024> queue;
	std::atomic<bool> order_violation{false};
	uint64_t last = 0;
	uint64_t count = 0;
	uint64_t sum = 0;

	std::thread producer([&queue]() {
		for (uint64_t i = 1; i <= kTotal; ++i) {
			while (!queue.try_push(i)) {
				std::this_thread::yield();
			}
		}
	});
	std::thread consumer([&]() {
		while (count < kTotal) {
			uint64_t value = 0;
			if (queue.try_pop(value)) {
				if (value != last + 1) {
					order_violation.store(true, std::memory_order_relaxed);
				}
				last = value;
				sum += value;
				++count;
			} else {
				std::this_thread::yield();
			}
		}
	});
	producer.join();
	consumer.join();

	suite.check(!order_violation.load(), "no element is lost, duplicated or reordered across threads");
	suite.check(count == kTotal && last == kTotal, "the consumer receives every element");
	suite.check(sum == (static_cast<uint64_t>(kTotal) * (kTotal + 1)) / 2, "the sum of consumed elements matches the arithmetic series");
	suite.check(queue.empty(), "the queue is empty after the transfer");
	suite.measure("transferred_elements", static_cast<double>(count), "elements");
}

}

int main() {
	TestSuite suite(
		"spsc_queue",
		"Verifies the lock-free single-producer single-consumer ring buffer: capacity, ordering, wrap-around and an integrity check over half a million elements transferred between two threads."
	);
	check_sequential(suite);
	check_concurrent(suite);
	return suite.finish();
}
