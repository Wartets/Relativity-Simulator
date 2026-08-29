#include "relativistic/orchestrator/scheduler.hpp"
#include <cassert>
#include <cmath>

int main() {
	using namespace Relativistic::Orchestrator;

	{
		Scheduler<double> scheduler;
		assert(scheduler.current_tick() == 0);
		assert(std::abs(scheduler.logical_time() - 0.0) < 1e-15);
		assert(std::abs(scheduler.tick_rate() - 60.0) < 1e-15);
		assert(std::abs(scheduler.tick_dt() - (1.0 / 60.0)) < 1e-15);
		assert(!scheduler.is_paused());
		assert(scheduler.remaining_steps() == 0);
	}

	{
		Scheduler<double> scheduler;
		scheduler.set_tick_rate(100.0);
		assert(std::abs(scheduler.tick_rate() - 100.0) < 1e-15);
		assert(std::abs(scheduler.tick_dt() - 0.01) < 1e-15);

		scheduler.set_tick_rate(5.0);
		assert(std::abs(scheduler.tick_rate() - 10.0) < 1e-15);

		scheduler.set_tick_rate(2000.0);
		assert(std::abs(scheduler.tick_rate() - 1000.0) < 1e-15);
	}

	{
		Scheduler<double> scheduler;
		scheduler.set_tick_rate(100.0);

		for (uint64_t i = 0; i < 100; ++i) {
			scheduler.add_real_time_nanoseconds(10000000);
			assert(scheduler.can_advance_tick());
			const bool advanced = scheduler.advance_tick();
			assert(advanced);
			assert(scheduler.current_tick() == i + 1);
		}

		assert(scheduler.current_tick() == 100);
		assert(std::abs(scheduler.logical_time() - 1.0) < 1e-12);
	}

	{
		Scheduler<double> scheduler;
		scheduler.set_tick_rate(100.0);
		scheduler.set_warp_factor(2.5);

		for (uint64_t i = 0; i < 100; ++i) {
			scheduler.add_real_time_nanoseconds(10000000);
			assert(scheduler.advance_tick());
		}

		assert(scheduler.current_tick() == 100);
		assert(std::abs(scheduler.logical_time() - 2.5) < 1e-12);
	}

	{
		Scheduler<double> scheduler;
		scheduler.set_tick_rate(100.0);
		scheduler.pause();
		assert(scheduler.is_paused());

		scheduler.add_real_time_nanoseconds(50000000);
		assert(!scheduler.can_advance_tick());
		assert(!scheduler.advance_tick());
		assert(scheduler.current_tick() == 0);

		scheduler.request_steps(5);
		assert(scheduler.remaining_steps() == 5);
		assert(scheduler.can_advance_tick());

		for (uint64_t i = 0; i < 5; ++i) {
			assert(scheduler.advance_tick());
		}

		assert(scheduler.current_tick() == 5);
		assert(scheduler.remaining_steps() == 0);
		assert(scheduler.is_paused());
		assert(!scheduler.can_advance_tick());

		scheduler.resume();
		assert(!scheduler.is_paused());
	}

	{
		Scheduler<double> scheduler;
		scheduler.set_tick_rate(100.0);
		for (uint64_t i = 0; i < 10; ++i) {
			scheduler.add_real_time_nanoseconds(10000000);
			assert(scheduler.advance_tick());
		}
		assert(scheduler.current_tick() == 10);

		scheduler.reset();
		assert(scheduler.current_tick() == 0);
		assert(std::abs(scheduler.logical_time() - 0.0) < 1e-15);
		assert(!scheduler.is_paused());
	}

	return 0;
}
