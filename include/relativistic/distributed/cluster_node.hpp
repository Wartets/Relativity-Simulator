#pragma once

#include "relativistic/distributed/network_transport.hpp"
#include "relativistic/distributed/packet_protocol.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <type_traits>

namespace Relativistic::Distributed {

using ClusterClock = std::chrono::steady_clock;
using ClusterTimePoint = ClusterClock::time_point;

inline constexpr uint32_t kMaximumClusterNodes = 256;
inline constexpr size_t kMaximumNodeNameBytes = 32;
inline constexpr double kDeadlineSigmaMultiplier = 3.5;
inline constexpr double kDefaultSecondsPerKiloRay = 1.0e-3;
inline constexpr double kLatencyReferenceSeconds = 1.0e-3;

[[nodiscard]] inline double seconds_between(ClusterTimePoint from, ClusterTimePoint to) noexcept {
	return std::chrono::duration<double>(to - from).count();
}

[[nodiscard]] constexpr bool is_valid_node_transition(NodeState from, NodeState to) noexcept {
	if (from == to) {
		return false;
	}
	if (to == NodeState::Disconnected) {
		return true;
	}
	switch (from) {
		case NodeState::Unauthenticated:
			return to == NodeState::Handshaking;
		case NodeState::Handshaking:
			return to == NodeState::Synchronized || to == NodeState::Unauthenticated;
		case NodeState::Synchronized:
			return to == NodeState::Ready || to == NodeState::Handshaking;
		case NodeState::Ready:
			return to == NodeState::Computing || to == NodeState::Stalled || to == NodeState::Synchronized;
		case NodeState::Computing:
			return to == NodeState::Ready || to == NodeState::Stalled;
		case NodeState::Stalled:
			return to == NodeState::Ready || to == NodeState::Computing;
		case NodeState::Disconnected:
			return to == NodeState::Unauthenticated;
	}
	return false;
}

[[nodiscard]] constexpr bool node_accepts_work(NodeState state) noexcept {
	return state == NodeState::Ready || state == NodeState::Computing;
}

class LatencyEstimator {
private:
	static constexpr double kGain = 0.125;

	double mean_seconds_{0.005};
	double variance_seconds_{1.0e-4};
	uint32_t sample_count_{0};

public:
	void record(double round_trip_seconds) noexcept {
		if (!std::isfinite(round_trip_seconds) || round_trip_seconds <= 0.0) {
			return;
		}
		if (sample_count_ == 0U) {
			mean_seconds_ = round_trip_seconds;
			variance_seconds_ = 0.0625 * round_trip_seconds * round_trip_seconds;
		} else {
			const double delta = round_trip_seconds - mean_seconds_;
			mean_seconds_ += kGain * delta;
			variance_seconds_ = (1.0 - kGain) * (variance_seconds_ + kGain * delta * delta);
		}
		if (sample_count_ < std::numeric_limits<uint32_t>::max()) {
			++sample_count_;
		}
	}

	[[nodiscard]] double mean_seconds() const noexcept { return mean_seconds_; }
	[[nodiscard]] double sigma_seconds() const noexcept { return std::sqrt(std::max(variance_seconds_, 0.0)); }
	[[nodiscard]] uint32_t sample_count() const noexcept { return sample_count_; }
	[[nodiscard]] double deadline_margin_seconds() const noexcept {
		return mean_seconds_ + kDeadlineSigmaMultiplier * sigma_seconds();
	}
};

class ThroughputEstimator {
private:
	static constexpr double kGain = 0.2;

	double seconds_per_kilo_ray_{kDefaultSecondsPerKiloRay};
	uint32_t sample_count_{0};

public:
	void seed(double seconds_per_kilo_ray) noexcept {
		if (sample_count_ == 0U && std::isfinite(seconds_per_kilo_ray) && seconds_per_kilo_ray > 0.0) {
			seconds_per_kilo_ray_ = seconds_per_kilo_ray;
		}
	}

