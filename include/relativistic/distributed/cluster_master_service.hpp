#pragma once

#include "relativistic/distributed/cluster_config.hpp"
#include "relativistic/distributed/cluster_orchestrator.hpp"
#include "relativistic/distributed/cluster_telemetry.hpp"
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace Relativistic::Distributed {

struct ClusterStatus {
	bool configured{false};
	bool operational{false};
	std::string error{};
	std::vector<NodeSnapshot> nodes{};
	ClusterFrameTelemetry last_frame{};
	SchedulerStatistics scheduler{};
};

class ClusterMasterService {
public:
	using AttachHandler = std::function<void(ClusterOrchestrator&)>;
	using DetachHandler = std::function<void()>;

private:
	AttachHandler attach_{};
	DetachHandler detach_{};
	mutable std::mutex mutex_{};
	std::unique_ptr<ClusterOrchestrator> orchestrator_{};
	ClusterConfiguration configuration_{};
	std::string last_error_{};

	void stop_unlocked() noexcept;

public:
	ClusterMasterService(AttachHandler attach, DetachHandler detach);
	~ClusterMasterService();

	ClusterMasterService(const ClusterMasterService&) = delete;
	ClusterMasterService& operator=(const ClusterMasterService&) = delete;

	[[nodiscard]] static std::filesystem::path configuration_path();

	bool start() noexcept;
	void stop() noexcept;
	bool restart() noexcept;

	[[nodiscard]] bool is_operational() const noexcept;
	[[nodiscard]] std::string last_error() const;
	[[nodiscard]] ClusterStatus status() const;
};

}
