#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <span>
#include <string_view>
#include <concepts>
#include <type_traits>
#include <algorithm>
#include <cstring>

namespace Relativistic::Distributed {

enum class PacketType : uint16_t {
	HeartbeatPing = 0x0001,
	HeartbeatPong = 0x0002,
	NodeAuthentication = 0x0010,
	NodeAuthChallenge = 0x0011,
	NodeAuthResponse = 0x0012,
	CapabilitiesAdvertisement = 0x0020,
	SynchronizeClock = 0x0030,
	UpdateConstants = 0x0040,
	TaskAssignment = 0x0100,
	TaskCancel = 0x0101,
	TileResultData = 0x0200,
	SimulationSubStepData = 0x0300,
	MonteCarloBatchData = 0x0400,
	NodeDisconnect = 0x0FFF
};

[[nodiscard]] constexpr const char* packet_type_name(PacketType type) noexcept {
	switch (type) {
		case PacketType::HeartbeatPing: return "HeartbeatPing";
		case PacketType::HeartbeatPong: return "HeartbeatPong";
		case PacketType::NodeAuthentication: return "NodeAuthentication";
		case PacketType::NodeAuthChallenge: return "NodeAuthChallenge";
		case PacketType::NodeAuthResponse: return "NodeAuthResponse";
		case PacketType::CapabilitiesAdvertisement: return "CapabilitiesAdvertisement";
		case PacketType::SynchronizeClock: return "SynchronizeClock";
		case PacketType::UpdateConstants: return "UpdateConstants";
		case PacketType::TaskAssignment: return "TaskAssignment";
		case PacketType::TaskCancel: return "TaskCancel";
		case PacketType::TileResultData: return "TileResultData";
		case PacketType::SimulationSubStepData: return "SimulationSubStepData";
		case PacketType::MonteCarloBatchData: return "MonteCarloBatchData";
		case PacketType::NodeDisconnect: return "NodeDisconnect";
		default: return "Unknown";
	}
}

enum class ClusterNodeRole : uint32_t {
	Standalone = 0,
	Master = 1,
	Worker = 2,
	Hybrid = 3
};

[[nodiscard]] constexpr const char* cluster_node_role_name(ClusterNodeRole role) noexcept {
	switch (role) {
		case ClusterNodeRole::Standalone: return "Standalone";
		case ClusterNodeRole::Master: return "Master";
		case ClusterNodeRole::Worker: return "Worker";
		case ClusterNodeRole::Hybrid: return "Hybrid";
		default: return "Unknown";
	}
}

enum class NodeState : uint32_t {
	Unauthenticated = 0,
	Handshaking = 1,
	Synchronized = 2,
	Ready = 3,
	Computing = 4,
	Stalled = 5,
	Disconnected = 6
};

[[nodiscard]] constexpr const char* node_state_name(NodeState state) noexcept {
	switch (state) {
		case NodeState::Unauthenticated: return "Unauthenticated";
		case NodeState::Handshaking: return "Handshaking";
		case NodeState::Synchronized: return "Synchronized";
		case NodeState::Ready: return "Ready";
		case NodeState::Computing: return "Computing";
		case NodeState::Stalled: return "Stalled";
		case NodeState::Disconnected: return "Disconnected";
		default: return "Unknown";
	}
}

struct NodeHardwareCapabilities {
	uint32_t engine_abi_signature{0};
	uint32_t logical_cores{1};
	uint64_t physical_memory_bytes{0};
	uint32_t vulkan_api_version{0};
	uint32_t vendor_id{0};
	uint32_t device_id{0};
	bool has_vulkan_compute{false};
	bool has_fp64_support{false};
	bool has_neon{false};
	bool has_avx2{false};
	bool has_avx512{false};
	float relative_performance_weight{1.0f};
};

#pragma pack(push, 1)

struct alignas(16) PacketEnvelope {
	static constexpr uint32_t kMagicIdentifier = 0x52454C44;
	static constexpr uint16_t kProtocolVersion = 1;
	static constexpr size_t kHeaderSize = 28;
	static constexpr size_t kTagSize = 16;
	static constexpr size_t kEnvelopeSize = 48;