	void record(uint32_t ray_count, double execution_seconds) noexcept {
		if (ray_count == 0U || !std::isfinite(execution_seconds) || execution_seconds <= 0.0) {
			return;
		}
		const double sample = execution_seconds / (static_cast<double>(ray_count) * 1.0e-3);
		if (sample_count_ == 0U) {
			seconds_per_kilo_ray_ = sample;
		} else {
			seconds_per_kilo_ray_ += kGain * (sample - seconds_per_kilo_ray_);
		}
		if (sample_count_ < std::numeric_limits<uint32_t>::max()) {
			++sample_count_;
		}
	}

	[[nodiscard]] double seconds_per_kilo_ray() const noexcept { return seconds_per_kilo_ray_; }
	[[nodiscard]] double rays_per_second() const noexcept { return 1.0e3 / seconds_per_kilo_ray_; }
	[[nodiscard]] uint32_t sample_count() const noexcept { return sample_count_; }
};

class ClockOffsetEstimator {
private:
	struct Sample {
		double offset_seconds{0.0};
		double delay_seconds{0.0};
	};

	static constexpr size_t kWindow = 8;
	static constexpr size_t kConvergenceSamples = 4;

	std::array<Sample, kWindow> samples_{};
	size_t count_{0};
	size_t cursor_{0};

	[[nodiscard]] const Sample* best_sample() const noexcept {
		const size_t valid = std::min(count_, kWindow);
		if (valid == 0U) {
			return nullptr;
		}
		const Sample* best = &samples_[0];
		for (size_t i = 1; i < valid; ++i) {
			if (samples_[i].delay_seconds < best->delay_seconds) {
				best = &samples_[i];
			}
		}
		return best;
	}

public:
	void record(double origin, double receive, double transmit, double destination) noexcept {
		const double delay = (destination - origin) - (transmit - receive);
		const double offset = 0.5 * ((receive - origin) + (transmit - destination));
		if (!std::isfinite(delay) || !std::isfinite(offset) || delay < -1.0e-3) {
			return;
		}
		samples_[cursor_] = Sample{offset, std::max(delay, 0.0)};
		cursor_ = (cursor_ + 1U) % kWindow;
		if (count_ < kWindow) {
			++count_;
		}
	}

