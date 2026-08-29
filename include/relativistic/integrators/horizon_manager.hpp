#pragma once

#include "relativistic/core/tensor.hpp"
#include <cstdint>
#include <cmath>

namespace Relativistic::Integrators {

enum class HorizonCrossingMode : uint8_t {
	Absorption,
	Continuity
};

enum class TrajectoryRegion : uint8_t {
	Exterior,
	HorizonSurface,
	Interior,
	PhysicalSingularity
};

template <typename Scalar = double>
struct HorizonConfig {
	HorizonCrossingMode crossing_mode = HorizonCrossingMode::Continuity;
	Scalar horizon_radius = static_cast<Scalar>(0);
	Scalar horizon_epsilon = static_cast<Scalar>(1e-5);
	Scalar singularity_threshold = static_cast<Scalar>(1e-4);
};

template <typename Scalar = double>
struct HorizonEventStatus {
	bool crossed_horizon = false;
	bool absorbed_at_horizon = false;
	bool reached_singularity = false;
	TrajectoryRegion current_region = TrajectoryRegion::Exterior;
	Scalar last_radius = static_cast<Scalar>(0);
};

template <typename Scalar = double>
class HorizonDetector {
private:
	HorizonConfig<Scalar> config_;
	HorizonEventStatus<Scalar> status_;

public:
	explicit constexpr HorizonDetector(const HorizonConfig<Scalar>& config = {}) noexcept
		: config_(config), status_{} {}

	constexpr void set_horizon_radius(Scalar r_s) noexcept {
		config_.horizon_radius = r_s;
	}

	constexpr void set_crossing_mode(HorizonCrossingMode mode) noexcept {
		config_.crossing_mode = mode;
	}

	constexpr void set_singularity_threshold(Scalar threshold) noexcept {
		config_.singularity_threshold = threshold;
	}

	[[nodiscard]] constexpr const HorizonConfig<Scalar>& config() const noexcept {
		return config_;
	}

	[[nodiscard]] constexpr const HorizonEventStatus<Scalar>& status() const noexcept {
		return status_;
	}

	[[nodiscard]] constexpr bool should_terminate(Scalar r) noexcept {
		status_.last_radius = r;
		const Scalar r_s = config_.horizon_radius;

		if (r_s <= static_cast<Scalar>(0)) {
			status_.current_region = TrajectoryRegion::Exterior;
			return false;
		}

		const Scalar sing_r = config_.singularity_threshold * r_s;
		if (r <= sing_r) {
			status_.current_region = TrajectoryRegion::PhysicalSingularity;
			status_.reached_singularity = true;
			return true;
		}

		if (r <= r_s) {
			status_.crossed_horizon = true;
			if (std::abs(r - r_s) <= config_.horizon_epsilon * r_s) {
				status_.current_region = TrajectoryRegion::HorizonSurface;
			} else {
				status_.current_region = TrajectoryRegion::Interior;
			}

			if (config_.crossing_mode == HorizonCrossingMode::Absorption) {
				status_.absorbed_at_horizon = true;
				return true;
			}
			return false;
		}

		status_.current_region = TrajectoryRegion::Exterior;
		return false;
	}

	constexpr void reset() noexcept {
		status_ = HorizonEventStatus<Scalar>{};
	}
};

}
