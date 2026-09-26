#pragma once

#include "relativistic/optics/disk_thermal_profile.hpp"
#include "relativistic/render/gpu_types.hpp"
#include "relativistic/metrics/kerr_schild.hpp"
#include "relativistic/core/tensor.hpp"
#include <array>
#include <cmath>
#include <algorithm>
#include <optional>
#include <vector>

namespace Relativistic::Metrics {

struct SubsidiarySourceParams {
	std::array<double, 3> position{0.0, 0.0, 0.0};
	std::array<double, 3> velocity{0.0, 0.0, 0.0};
	double mass{0.0};
	double spin{0.0};
	double horizon_radius{0.0};
	double isco_radius{0.0};
	double disk_outer_radius{0.0};
	KerrSchildMetric<double> metric{1.0, 0.0};

	[[nodiscard]] static SubsidiarySourceParams from_gpu_body(const Render::GpuBodyData& body) noexcept {
		SubsidiarySourceParams p;
		p.position = {body.position[0], body.position[1], body.position[2]};
		p.velocity = {body.velocity[0], body.velocity[1], body.velocity[2]};
		p.mass = std::max(body.mass, 1e-9);
		const double a_star = std::clamp(body.spin_parameter, -0.9999, 0.9999);
		p.spin = a_star * p.mass;
		p.metric = KerrSchildMetric<double>(p.mass, p.spin);
		p.horizon_radius = p.metric.outer_horizon_radius();
		p.isco_radius = Optics::DiskThermalProfile::kerr_isco_radius(p.mass, p.spin);
		p.disk_outer_radius = Optics::DiskThermalProfile::disk_outer_radius(p.mass);
		return p;
	}

	[[nodiscard]] bool is_within_horizon(const std::array<double, 3>& world_pos) const noexcept {
		const double dx = world_pos[0] - position[0];
		const double dy = world_pos[1] - position[1];
		const double dz = world_pos[2] - position[2];
		return (dx * dx + dy * dy + dz * dz) <= (horizon_radius * horizon_radius);
	}
};

struct DiskCrossingSample {
	double radius{0.0};
	double phi{0.0};
	double doppler_factor{1.0};
	double temperature_kelvin{0.0};
	double normalized_flux{0.0};
};

class SubsidiarySourceField {
public:
	[[nodiscard]] static bool segment_crosses_horizon(
		const std::array<double, 3>& seg_start,
		const std::array<double, 3>& seg_end,
		const SubsidiarySourceParams& source
	) noexcept {
		const double dx = seg_end[0] - seg_start[0];
		const double dy = seg_end[1] - seg_start[1];
		const double dz = seg_end[2] - seg_start[2];
		const double a_coeff = dx * dx + dy * dy + dz * dz;
		if (a_coeff <= 1e-18) {
			return source.is_within_horizon(seg_start);
		}
		const double ox = seg_start[0] - source.position[0];
		const double oy = seg_start[1] - source.position[1];
		const double oz = seg_start[2] - source.position[2];
		const double b_coeff = 2.0 * (ox * dx + oy * dy + oz * dz);
		const double c_coeff = ox * ox + oy * oy + oz * oz - source.horizon_radius * source.horizon_radius;
		const double discriminant = b_coeff * b_coeff - 4.0 * a_coeff * c_coeff;
		if (discriminant < 0.0) return false;
		const double sqrt_disc = std::sqrt(discriminant);
		const double t1 = (-b_coeff - sqrt_disc) / (2.0 * a_coeff);
		const double t2 = (-b_coeff + sqrt_disc) / (2.0 * a_coeff);
		return (t1 >= 0.0 && t1 <= 1.0) || (t2 >= 0.0 && t2 <= 1.0);
	}

	[[nodiscard]] static std::optional<DiskCrossingSample> segment_crosses_disk(
		const std::array<double, 3>& seg_start,
		const std::array<double, 3>& seg_end,
		const SubsidiarySourceParams& source
	) noexcept {
		const double prev_z_rel = seg_start[2] - source.position[2];
		const double curr_z_rel = seg_end[2] - source.position[2];
		const double z_span = curr_z_rel - prev_z_rel;
		if (prev_z_rel * curr_z_rel > 0.0 || std::abs(z_span) <= 1e-15) {
			return std::nullopt;
		}
		const double s_cross = std::clamp(std::abs(prev_z_rel) / std::abs(z_span), 0.0, 1.0);
		const double cross_x = seg_start[0] + s_cross * (seg_end[0] - seg_start[0]) - source.position[0];
		const double cross_y = seg_start[1] + s_cross * (seg_end[1] - seg_start[1]) - source.position[1];
		const double r_cross = std::sqrt(cross_x * cross_x + cross_y * cross_y);
		if (r_cross < source.isco_radius || r_cross > source.disk_outer_radius) {
			return std::nullopt;
		}

		DiskCrossingSample sample;
		sample.radius = r_cross;
		sample.phi = std::atan2(cross_y, cross_x);

		const double gamma_orb = 1.0 / std::sqrt(std::max(1.0 - 3.0 * source.mass / r_cross + 2.0 * source.spin * std::sqrt(source.mass / (r_cross * r_cross * r_cross)), 1e-4));
		const double g_doppler = std::sqrt(std::max(1.0 - source.horizon_radius / r_cross, 1e-4)) / std::max(gamma_orb, 1e-6);
		sample.doppler_factor = g_doppler;

		const double t_norm = Optics::DiskThermalProfile::normalized_temperature(source.isco_radius, r_cross);
		sample.normalized_flux = t_norm;
		sample.temperature_kelvin = Optics::DiskThermalProfile::effective_temperature_kelvin(source.isco_radius, r_cross) * g_doppler;
		return sample;
	}

