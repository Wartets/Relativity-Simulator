#include "relativistic/core/tensor.hpp"
#include "relativistic/core/geodesic_bundle.hpp"
#include "relativistic/core/pcg64.hpp"
#include "relativistic/core/deterministic_replay.hpp"
#include "relativistic/core/sha256.hpp"
#include "relativistic/core/memory_arena.hpp"
#include <cassert>
#include <iostream>
#include <iomanip>
#include <vector>

using namespace Relativistic::Core;

std::array<uint8_t, 32> hash_bundle_state(const GeodesicBundle4d& bundle) {
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

void print_hash(const std::array<uint8_t, 32>& hash) {
	for (const auto b : hash) {
		std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
	}
	std::cout << std::dec << '\n';
}

void execute_simulation_loop(
	GeodesicBundle4d& bundle,
	EventJournal& journal,
	ReplayMode mode,
	uint64_t ticks_to_simulate,
	uint64_t seed = 0x1337
) {
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
			auto opt_event = journal.peek_next_replay_event();
			if (!opt_event || opt_event->tick_index > tick) {
				break;
			}
			
			const auto& event = *opt_event;
			if (event.action_id == static_cast<uint32_t>(SimulationAction::ApplyRadialImpulse)) {
				bundle.p1 += event.value;
			} else if (event.action_id == static_cast<uint32_t>(SimulationAction::ModifyMass)) {
				mass = event.value;
			}
			journal.consume_replay_event();
		}

		bundle.step_rk4_schwarzschild(mass);
	}
}

int main() {
	std::vector<SimulationInputEvent> event_buffer(10000);
	EventJournal journal(event_buffer);

	GeodesicBundle4d initial_bundle;
	initial_bundle.set_ray(0, {0.0, 10.0, 1.57079632679, 0.0}, {1.0, 0.0, 0.0, 0.05}, 0.01);
	initial_bundle.set_ray(1, {0.0, 20.0, 1.57079632679, 0.0}, {1.0, -0.01, 0.0, 0.02}, 0.01);
	initial_bundle.set_ray(2, {0.0, 5.0, 1.57079632679, 0.0}, {1.0, 0.1, 0.0, 0.08}, 0.01);
	initial_bundle.set_ray(3, {0.0, 30.0, 0.5, 1.0}, {1.0, -0.05, 0.01, 0.01}, 0.01);

	constexpr uint64_t SIM_TICKS = 10000;

	journal.reset_cursors();
	GeodesicBundle4d record_bundle = initial_bundle;
	execute_simulation_loop(record_bundle, journal, ReplayMode::Record, SIM_TICKS);
	
	const auto hash_record = hash_bundle_state(record_bundle);

	journal.rewind_for_replay();
	GeodesicBundle4d replay_bundle = initial_bundle;
	execute_simulation_loop(replay_bundle, journal, ReplayMode::Replay, SIM_TICKS);
	
	const auto hash_replay = hash_bundle_state(replay_bundle);

	std::cout << "Record Hash : "; print_hash(hash_record);
	std::cout << "Replay Hash : "; print_hash(hash_replay);
	std::cout << "Events processed : " << journal.recorded_count() << '\n';

	assert(hash_record == hash_replay && "Determinism failure: Record and Replay hashes mismatch!");

	return 0;
}
