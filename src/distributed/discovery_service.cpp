#include "relativistic/distributed/discovery_service.hpp"
#include "relativistic/core/pcg64.hpp"
#include "relativistic/core/sha256.hpp"
#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstring>
#include <deque>
#include <map>
#include <mutex>
#include <thread>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace Relativistic::Distributed {

namespace {

#if defined(_WIN32)
using NativeSocket = SOCKET;
using SocketLength = int;
constexpr NativeSocket kInvalidNativeSocket = INVALID_SOCKET;
#else
using NativeSocket = int;
using SocketLength = socklen_t;
constexpr NativeSocket kInvalidNativeSocket = -1;
#endif

constexpr size_t kReceiveBufferBytes = 512;
constexpr size_t kMaximumDatagramsPerPass = 64;
constexpr uint32_t kMinimumAnnounceIntervalMs = 100;
constexpr uint32_t kMaximumAnnounceIntervalMs = 60000;
constexpr int kPollSliceMs = 50;

std::mutex g_socket_library_mutex;
size_t g_socket_library_users = 0;

[[nodiscard]] bool acquire_socket_library() noexcept {
	std::lock_guard<std::mutex> lock(g_socket_library_mutex);
#if defined(_WIN32)
	if (g_socket_library_users == 0U) {
		WSADATA data{};
		if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
			return false;
		}
	}
#endif
	++g_socket_library_users;
	return true;
}

void release_socket_library() noexcept {
	std::lock_guard<std::mutex> lock(g_socket_library_mutex);
	if (g_socket_library_users > 0U && --g_socket_library_users == 0U) {
#if defined(_WIN32)
		WSACleanup();
#endif
	}
}

void close_native_socket(NativeSocket socket) noexcept {
#if defined(_WIN32)
	closesocket(socket);
#else
	close(socket);
#endif
}

[[nodiscard]] bool make_nonblocking(NativeSocket socket) noexcept {
#if defined(_WIN32)
	u_long enabled = 1;
	return ioctlsocket(socket, FIONBIO, &enabled) == 0;
#else
	const int flags = fcntl(socket, F_GETFL, 0);
	return flags >= 0 && fcntl(socket, F_SETFL, flags | O_NONBLOCK) == 0;
#endif
}

[[nodiscard]] std::string describe_socket_error() {
#if defined(_WIN32)
	return "winsock error " + std::to_string(WSAGetLastError());
#else
	return std::strerror(errno);
#endif
}

[[nodiscard]] bool wait_for_datagram(NativeSocket socket, int timeout_ms) noexcept {
#if defined(_WIN32)
	WSAPOLLFD descriptor{};
	descriptor.fd = socket;
	descriptor.events = POLLRDNORM;
	return WSAPoll(&descriptor, 1, timeout_ms) > 0 && (descriptor.revents & POLLRDNORM) != 0;
#else
	pollfd descriptor{};
	descriptor.fd = socket;
	descriptor.events = POLLIN;
	return poll(&descriptor, 1, timeout_ms) > 0 && (descriptor.revents & POLLIN) != 0;
#endif
}

template <typename Value>
[[nodiscard]] bool set_socket_option(NativeSocket socket, int level, int name, const Value& value) noexcept {
	return setsockopt(socket, level, name, reinterpret_cast<const char*>(&value), static_cast<SocketLength>(sizeof(Value))) == 0;
}

[[nodiscard]] bool parse_ipv4(const std::string& text, in_addr& address) noexcept {
	return inet_pton(AF_INET, text.c_str(), &address) == 1;
}

[[nodiscard]] bool is_multicast_address(const in_addr& address) noexcept {
	const uint32_t host_order = ntohl(address.s_addr);
	return (host_order >> 28U) == 0xEU;
}

[[nodiscard]] std::string format_address(const sockaddr_in& address) {
	char buffer[INET_ADDRSTRLEN]{};
	if (inet_ntop(AF_INET, &address.sin_addr, buffer, sizeof(buffer)) == nullptr) {
		return {};
	}
	return std::string(buffer);
}

[[nodiscard]] uint64_t generate_instance_identifier() noexcept {
	const auto ticks = static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
	const auto wall = static_cast<uint64_t>(std::chrono::system_clock::now().time_since_epoch().count());
	const auto thread_hash = static_cast<uint64_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
	Core::PCG64Engine engine(ticks ^ (wall * 0x9E3779B97F4A7C15ULL), thread_hash | 1ULL);
	uint64_t value = engine();
	return (value != 0ULL) ? value : 1ULL;
}