	[[nodiscard]] bool converged() const noexcept { return count_ >= kConvergenceSamples; }
	[[nodiscard]] size_t sample_count() const noexcept { return count_; }
	[[nodiscard]] double offset_seconds() const noexcept {
		const Sample* best = best_sample();
		return (best != nullptr) ? best->offset_seconds : 0.0;
	}
	[[nodiscard]] double delay_seconds() const noexcept {
		const Sample* best = best_sample();
		return (best != nullptr) ? best->delay_seconds : 0.0;
	}
};

struct NodeCapacityInput {
	uint32_t node_id{0};
	bool is_local{false};
	bool eligible{true};
	double static_weight{1.0};
	double seconds_per_kilo_ray{kDefaultSecondsPerKiloRay};
	double round_trip_seconds{0.0};
};

struct DispatchTiming {
	double deadline_margin_seconds{0.0};
	double seconds_per_kilo_ray{kDefaultSecondsPerKiloRay};
};

struct ClusterNodeDescriptor {
	std::string name{};
	TransportEndpoint endpoint{};
	double static_weight{1.0};
	uint32_t preferred_device{0};
	bool prefer_fp64{true};
	bool enabled{true};
};

struct NodeAdvertisement {
	NodeHardwareCapabilities hardware{};
	ClusterNodeRole role{ClusterNodeRole::Worker};
	uint16_t listen_port{kDefaultClusterPort};
	std::string name{};
};

struct LocalGpuInfo {
	bool has_vulkan_compute{false};
	bool has_fp64_support{false};
	uint32_t vulkan_api_version{0};
	uint32_t vendor_id{0};
	uint32_t device_id{0};
};

enum class HandshakeVerdict : uint32_t {
	Accepted = 0,
	AbiMismatch = 1,
	InvalidCapabilities = 2,
	InsufficientCapabilities = 3,
	InvalidState = 4
};

[[nodiscard]] constexpr const char* handshake_verdict_name(HandshakeVerdict verdict) noexcept {
	switch (verdict) {
		case HandshakeVerdict::Accepted: return "Accepted";
		case HandshakeVerdict::AbiMismatch: return "AbiMismatch";
		case HandshakeVerdict::InvalidCapabilities: return "InvalidCapabilities";
		case HandshakeVerdict::InsufficientCapabilities: return "InsufficientCapabilities";
		case HandshakeVerdict::InvalidState: return "InvalidState";
	}
	return "Unknown";
}

#pragma pack(push, 1)

struct HeartbeatPayload {
	double sender_timestamp_seconds{0.0};
	double echoed_timestamp_seconds{0.0};
	uint32_t sender_state{0};
	uint32_t in_flight_tasks{0};
};

struct ClockSyncPayload {
	double origin_timestamp_seconds{0.0};
	double receive_timestamp_seconds{0.0};
	double transmit_timestamp_seconds{0.0};
	uint32_t phase{0};
	uint32_t reserved{0};
};

struct CapabilitiesAdvertisementWire {
	uint32_t engine_abi_signature{0};
	uint32_t logical_cores{1};
	uint64_t physical_memory_bytes{0};
	uint32_t vulkan_api_version{0};
	uint32_t vendor_id{0};
	uint32_t device_id{0};
	uint8_t feature_flags{0};
	std::array<uint8_t, 3> reserved{};
	float relative_performance_weight{1.0f};
	std::array<char, kMaximumNodeNameBytes> node_name{};
	uint32_t role{0};
	uint16_t listen_port{0};
	uint16_t padding{0};
};

#pragma pack(pop)

static_assert(sizeof(HeartbeatPayload) == 24);
static_assert(sizeof(ClockSyncPayload) == 32);
static_assert(sizeof(CapabilitiesAdvertisementWire) == 76);
static_assert(DistributedPayload<HeartbeatPayload>);
static_assert(DistributedPayload<ClockSyncPayload>);
static_assert(DistributedPayload<CapabilitiesAdvertisementWire>);

inline constexpr uint32_t kClockSyncRequestPhase = 0;
inline constexpr uint32_t kClockSyncResponsePhase = 1;

[[nodiscard]] uint32_t local_engine_abi_signature();

[[nodiscard]] double estimate_relative_performance_weight(const NodeHardwareCapabilities& capabilities) noexcept;

[[nodiscard]] NodeHardwareCapabilities detect_local_hardware_capabilities(const LocalGpuInfo& gpu);

[[nodiscard]] HandshakeVerdict evaluate_remote_capabilities(
	const NodeHardwareCapabilities& local,
	const NodeHardwareCapabilities& remote,
	bool require_gpu_fp64
) noexcept;

[[nodiscard]] size_t encode_capabilities_advertisement(const NodeAdvertisement& advertisement, std::span<uint8_t> output) noexcept;

[[nodiscard]] std::optional<NodeAdvertisement> decode_capabilities_advertisement(std::span<const uint8_t> input);

struct NodeSnapshot {
	uint32_t node_id{0};
	std::string name{};
	TransportEndpoint endpoint{};
	NodeState state{NodeState::Disconnected};
	ClusterNodeRole role{ClusterNodeRole::Worker};
	NodeHardwareCapabilities hardware{};
	uint32_t peer_id{kInvalidPeerId};
	double round_trip_mean_seconds{0.0};
	double round_trip_sigma_seconds{0.0};
	double seconds_per_kilo_ray{kDefaultSecondsPerKiloRay};
	double clock_offset_seconds{0.0};
	double heartbeat_age_seconds{0.0};
	double state_age_seconds{0.0};
	uint32_t in_flight_tasks{0};
	uint64_t tiles_completed{0};
	uint64_t deadline_misses{0};
	uint64_t late_results{0};
	uint64_t frames_served{0};
};

class ClusterNode {
private:
	uint32_t identifier_{0};
	ClusterNodeDescriptor descriptor_{};
	ClusterNodeRole role_{ClusterNodeRole::Worker};
	NodeState state_{NodeState::Disconnected};
	NodeHardwareCapabilities hardware_{};
	uint32_t peer_identifier_{kInvalidPeerId};
	LatencyEstimator latency_{};
	ThroughputEstimator throughput_{};
	ClockOffsetEstimator clock_{};
	ClusterTimePoint state_since_{};
	ClusterTimePoint last_heartbeat_{};
	uint32_t in_flight_tasks_{0};
	uint64_t tiles_completed_{0};
	uint64_t deadline_misses_{0};
	uint64_t late_results_{0};
	uint64_t frames_served_{0};

public:
	ClusterNode() = default;
	ClusterNode(uint32_t identifier, ClusterNodeDescriptor descriptor, NodeState initial_state, ClusterTimePoint now);

