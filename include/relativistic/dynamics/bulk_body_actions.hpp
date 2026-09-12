#pragma once

#include "relativistic/dynamics/pn_nbody_system.hpp"
#include <cmath>
#include <vector>
#include <array>
#include <algorithm>
#include <string_view>

namespace Relativistic::Dynamics {

enum class BulkScalarParameter : uint32_t {
	Mass = 0,
	Radius = 1,
	Charge = 2,
	MagneticMoment = 3,
	RotationSpeed = 4,
	FrictionCoefficient = 5,
	Restitution = 6,
	Integrity = 7,
	Lifetime = 8,
	Temperature = 9,
	HeatCapacity = 10,
	QuadrupoleMoment = 11,
	J2 = 12,
	J3 = 13,
	J4 = 14,
	ReferenceRadius = 15
};

[[nodiscard]] constexpr std::string_view bulk_scalar_parameter_name(BulkScalarParameter param) noexcept {
	switch (param) {
		case BulkScalarParameter::Mass: return "Mass";
		case BulkScalarParameter::Radius: return "Radius";
		case BulkScalarParameter::Charge: return "Charge";
		case BulkScalarParameter::MagneticMoment: return "Magnetic Moment";
		case BulkScalarParameter::RotationSpeed: return "Rotation Speed";
		case BulkScalarParameter::FrictionCoefficient: return "Friction Coefficient";
		case BulkScalarParameter::Restitution: return "Restitution";
		case BulkScalarParameter::Integrity: return "Integrity";
		case BulkScalarParameter::Lifetime: return "Lifetime";
		case BulkScalarParameter::Temperature: return "Temperature";
		case BulkScalarParameter::HeatCapacity: return "Heat Capacity";
		case BulkScalarParameter::QuadrupoleMoment: return "Quadrupole Moment";
		case BulkScalarParameter::J2: return "Zonal J2";
		case BulkScalarParameter::J3: return "Zonal J3";
		case BulkScalarParameter::J4: return "Zonal J4";
		case BulkScalarParameter::ReferenceRadius: return "Multipole Reference Radius";
		default: return "Unknown";
	}
}

[[nodiscard]] constexpr double get_bulk_scalar_parameter(const PostNewtonianBody& body, BulkScalarParameter param) noexcept {
	switch (param) {
		case BulkScalarParameter::Mass: return body.mass;
		case BulkScalarParameter::Radius: return body.radius;
		case BulkScalarParameter::Charge: return body.charge;
		case BulkScalarParameter::MagneticMoment: return body.magnetic_moment;
		case BulkScalarParameter::RotationSpeed: return body.rotation_speed;
		case BulkScalarParameter::FrictionCoefficient: return body.friction_coefficient;
		case BulkScalarParameter::Restitution: return body.restitution;
		case BulkScalarParameter::Integrity: return body.integrity;
		case BulkScalarParameter::Lifetime: return body.lifetime;
		case BulkScalarParameter::Temperature: return body.temperature;
		case BulkScalarParameter::HeatCapacity: return body.heat_capacity;
		case BulkScalarParameter::QuadrupoleMoment: return body.quadrupole_moment;
		case BulkScalarParameter::J2: return body.j2;
		case BulkScalarParameter::J3: return body.j3;
		case BulkScalarParameter::J4: return body.j4;
		case BulkScalarParameter::ReferenceRadius: return body.reference_radius;
		default: return 0.0;
	}
}

constexpr void set_bulk_scalar_parameter(PostNewtonianBody& body, BulkScalarParameter param, double value) noexcept {
	switch (param) {
		case BulkScalarParameter::Mass: body.mass = value; break;
		case BulkScalarParameter::Radius: body.radius = value; break;
		case BulkScalarParameter::Charge: body.charge = value; break;
		case BulkScalarParameter::MagneticMoment: body.magnetic_moment = value; break;
		case BulkScalarParameter::RotationSpeed: body.rotation_speed = value; break;
		case BulkScalarParameter::FrictionCoefficient: body.friction_coefficient = value; break;
		case BulkScalarParameter::Restitution: body.restitution = value; break;
		case BulkScalarParameter::Integrity: body.integrity = value; break;
		case BulkScalarParameter::Lifetime: body.lifetime = value; break;
		case BulkScalarParameter::Temperature: body.temperature = value; break;
		case BulkScalarParameter::HeatCapacity: body.heat_capacity = value; break;
		case BulkScalarParameter::QuadrupoleMoment: body.quadrupole_moment = value; break;
		case BulkScalarParameter::J2: body.j2 = value; break;
		case BulkScalarParameter::J3: body.j3 = value; break;
		case BulkScalarParameter::J4: body.j4 = value; break;
		case BulkScalarParameter::ReferenceRadius: body.reference_radius = value; break;
		default: break;
	}
}

class BulkBodyActions {
public:
	static void invert_all_velocities(PostNewtonianSystem& sys) noexcept {
		for (auto& body : sys.bodies()) {
			if (!body.enabled) continue;
			for (double& component : body.velocity) component = -component;
		}
		sys.update_accelerations();
	}