	[[nodiscard]] static std::array<double, 3> compute_weak_field_deflection(
		const std::array<double, 3>& ray_pos,
		const std::array<double, 3>& ray_dir,
		double step_length,
		const SubsidiarySourceParams& source
	) noexcept {
		const double rx = ray_pos[0] - source.position[0];
		const double ry = ray_pos[1] - source.position[1];
		const double rz = ray_pos[2] - source.position[2];
		const double r_len = std::sqrt(rx * rx + ry * ry + rz * rz);
		if (r_len < source.horizon_radius * 1.5) {
			return ray_dir;
		}

		const double proj = rx * ray_dir[0] + ry * ray_dir[1] + rz * ray_dir[2];
		const double perp_x = rx - proj * ray_dir[0];
		const double perp_y = ry - proj * ray_dir[1];
		const double perp_z = rz - proj * ray_dir[2];
		const double impact_param = std::sqrt(perp_x * perp_x + perp_y * perp_y + perp_z * perp_z);
		if (impact_param < source.horizon_radius * 0.1) {
			return ray_dir;
		}

		const double cross_x = ry * ray_dir[2] - rz * ray_dir[1];
		const double cross_y = rz * ray_dir[0] - rx * ray_dir[2];
		const double cross_z = rx * ray_dir[1] - ry * ray_dir[0];
		const double cross_norm = std::sqrt(cross_x * cross_x + cross_y * cross_y + cross_z * cross_z);
		if (cross_norm < 1e-12) {
			return ray_dir;
		}

		const double nx = cross_x / cross_norm;
		const double ny = cross_y / cross_norm;
		const double nz = cross_z / cross_norm;

		const double schwarzschild_term = (4.0 * source.mass) / std::max(impact_param, source.mass * 1e-3);
		const double gravitomagnetic_term = (4.0 * source.spin * source.mass) / std::max(impact_param * impact_param * impact_param, source.mass * source.mass * 1e-6);
		const double weight = std::exp(-r_len / (30.0 * std::max(source.mass, 1e-6)));
		const double total_deflection = std::min((schwarzschild_term + gravitomagnetic_term) * weight * (step_length / std::max(r_len, source.mass)), 0.5);

		const double wx = ny * ray_dir[2] - nz * ray_dir[1];
		const double wy = nz * ray_dir[0] - nx * ray_dir[2];
		const double wz = nx * ray_dir[1] - ny * ray_dir[0];

		const double cos_a = std::cos(total_deflection);
		const double sin_a = std::sin(total_deflection);

		std::array<double, 3> new_dir{
			ray_dir[0] * cos_a + wx * sin_a,
			ray_dir[1] * cos_a + wy * sin_a,
			ray_dir[2] * cos_a + wz * sin_a
		};
		const double len = std::sqrt(new_dir[0] * new_dir[0] + new_dir[1] * new_dir[1] + new_dir[2] * new_dir[2]);
		if (len > 1e-12) {
			new_dir[0] /= len;
			new_dir[1] /= len;
			new_dir[2] /= len;
		}
		return new_dir;
	}

	[[nodiscard]] static double distance_squared(const std::array<double, 3>& a, const std::array<double, 3>& b) noexcept {
		const double dx = a[0] - b[0];
		const double dy = a[1] - b[1];
		const double dz = a[2] - b[2];
		return dx * dx + dy * dy + dz * dz;
	}

