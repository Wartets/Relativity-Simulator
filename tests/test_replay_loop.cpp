#include "support/test_harness.hpp"
#include "relativistic/core/math/tensor.hpp"
#include "relativistic/core/math/geodesic_bundle.hpp"
#include "relativistic/core/pcg64.hpp"
#include "relativistic/core/deterministic_replay.hpp"
#include "relativistic/core/sha256.hpp"
#include "relativistic/core/memory_arena.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

using namespace Relativistic::Core;
using Relativistic::Testing::TestSuite;

namespace {

[[nodiscard]] std::array<uint8_t, 32> hash_bundle_state(const GeodesicBundle4d& bundle) {
	SHA256 hasher;
	hasher.update_value(bundle.x0);
	hasher.update_value(bundle.x1);
	hasher.update_value(bundle.x2);
	hasher.update_value(bundle.x3);
	hasher.update_value(bundle.p0);
	hasher.update_value(bundle.p1);
	hasher.update_value(bundle.p2);
	hasher.update_value(bundle.p3);
	return hasher.finalize();
}

[[nodiscard]] std::string to_hex(const std::array<uint8_t, 32>& hash) {
	static constexpr char digits[] = "0123456789abcdef";
	std::string text;
	text.reserve(64);
	for (const uint8_t byte : hash) {
		text.push_back(digits[byte >> 4]);
		text.push_back(digits[byte & 0x0F]);
	}
	return text;
}

void execute_simulation_loop(GeodesicBundle4d& bundle, EventJournal& journal, ReplayMode mode, uint64_t ticks_to_simulate, uint64_t seed = 0x1337) {
	PCG64Engine rng(seed);
	double mass = 1.0;
	for (uint64_t tick = 0; tick < ticks_to_simulate; ++tick) {
		if (mode == ReplayMode::Record) {
			if (rng.next_bounded(100) < 5) {
				SimulationInputEvent event{};
				event.tick_index = tick;
				const uint32_t action_type = static_cast<uint32_t>(rng.next_bounded(2));
				if (action_type == 0) {
					event.action_id = static_cast<uint32_t>(SimulationAction::ApplyRadialImpulse);
					event.value = rng.next_uniform_range(-0.1, 0.1);
				} else {
					event.action_id = static_cast<uint32_t>(SimulationAction::ModifyMass);
					event.value = rng.next_uniform_range(0.9, 1.1);
				}
				journal.record_event(event);
			}
		}
		while (true) {
			const auto next_event = journal.peek_next_replay_event();
			if (!next_event || next_event->tick_index > tick) {
				break;
			}
			if (next_event->action_id == static_cast<uint32_t>(SimulationAction::ApplyRadialImpulse)) {
				bundle.p1 += next_event->value;
			} else if (next_event->action_id == static_cast<uint32_t>(SimulationAction::ModifyMass)) {
				mass = next_event->value;
			}
			journal.consume_replay_event();
		}
		bundle.step_rk4_schwarzschild(mass);
	}
}

}

int main() {
	TestSuite suite(
		"replay_loop",
		"Verifies deterministic record and replay: a geodesic bundle is evolved for ten thousand ticks while randomly generated impulse and mass events are recorded in the journal, then the journal is rewound and the same evolution is replayed from the recorded events alone, and the SHA-256 digests of the two final bundle states must be identical."
	);

	constexpr uint64_t simulation_ticks = 10000;
	std::vector<SimulationInputEvent> event_buffer(10000);
	EventJournal journal(event_buffer);

	GeodesicBundle4d initial_bundle;
	initial_bundle.set_ray(0, {0.0, 10.0, 1.57079632679, 0.0}, {1.0, 0.0, 0.0, 0.05}, 0.01);
	initial_bundle.set_ray(1, {0.0, 20.0, 1.57079632679, 0.0}, {1.0, -0.01, 0.0, 0.02}, 0.01);
	initial_bundle.set_ray(2, {0.0, 5.0, 1.57079632679, 0.0}, {1.0, 0.1, 0.0, 0.08}, 0.01);
	initial_bundle.set_ray(3, {0.0, 30.0, 0.5, 1.0}, {1.0, -0.05, 0.01, 0.01}, 0.01);
	const auto hash_initial = hash_bundle_state(initial_bundle);

	suite.section("Recording");
	journal.reset_cursors();
	GeodesicBundle4d record_bundle = initial_bundle;
	execute_simulation_loop(record_bundle, journal, ReplayMode::Record, simulation_ticks);
	const auto hash_record = hash_bundle_state(record_bundle);
	suite.check(journal.recorded_count() > 0, "the recording session stores events in the journal", "events=" + std::to_string(journal.recorded_count()));
	suite.check(hash_record != hash_initial, "the recorded evolution changes the bundle state");

	suite.section("Replay");
	journal.rewind_for_replay();
	GeodesicBundle4d replay_bundle = initial_bundle;
	execute_simulation_loop(replay_bundle, journal, ReplayMode::Replay, simulation_ticks);
	const auto hash_replay = hash_bundle_state(replay_bundle);
	suite.check(!journal.peek_next_replay_event().has_value(), "the replay consumes every recorded event");
	suite.check(hash_record == hash_replay, "the record and replay digests are identical", "record=" + to_hex(hash_record) + " replay=" + to_hex(hash_replay));
	suite.measure("recorded_events", static_cast<double>(journal.recorded_count()), "events");
	return suite.finish();
}