[[nodiscard]] uint32_t beacon_checksum(const DiscoveryBeacon& beacon) noexcept {
	DiscoveryBeacon copy = beacon;
	copy.payload_crc32 = 0;
	std::array<uint8_t, sizeof(DiscoveryBeacon)> bytes{};
	std::memcpy(bytes.data(), &copy, sizeof(copy));
	return compute_crc32(bytes);
}

}

std::array<uint8_t, kDiscoveryClusterTagBytes> derive_discovery_cluster_tag(
	std::string_view cluster_name,
	std::span<const uint8_t, 32> cluster_token
) noexcept {
	static constexpr std::string_view kDomain = "RELATIVISTIC-DISCOVERY-V1";
	Core::SHA256 hasher;
	hasher.update(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(kDomain.data()), kDomain.size()));
	const uint8_t separator = 0;
	hasher.update(std::span<const uint8_t>(&separator, 1));
	hasher.update(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(cluster_name.data()), cluster_name.size()));
	hasher.update(std::span<const uint8_t>(&separator, 1));
	hasher.update(std::span<const uint8_t>(cluster_token.data(), cluster_token.size()));
	const auto digest = hasher.finalize();
	std::array<uint8_t, kDiscoveryClusterTagBytes> tag{};
	std::copy_n(digest.begin(), tag.size(), tag.begin());
	return tag;
}

DiscoveryBeacon make_discovery_beacon(
	const DiscoveryConfiguration& configuration,
	uint64_t instance_identifier,
	DiscoveryMessageKind kind,
	const std::array<uint8_t, kDiscoveryClusterTagBytes>& cluster_tag
) noexcept {
	DiscoveryBeacon beacon{};
	beacon.kind = static_cast<uint8_t>(kind);
	beacon.role = static_cast<uint8_t>(configuration.role);
	beacon.instance_identifier = instance_identifier;
	beacon.cluster_tag = cluster_tag;
	beacon.abi_signature = configuration.abi_signature;
	beacon.service_port = configuration.service_port;
	beacon.performance_weight = configuration.performance_weight;
	const size_t name_length = std::min(configuration.node_name.size(), kMaximumNodeNameBytes - 1U);
	std::memcpy(beacon.node_name.data(), configuration.node_name.data(), name_length);
	beacon.payload_crc32 = beacon_checksum(beacon);
	return beacon;
}

std::optional<DiscoveryBeacon> parse_discovery_beacon(std::span<const uint8_t> bytes) noexcept {
	if (bytes.size() != sizeof(DiscoveryBeacon)) {
		return std::nullopt;
	}
	DiscoveryBeacon beacon{};
	std::memcpy(&beacon, bytes.data(), sizeof(beacon));
	if (beacon.magic_identifier != kDiscoveryMagicIdentifier || beacon.protocol_version != kDiscoveryProtocolVersion) {
		return std::nullopt;
	}
	if (beacon.payload_crc32 != beacon_checksum(beacon)) {
		return std::nullopt;
	}
	if (beacon.kind < static_cast<uint8_t>(DiscoveryMessageKind::Announce) || beacon.kind > static_cast<uint8_t>(DiscoveryMessageKind::Goodbye)) {
		return std::nullopt;
	}
	if (beacon.role < static_cast<uint8_t>(ClusterNodeRole::Master) || beacon.role > static_cast<uint8_t>(ClusterNodeRole::Hybrid)) {
		return std::nullopt;
	}
	if (beacon.instance_identifier == 0ULL || beacon.service_port == 0U) {
		return std::nullopt;
	}
	const float weight = beacon.performance_weight;
	if (!std::isfinite(weight) || !(weight > 0.0f)) {
		return std::nullopt;
	}
	beacon.node_name[beacon.node_name.size() - 1U] = '\0';
	return beacon;
}

std::string validate_discovery_configuration(const DiscoveryConfiguration& configuration) {
	if (configuration.role == ClusterNodeRole::Standalone) {
		return "Standalone nodes do not participate in discovery";
	}
	in_addr group{};
	if (!parse_ipv4(configuration.multicast_group, group) || !is_multicast_address(group)) {
		return "The discovery group must be an IPv4 multicast address";
	}
	if (!configuration.interface_address.empty()) {
		in_addr interface_address{};
		if (!parse_ipv4(configuration.interface_address, interface_address)) {
			return "The discovery interface must be an IPv4 address";
		}
	}
	if (configuration.port == 0U || configuration.service_port == 0U) {
		return "Discovery and service ports must be non-zero";
	}
	if (configuration.announce_interval_ms < kMinimumAnnounceIntervalMs || configuration.announce_interval_ms > kMaximumAnnounceIntervalMs) {
		return "The announce interval is outside the supported range";
	}
	if (configuration.peer_expiry_ms < 2U * configuration.announce_interval_ms) {
		return "The peer expiry must cover at least two announce intervals";
	}
	if (!std::isfinite(configuration.performance_weight) || !(configuration.performance_weight > 0.0f)) {
		return "The advertised performance weight must be positive";
	}
	return {};
}

