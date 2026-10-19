#include "support/test_harness.hpp"
#include "relativistic/orchestrator/scheduler.hpp"
#include <cstdint>

using Relativistic::Orchestrator::Scheduler;
using Relativistic::Testing::TestSuite;

namespace {

[[nodiscard]] uint64_t drain_ticks(Scheduler<double>& scheduler) noexcept {
	uint64_t executed = 0;
	while (scheduler.can_advance_tick() && scheduler.advance_tick()) {
		++executed;
	}
	return executed;
}

void check_initial_state(TestSuite& suite) {
	suite.section("Initial state");
	Scheduler<double> scheduler;
	const auto snapshot = scheduler.snapshot();
	suite.check(scheduler.current_tick() == 0, "a new scheduler starts at tick zero");
	suite.check_near(scheduler.logical_time(), 0.0, 1e-15, "a new scheduler starts at logical time zero");
	suite.check_near(scheduler.tick_rate(), 60.0, 1e-15, "the default tick rate is 60 Hz");
	suite.check_near(scheduler.tick_dt(), 1.0 / 60.0, 1e-15, "the default tick interval is 1/60 s");
	suite.check_near(scheduler.warp_factor(), 1.0, 1e-15, "the default warp factor is one");
	suite.check(scheduler.is_paused(), "a new scheduler starts paused");
	suite.check(scheduler.remaining_steps() == 0, "a new scheduler has no requested steps");
	suite.check(snapshot.tick_index == 0 && snapshot.is_paused && snapshot.remaining_steps == 0, "the snapshot reports the initial state");
	suite.check(!scheduler.can_advance_tick(), "a paused scheduler without requested steps cannot advance");
}

void check_tick_rate(TestSuite& suite) {
	suite.section("Tick rate limits");
	Scheduler<double> scheduler;
	scheduler.set_tick_rate(100.0);
	suite.check_near(scheduler.tick_rate(), 100.0, 1e-15, "an in-range tick rate is stored unchanged");
	suite.check_near(scheduler.tick_dt(), 0.01, 1e-15, "the tick interval follows the tick rate");
	scheduler.set_tick_rate(5.0);
	suite.check_near(scheduler.tick_rate(), 10.0, 1e-15, "a tick rate below the minimum is clamped to 10 Hz");
	scheduler.set_tick_rate(2000.0);
	suite.check_near(scheduler.tick_rate(), 1000.0, 1e-15, "a tick rate above the maximum is clamped to 1000 Hz");
	suite.check_near(scheduler.tick_dt(), 0.001, 1e-15, "the tick interval follows the clamped tick rate");
}

void check_real_time_advance(TestSuite& suite) {
	suite.section("Real-time advance and warp");
	{
		Scheduler<double> scheduler;
		scheduler.set_tick_rate(100.0);
		scheduler.resume();
		bool every_tick_advanced = true;
		for (uint64_t i = 0; i < 100; ++i) {
			scheduler.add_real_time_nanoseconds(10000000);
			every_tick_advanced = every_tick_advanced && scheduler.can_advance_tick() && scheduler.advance_tick() && scheduler.current_tick() == i + 1;
		}
		suite.check(every_tick_advanced, "one tick interval of real time yields exactly one tick");
		suite.check(scheduler.current_tick() == 100, "one hundred intervals yield one hundred ticks");
		suite.check_near(scheduler.logical_time(), 1.0, 1e-12, "one hundred ticks at 100 Hz span one logical second");
	}
	{
		Scheduler<double> scheduler;
		scheduler.set_tick_rate(100.0);
		scheduler.set_warp_factor(2.5);
		scheduler.resume();
		for (uint64_t i = 0; i < 100; ++i) {
			scheduler.add_real_time_nanoseconds(10000000);
			static_cast<void>(scheduler.advance_tick());
		}
		suite.check_near(scheduler.logical_time(), 2.5, 1e-12, "the warp factor scales the logical time per tick");
	}
	{
		Scheduler<double> scheduler;
		scheduler.set_warp_factor(0.0);
		suite.check_near(scheduler.warp_factor(), 1.0, 1e-15, "a zero warp factor is rejected");
		scheduler.set_warp_factor(-2.0);
		suite.check_near(scheduler.warp_factor(), 1.0, 1e-15, "a negative warp factor is rejected");
	}
}

void check_accumulator(TestSuite& suite) {
	suite.section("Accumulator behaviour");
	{
		Scheduler<double> scheduler;
		scheduler.resume();
		scheduler.add_real_time_nanoseconds(10000000000LL);
		suite.check(drain_ticks(scheduler) == 15, "a long stall is clamped to 0.25 s, which yields 15 ticks at 60 Hz");
	}
	{
		Scheduler<double> scheduler;
		scheduler.resume();
		scheduler.add_real_time_nanoseconds(0);
		scheduler.add_real_time_nanoseconds(-5000);
		suite.check(!scheduler.can_advance_tick(), "non-positive real-time increments are ignored");
	}
	{
		Scheduler<double> scheduler;
		scheduler.set_tick_rate(100.0);
		suite.check_near(scheduler.alpha(), 0.0, 1e-15, "the interpolation factor is zero with an empty accumulator");
		scheduler.add_real_time_nanoseconds(5000000);
		suite.check_near(scheduler.alpha(), 0.5, 1e-12, "half an interval of accumulated time gives an interpolation factor of 0.5");
		scheduler.add_real_time_nanoseconds(100000000);
		suite.check_near(scheduler.alpha(), 1.0, 1e-12, "the interpolation factor saturates at one");
	}
}

void check_stepping(TestSuite& suite) {
	suite.section("Pause and single stepping");
	Scheduler<double> scheduler;
	scheduler.set_tick_rate(100.0);
	scheduler.pause();
	scheduler.add_real_time_nanoseconds(50000000);
	suite.check(!scheduler.can_advance_tick() && !scheduler.advance_tick() && scheduler.current_tick() == 0, "a paused scheduler does not consume accumulated time");
	scheduler.request_steps(5);
	suite.check(scheduler.remaining_steps() == 5 && scheduler.can_advance_tick(), "requested steps allow advancing while paused");
	const uint64_t executed = drain_ticks(scheduler);
	suite.check(executed == 5 && scheduler.current_tick() == 5, "exactly the requested number of ticks executes");
	suite.check(scheduler.remaining_steps() == 0 && scheduler.is_paused(), "the scheduler pauses again when the requested steps are exhausted");
	scheduler.resume();
	suite.check(!scheduler.is_paused(), "resume clears the paused state");
	scheduler.request_steps(3);
	scheduler.pause();
	suite.check(scheduler.remaining_steps() == 0, "pause discards pending requested steps");
}

void check_reset(TestSuite& suite) {
	suite.section("Reset");
	Scheduler<double> scheduler;
	scheduler.set_tick_rate(100.0);
	scheduler.set_warp_factor(3.0);
	scheduler.resume();
	for (uint64_t i = 0; i < 10; ++i) {
		scheduler.add_real_time_nanoseconds(10000000);
		static_cast<void>(scheduler.advance_tick());
	}
	suite.check(scheduler.current_tick() == 10, "ten ticks execute before the reset");
	scheduler.reset();
	suite.check(scheduler.current_tick() == 0, "reset restores tick zero");
	suite.check_near(scheduler.logical_time(), 0.0, 1e-15, "reset restores logical time zero");
	suite.check(scheduler.is_paused(), "reset leaves the scheduler paused");
	suite.check_near(scheduler.warp_factor(), 1.0, 1e-15, "reset restores the default warp factor");
	suite.check_near(scheduler.tick_rate(), 60.0, 1e-15, "reset restores the default tick rate");
	suite.check_near(scheduler.alpha(), 0.0, 1e-15, "reset clears the accumulator");
}

}

int main() {
	TestSuite suite(
		"scheduler",
		"Verifies the deterministic simulation scheduler: initial paused state, tick rate limits, exact real-time to tick conversion, warp scaling, accumulator clamping, interpolation factor, pause and single-step semantics, and reset."
	);
	check_initial_state(suite);
	check_tick_rate(suite);
	check_real_time_advance(suite);
	check_accumulator(suite);
	check_stepping(suite);
	check_reset(suite);
	return suite.finish();
}
