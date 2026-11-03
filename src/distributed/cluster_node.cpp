#include "relativistic/distributed/cluster_node.hpp"
#include "relativistic/core/engine_signature.hpp"
#include <algorithm>
#include <cstring>
#include <thread>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <unistd.h>
#endif

namespace Relativistic::Distributed {

namespace {

constexpr uint8_t kFeatureVulkanCompute = 1U << 0U;
constexpr uint8_t kFeatureFp64 = 1U << 1U;
constexpr uint8_t kFeatureNeon = 1U << 2U;
constexpr uint8_t kFeatureAvx2 = 1U << 3U;
constexpr uint8_t kFeatureAvx512 = 1U << 4U;

[[nodiscard]] uint64_t query_physical_memory_bytes() noexcept {
#if defined(_WIN32)
	MEMORYSTATUSEX status{};
	status.dwLength = sizeof(status);
	return (GlobalMemoryStatusEx(&status) != FALSE) ? static_cast<uint64_t>(status.ullTotalPhys) : 0ULL;
#elif defined(__linux__) || defined(__APPLE__)
	const long pages = sysconf(_SC_PHYS_PAGES);
	const long page_size = sysconf(_SC_PAGE_SIZE);
	return (pages > 0 && page_size > 0) ? static_cast<uint64_t>(pages) * static_cast<uint64_t>(page_size) : 0ULL;
#else
	return 0ULL;
#endif
}

[[nodiscard]] std::string extract_name(const std::array<char, kMaximumNodeNameBytes>& raw) {
	size_t length = 0;
	while (length < raw.size() && raw[length] != '\0') {
		++length;
	}
	return std::string(raw.data(), length);
}

}

uint32_t local_engine_abi_signature() {
	static const uint32_t signature = [] {
		const std::string digest = Core::EngineSignature::compute();
		uint32_t value = 0;
		for (size_t i = 0; i < 8U && i < digest.size(); ++i) {
			const char character = digest[i];
			const uint32_t nibble = (character >= 'a') ? static_cast<uint32_t>(character - 'a' + 10) : static_cast<uint32_t>(character - '0');
			value = (value << 4U) | (nibble & 0xFU);
		}
		return value;
	}();
	return signature;
}

double estimate_relative_performance_weight(const NodeHardwareCapabilities& capabilities) noexcept {
	const double simd_factor = capabilities.has_avx512 ? 2.0 : (capabilities.has_avx2 ? 1.5 : 1.0);
	const double cpu_score = static_cast<double>(std::max(capabilities.logical_cores, 1U)) * simd_factor;
	double gpu_score = 0.0;
	if (capabilities.has_vulkan_compute) {
		gpu_score = capabilities.has_fp64_support ? 48.0 : 6.0;
	}
	return std::clamp((cpu_score + gpu_score) / 8.0, 0.05, 4096.0);
}

NodeHardwareCapabilities detect_local_hardware_capabilities(const LocalGpuInfo& gpu) {
	NodeHardwareCapabilities capabilities{};
	capabilities.engine_abi_signature = local_engine_abi_signature();
	capabilities.logical_cores = std::max(1U, std::thread::hardware_concurrency());
	capabilities.physical_memory_bytes = query_physical_memory_bytes();
	capabilities.vulkan_api_version = gpu.vulkan_api_version;
	capabilities.vendor_id = gpu.vendor_id;
	capabilities.device_id = gpu.device_id;
	capabilities.has_vulkan_compute = gpu.has_vulkan_compute;
	capabilities.has_fp64_support = gpu.has_vulkan_compute && gpu.has_fp64_support;
#if defined(__ARM_NEON) || defined(__aarch64__) || defined(_M_ARM64)
	capabilities.has_neon = true;
#endif
#if defined(__AVX2__)
	capabilities.has_avx2 = true;
#endif
#if defined(__AVX512F__)
	capabilities.has_avx512 = true;
#endif
	capabilities.relative_performance_weight = static_cast<float>(estimate_relative_performance_weight(capabilities));
	return capabilities;
}

HandshakeVerdict evaluate_remote_capabilities(
	const NodeHardwareCapabilities& local,
	const NodeHardwareCapabilities& remote,
	bool require_gpu_fp64
) noexcept {
	if (remote.engine_abi_signature != local.engine_abi_signature) {
		return HandshakeVerdict::AbiMismatch;
	}
	if (remote.logical_cores == 0U || !std::isfinite(remote.relative_performance_weight) || !(remote.relative_performance_weight > 0.0f)) {
		return HandshakeVerdict::InvalidCapabilities;
	}
	if (require_gpu_fp64 && !(remote.has_vulkan_compute && remote.has_fp64_support)) {
		return HandshakeVerdict::InsufficientCapabilities;
	}
	return HandshakeVerdict::Accepted;
}

size_t encode_capabilities_advertisement(const NodeAdvertisement& advertisement, std::span<uint8_t> output) noexcept {
	if (output.size() < sizeof(CapabilitiesAdvertisementWire)) {
		return 0;
	}
	CapabilitiesAdvertisementWire wire{};
	const NodeHardwareCapabilities& hardware = advertisement.hardware;
	wire.engine_abi_signature = hardware.engine_abi_signature;
	wire.logical_cores = hardware.logical_cores;
	wire.physical_memory_bytes = hardware.physical_memory_bytes;
	wire.vulkan_api_version = hardware.vulkan_api_version;
	wire.vendor_id = hardware.vendor_id;
	wire.device_id = hardware.device_id;
	uint8_t flags = 0;
	flags = static_cast<uint8_t>(flags | (hardware.has_vulkan_compute ? kFeatureVulkanCompute : 0U));
	flags = static_cast<uint8_t>(flags | (hardware.has_fp64_support ? kFeatureFp64 : 0U));
	flags = static_cast<uint8_t>(flags | (hardware.has_neon ? kFeatureNeon : 0U));
	flags = static_cast<uint8_t>(flags | (hardware.has_avx2 ? kFeatureAvx2 : 0U));
	flags = static_cast<uint8_t>(flags | (hardware.has_avx512 ? kFeatureAvx512 : 0U));
	wire.feature_flags = flags;
	wire.relative_performance_weight = hardware.relative_performance_weight;
	const size_t name_length = std::min(advertisement.name.size(), kMaximumNodeNameBytes - 1U);
	std::memcpy(wire.node_name.data(), advertisement.name.data(), name_length);
	wire.role = static_cast<uint32_t>(advertisement.role);
	wire.listen_port = advertisement.listen_port;
	std::memcpy(output.data(), &wire, sizeof(wire));
	return sizeof(wire);
}

std::optional<NodeAdvertisement> decode_capabilities_advertisement(std::span<const uint8_t> input) {
	if (input.size() != sizeof(CapabilitiesAdvertisementWire)) {
		return std::nullopt;
	}
	CapabilitiesAdvertisementWire wire{};
	std::memcpy(&wire, input.data(), sizeof(wire));
	if (wire.role > static_cast<uint32_t>(ClusterNodeRole::Hybrid) || wire.logical_cores == 0U) {
		return std::nullopt;
	}
	const float weight = wire.relative_performance_weight;
	if (!std::isfinite(weight) || !(weight > 0.0f)) {
		return std::nullopt;
	}
	NodeAdvertisement advertisement{};
	NodeHardwareCapabilities& hardware = advertisement.hardware;
	hardware.engine_abi_signature = wire.engine_abi_signature;
	hardware.logical_cores = wire.logical_cores;
	hardware.physical_memory_bytes = wire.physical_memory_bytes;
	hardware.vulkan_api_version = wire.vulkan_api_version;
	hardware.vendor_id = wire.vendor_id;
	hardware.device_id = wire.device_id;
	hardware.has_vulkan_compute = (wire.feature_flags & kFeatureVulkanCompute) != 0U;
	hardware.has_fp64_support = (wire.feature_flags & kFeatureFp64) != 0U;
	hardware.has_neon = (wire.feature_flags & kFeatureNeon) != 0U;
	hardware.has_avx2 = (wire.feature_flags & kFeatureAvx2) != 0U;
	hardware.has_avx512 = (wire.feature_flags & kFeatureAvx512) != 0U;
	hardware.relative_performance_weight = weight;
	advertisement.role = static_cast<ClusterNodeRole>(wire.role);
	advertisement.listen_port = wire.listen_port;
	advertisement.name = extract_name(wire.node_name);
	return advertisement;
}

ClusterNode::ClusterNode(uint32_t identifier, ClusterNodeDescriptor descriptor, NodeState initial_state, ClusterTimePoint now)
	: identifier_(identifier),
	  descriptor_(std::move(descriptor)),
	  state_(initial_state),
	  state_since_(now),
	  last_heartbeat_(now) {
	if (!(descriptor_.static_weight > 0.0) || !std::isfinite(descriptor_.static_weight)) {
		descriptor_.static_weight = 1.0;
	}
}

bool ClusterNode::transition_to(NodeState next, ClusterTimePoint now) noexcept {
	if (!is_valid_node_transition(state_, next)) {
		return false;
	}
	state_ = next;
	state_since_ = now;
	return true;
}

void ClusterNode::on_transport_connected(uint32_t peer_identifier, ClusterTimePoint now) noexcept {
	peer_identifier_ = peer_identifier;
	last_heartbeat_ = now;
	if (state_ == NodeState::Disconnected) {
		static_cast<void>(transition_to(NodeState::Unauthenticated, now));
	}
}

void ClusterNode::on_transport_lost(ClusterTimePoint now) noexcept {
	peer_identifier_ = kInvalidPeerId;
	in_flight_tasks_ = 0;
	static_cast<void>(transition_to(NodeState::Disconnected, now));
}

bool ClusterNode::begin_handshake(ClusterTimePoint now) noexcept {
	return transition_to(NodeState::Handshaking, now);
}

HandshakeVerdict ClusterNode::accept_advertisement(
	const NodeAdvertisement& advertisement,
	const NodeHardwareCapabilities& local,
	bool require_gpu_fp64,
	ClusterTimePoint now
) {
	if (state_ != NodeState::Handshaking) {
		return HandshakeVerdict::InvalidState;
	}
	const HandshakeVerdict verdict = evaluate_remote_capabilities(local, advertisement.hardware, require_gpu_fp64);
	if (verdict != HandshakeVerdict::Accepted) {
		static_cast<void>(transition_to(NodeState::Disconnected, now));
		return verdict;
	}
	hardware_ = advertisement.hardware;
	role_ = advertisement.role;
	if (descriptor_.name.empty()) {
		descriptor_.name = advertisement.name;
	}
	throughput_.seed(kDefaultSecondsPerKiloRay / static_cast<double>(hardware_.relative_performance_weight));
	return HandshakeVerdict::Accepted;
}

bool ClusterNode::complete_synchronization(ClusterTimePoint now) noexcept {
	if (state_ != NodeState::Handshaking || !clock_.converged()) {
		return false;
	}
	return transition_to(NodeState::Synchronized, now);
}

bool ClusterNode::mark_ready(ClusterTimePoint now) noexcept {
	return transition_to(NodeState::Ready, now);
}

bool ClusterNode::request_resynchronization(ClusterTimePoint now) noexcept {
	if (state_ == NodeState::Ready) {
		return transition_to(NodeState::Synchronized, now);
	}
	if (state_ == NodeState::Synchronized) {
		return transition_to(NodeState::Handshaking, now);
	}
	return false;
}

void ClusterNode::record_heartbeat(ClusterTimePoint now) noexcept {
	last_heartbeat_ = now;
}

void ClusterNode::record_round_trip(double round_trip_seconds) noexcept {
	latency_.record(round_trip_seconds);
}

void ClusterNode::record_clock_sample(double origin, double receive, double transmit, double destination) noexcept {
	clock_.record(origin, receive, transmit, destination);
}

bool ClusterNode::enforce_timeout(ClusterTimePoint now, double timeout_seconds) noexcept {
	if (state_ == NodeState::Disconnected) {
		return false;
	}
	if (seconds_between(last_heartbeat_, now) <= timeout_seconds) {
		return false;
	}
	in_flight_tasks_ = 0;
	return transition_to(NodeState::Disconnected, now);
}

bool ClusterNode::register_dispatch(ClusterTimePoint now) noexcept {
	if (!node_accepts_work(state_)) {
		return false;
	}
	++in_flight_tasks_;
	if (state_ == NodeState::Ready) {
		static_cast<void>(transition_to(NodeState::Computing, now));
	}
	return true;
}

void ClusterNode::register_completion(uint32_t pixel_count, double execution_seconds, double round_trip_seconds, ClusterTimePoint now) noexcept {
	if (in_flight_tasks_ > 0U) {
		--in_flight_tasks_;
	}
	++tiles_completed_;
	throughput_.record(pixel_count, execution_seconds);
	const double network_seconds = round_trip_seconds - execution_seconds;
	if (network_seconds > 0.0) {
		latency_.record(network_seconds);
	}
	last_heartbeat_ = now;
	if (state_ == NodeState::Stalled) {
		static_cast<void>(transition_to((in_flight_tasks_ > 0U) ? NodeState::Computing : NodeState::Ready, now));
	} else if (state_ == NodeState::Computing && in_flight_tasks_ == 0U) {
		static_cast<void>(transition_to(NodeState::Ready, now));
	}
}

void ClusterNode::register_deadline_miss(ClusterTimePoint now) noexcept {
	++deadline_misses_;
	if (in_flight_tasks_ > 0U) {
		--in_flight_tasks_;
	}
	if (state_ == NodeState::Ready || state_ == NodeState::Computing) {
		static_cast<void>(transition_to(NodeState::Stalled, now));
	}
}

void ClusterNode::register_late_result() noexcept {
	++late_results_;
}

void ClusterNode::register_frame_served() noexcept {
	++frames_served_;
}

void ClusterNode::release_in_flight(ClusterTimePoint now) noexcept {
	in_flight_tasks_ = 0;
	if (state_ == NodeState::Computing) {
		static_cast<void>(transition_to(NodeState::Ready, now));
	}
}

NodeCapacityInput ClusterNode::capacity_input() const noexcept {
	NodeCapacityInput input{};
	input.node_id = identifier_;
	input.is_local = false;
	input.eligible = is_schedulable();
	input.static_weight = descriptor_.static_weight;
	input.seconds_per_kilo_ray = throughput_.seconds_per_kilo_ray();
	input.round_trip_seconds = latency_.mean_seconds();
	return input;
}

DispatchTiming ClusterNode::dispatch_timing() const noexcept {
	return DispatchTiming{latency_.deadline_margin_seconds(), throughput_.seconds_per_kilo_ray()};
}

NodeSnapshot ClusterNode::snapshot(ClusterTimePoint now) const {
	NodeSnapshot result{};
	result.node_id = identifier_;
	result.name = descriptor_.name;
	result.endpoint = descriptor_.endpoint;
	result.state = state_;
	result.role = role_;
	result.hardware = hardware_;
	result.peer_id = peer_identifier_;
	result.round_trip_mean_seconds = latency_.mean_seconds();
	result.round_trip_sigma_seconds = latency_.sigma_seconds();
	result.seconds_per_kilo_ray = throughput_.seconds_per_kilo_ray();
	result.clock_offset_seconds = clock_.offset_seconds();
	result.heartbeat_age_seconds = seconds_between(last_heartbeat_, now);
	result.state_age_seconds = seconds_between(state_since_, now);
	result.in_flight_tasks = in_flight_tasks_;
	result.tiles_completed = tiles_completed_;
	result.deadline_misses = deadline_misses_;
	result.late_results = late_results_;
	result.frames_served = frames_served_;
	return result;
}

}