struct DiscoveryService::Impl {
	struct PeerRecord {
		DiscoveredPeer peer{};
		ClusterTimePoint last_reply{};
	};

	DiscoveryConfiguration configuration{};
	std::array<uint8_t, kDiscoveryClusterTagBytes> cluster_tag{};
	uint64_t instance_identifier{0};
	NativeSocket socket{kInvalidNativeSocket};
	sockaddr_in group_address{};
	mutable std::mutex mutex{};
	std::map<uint64_t, PeerRecord> peers{};
	std::deque<DiscoveryEvent> events{};
	std::string error{};
	std::atomic<bool> announce_requested{false};
	std::atomic<bool> running{false};
	std::atomic<uint64_t> published_identifier{0};
	std::jthread worker{};

	void push_event_unlocked(DiscoveryEventType type, const DiscoveredPeer& peer) {
		if (events.size() >= kMaximumQueuedDiscoveryEvents) {
			events.pop_front();
		}
		events.push_back(DiscoveryEvent{type, peer});
	}

	[[nodiscard]] bool open_socket(std::string& failure) {
		socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (socket == kInvalidNativeSocket) {
			failure = "Socket creation failed: " + describe_socket_error();
			return false;
		}
		const int enabled = 1;
		if (!set_socket_option(socket, SOL_SOCKET, SO_REUSEADDR, enabled)) {
			failure = "SO_REUSEADDR failed: " + describe_socket_error();
			return false;
		}
#if defined(SO_REUSEPORT) && !defined(_WIN32)
		static_cast<void>(set_socket_option(socket, SOL_SOCKET, SO_REUSEPORT, enabled));
#endif
		sockaddr_in bind_address{};
		bind_address.sin_family = AF_INET;
		bind_address.sin_addr.s_addr = htonl(INADDR_ANY);
		bind_address.sin_port = htons(configuration.port);
		if (bind(socket, reinterpret_cast<const sockaddr*>(&bind_address), static_cast<SocketLength>(sizeof(bind_address))) != 0) {
			failure = "Bind failed: " + describe_socket_error();
			return false;
		}

		in_addr group{};
		in_addr interface_address{};
		interface_address.s_addr = htonl(INADDR_ANY);
		if (!parse_ipv4(configuration.multicast_group, group)) {
			failure = "Invalid multicast group";
			return false;
		}
		if (!configuration.interface_address.empty() && !parse_ipv4(configuration.interface_address, interface_address)) {
			failure = "Invalid interface address";
			return false;
		}

		ip_mreq membership{};
		membership.imr_multiaddr = group;
		membership.imr_interface = interface_address;
		if (!set_socket_option(socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, membership)) {
			failure = "Multicast membership failed: " + describe_socket_error();
			return false;
		}
#if defined(_WIN32)
		const DWORD ttl = configuration.time_to_live;
		const DWORD loopback = configuration.multicast_loopback ? 1U : 0U;
#else
		const unsigned char ttl = configuration.time_to_live;
		const unsigned char loopback = configuration.multicast_loopback ? 1U : 0U;
#endif
		if (!set_socket_option(socket, IPPROTO_IP, IP_MULTICAST_TTL, ttl) || !set_socket_option(socket, IPPROTO_IP, IP_MULTICAST_LOOP, loopback)) {
			failure = "Multicast options failed: " + describe_socket_error();
			return false;
		}
		if (!configuration.interface_address.empty() && !set_socket_option(socket, IPPROTO_IP, IP_MULTICAST_IF, interface_address)) {
			failure = "Multicast interface selection failed: " + describe_socket_error();
			return false;
		}
		if (!make_nonblocking(socket)) {
			failure = "Non-blocking mode failed: " + describe_socket_error();
			return false;
		}

		group_address = sockaddr_in{};
		group_address.sin_family = AF_INET;
		group_address.sin_addr = group;
		group_address.sin_port = htons(configuration.port);
		return true;
	}

