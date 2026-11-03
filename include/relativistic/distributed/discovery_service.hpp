#pragma once

#include "relativistic/distributed/cluster_node.hpp"
#include "relativistic/distributed/packet_protocol.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Relativistic::Distributed {

inline constexpr uint32_t kDiscoveryMagicIdentifier = 0x52444953;
inline constexpr uint16_t kDiscoveryProtocolVersion = 1;
inline constexpr size_t kDiscoveryClusterTagBytes = 8;
inline constexpr size_t kMaximumQueuedDiscoveryEvents = 1024;

enum class DiscoveryMessageKind : uint8_t {
	Announce = 1,
	Reply = 2,
	Goodbye = 3
};

enum class DiscoveryEventType : uint8_t {
	PeerDiscovered = 0,
	PeerUpdated = 1,
	PeerLost = 2
};

#pragma pack(push, 1)

struct DiscoveryBeacon {
	uint32_t magic_identifier{kDiscoveryMagicIdentifier};
	uint16_t protocol_version{kDiscoveryProtocolVersion};
	uint8_t kind{static_cast<uint8_t>(DiscoveryMessageKind::Announce)};
	uint8_t role{static_cast<uint8_t>(ClusterNodeRole::Master)};
	uint64_t instance_identifier{0};
	std::array<uint8_t, kDiscoveryClusterTagBytes> cluster_tag{};
	uint32_t abi_signature{0};
	uint16_t service_port{0};
	uint16_t reserved{0};
	float performance_weight{1.0f};
	uint32_t payload_crc32{0};
	std::array<char, kMaximumNodeNameBytes> node_name{};
};

#pragma pack(pop)

static_assert(sizeof(DiscoveryBeacon) == 72);
static_assert(DistributedPayload<DiscoveryBeacon>);

struct DiscoveryConfiguration {
	std::string multicast_group{std::string(kDefaultMulticastGroup)};
	uint16_t port{kDefaultDiscoveryPort};
	std::string interface_address{};
	uint32_t announce_interval_ms{1000};
	uint32_t peer_expiry_ms{5000};
	uint8_t time_to_live{1};
	bool multicast_loopback{true};
	ClusterNodeRole role{ClusterNodeRole::Master};
	uint16_t service_port{kDefaultClusterPort};
	std::string node_name{};
	std::string cluster_name{};
	std::array<uint8_t, 32> cluster_token{};
	uint32_t abi_signature{0};
	float performance_weight{1.0f};
};

struct DiscoveredPeer {
	uint64_t instance_identifier{0};
	std::string address{};
	uint16_t service_port{0};
	ClusterNodeRole role{ClusterNodeRole::Worker};
	std::string name{};
	uint32_t abi_signature{0};
	float performance_weight{1.0f};
	ClusterTimePoint last_seen{};
};

struct DiscoveryEvent {
	DiscoveryEventType type{DiscoveryEventType::PeerDiscovered};
	DiscoveredPeer peer{};
};

[[nodiscard]] constexpr bool roles_are_complementary(ClusterNodeRole local, ClusterNodeRole remote) noexcept {
	switch (local) {
		case ClusterNodeRole::Master:
			return remote == ClusterNodeRole::Worker || remote == ClusterNodeRole::Hybrid;
		case ClusterNodeRole::Worker:
			return remote == ClusterNodeRole::Master || remote == ClusterNodeRole::Hybrid;
		case ClusterNodeRole::Hybrid:
			return remote == ClusterNodeRole::Master || remote == ClusterNodeRole::Worker || remote == ClusterNodeRole::Hybrid;
		case ClusterNodeRole::Standalone:
			return false;
	}
	return false;
}

[[nodiscard]] std::array<uint8_t, kDiscoveryClusterTagBytes> derive_discovery_cluster_tag(
	std::string_view cluster_name,
	std::span<const uint8_t, 32> cluster_token
) noexcept;

[[nodiscard]] DiscoveryBeacon make_discovery_beacon(
	const DiscoveryConfiguration& configuration,
	uint64_t instance_identifier,
	DiscoveryMessageKind kind,
	const std::array<uint8_t, kDiscoveryClusterTagBytes>& cluster_tag
) noexcept;

[[nodiscard]] std::optional<DiscoveryBeacon> parse_discovery_beacon(std::span<const uint8_t> bytes) noexcept;

[[nodiscard]] std::string validate_discovery_configuration(const DiscoveryConfiguration& configuration);

class DiscoveryService {
private:
	struct Impl;
	std::unique_ptr<Impl> impl_;

public:
	DiscoveryService();
	~DiscoveryService();

	DiscoveryService(const DiscoveryService&) = delete;
	DiscoveryService& operator=(const DiscoveryService&) = delete;
	DiscoveryService(DiscoveryService&&) = delete;
	DiscoveryService& operator=(DiscoveryService&&) = delete;

	[[nodiscard]] bool start(const DiscoveryConfiguration& configuration);
	void stop() noexcept;
	[[nodiscard]] bool is_running() const noexcept;
	[[nodiscard]] std::string last_error() const;
	[[nodiscard]] uint64_t instance_identifier() const noexcept;
	void announce_now() noexcept;
	[[nodiscard]] std::vector<DiscoveredPeer> peers() const;
	size_t drain_events(std::vector<DiscoveryEvent>& events);
};

}