	uint32_t magic_identifier{kMagicIdentifier};
	uint16_t protocol_version{kProtocolVersion};
	PacketType type{PacketType::HeartbeatPing};
	uint32_t session_identifier{0};
	uint32_t packet_sequence{0};
	uint32_t frame_index{0};
	uint32_t payload_bytes{0};
	uint32_t payload_crc32{0};
	std::array<uint8_t, 16> authentication_tag{};

	[[nodiscard]] constexpr bool is_valid_magic() const noexcept {
		return magic_identifier == kMagicIdentifier;
	}

	[[nodiscard]] std::span<const uint8_t, kHeaderSize> authenticated_header() const noexcept {
		return std::span<const uint8_t, kHeaderSize>(reinterpret_cast<const uint8_t*>(this), kHeaderSize);
	}

	[[nodiscard]] std::span<uint8_t, kHeaderSize> authenticated_header() noexcept {
		return std::span<uint8_t, kHeaderSize>(reinterpret_cast<uint8_t*>(this), kHeaderSize);
	}
};

static_assert(sizeof(PacketEnvelope) == 48);
static_assert(offsetof(PacketEnvelope, authentication_tag) == PacketEnvelope::kHeaderSize);

struct TileWorkAssignment {
	uint32_t tile_index{0};
	uint16_t tile_x{0};
	uint16_t tile_y{0};
	uint16_t tile_width{32};
	uint16_t tile_height{32};
	uint16_t lod_step_budget{0};
	uint8_t flags{0};
	uint8_t reserved{0};
	double dispatch_timestamp_seconds{0.0};
};

static_assert(sizeof(TileWorkAssignment) == 24);

struct RenderTileTaskHeader {
	uint32_t tile_index{0};
	uint16_t tile_x{0};
	uint16_t tile_y{0};
	uint16_t tile_width{32};
	uint16_t tile_height{32};
	uint16_t lod_reduced_steps{0};
	uint8_t flags{0};
	uint8_t compression_type{0};
};

static_assert(sizeof(RenderTileTaskHeader) == 16);

struct RenderTileResultHeader {
	uint32_t tile_index{0};
	uint64_t frame_index{0};
	uint16_t pixel_count{1024};
	uint8_t compression_type{0};
	uint8_t reserved{0};
	uint32_t uncompressed_bytes{0};
	uint32_t execution_time_us{0};
};

static_assert(sizeof(RenderTileResultHeader) == 24);

#pragma pack(pop)

template <typename PayloadType>
concept DistributedPayload = requires(PayloadType payload) {
	requires std::is_trivially_copyable_v<PayloadType>;
	requires sizeof(PayloadType) <= 65536;
};

inline constexpr uint16_t kDefaultClusterPort = 42425;
inline constexpr uint16_t kDefaultDiscoveryPort = 42424;
inline constexpr std::string_view kDefaultMulticastGroup = "239.255.77.88";
inline constexpr uint32_t kDefaultHeartbeatIntervalMs = 500;
inline constexpr uint32_t kDefaultNodeTimeoutMs = 2500;

[[nodiscard]] uint32_t compute_crc32(std::span<const uint8_t> data) noexcept;
[[nodiscard]] bool verify_crc32(std::span<const uint8_t> data, uint32_t expected_crc32) noexcept;

[[nodiscard]] PacketEnvelope make_envelope(
	PacketType type,
	uint32_t session_identifier,
	uint32_t packet_sequence,
	uint64_t frame_index,
	std::span<const uint8_t> payload = {}
) noexcept;

[[nodiscard]] bool validate_envelope_structure(const PacketEnvelope& envelope, size_t received_buffer_size) noexcept;

}