	[[nodiscard]] uint32_t identifier() const noexcept { return identifier_; }
	[[nodiscard]] const ClusterNodeDescriptor& descriptor() const noexcept { return descriptor_; }
	[[nodiscard]] ClusterNodeRole role() const noexcept { return role_; }
	[[nodiscard]] NodeState state() const noexcept { return state_; }
	[[nodiscard]] const NodeHardwareCapabilities& hardware() const noexcept { return hardware_; }
	[[nodiscard]] uint32_t peer_identifier() const noexcept { return peer_identifier_; }
	[[nodiscard]] uint32_t in_flight_tasks() const noexcept { return in_flight_tasks_; }
	[[nodiscard]] const LatencyEstimator& latency() const noexcept { return latency_; }
	[[nodiscard]] const ThroughputEstimator& throughput() const noexcept { return throughput_; }
	[[nodiscard]] const ClockOffsetEstimator& clock() const noexcept { return clock_; }
	[[nodiscard]] bool is_schedulable() const noexcept { return descriptor_.enabled && node_accepts_work(state_); }

	[[nodiscard]] bool transition_to(NodeState next, ClusterTimePoint now) noexcept;

	void on_transport_connected(uint32_t peer_identifier, ClusterTimePoint now) noexcept;
	void on_transport_lost(ClusterTimePoint now) noexcept;

	[[nodiscard]] bool begin_handshake(ClusterTimePoint now) noexcept;
	[[nodiscard]] HandshakeVerdict accept_advertisement(
		const NodeAdvertisement& advertisement,
		const NodeHardwareCapabilities& local,
		bool require_gpu_fp64,
		ClusterTimePoint now
	);
	[[nodiscard]] bool complete_synchronization(ClusterTimePoint now) noexcept;
	[[nodiscard]] bool mark_ready(ClusterTimePoint now) noexcept;
	[[nodiscard]] bool request_resynchronization(ClusterTimePoint now) noexcept;

	void record_heartbeat(ClusterTimePoint now) noexcept;
	void record_round_trip(double round_trip_seconds) noexcept;
	void record_clock_sample(double origin, double receive, double transmit, double destination) noexcept;
	[[nodiscard]] bool enforce_timeout(ClusterTimePoint now, double timeout_seconds) noexcept;

	[[nodiscard]] bool register_dispatch(ClusterTimePoint now) noexcept;
	void register_completion(uint32_t pixel_count, double execution_seconds, double round_trip_seconds, ClusterTimePoint now) noexcept;
	void register_deadline_miss(ClusterTimePoint now) noexcept;
	void register_late_result() noexcept;
	void register_frame_served() noexcept;
	void release_in_flight(ClusterTimePoint now) noexcept;

	[[nodiscard]] NodeCapacityInput capacity_input() const noexcept;
	[[nodiscard]] DispatchTiming dispatch_timing() const noexcept;
	[[nodiscard]] NodeSnapshot snapshot(ClusterTimePoint now) const;
};

}
