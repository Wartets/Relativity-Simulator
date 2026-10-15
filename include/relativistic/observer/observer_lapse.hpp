#pragma once

#include "relativistic/core/math/tensor.hpp"
#include "relativistic/metrics/kerr.hpp"
#include "relativistic/modified_gravity/teves.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace Relativistic::Observer {

struct StaticObserverInputs {
	uint32_t metric_id{1};
	double mass{1.0};
	double spin{0.0};
	double charge{0.0};
	double cosmological_lambda{0.0};
	double wormhole_tidal_potential{0.0};
	double teves_critical_acceleration{1.0e-3};
	double radius{10.0};
	double theta{1.5707963267948966};
	double phi{0.0};
};

class StaticObserverFrame {
public:
	[[nodiscard]] static bool has_frame_dragging(uint32_t metric_id) noexcept {
		return metric_id == 2U || metric_id == 3U || metric_id == 5U;
	}

	[[nodiscard]] static double lapse(const StaticObserverInputs& in) noexcept {
		const double m = std::max(in.mass, 1e-9);
		const double r = std::max(in.radius, 1e-9);
		double squared = 1.0;
		switch (in.metric_id) {
			case 0U:
			case 7U:
			case 9U:
			case 10U:
			case 11U:
			case 12U:
				return 1.0;
			case 8U:
				return std::exp(-in.wormhole_tidal_potential / r);
			case 4U:
				squared = 1.0 - 2.0 * m / r + in.charge * in.charge / (r * r);
				break;
			case 6U:
				squared = 1.0 - 2.0 * m / r - in.cosmological_lambda * r * r / 3.0;
				break;
			case 13U:
			case 14U:
			case 15U:
				squared = 1.0 - 2.0 * m / r;
				break;
			case 16U: {
				const ModifiedGravity::TeVeSSpacetimeMetric<double> metric(m, std::max(in.teves_critical_acceleration, 1e-12), 0.01, 1.0, 1.0);
				const double factor = std::max(1.0 - 2.0 * m / r, 0.0);
				return std::exp(metric.scalar_field_phi(r)) * std::sqrt(factor);
			}
			default: {
				const Metrics::KerrMetric<double> metric(m, in.spin, 1.0, 1.0);
				const Core::FourVector<double> position(0.0, r, in.theta, in.phi);
				squared = -metric.metric_tensor(position)(0, 0);
				break;
			}
		}
		return std::sqrt(std::max(squared, 1e-30));
	}
};

}
