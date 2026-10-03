#pragma once

#include "relativistic/observer/surface_geometry.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <string_view>
#include <vector>

namespace Relativistic::Observer {

class CameraCollisionField {
public:
	using Vector3 = SurfaceGeometry::Vector3;

	static constexpr uint32_t kPrimaryHorizonId = 0U;

	struct Obstacle {
		Vector3 center{0.0, 0.0, 0.0};
		Vector3 semi_axes{1.0, 1.0, 1.0};
		uint32_t body_id{0};
	};

	struct Resolution {
		Vector3 position{0.0, 0.0, 0.0};
		Vector3 normal{0.0, 0.0, 1.0};
		bool adjusted{false};
	};

	[[nodiscard]] static double primary_horizon_radius(const Orchestrator::PhysicalParameters& parameters, std::string_view metric_name) noexcept {
		const auto contains = [metric_name](std::string_view token) noexcept { return metric_name.find(token) != std::string_view::npos; };
		if (parameters.mass <= 0.0 || contains("Minkowski") || contains("Morris") || contains("Wormhole") || contains("Alcubierre") || contains("Warp") || contains("FLRW")) {
			return 0.0;
		}
		const double mass = parameters.mass;
		const bool rotating = contains("Kerr");
		const bool charged = contains("Reissner") || contains("Newman");
		const double spin = rotating ? std::clamp(parameters.spin, -0.999 * mass, 0.999 * mass) : 0.0;
		const double charge = charged ? parameters.charge : 0.0;
		return mass + std::sqrt(std::max(mass * mass - spin * spin - charge * charge, 0.0));
	}

	void rebuild(const Orchestrator::SimulationOrchestrator<1024>& orchestrator) {
		obstacles_.clear();
		const double horizon = primary_horizon_radius(orchestrator.parameters(), orchestrator.active_metric_name());
		if (horizon > 0.0) {
			obstacles_.push_back(Obstacle{{0.0, 0.0, 0.0}, {horizon, horizon, horizon}, kPrimaryHorizonId});
		}
		std::lock_guard<std::recursive_mutex> lock(orchestrator.nbody_system().bodies_mutex());
		for (const auto& body : orchestrator.nbody_system().bodies()) {
			if (!body.enabled) {
				continue;
			}
			obstacles_.push_back(Obstacle{body.position, SurfaceGeometry::body_semi_axes(body), body.id});
		}
	}

	[[nodiscard]] size_t obstacle_count() const noexcept {
		return obstacles_.size();
	}

	[[nodiscard]] Resolution resolve(const Vector3& from, const Vector3& to, double clearance) const noexcept {
		Resolution result;
		result.position = to;
		const double margin = std::max(clearance, 0.0);
		for (int pass = 0; pass < 4; ++pass) {
			bool changed = false;
			for (const Obstacle& obstacle : obstacles_) {
				const Vector3 inflated{obstacle.semi_axes[0] + margin, obstacle.semi_axes[1] + margin, obstacle.semi_axes[2] + margin};
				const auto to_unit_space = [&](const Vector3& point) noexcept -> Vector3 {
					const Vector3 offset = SurfaceGeometry::subtract(point, obstacle.center);
					return {offset[0] / inflated[0], offset[1] / inflated[1], offset[2] / inflated[2]};
				};
				const auto from_unit_space = [&](const Vector3& unit) noexcept -> Vector3 {
					return {
						obstacle.center[0] + unit[0] * inflated[0],
						obstacle.center[1] + unit[1] * inflated[1],
						obstacle.center[2] + unit[2] * inflated[2]
					};
				};
				const Vector3 start = to_unit_space(from);
				const Vector3 target = to_unit_space(result.position);
				const double target_length = SurfaceGeometry::length(target);

				Vector3 surface_unit{};
				bool collided = false;
				if (target_length < 1.0) {
					const Vector3 fallback = SurfaceGeometry::normalized(start, {0.0, 0.0, 1.0});
					surface_unit = (target_length > 1e-12) ? SurfaceGeometry::scale(target, 1.0 / target_length) : fallback;
					collided = true;
				} else {
					const Vector3 delta = SurfaceGeometry::subtract(target, start);
					const double a = SurfaceGeometry::dot(delta, delta);
					const double c = SurfaceGeometry::dot(start, start) - 1.0;
					if (a > 1e-24 && c > 0.0) {
						const double b = 2.0 * SurfaceGeometry::dot(start, delta);
						const double discriminant = b * b - 4.0 * a * c;
						if (discriminant >= 0.0) {
							const double t = (-b - std::sqrt(discriminant)) / (2.0 * a);
							if (t >= 0.0 && t <= 1.0) {
								surface_unit = SurfaceGeometry::normalized(SurfaceGeometry::add(start, SurfaceGeometry::scale(delta, t)), {0.0, 0.0, 1.0});
								collided = true;
							}
						}
					}
				}

				if (collided) {
					result.position = from_unit_space(SurfaceGeometry::scale(surface_unit, 1.0 + 1e-9));
					result.normal = SurfaceGeometry::normalized({surface_unit[0] / inflated[0], surface_unit[1] / inflated[1], surface_unit[2] / inflated[2]}, surface_unit);
					result.adjusted = true;
					changed = true;
				}
			}
			if (!changed) {
				break;
			}
		}
		return result;
	}

private:
	std::vector<Obstacle> obstacles_{};
};

}
