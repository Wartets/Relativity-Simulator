#pragma once

#include "relativistic/distributed/cluster_node.hpp"
#include <algorithm>
#include <map>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

namespace Relativistic::Distributed {

class NodeRegistry {
private:
	mutable std::mutex mutex_{};
	std::map<uint32_t, ClusterNode> nodes_{};
	std::unordered_map<uint32_t, uint32_t> peer_index_{};
	uint32_t next_identifier_{1};

	[[nodiscard]] static bool same_endpoint(const TransportEndpoint& a, const TransportEndpoint& b) {
		return a.port == b.port && a.host == b.host;
	}

	[[nodiscard]] bool endpoint_in_use_unlocked(const TransportEndpoint& endpoint) const {
		if (endpoint.host.empty()) {
			return false;
		}
		return std::any_of(nodes_.begin(), nodes_.end(), [&endpoint](const auto& entry) {
			return same_endpoint(entry.second.descriptor().endpoint, endpoint);
		});
	}

public:
	NodeRegistry() = default;
	NodeRegistry(const NodeRegistry&) = delete;
	NodeRegistry& operator=(const NodeRegistry&) = delete;

	[[nodiscard]] std::optional<uint32_t> register_node(
		const ClusterNodeDescriptor& descriptor,
		NodeState initial_state = NodeState::Disconnected,
		ClusterTimePoint now = ClusterClock::now()
	) {
		std::lock_guard<std::mutex> lock(mutex_);
		if (nodes_.size() >= kMaximumClusterNodes || next_identifier_ == 0U) {
			return std::nullopt;
		}
		if (endpoint_in_use_unlocked(descriptor.endpoint)) {
			return std::nullopt;
		}
		const uint32_t identifier = next_identifier_++;
		nodes_.emplace(identifier, ClusterNode(identifier, descriptor, initial_state, now));
		return identifier;
	}

	bool unregister_node(uint32_t node_id) {
		std::lock_guard<std::mutex> lock(mutex_);
		const auto it = nodes_.find(node_id);
		if (it == nodes_.end()) {
			return false;
		}
		const uint32_t peer = it->second.peer_identifier();
		if (peer != kInvalidPeerId) {
			peer_index_.erase(peer);
		}
		nodes_.erase(it);
		return true;
	}

	[[nodiscard]] bool bind_peer(uint32_t node_id, uint32_t peer_id, ClusterTimePoint now) {
		std::lock_guard<std::mutex> lock(mutex_);
		const auto it = nodes_.find(node_id);
		if (it == nodes_.end() || peer_id == kInvalidPeerId) {
			return false;
		}
		const auto existing = peer_index_.find(peer_id);
		if (existing != peer_index_.end() && existing->second != node_id) {
			return false;
		}
		const uint32_t previous = it->second.peer_identifier();
		if (previous != kInvalidPeerId && previous != peer_id) {
			peer_index_.erase(previous);
		}
		peer_index_[peer_id] = node_id;
		it->second.on_transport_connected(peer_id, now);
		return true;
	}

	[[nodiscard]] std::optional<uint32_t> release_peer(uint32_t peer_id, ClusterTimePoint now) {
		std::lock_guard<std::mutex> lock(mutex_);
		const auto index = peer_index_.find(peer_id);
		if (index == peer_index_.end()) {
			return std::nullopt;
		}
		const uint32_t node_id = index->second;
		peer_index_.erase(index);
		const auto it = nodes_.find(node_id);
		if (it != nodes_.end()) {
			it->second.on_transport_lost(now);
		}
		return node_id;
	}

	[[nodiscard]] std::optional<uint32_t> find_by_peer(uint32_t peer_id) const {
		std::lock_guard<std::mutex> lock(mutex_);
		const auto it = peer_index_.find(peer_id);
		return (it != peer_index_.end()) ? std::optional<uint32_t>(it->second) : std::nullopt;
	}

	[[nodiscard]] std::optional<uint32_t> find_by_endpoint(const TransportEndpoint& endpoint) const {
		std::lock_guard<std::mutex> lock(mutex_);
		for (const auto& [identifier, node] : nodes_) {
			if (same_endpoint(node.descriptor().endpoint, endpoint)) {
				return identifier;
			}
		}
		return std::nullopt;
	}

	template <typename Visitor>
	bool visit(uint32_t node_id, Visitor&& visitor) {
		std::lock_guard<std::mutex> lock(mutex_);
		const auto it = nodes_.find(node_id);
		if (it == nodes_.end()) {
			return false;
		}
		visitor(it->second);
		return true;
	}

	template <typename Visitor>
	bool visit(uint32_t node_id, Visitor&& visitor) const {
		std::lock_guard<std::mutex> lock(mutex_);
		const auto it = nodes_.find(node_id);
		if (it == nodes_.end()) {
			return false;
		}
		visitor(it->second);
		return true;
	}

	template <typename Visitor>
	void visit_all(Visitor&& visitor) {
		std::lock_guard<std::mutex> lock(mutex_);
		for (auto& [identifier, node] : nodes_) {
			visitor(node);
		}
	}

	template <typename Visitor>
	void visit_all(Visitor&& visitor) const {
		std::lock_guard<std::mutex> lock(mutex_);
		for (const auto& [identifier, node] : nodes_) {
			visitor(node);
		}
	}

	[[nodiscard]] std::vector<NodeSnapshot> snapshots(ClusterTimePoint now = ClusterClock::now()) const {
		std::lock_guard<std::mutex> lock(mutex_);
		std::vector<NodeSnapshot> result;
		result.reserve(nodes_.size());
		for (const auto& [identifier, node] : nodes_) {
			result.push_back(node.snapshot(now));
		}
		return result;
	}

	[[nodiscard]] size_t size() const {
		std::lock_guard<std::mutex> lock(mutex_);
		return nodes_.size();
	}

	[[nodiscard]] size_t count_in_state(NodeState state) const {
		std::lock_guard<std::mutex> lock(mutex_);
		return static_cast<size_t>(std::count_if(nodes_.begin(), nodes_.end(), [state](const auto& entry) {
			return entry.second.state() == state;
		}));
	}

	[[nodiscard]] size_t schedulable_count() const {
		std::lock_guard<std::mutex> lock(mutex_);
		return static_cast<size_t>(std::count_if(nodes_.begin(), nodes_.end(), [](const auto& entry) {
			return entry.second.is_schedulable();
		}));
	}

	size_t collect_capacity_inputs(std::vector<NodeCapacityInput>& output) const {
		std::lock_guard<std::mutex> lock(mutex_);
		output.clear();
		for (const auto& [identifier, node] : nodes_) {
			if (node.is_schedulable()) {
				output.push_back(node.capacity_input());
			}
		}
		return output.size();
	}

	size_t enforce_timeouts(ClusterTimePoint now, uint32_t timeout_ms, std::vector<uint32_t>& expired) {
		std::lock_guard<std::mutex> lock(mutex_);
		expired.clear();
		const double timeout_seconds = static_cast<double>(timeout_ms) * 1.0e-3;
		for (auto& [identifier, node] : nodes_) {
			if (node.enforce_timeout(now, timeout_seconds)) {
				expired.push_back(identifier);
			}
		}
		return expired.size();
	}

	void clear() {
		std::lock_guard<std::mutex> lock(mutex_);
		nodes_.clear();
		peer_index_.clear();
	}
};

}