	static void zero_all_spins(PostNewtonianSystem& sys) noexcept {
		for (auto& body : sys.bodies()) {
			if (!body.enabled) continue;
			body.spin = {0.0, 0.0, 0.0};
		}
		sys.update_accelerations();
	}

	static void scatter_positions(PostNewtonianSystem& sys) noexcept {
		for (auto& body : sys.bodies()) {
			if (!body.enabled) continue;
			const double phase = static_cast<double>(body.id) * 2.399963229728653;
			const double radius = std::max(1.0, std::sqrt(body.position[0] * body.position[0] + body.position[1] * body.position[1] + body.position[2] * body.position[2]));
			body.position = {radius * std::cos(phase), radius * std::sin(phase), radius * 0.15 * std::sin(phase * 0.5)};
		}
		sys.update_accelerations();
	}

	static void snap_to_grid(PostNewtonianSystem& sys, double spacing) noexcept {
		const double safe_spacing = std::max(spacing, 1e-9);
		for (auto& body : sys.bodies()) {
			if (!body.enabled) continue;
			for (double& component : body.position) component = std::round(component / safe_spacing) * safe_spacing;
		}
		sys.update_accelerations();
	}

	static void equalize_masses(PostNewtonianSystem& sys) noexcept {
		double total_mass = 0.0;
		size_t enabled_count = 0;
		for (const auto& body : sys.bodies()) {
			if (!body.enabled) continue;
			total_mass += body.mass;
			++enabled_count;
		}
		if (enabled_count == 0) return;
		const double average_mass = total_mass / static_cast<double>(enabled_count);
		for (auto& body : sys.bodies()) {
			if (body.enabled) body.mass = average_mass;
		}
		sys.update_accelerations();
	}

	static void average_masses(PostNewtonianSystem& sys) noexcept {
		equalize_masses(sys);
	}

	static void cull_outside_radius(PostNewtonianSystem& sys, double max_radius) noexcept {
		std::vector<PostNewtonianBody> survivors;
		for (const auto& body : sys.bodies()) {
			const double r = std::sqrt(body.position[0] * body.position[0] + body.position[1] * body.position[1] + body.position[2] * body.position[2]);
			if (r <= max_radius) survivors.push_back(body);
		}
		sys.clear_bodies();
		for (auto& body : survivors) sys.add_body(body);
		sys.update_accelerations();
	}

	static void set_parameter_for_all(PostNewtonianSystem& sys, BulkScalarParameter param, double value) noexcept {
		for (auto& body : sys.bodies()) {
			if (body.enabled) set_bulk_scalar_parameter(body, param, value);
		}
		sys.update_accelerations();
	}

	static void equalize_parameter(PostNewtonianSystem& sys, BulkScalarParameter param) noexcept {
		double total = 0.0;
		size_t count = 0;
		for (const auto& body : sys.bodies()) {
			if (!body.enabled) continue;
			total += get_bulk_scalar_parameter(body, param);
			++count;
		}
		if (count == 0) return;
		set_parameter_for_all(sys, param, total / static_cast<double>(count));
	}

	static void average_parameter(PostNewtonianSystem& sys, BulkScalarParameter param) noexcept {
		equalize_parameter(sys, param);
	}

	static void cull_outside_camera_frustum(
		PostNewtonianSystem& sys,
		const std::array<double, 3>& camera_position,
		const std::array<double, 3>& camera_forward,
		double half_fov_radians,
		double max_distance
	) noexcept {
		std::vector<PostNewtonianBody> survivors;
		for (const auto& body : sys.bodies()) {
			const std::array<double, 3> to_body{
				body.position[0] - camera_position[0],
				body.position[1] - camera_position[1],
				body.position[2] - camera_position[2]
			};
			const double distance = std::sqrt(to_body[0] * to_body[0] + to_body[1] * to_body[1] + to_body[2] * to_body[2]);
			if (distance > max_distance) continue;
			if (distance < 1e-9) {
				survivors.push_back(body);
				continue;
			}
			const double cos_angle = std::clamp(
				(to_body[0] * camera_forward[0] + to_body[1] * camera_forward[1] + to_body[2] * camera_forward[2]) / distance,
				-1.0, 1.0
			);
			if (std::acos(cos_angle) <= half_fov_radians) {
				survivors.push_back(body);
			}
		}
		sys.clear_bodies();
		for (auto& body : survivors) sys.add_body(body);
		sys.update_accelerations();
	}
};

}