	[[nodiscard]] static std::array<double, 3> integrate_local_geodesic_correction(
		double rx, double ry, double rz,
		const std::array<double, 3>& ray_dir,
		double step_length,
		const KerrSchildMetric<double>& metric
	) noexcept {
		Core::FourVector<double> x(0.0, rx, ry, rz);
		Core::FourVector<double> p(1.0, ray_dir[0], ray_dir[1], ray_dir[2]);

		auto acceleration = [&](const Core::FourVector<double>& xs, const Core::FourVector<double>& ps) noexcept -> Core::FourVector<double> {
			const auto gamma = metric.christoffel_symbols(xs);
			Core::FourVector<double> acc;
			acc.zero();
			for (size_t mu = 0; mu < 4; ++mu) {
				double sum = 0.0;
				for (size_t a = 0; a < 4; ++a) {
					const double pa = ps(a);
					if (pa == 0.0) continue;
					sum -= gamma(mu, a, a) * pa * pa;
					for (size_t b = a + 1; b < 4; ++b) {
						const double pb = ps(b);
						if (pb != 0.0) {
							sum -= 2.0 * gamma(mu, a, b) * pa * pb;
						}
					}
				}
				acc(mu) = sum;
			}
			return acc;
		};

		const double dt = std::clamp(step_length, 1e-4, 2.0);

		const auto k1_x = p;
		const auto k1_p = acceleration(x, p);

		Core::FourVector<double> x2 = x, p2 = p;
		for (size_t i = 0; i < 4; ++i) { x2(i) += 0.5 * dt * k1_x(i); p2(i) += 0.5 * dt * k1_p(i); }
		const auto k2_x = p2;
		const auto k2_p = acceleration(x2, p2);

		Core::FourVector<double> x3 = x, p3 = p;
		for (size_t i = 0; i < 4; ++i) { x3(i) += 0.5 * dt * k2_x(i); p3(i) += 0.5 * dt * k2_p(i); }
		const auto k3_x = p3;
		const auto k3_p = acceleration(x3, p3);

		Core::FourVector<double> x4 = x, p4 = p;
		for (size_t i = 0; i < 4; ++i) { x4(i) += dt * k3_x(i); p4(i) += dt * k3_p(i); }
		// const auto k4_x = p4;
		const auto k4_p = acceleration(x4, p4);

		Core::FourVector<double> p_next = p;
		const double sixth = dt / 6.0;
		for (size_t i = 0; i < 4; ++i) {
			p_next(i) += sixth * (k1_p(i) + 2.0 * k2_p(i) + 2.0 * k3_p(i) + k4_p(i));
		}

		std::array<double, 3> new_dir{p_next(1), p_next(2), p_next(3)};
		const double len = std::sqrt(new_dir[0] * new_dir[0] + new_dir[1] * new_dir[1] + new_dir[2] * new_dir[2]);
		if (len > 1e-12) {
			new_dir[0] /= len;
			new_dir[1] /= len;
			new_dir[2] /= len;
		}
		return new_dir;
	}

	[[nodiscard]] static std::array<double, 3> compute_source_deflection(
		const std::array<double, 3>& ray_pos,
		const std::array<double, 3>& ray_dir,
		double step_length,
		const SubsidiarySourceParams& source
	) noexcept {
		const double rx = ray_pos[0] - source.position[0];
		const double ry = ray_pos[1] - source.position[1];
		const double rz = ray_pos[2] - source.position[2];
		const double r_len = std::sqrt(rx * rx + ry * ry + rz * rz);
		if (r_len < source.horizon_radius * 1.5) {
			return ray_dir;
		}

		const double strong_field_radius = std::max(source.disk_outer_radius, source.horizon_radius * 40.0);
		if (r_len < strong_field_radius) {
			return integrate_local_geodesic_correction(rx, ry, rz, ray_dir, step_length, source.metric);
		}
		return compute_weak_field_deflection(ray_pos, ray_dir, step_length, source);
	}

	[[nodiscard]] static std::array<double, 3> apply_all_source_corrections(
		const std::array<double, 3>& ray_pos,
		const std::array<double, 3>& ray_dir,
		double step_length,
		const std::vector<SubsidiarySourceParams>& sources
	) noexcept {
		if (sources.empty()) {
			return ray_dir;
		}

		std::vector<size_t> order(sources.size());
		for (size_t i = 0; i < sources.size(); ++i) {
			order[i] = i;
		}
		std::sort(order.begin(), order.end(), [&](size_t a, size_t b) noexcept {
			return distance_squared(ray_pos, sources[a].position) < distance_squared(ray_pos, sources[b].position);
		});

		std::array<double, 3> current_dir = ray_dir;
		for (const size_t idx : order) {
			current_dir = compute_source_deflection(ray_pos, current_dir, step_length, sources[idx]);
		}
		return current_dir;
	}

	[[nodiscard]] static const SubsidiarySourceParams* find_dominant_source(
		const std::array<double, 3>& ray_pos,
		const std::vector<SubsidiarySourceParams>& sources
	) noexcept {
		const SubsidiarySourceParams* dominant = nullptr;
		double strongest_influence = 0.0;
		for (const auto& source : sources) {
			const double dx = ray_pos[0] - source.position[0];
			const double dy = ray_pos[1] - source.position[1];
			const double dz = ray_pos[2] - source.position[2];
			const double dist_sq = std::max(dx * dx + dy * dy + dz * dz, 1e-9);
			const double influence = source.mass / dist_sq;
			if (influence > strongest_influence) {
				strongest_influence = influence;
				dominant = &source;
			}
		}
		return dominant;
	}
};

}