	void close_socket_if_open() noexcept {
		if (socket != kInvalidNativeSocket) {
			close_native_socket(socket);
			socket = kInvalidNativeSocket;
		}
	}

	void send_beacon(DiscoveryMessageKind kind, const sockaddr_in& destination) noexcept {
		if (socket == kInvalidNativeSocket) {
			return;
		}
		const DiscoveryBeacon beacon = make_discovery_beacon(configuration, instance_identifier, kind, cluster_tag);
		std::array<uint8_t, sizeof(DiscoveryBeacon)> bytes{};
		std::memcpy(bytes.data(), &beacon, sizeof(beacon));
		static_cast<void>(sendto(socket, reinterpret_cast<const char*>(bytes.data()), static_cast<int>(bytes.size()), 0, reinterpret_cast<const sockaddr*>(&destination), static_cast<SocketLength>(sizeof(destination))));
	}

	void handle_datagram(std::span<const uint8_t> bytes, const sockaddr_in& from, ClusterTimePoint now) {
		const std::optional<DiscoveryBeacon> parsed = parse_discovery_beacon(bytes);
		if (!parsed.has_value()) {
			return;
		}
		const DiscoveryBeacon& beacon = *parsed;
		if (beacon.instance_identifier == instance_identifier) {
			return;
		}
		if (!std::equal(cluster_tag.begin(), cluster_tag.end(), beacon.cluster_tag.begin())) {
			return;
		}
		const auto remote_role = static_cast<ClusterNodeRole>(beacon.role);
		if (!roles_are_complementary(configuration.role, remote_role)) {
			return;
		}
		const auto kind = static_cast<DiscoveryMessageKind>(beacon.kind);

		bool reply_required = false;
		{
			std::lock_guard<std::mutex> lock(mutex);
			if (kind == DiscoveryMessageKind::Goodbye) {
				const auto it = peers.find(beacon.instance_identifier);
				if (it != peers.end()) {
					push_event_unlocked(DiscoveryEventType::PeerLost, it->second.peer);
					peers.erase(it);
				}
				return;
			}

			DiscoveredPeer candidate{};
			candidate.instance_identifier = beacon.instance_identifier;
			candidate.address = format_address(from);
			candidate.service_port = beacon.service_port;
			candidate.role = remote_role;
			candidate.name = std::string(beacon.node_name.data());
			candidate.abi_signature = beacon.abi_signature;
			candidate.performance_weight = beacon.performance_weight;
			candidate.last_seen = now;

			auto [it, inserted] = peers.try_emplace(beacon.instance_identifier);
			PeerRecord& record = it->second;
			const bool changed = inserted
				|| record.peer.address != candidate.address
				|| record.peer.service_port != candidate.service_port
				|| record.peer.role != candidate.role
				|| record.peer.abi_signature != candidate.abi_signature;
			record.peer = candidate;
			if (inserted) {
				push_event_unlocked(DiscoveryEventType::PeerDiscovered, record.peer);
			} else if (changed) {
				push_event_unlocked(DiscoveryEventType::PeerUpdated, record.peer);
			}
			if (kind == DiscoveryMessageKind::Announce) {
				const auto reply_gap = std::chrono::milliseconds(configuration.announce_interval_ms / 2U);
				if (inserted || now - record.last_reply >= reply_gap) {
					record.last_reply = now;
					reply_required = true;
				}
			}
		}

		if (reply_required) {
			sockaddr_in destination = from;
			destination.sin_port = htons(configuration.port);
			send_beacon(DiscoveryMessageKind::Reply, destination);
		}
	}

	void expire_peers(ClusterTimePoint now) {
		const auto expiry = std::chrono::milliseconds(configuration.peer_expiry_ms);
		std::lock_guard<std::mutex> lock(mutex);
		for (auto it = peers.begin(); it != peers.end();) {
			if (now - it->second.peer.last_seen > expiry) {
				push_event_unlocked(DiscoveryEventType::PeerLost, it->second.peer);
				it = peers.erase(it);
			} else {
				++it;
			}
		}
	}

	void receive_pending(ClusterTimePoint now) {
		std::array<uint8_t, kReceiveBufferBytes> buffer{};
		for (size_t pass = 0; pass < kMaximumDatagramsPerPass; ++pass) {
			sockaddr_in from{};
			SocketLength from_length = static_cast<SocketLength>(sizeof(from));
			const auto received = recvfrom(socket, reinterpret_cast<char*>(buffer.data()), static_cast<int>(buffer.size()), 0, reinterpret_cast<sockaddr*>(&from), &from_length);
			if (received <= 0) {
				break;
			}
			handle_datagram(std::span<const uint8_t>(buffer.data(), static_cast<size_t>(received)), from, now);
		}
	}

