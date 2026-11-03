#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "relativistic/distributed/network_transport.hpp"
#include <enet/enet.h>
#include "relativistic/core/engine_log.hpp"
#include <algorithm>
#include <cstring>
#include <mutex>
#include <string>

namespace Relativistic::Distributed {

namespace {

std::mutex g_library_mutex;
size_t g_library_users = 0;

constexpr uint32_t kMaximumWaitSliceMs = 100;
constexpr size_t kProtocolPeerCeiling = static_cast<size_t>(ENET_PROTOCOL_MAXIMUM_PEER_ID);

[[nodiscard]] bool acquire_library() noexcept {
	std::lock_guard<std::mutex> lock(g_library_mutex);
	if (g_library_users == 0U && enet_initialize() != 0) {
		return false;
	}
	++g_library_users;
	return true;
}

void release_library() noexcept {
	std::lock_guard<std::mutex> lock(g_library_mutex);
	if (g_library_users > 0U && --g_library_users == 0U) {
		enet_deinitialize();
	}
}

[[nodiscard]] constexpr enet_uint32 packet_flags(TransportChannel channel) noexcept {
	return (channel == TransportChannel::Control)
		? static_cast<enet_uint32>(ENET_PACKET_FLAG_RELIABLE)
		: static_cast<enet_uint32>(ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
}

[[nodiscard]] constexpr TransportChannel channel_from_index(enet_uint8 index) noexcept {
	return (index == static_cast<enet_uint8>(TransportChannel::TileData)) ? TransportChannel::TileData : TransportChannel::Control;
}

[[nodiscard]] bool resolve_address(const TransportEndpoint& endpoint, ENetAddress& address) noexcept {
	if (endpoint.host.empty() || endpoint.host == "0.0.0.0") {
		address.host = ENET_HOST_ANY;
	} else if (enet_address_set_host(&address, endpoint.host.c_str()) != 0) {
		return false;
	}
	address.port = endpoint.port;
	return true;
}

}

struct NetworkTransport::Impl {
	mutable std::mutex mutex;
	ENetHost* host{nullptr};
	TransportConfiguration configuration{};

	[[nodiscard]] ENetPeer* connected_peer(uint32_t peer_id) const noexcept {
		if (host == nullptr || peer_id >= host->peerCount) {
			return nullptr;
		}
		ENetPeer* peer = &host->peers[peer_id];
		return (peer->state == ENET_PEER_STATE_CONNECTED) ? peer : nullptr;
	}

	void configure_peer(ENetPeer* peer) const noexcept {
		enet_peer_ping_interval(peer, configuration.ping_interval_ms);
		enet_peer_timeout(peer, configuration.timeout_limit, configuration.timeout_minimum_ms, configuration.timeout_maximum_ms);
	}

