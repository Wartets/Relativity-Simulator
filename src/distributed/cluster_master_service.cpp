#include "relativistic/distributed/cluster_master_service.hpp"
#include "relativistic/core/engine_log.hpp"
#include "relativistic/distributed/slurm_interface.hpp"
#include <cstdlib>
#include <random>

namespace Relativistic::Distributed {

namespace {

constexpr const char* kTokenEnvironmentVariable = "RELATIVISTIC_CLUSTER_TOKEN";

}

ClusterMasterService::ClusterMasterService(AttachHandler attach, DetachHandler detach)
	: attach_(std::move(attach)), detach_(std::move(detach)) {}

ClusterMasterService::~ClusterMasterService() {
	stop();
}

std::filesystem::path ClusterMasterService::configuration_path() {
	return std::filesystem::path("config") / "distributed_cluster.cfg";
}

void ClusterMasterService::stop_unlocked() noexcept {
	if (orchestrator_ == nullptr) {
		return;
	}
	orchestrator_->shutdown();
	if (detach_) {
		detach_();
	}
	orchestrator_.reset();
}

void ClusterMasterService::stop() noexcept {
	try {
		std::lock_guard<std::mutex> lock(mutex_);
		stop_unlocked();
	} catch (...) {
	}
}

bool ClusterMasterService::restart() noexcept {
	stop();
	return start();
}

bool ClusterMasterService::start() noexcept {
	try {
		std::lock_guard<std::mutex> lock(mutex_);
		stop_unlocked();
		last_error_.clear();

		ClusterConfiguration loaded{};
		const std::filesystem::path path = configuration_path();
		std::error_code ec;
		if (std::filesystem::exists(path, ec)) {
			if (!loaded.load_from_file(path.string())) {
				last_error_ = "The cluster configuration '" + path.string() + "' could not be read";
				Core::log_warning(last_error_);
				return false;
			}
		} else {
			std::random_device device;
			static constexpr char digits[] = "0123456789abcdef";
			std::string token_hex;
			token_hex.reserve(64);
			for (size_t i = 0; i < 32; ++i) {
				const auto value = static_cast<uint8_t>(device() & 0xFFU);
				token_hex.push_back(digits[value >> 4U]);
				token_hex.push_back(digits[value & 0x0FU]);
			}
			static_cast<void>(loaded.apply_token_hex(token_hex));
			std::filesystem::create_directories(path.parent_path(), ec);
			static_cast<void>(loaded.save_to_file(path.string()));
		}
		if (!loaded.security.has_shared_token) {
			if (const char* token = std::getenv(kTokenEnvironmentVariable)) {
				static_cast<void>(loaded.apply_token_hex(token));
			}
		}
		if (SlurmInterface::is_running_under_slurm() && SlurmInterface::get_slurm_rank() == 0U && SlurmInterface::get_slurm_world_size() > 1U) {
			std::string slurm_error;
			if (!SlurmInterface::populate_master_configuration(loaded, slurm_error)) {
				Core::log_warning("The Slurm allocation could not be used for the cluster: " + slurm_error);
			}
		}
		if (loaded.cluster.node_role == ClusterNodeRole::Standalone || loaded.cluster.node_role == ClusterNodeRole::Worker) {
			loaded.cluster.node_role = ClusterNodeRole::Master;
		}
		configuration_ = loaded;
		if (!configuration_.cluster.enabled) {
			return true;
		}

		auto orchestrator = std::make_unique<ClusterOrchestrator>(configuration_);
		if (!orchestrator->start()) {
			last_error_ = orchestrator->last_error();
			return false;
		}
		if (attach_) {
			attach_(*orchestrator);
		}
		orchestrator_ = std::move(orchestrator);
		return true;
	} catch (const std::exception& ex) {
		last_error_ = ex.what();
		return false;
	}
}

bool ClusterMasterService::is_operational() const noexcept {
	try {
		std::lock_guard<std::mutex> lock(mutex_);
		return orchestrator_ != nullptr && orchestrator_->is_cluster_operational();
	} catch (...) {
		return false;
	}
}

std::string ClusterMasterService::last_error() const {
	std::lock_guard<std::mutex> lock(mutex_);
	return last_error_;
}

bool ClusterMasterService::ping_node(uint32_t node_id) noexcept {
	std::lock_guard<std::mutex> lock(mutex_);
	return (orchestrator_ != nullptr) ? orchestrator_->ping_node(node_id) : false;
}

ClusterStatus ClusterMasterService::status() const {
	std::lock_guard<std::mutex> lock(mutex_);
	ClusterStatus result{};
	result.configured = configuration_.cluster.enabled;
	result.error = last_error_;
	if (orchestrator_ != nullptr) {
		result.operational = orchestrator_->is_cluster_operational();
		result.nodes = orchestrator_->node_snapshots();
		result.discovered = orchestrator_->discovered_peers();
		result.last_frame = orchestrator_->last_frame_telemetry();
		result.scheduler = orchestrator_->scheduler().statistics();
	}
	return result;
}

}