	void run(std::stop_token stop) {
		const auto interval = std::chrono::milliseconds(configuration.announce_interval_ms);
		auto next_announce = ClusterClock::now();
		while (!stop.stop_requested()) {
			const auto now = ClusterClock::now();
			if (now >= next_announce || announce_requested.exchange(false, std::memory_order_acq_rel)) {
				send_beacon(DiscoveryMessageKind::Announce, group_address);
				next_announce = now + interval;
			}
			expire_peers(now);
			const auto until_announce = std::chrono::duration_cast<std::chrono::milliseconds>(next_announce - now).count();
			const int wait_ms = static_cast<int>(std::clamp<long long>(static_cast<long long>(until_announce), 1LL, static_cast<long long>(kPollSliceMs)));
			if (wait_for_datagram(socket, wait_ms)) {
				receive_pending(ClusterClock::now());
			}
		}
		send_beacon(DiscoveryMessageKind::Goodbye, group_address);
	}
};

DiscoveryService::DiscoveryService() : impl_(std::make_unique<Impl>()) {}

DiscoveryService::~DiscoveryService() {
	stop();
}

bool DiscoveryService::start(const DiscoveryConfiguration& configuration) {
	if (impl_->running.load(std::memory_order_acquire)) {
		return false;
	}
	{
		std::lock_guard<std::mutex> lock(impl_->mutex);
		impl_->error.clear();
		impl_->peers.clear();
		impl_->events.clear();
	}
	std::string failure = validate_discovery_configuration(configuration);
	if (failure.empty()) {
		if (!acquire_socket_library()) {
			failure = "The socket library could not be initialized";
		}
	}
	if (!failure.empty()) {
		std::lock_guard<std::mutex> lock(impl_->mutex);
		impl_->error = failure;
		return false;
	}

	impl_->configuration = configuration;
	impl_->cluster_tag = derive_discovery_cluster_tag(configuration.cluster_name, std::span<const uint8_t, 32>(impl_->configuration.cluster_token));
	impl_->instance_identifier = generate_instance_identifier();
	impl_->published_identifier.store(impl_->instance_identifier, std::memory_order_release);

	if (!impl_->open_socket(failure)) {
		impl_->close_socket_if_open();
		release_socket_library();
		std::lock_guard<std::mutex> lock(impl_->mutex);
		impl_->error = failure;
		return false;
	}

	impl_->announce_requested.store(false, std::memory_order_release);
	impl_->running.store(true, std::memory_order_release);
	impl_->worker = std::jthread([impl = impl_.get()](std::stop_token stop) { impl->run(stop); });
	return true;
}

void DiscoveryService::stop() noexcept {
	if (!impl_->running.exchange(false, std::memory_order_acq_rel)) {
		return;
	}
	impl_->worker.request_stop();
	if (impl_->worker.joinable()) {
		impl_->worker.join();
	}
	impl_->close_socket_if_open();
	release_socket_library();
	impl_->published_identifier.store(0, std::memory_order_release);
	try {
		std::lock_guard<std::mutex> lock(impl_->mutex);
		impl_->peers.clear();
	} catch (...) {
	}
}

bool DiscoveryService::is_running() const noexcept {
	return impl_->running.load(std::memory_order_acquire);
}

std::string DiscoveryService::last_error() const {
	std::lock_guard<std::mutex> lock(impl_->mutex);
	return impl_->error;
}

uint64_t DiscoveryService::instance_identifier() const noexcept {
	return impl_->published_identifier.load(std::memory_order_acquire);
}

void DiscoveryService::announce_now() noexcept {
	impl_->announce_requested.store(true, std::memory_order_release);
}

std::vector<DiscoveredPeer> DiscoveryService::peers() const {
	std::lock_guard<std::mutex> lock(impl_->mutex);
	std::vector<DiscoveredPeer> result;
	result.reserve(impl_->peers.size());
	for (const auto& [identifier, record] : impl_->peers) {
		result.push_back(record.peer);
	}
	return result;
}

size_t DiscoveryService::drain_events(std::vector<DiscoveryEvent>& events) {
	std::lock_guard<std::mutex> lock(impl_->mutex);
	const size_t count = impl_->events.size();
	events.insert(events.end(), impl_->events.begin(), impl_->events.end());
	impl_->events.clear();
	return count;
}

}
