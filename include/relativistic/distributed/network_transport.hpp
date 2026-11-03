#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>

namespace Relativistic::Distributed {

enum class TransportChannel : uint8_t {
	Control = 0,
	TileData = 1
};

inline constexpr size_t kTransportChannelCount = 2;
inline constexpr uint32_t kInvalidPeerId = 0xFFFFFFFFU;
inline constexpr size_t kMaximumTransportPacketBytes = 256U * 1024U;

struct TransportEndpoint {
	std::string host{};
	uint16_t port{0};
};

struct TransportConfiguration {
	size_t maximum_peers{64};
	uint32_t incoming_bandwidth_bytes_per_second{0};
	uint32_t outgoing_bandwidth_bytes_per_second{0};
	uint32_t ping_interval_ms{500};
	uint32_t timeout_limit{32};
	uint32_t timeout_minimum_ms{2000};
	uint32_t timeout_maximum_ms{2500};
};

enum class TransportEventType : uint8_t {
	None = 0,
	Connected = 1,
	Disconnected = 2,
	Received = 3
};

struct TransportEvent {
	TransportEventType type{TransportEventType::None};
	uint32_t peer_id{kInvalidPeerId};
	TransportChannel channel{TransportChannel::Control};
	size_t payload_bytes{0};
	bool truncated{false};
	uint32_t disconnect_reason{0};
};

struct PeerStatistics {
	bool valid{false};
	uint32_t round_trip_time_ms{0};
	uint32_t round_trip_time_variance_ms{0};
	double packet_loss_fraction{0.0};
	uint32_t packets_sent{0};
	uint32_t packets_lost{0};
};

class NetworkTransport {
private:
	struct Impl;
	std::unique_ptr<Impl> impl_;

public:
	NetworkTransport();
	~NetworkTransport();

	NetworkTransport(const NetworkTransport&) = delete;
	NetworkTransport& operator=(const NetworkTransport&) = delete;
	NetworkTransport(NetworkTransport&&) noexcept;
	NetworkTransport& operator=(NetworkTransport&&) noexcept;

	[[nodiscard]] bool open(const std::optional<TransportEndpoint>& listen_endpoint, const TransportConfiguration& configuration);

	void close() noexcept;

	[[nodiscard]] bool is_open() const noexcept;

	[[nodiscard]] std::optional<uint32_t> connect(const TransportEndpoint& endpoint);

	void disconnect(uint32_t peer_id, uint32_t reason, bool graceful) noexcept;

	[[nodiscard]] bool send(uint32_t peer_id, TransportChannel channel, std::span<const uint8_t> payload) noexcept;

	[[nodiscard]] bool send_gather(uint32_t peer_id, TransportChannel channel, std::span<const uint8_t> head, std::span<const uint8_t> body) noexcept;

	[[nodiscard]] size_t broadcast_gather(TransportChannel channel, std::span<const uint8_t> head, std::span<const uint8_t> body) noexcept;

	[[nodiscard]] bool poll(TransportEvent& event, std::span<uint8_t> receive_buffer, uint32_t timeout_ms) noexcept;

	void flush() noexcept;

	[[nodiscard]] PeerStatistics peer_statistics(uint32_t peer_id) const noexcept;

	[[nodiscard]] std::string peer_address(uint32_t peer_id) const;

	[[nodiscard]] size_t connected_peer_count() const noexcept;
};

}