	[[nodiscard]] bool service(TransportEvent& event, std::span<uint8_t> receive_buffer) noexcept {
		std::lock_guard<std::mutex> lock(mutex);
		if (host == nullptr) {
			return false;
		}
		ENetEvent raw{};
		if (enet_host_service(host, &raw, 0) <= 0) {
			return false;
		}
		switch (raw.type) {
			case ENET_EVENT_TYPE_CONNECT:
				configure_peer(raw.peer);
				event.type = TransportEventType::Connected;
				event.peer_id = static_cast<uint32_t>(raw.peer - host->peers);
				return true;
			case ENET_EVENT_TYPE_DISCONNECT:
				event.type = TransportEventType::Disconnected;
				event.peer_id = static_cast<uint32_t>(raw.peer - host->peers);
				event.disconnect_reason = raw.data;
				raw.peer->data = nullptr;
				return true;
			case ENET_EVENT_TYPE_RECEIVE: {
				event.type = TransportEventType::Received;
				event.peer_id = static_cast<uint32_t>(raw.peer - host->peers);
				event.channel = channel_from_index(raw.channelID);
				const size_t length = raw.packet->dataLength;
				event.payload_bytes = length;
				if (length > receive_buffer.size()) {
					event.truncated = true;
				} else if (length > 0U) {
					std::memcpy(receive_buffer.data(), raw.packet->data, length);
				}
				enet_packet_destroy(raw.packet);
				return true;
			}
			case ENET_EVENT_TYPE_NONE:
			default:
				return false;
		}
	}
};

NetworkTransport::NetworkTransport() : impl_(std::make_unique<Impl>()) {}

NetworkTransport::~NetworkTransport() {
	if (impl_ != nullptr) {
		close();
	}
}

NetworkTransport::NetworkTransport(NetworkTransport&&) noexcept = default;

NetworkTransport& NetworkTransport::operator=(NetworkTransport&& other) noexcept {
	if (this != &other) {
		if (impl_ != nullptr) {
			close();
		}
		impl_ = std::move(other.impl_);
	}
	return *this;
}

bool NetworkTransport::open(const std::optional<TransportEndpoint>& listen_endpoint, const TransportConfiguration& configuration) {
	std::lock_guard<std::mutex> lock(impl_->mutex);
	if (impl_->host != nullptr) {
		return false;
	}
	if (!acquire_library()) {
		Core::log_error("Network transport initialization failed: the networking library could not be started.");
		return false;
	}

	ENetAddress address{};
	const ENetAddress* bind_address = nullptr;
	if (listen_endpoint.has_value()) {
		if (!resolve_address(*listen_endpoint, address)) {
			release_library();
			Core::log_error("Network transport initialization failed: the listen address '" + listen_endpoint->host + "' could not be resolved.");
			return false;
		}
		bind_address = &address;
	}

	const size_t peer_capacity = std::clamp(configuration.maximum_peers, size_t{1}, kProtocolPeerCeiling);
	impl_->host = enet_host_create(
		bind_address,
		peer_capacity,
		kTransportChannelCount,
		configuration.incoming_bandwidth_bytes_per_second,
		configuration.outgoing_bandwidth_bytes_per_second
	);
	if (impl_->host == nullptr) {
		release_library();
		Core::log_error("Network transport initialization failed: the network host could not be created.");
		return false;
	}
	impl_->configuration = configuration;
	return true;
}

void NetworkTransport::close() noexcept {
	std::lock_guard<std::mutex> lock(impl_->mutex);
	if (impl_->host == nullptr) {
		return;
	}
	for (size_t index = 0; index < impl_->host->peerCount; ++index) {
		ENetPeer* peer = &impl_->host->peers[index];
		if (peer->state != ENET_PEER_STATE_DISCONNECTED) {
			enet_peer_disconnect_now(peer, 0);
		}
	}
	enet_host_flush(impl_->host);
	enet_host_destroy(impl_->host);
	impl_->host = nullptr;
	release_library();
}

bool NetworkTransport::is_open() const noexcept {
	std::lock_guard<std::mutex> lock(impl_->mutex);
	return impl_->host != nullptr;
}

std::optional<uint32_t> NetworkTransport::connect(const TransportEndpoint& endpoint) {
	std::lock_guard<std::mutex> lock(impl_->mutex);
	if (impl_->host == nullptr) {
		return std::nullopt;
	}
	ENetAddress address{};
	if (endpoint.host.empty() || enet_address_set_host(&address, endpoint.host.c_str()) != 0) {
		return std::nullopt;
	}
	address.port = endpoint.port;
	ENetPeer* peer = enet_host_connect(impl_->host, &address, kTransportChannelCount, 0);
	if (peer == nullptr) {
		return std::nullopt;
	}
	impl_->configure_peer(peer);
	return static_cast<uint32_t>(peer - impl_->host->peers);
}

void NetworkTransport::disconnect(uint32_t peer_id, uint32_t reason, bool graceful) noexcept {
	std::lock_guard<std::mutex> lock(impl_->mutex);
	if (impl_->host == nullptr || peer_id >= impl_->host->peerCount) {
		return;
	}
	ENetPeer* peer = &impl_->host->peers[peer_id];
	if (peer->state == ENET_PEER_STATE_DISCONNECTED) {
		return;
	}
	if (graceful) {
		enet_peer_disconnect(peer, reason);
	} else {
		enet_peer_disconnect_now(peer, reason);
	}
}

bool NetworkTransport::send(uint32_t peer_id, TransportChannel channel, std::span<const uint8_t> payload) noexcept {
	return send_gather(peer_id, channel, payload, {});
}

bool NetworkTransport::send_gather(uint32_t peer_id, TransportChannel channel, std::span<const uint8_t> head, std::span<const uint8_t> body) noexcept {
	const size_t total = head.size() + body.size();
	if (total == 0U || total > kMaximumTransportPacketBytes) {
		return false;
	}
	std::lock_guard<std::mutex> lock(impl_->mutex);
	ENetPeer* peer = impl_->connected_peer(peer_id);
	if (peer == nullptr) {
		return false;
	}
	ENetPacket* packet = enet_packet_create(nullptr, total, packet_flags(channel));
	if (packet == nullptr) {
		return false;
	}
	if (!head.empty()) {
		std::memcpy(packet->data, head.data(), head.size());
	}
	if (!body.empty()) {
		std::memcpy(packet->data + head.size(), body.data(), body.size());
	}
	if (enet_peer_send(peer, static_cast<enet_uint8>(channel), packet) != 0) {
		enet_packet_destroy(packet);
		return false;
	}
	return true;
}

size_t NetworkTransport::broadcast_gather(TransportChannel channel, std::span<const uint8_t> head, std::span<const uint8_t> body) noexcept {
	const size_t total = head.size() + body.size();
	if (total == 0U || total > kMaximumTransportPacketBytes) {
		return 0;
	}
	std::lock_guard<std::mutex> lock(impl_->mutex);
	if (impl_->host == nullptr || impl_->host->connectedPeers == 0U) {
		return 0;
	}
	ENetPacket* packet = enet_packet_create(nullptr, total, packet_flags(channel));
	if (packet == nullptr) {
		return 0;
	}
	if (!head.empty()) {
		std::memcpy(packet->data, head.data(), head.size());
	}
	if (!body.empty()) {
		std::memcpy(packet->data + head.size(), body.data(), body.size());
	}
	const size_t recipients = impl_->host->connectedPeers;
	enet_host_broadcast(impl_->host, static_cast<enet_uint8>(channel), packet);
	return recipients;
}

bool NetworkTransport::poll(TransportEvent& event, std::span<uint8_t> receive_buffer, uint32_t timeout_ms) noexcept {
	event = TransportEvent{};
	if (impl_->service(event, receive_buffer)) {
		return true;
	}
	if (timeout_ms == 0U) {
		return false;
	}
	ENetSocket socket = ENET_SOCKET_NULL;
	{
		std::lock_guard<std::mutex> lock(impl_->mutex);
		if (impl_->host == nullptr) {
			return false;
		}
		socket = impl_->host->socket;
	}
	enet_uint32 condition = ENET_SOCKET_WAIT_RECEIVE | ENET_SOCKET_WAIT_INTERRUPT;
	enet_socket_wait(socket, &condition, std::min(timeout_ms, kMaximumWaitSliceMs));
	return impl_->service(event, receive_buffer);
}

void NetworkTransport::flush() noexcept {
	std::lock_guard<std::mutex> lock(impl_->mutex);
	if (impl_->host != nullptr) {
		enet_host_flush(impl_->host);
	}
}

PeerStatistics NetworkTransport::peer_statistics(uint32_t peer_id) const noexcept {
	std::lock_guard<std::mutex> lock(impl_->mutex);
	PeerStatistics statistics{};
	const ENetPeer* peer = impl_->connected_peer(peer_id);
	if (peer == nullptr) {
		return statistics;
	}
	statistics.valid = true;
	statistics.round_trip_time_ms = peer->roundTripTime;
	statistics.round_trip_time_variance_ms = peer->roundTripTimeVariance;
	statistics.packet_loss_fraction = static_cast<double>(peer->packetLoss) / static_cast<double>(ENET_PEER_PACKET_LOSS_SCALE);
	statistics.packets_sent = peer->packetsSent;
	statistics.packets_lost = peer->packetsLost;
	return statistics;
}

std::string NetworkTransport::peer_address(uint32_t peer_id) const {
	std::lock_guard<std::mutex> lock(impl_->mutex);
	const ENetPeer* peer = impl_->connected_peer(peer_id);
	if (peer == nullptr) {
		return {};
	}
	char buffer[64]{};
	if (enet_address_get_host_ip(&peer->address, buffer, sizeof(buffer)) != 0) {
		return {};
	}
	return std::string(buffer) + ":" + std::to_string(peer->address.port);
}

size_t NetworkTransport::connected_peer_count() const noexcept {
	std::lock_guard<std::mutex> lock(impl_->mutex);
	return (impl_->host != nullptr) ? impl_->host->connectedPeers : 0U;
}

}
