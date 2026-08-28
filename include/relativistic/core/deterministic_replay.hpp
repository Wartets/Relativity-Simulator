#pragma once

#include <cstdint>
#include <span>
#include <array>
#include <optional>

namespace Relativistic::Core {

enum class SimulationAction : uint32_t {
	None = 0,
	ApplyRadialImpulse = 1,
	ApplyAngularImpulse = 2,
	ModifyMass = 3,
	ModifySpin = 4,
	ChangeTimeStep = 5
};

#pragma pack(push, 1)
struct SimulationInputEvent {
	uint64_t tick_index;
	uint32_t action_id;
	double value;
	uint8_t flags;
};
#pragma pack(pop)

static_assert(sizeof(SimulationInputEvent) == 21, "SimulationInputEvent alignment must be strictly packed to 21 bytes");

class EventJournal {
private:
	std::span<SimulationInputEvent> events_;
	size_t written_count_;
	size_t replay_cursor_;

public:
	explicit constexpr EventJournal(std::span<SimulationInputEvent> buffer) noexcept
		: events_(buffer), written_count_(0), replay_cursor_(0) {}

	constexpr void reset_cursors() noexcept {
		written_count_ = 0;
		replay_cursor_ = 0;
	}

	constexpr void rewind_for_replay() noexcept {
		replay_cursor_ = 0;
	}

	constexpr bool record_event(const SimulationInputEvent& event) noexcept {
		if (written_count_ >= events_.size()) [[unlikely]] {
			return false;
		}
		events_[written_count_++] = event;
		return true;
	}

	[[nodiscard]] constexpr std::optional<SimulationInputEvent> peek_next_replay_event() const noexcept {
		if (replay_cursor_ >= written_count_) {
			return std::nullopt;
		}
		return events_[replay_cursor_];
	}

	constexpr void consume_replay_event() noexcept {
		if (replay_cursor_ < written_count_) {
			++replay_cursor_;
		}
	}

	[[nodiscard]] constexpr size_t recorded_count() const noexcept {
		return written_count_;
	}
};

enum class ReplayMode : uint8_t {
	Record,
	Replay,
	Detached
};

}
