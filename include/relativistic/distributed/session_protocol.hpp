#pragma once

#include "relativistic/distributed/packet_protocol.hpp"
#include <cstdint>

namespace Relativistic::Distributed {

inline constexpr uint32_t kAuthenticationProofMagic = 0x48545541U;
inline constexpr uint32_t kSessionRekeyPacketLimit = 1U << 24U;
inline constexpr uint32_t kSessionRekeyIntervalSeconds = 3600U;
inline constexpr uint32_t kHandshakeTimeoutMs = 5000U;
inline constexpr uint32_t kMaximumInFlightFrames = 2U;

inline constexpr uint8_t kTileTaskFlagForceCpu = 1U << 0U;
inline constexpr uint8_t kTileTaskFlagNoBodies = 1U << 1U;
inline constexpr uint8_t kTileTaskFlagReuseLatestBodies = 1U << 2U;

enum class DisconnectReason : uint32_t {
	Requested = 0,
	Shutdown = 1,
	AuthenticationFailed = 2,
	AbiMismatch = 3,
	InvalidCapabilities = 4,
	InsufficientCapabilities = 5,
	RoleMismatch = 6,
	HandshakeTimeout = 7,
	ProtocolViolation = 8,
	RekeyRequired = 9,
	SessionLimit = 10
};

enum class TaskCancelScope : uint32_t {
	SingleTile = 0,
	WholeFrame = 1,
	UpToFrame = 2
};

#pragma pack(push, 1)

struct AuthenticationProofPayload {
	uint32_t magic{kAuthenticationProofMagic};
	uint32_t role{0};
	uint64_t instance_identifier{0};
};

struct TaskCancelPayload {
	uint32_t frame_index{0};
	uint32_t tile_index{0};
	uint32_t scope{0};
};

#pragma pack(pop)

static_assert(sizeof(AuthenticationProofPayload) == 16);
static_assert(sizeof(TaskCancelPayload) == 12);
static_assert(DistributedPayload<AuthenticationProofPayload>);
static_assert(DistributedPayload<TaskCancelPayload>);

}
