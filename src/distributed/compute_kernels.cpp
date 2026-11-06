#include "relativistic/distributed/compute_kernels.hpp"
#include "relativistic/core/pcg64.hpp"
#include "relativistic/core/schwarzschild_null_integrator.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <numbers>
#include <thread>

namespace Relativistic::Distributed {

namespace {

constexpr double kInverseTwoPow53 = 1.0 / 9007199254740992.0;

[[nodiscard]] bool all_finite(const NBodyRowResult& row) noexcept {
	return std::isfinite(row.acceleration[0]) && std::isfinite(row.acceleration[1]) && std::isfinite(row.acceleration[2]) && std::isfinite(row.potential);
}

}

void evaluate_nbody_rows(
	std::span<const NBodyBodyState> bodies,
	uint32_t first_row,
	uint32_t row_count,
	uint32_t phase,
	double gravitational_constant,
	double speed_of_light,
	std::span<NBodyRowResult> output
) noexcept {
	const size_t total = bodies.size();
	const double inverse_c2 = (speed_of_light > 0.0) ? 1.0 / (speed_of_light * speed_of_light) : 0.0;
	const size_t rows = std::min<size_t>(row_count, output.size());
	for (size_t row = 0; row < rows; ++row) {
		const size_t i = static_cast<size_t>(first_row) + row;
		NBodyRowResult result{};
		if (i < total) {
			const NBodyBodyState& bi = bodies[i];
			const double vix = bi.velocity[0];
			const double viy = bi.velocity[1];
			const double viz = bi.velocity[2];
			const double vi2 = vix * vix + viy * viy + viz * viz;
			double ax = 0.0;
			double ay = 0.0;
			double az = 0.0;
			double potential = 0.0;
			for (size_t j = 0; j < total; ++j) {
				if (j == i) {
					continue;
				}
				const NBodyBodyState& bj = bodies[j];
				const double dx = bi.position[0] - bj.position[0];
				const double dy = bi.position[1] - bj.position[1];
				const double dz = bi.position[2] - bj.position[2];
				const double r2 = dx * dx + dy * dy + dz * dz;
				if (!(r2 > 1.0e-300)) {
					continue;
				}
				const double inverse_r = 1.0 / std::sqrt(r2);
				const double inverse_r3 = inverse_r / r2;
				const double gm = gravitational_constant * bj.mass;
				if (phase == kNBodyPhaseNewtonian) {
					const double radial = -gm * inverse_r3;
					ax += radial * dx;
					ay += radial * dy;
					az += radial * dz;
					potential += gm * inverse_r;
				} else {
					const double vjx = bj.velocity[0];
					const double vjy = bj.velocity[1];
					const double vjz = bj.velocity[2];
					const double vj2 = vjx * vjx + vjy * vjy + vjz * vjz;
					const double vivj = vix * vjx + viy * vjy + viz * vjz;
					const double projected = (dx * vjx + dy * vjy + dz * vjz) * inverse_r;
					const double separation_acceleration = dx * bj.acceleration[0] + dy * bj.acceleration[1] + dz * bj.acceleration[2];
					const double bracket = 1.0 + inverse_c2 * (-4.0 * bi.potential - bj.potential + vi2 + 2.0 * vj2 - 4.0 * vivj - 1.5 * projected * projected - 0.5 * separation_acceleration);
					const double radial = -gm * inverse_r3 * bracket;
					const double combined = dx * (4.0 * vix - 3.0 * vjx) + dy * (4.0 * viy - 3.0 * vjy) + dz * (4.0 * viz - 3.0 * vjz);
					const double longitudinal = gm * inverse_r3 * inverse_c2 * combined;
					const double acceleration_coupling = 3.5 * inverse_c2 * gm * inverse_r;
					ax += radial * dx + longitudinal * (vix - vjx) + acceleration_coupling * bj.acceleration[0];
					ay += radial * dy + longitudinal * (viy - vjy) + acceleration_coupling * bj.acceleration[1];
					az += radial * dz + longitudinal * (viz - vjz) + acceleration_coupling * bj.acceleration[2];
				}
			}
			result.acceleration[0] = ax;
			result.acceleration[1] = ay;
			result.acceleration[2] = az;
			result.potential = (phase == kNBodyPhaseNewtonian) ? potential : bi.potential;
		}
		output[row] = result;
	}
}

bool evaluate_nbody_accelerations_local(
	std::span<const NBodyBodyState> bodies,
	const NBodyEvaluationOptions& options,
	std::span<NBodyRowResult> output
) {
	const size_t count = bodies.size();
	if (count == 0U || count > kNBodyMaximumBodies || output.size() != count || !(options.speed_of_light > 0.0) || !std::isfinite(options.gravitational_constant)) {
		return false;
	}
	std::vector<NBodyBodyState> state(bodies.begin(), bodies.end());
	for (NBodyBodyState& body : state) {
		body.acceleration[0] = 0.0;
		body.acceleration[1] = 0.0;
		body.acceleration[2] = 0.0;
		body.potential = 0.0;
	}
	const auto total = static_cast<uint32_t>(count);
	evaluate_nbody_rows(state, 0U, total, kNBodyPhaseNewtonian, options.gravitational_constant, options.speed_of_light, output);
	if (!options.post_newtonian) {
		return true;
	}
	for (size_t i = 0; i < count; ++i) {
		state[i].acceleration[0] = output[i].acceleration[0];
		state[i].acceleration[1] = output[i].acceleration[1];
		state[i].acceleration[2] = output[i].acceleration[2];
		state[i].potential = output[i].potential;
	}
	std::vector<NBodyRowResult> corrected(count);
	evaluate_nbody_rows(state, 0U, total, kNBodyPhasePostNewtonian, options.gravitational_constant, options.speed_of_light, corrected);
	for (size_t i = 0; i < count; ++i) {
		if (!all_finite(corrected[i])) {
			return false;
		}
		output[i] = corrected[i];
	}
	return true;
}

bool validate_ensemble_spec(const GeodesicEnsembleSpec& spec) noexcept {
	if (!std::isfinite(spec.mass) || !(spec.mass > 0.0) || !std::isfinite(spec.observer_radius) || !std::isfinite(spec.escape_radius)) {
		return false;
	}
	if (!(spec.observer_radius > 2.04 * spec.mass) || !(spec.escape_radius > spec.observer_radius)) {
		return false;
	}
	if (!std::isfinite(spec.launch_angle_rad) || !std::isfinite(spec.angular_sigma_rad) || spec.angular_sigma_rad < 0.0) {
		return false;
	}
	if (!(spec.step_size_factor > 0.0) || !(spec.min_step_size > 0.0) || !(spec.max_step_size >= spec.min_step_size) || !std::isfinite(spec.max_step_size)) {
		return false;
	}
	return spec.total_samples > 0U && spec.max_steps >= 1U && spec.max_steps <= kEnsembleMaximumSteps;
}

GeodesicEnsemblePartial integrate_geodesic_ensemble(const GeodesicEnsembleSpec& spec, uint64_t first_sample, uint32_t sample_count) noexcept {
	GeodesicEnsemblePartial partial{};
	const double mass = std::max(spec.mass, 1.0e-6);
	const double horizon = 2.0 * mass;
	const double half_pi = std::numbers::pi_v<double> * 0.5;
	const double factor_observer = std::max(1.0 - horizon / spec.observer_radius, 1.0e-6);
	const double sqrt_factor_observer = std::sqrt(factor_observer);

	for (uint32_t s = 0; s < sample_count; ++s) {
		Core::PCG64Engine engine(spec.seed, first_sample + static_cast<uint64_t>(s));
		const auto uniform = [&engine]() noexcept {
			return (static_cast<double>(engine() >> 11U) + 0.5) * kInverseTwoPow53;
		};
		const double u1 = uniform();
		const double u2 = uniform();
		const double gaussian = std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * std::numbers::pi_v<double> * u2);
		const double angle = spec.launch_angle_rad + spec.angular_sigma_rad * gaussian;

		Core::SphericalNullState<double> state{
			spec.observer_radius, half_pi, 0.0,
			-std::cos(angle), 0.0, -std::sin(angle) / (spec.observer_radius * sqrt_factor_observer)
		};

		double minimum_radius = spec.observer_radius;
		uint32_t steps = 0;
		int outcome = 0;
		for (uint32_t step = 0; step < spec.max_steps; ++step) {
			steps = step + 1U;
			const double radius = state.r;
			if (radius <= horizon * 1.0001) {
				outcome = 1;
				break;
			}
			if (radius >= spec.escape_radius) {
				outcome = 2;
				break;
			}
			const double scale = std::max(radius - horizon, 0.02 * mass);
			const double size = std::clamp(spec.step_size_factor * std::sqrt(radius * scale), spec.min_step_size, spec.max_step_size);
			Core::step_schwarzschild_null_rk4<double>(state, -size, horizon);
			if (!std::isfinite(state.r) || !std::isfinite(state.theta) || !std::isfinite(state.phi)) {
				outcome = 1;
				break;
			}
			if (state.r < minimum_radius) {
				minimum_radius = state.r;
			}
		}

		++partial.samples;
		const double count = static_cast<double>(partial.samples);
		const double radius_delta = minimum_radius - partial.mean_minimum_radius;
		partial.mean_minimum_radius += radius_delta / count;
		partial.m2_minimum_radius += radius_delta * (minimum_radius - partial.mean_minimum_radius);
		const double step_delta = static_cast<double>(steps) - partial.mean_steps;
		partial.mean_steps += step_delta / count;
		partial.m2_steps += step_delta * (static_cast<double>(steps) - partial.mean_steps);

		if (outcome == 1) {
			++partial.absorbed;
		} else if (outcome == 2) {
			++partial.escaped;
			const double sin_theta = std::sin(state.theta);
			const double position[3]{state.r * sin_theta * std::cos(state.phi), state.r * sin_theta * std::sin(state.phi), state.r * std::cos(state.theta)};
			for (size_t axis = 0; axis < 3; ++axis) {
				if (partial.has_bounds == 0U || position[axis] < partial.bounds_min[axis]) {
					partial.bounds_min[axis] = position[axis];
				}
				if (partial.has_bounds == 0U || position[axis] > partial.bounds_max[axis]) {
					partial.bounds_max[axis] = position[axis];
				}
			}
			partial.has_bounds = 1U;
		} else {
			++partial.saturated;
		}
	}
	return partial;
}

void merge_ensemble_partials(GeodesicEnsemblePartial& accumulator, const GeodesicEnsemblePartial& addition) noexcept {
	if (addition.samples == 0U) {
		return;
	}
	if (accumulator.samples == 0U) {
		accumulator = addition;
		return;
	}
	const double na = static_cast<double>(accumulator.samples);
	const double nb = static_cast<double>(addition.samples);
	const double total = na + nb;
	const double radius_delta = addition.mean_minimum_radius - accumulator.mean_minimum_radius;
	const double step_delta = addition.mean_steps - accumulator.mean_steps;
	accumulator.m2_minimum_radius += addition.m2_minimum_radius + radius_delta * radius_delta * na * nb / total;
	accumulator.m2_steps += addition.m2_steps + step_delta * step_delta * na * nb / total;
	accumulator.mean_minimum_radius += radius_delta * nb / total;
	accumulator.mean_steps += step_delta * nb / total;
	accumulator.samples += addition.samples;
	accumulator.absorbed += addition.absorbed;
	accumulator.escaped += addition.escaped;
	accumulator.saturated += addition.saturated;
	if (addition.has_bounds != 0U) {
		for (size_t axis = 0; axis < 3; ++axis) {
			if (accumulator.has_bounds == 0U || addition.bounds_min[axis] < accumulator.bounds_min[axis]) {
				accumulator.bounds_min[axis] = addition.bounds_min[axis];
			}
			if (accumulator.has_bounds == 0U || addition.bounds_max[axis] > accumulator.bounds_max[axis]) {
				accumulator.bounds_max[axis] = addition.bounds_max[axis];
			}
		}
		accumulator.has_bounds = 1U;
	}
}

bool run_geodesic_ensemble_local(const GeodesicEnsembleSpec& spec, uint32_t samples_per_unit, GeodesicEnsemblePartial& result) {
	result = GeodesicEnsemblePartial{};
	if (!validate_ensemble_spec(spec) || samples_per_unit == 0U) {
		return false;
	}
	const uint32_t unit_samples = std::min(samples_per_unit, kEnsembleMaximumSamplesPerUnit);
	const uint64_t total = spec.total_samples;
	const uint64_t unit_count = (total + unit_samples - 1U) / unit_samples;
	const size_t hardware_threads = std::max<size_t>(1U, std::thread::hardware_concurrency());

	if (unit_count > 1 && hardware_threads > 1) {
		std::vector<GeodesicEnsemblePartial> partials(static_cast<size_t>(unit_count));
		std::atomic<size_t> cursor{0};
		const auto worker = [&]() noexcept {
			for (;;) {
				const size_t u = cursor.fetch_add(1U, std::memory_order_relaxed);
				if (u >= unit_count) {
					return;
				}
				const uint64_t first = static_cast<uint64_t>(u) * unit_samples;
				const auto count = static_cast<uint32_t>(std::min<uint64_t>(unit_samples, total - first));
				partials[u] = integrate_geodesic_ensemble(spec, first, count);
			}
		};
		const size_t thread_total = std::min<size_t>(static_cast<size_t>(unit_count), hardware_threads);
		std::vector<std::jthread> threads;
		threads.reserve(thread_total - 1);
		for (size_t t = 1; t < thread_total; ++t) {
			threads.emplace_back(worker);
		}
		worker();
		threads.clear();
		for (const auto& partial : partials) {
			merge_ensemble_partials(result, partial);
		}
	} else {
		for (uint64_t first = 0; first < total; first += unit_samples) {
			const auto count = static_cast<uint32_t>(std::min<uint64_t>(unit_samples, total - first));
			merge_ensemble_partials(result, integrate_geodesic_ensemble(spec, first, count));
		}
	}
	return true;
}

NBodyStateStore::IngestResult NBodyStateStore::ingest(std::span<const uint8_t> payload, uint32_t& completed_job) {
	if (payload.size() < sizeof(NBodyStateChunkHeader)) {
		return IngestResult::Invalid;
	}
	NBodyStateChunkHeader header{};
	std::memcpy(&header, payload.data(), sizeof(header));
	if (header.base.kind != static_cast<uint32_t>(ComputeMessageKind::StateChunk)) {
		return IngestResult::Invalid;
	}
	const size_t body_bytes = payload.size() - sizeof(header);
	if (header.total_bodies == 0U || header.total_bodies > kNBodyMaximumBodies || header.chunk_bodies == 0U || header.chunk_bodies > kNBodyBodiesPerChunk) {
		return IngestResult::Invalid;
	}
	if (body_bytes != static_cast<size_t>(header.chunk_bodies) * sizeof(NBodyBodyState)) {
		return IngestResult::Invalid;
	}
	if (static_cast<size_t>(header.first_index) + header.chunk_bodies > header.total_bodies) {
		return IngestResult::Invalid;
	}
	for (const auto& entry : published_) {
		if (entry.first == header.base.job_id) {
			return IngestResult::Accepted;
		}
	}
	auto it = std::find_if(pending_.begin(), pending_.end(), [&header](const Pending& candidate) noexcept {
		return candidate.job_id == header.base.job_id;
	});
	if (it == pending_.end()) {
		if (pending_.size() >= kNBodyRetainedJobs) {
			pending_.pop_front();
		}
		Pending fresh{};
		fresh.job_id = header.base.job_id;
		fresh.total = header.total_bodies;
		fresh.bodies.resize(header.total_bodies);
		fresh.received.assign(header.total_bodies, 0U);
		pending_.push_back(std::move(fresh));
		it = std::prev(pending_.end());
	} else if (it->total != header.total_bodies) {
		return IngestResult::Invalid;
	}
	const uint8_t* source = payload.data() + sizeof(header);
	for (uint32_t i = 0; i < header.chunk_bodies; ++i) {
		const size_t index = static_cast<size_t>(header.first_index) + i;
		if (it->received[index] != 0U) {
			continue;
		}
		std::memcpy(&it->bodies[index], source + static_cast<size_t>(i) * sizeof(NBodyBodyState), sizeof(NBodyBodyState));
		it->received[index] = 1U;
		++it->received_count;
	}
	if (it->received_count != it->total) {
		return IngestResult::Accepted;
	}
	completed_job = it->job_id;
	auto shared = std::make_shared<const std::vector<NBodyBodyState>>(std::move(it->bodies));
	pending_.erase(it);
	published_.emplace_back(completed_job, std::move(shared));
	while (published_.size() > kNBodyRetainedJobs) {
		published_.pop_front();
	}
	return IngestResult::Completed;
}

NBodyStateStore::SharedState NBodyStateStore::find(uint32_t job_id) const {
	for (auto it = published_.rbegin(); it != published_.rend(); ++it) {
		if (it->first == job_id) {
			return it->second;
		}
	}
	return nullptr;
}

void NBodyStateStore::clear() noexcept {
	pending_.clear();
	published_.clear();
}

bool validate_stochastic_spec(const StochasticEvaluationSpec& spec) noexcept {
	if (spec.total_quadrature_points == 0U || spec.dimension == 0U || spec.dimension > 4U) {
		return false;
	}
	if (!std::isfinite(spec.integration_step) || !(spec.integration_step > 0.0) || spec.max_steps == 0U || spec.max_steps > kEnsembleMaximumSteps) {
		return false;
	}
	for (size_t i = 0; i < spec.dimension; ++i) {
		if (!std::isfinite(spec.parameter_means[i]) || !std::isfinite(spec.parameter_sigmas[i]) || spec.parameter_sigmas[i] < 0.0) {
			return false;
		}
	}
	return true;
}

void evaluate_stochastic_quadrature_range(
	const StochasticEvaluationSpec& spec,
	uint32_t first_point,
	uint32_t point_count,
	std::span<StochasticQuadratureResult> output
) noexcept {
	const size_t run = std::min<size_t>(point_count, output.size());
	for (size_t i = 0; i < run; ++i) {
		const uint32_t point = first_point + static_cast<uint32_t>(i);
		StochasticQuadratureResult result{};
		result.point_index = point;
		const double mass = std::max(spec.parameter_means[0] + spec.parameter_sigmas[0] * (static_cast<double>(point % 17U) - 8.0) * 0.125, 1.0e-4);
		const double horizon = 2.0 * mass;
		const double observer_radius = 50.0 * mass;
		const double factor = std::max(1.0 - horizon / observer_radius, 1.0e-6);
		const double angle = 0.15 + (static_cast<double>(point) / static_cast<double>(std::max(spec.total_quadrature_points, 1U))) * 0.05;
		Core::SphericalNullState<double> state{
			observer_radius, std::numbers::pi_v<double> * 0.5, 0.0,
			-std::cos(angle), 0.0, -std::sin(angle) / (observer_radius * std::sqrt(factor))
		};
		double min_r = observer_radius;
		uint32_t status = 0U;
		for (uint32_t step = 0; step < spec.max_steps; ++step) {
			if (state.r <= horizon * 1.0001) {
				status = 1U;
				break;
			}
			if (state.r >= 200.0 * mass) {
				status = 2U;
				break;
			}
			const double scale = std::max(state.r - horizon, 0.02 * mass);
			const double dt = -std::clamp(spec.integration_step * std::sqrt(state.r * scale), 0.001, 2.0);
			Core::step_schwarzschild_null_rk4<double>(state, dt, horizon);
			if (state.r < min_r) {
				min_r = state.r;
			}
		}
		result.response_value = (status == 2U) ? (observer_radius / min_r) : 0.0;
		result.secondary_value = min_r;
		result.status = status;
		output[i] = result;
	}
}

bool run_stochastic_pce_quadrature_local(
	const StochasticEvaluationSpec& spec,
	uint32_t points_per_unit,
	std::span<StochasticQuadratureResult> results
) {
	if (!validate_stochastic_spec(spec) || points_per_unit == 0U || results.size() < spec.total_quadrature_points) {
		return false;
	}
	const uint32_t total = spec.total_quadrature_points;
	const uint32_t unit_count = (total + points_per_unit - 1U) / points_per_unit;
	const size_t hardware_threads = std::max<size_t>(1U, std::thread::hardware_concurrency());

	if (unit_count > 1 && hardware_threads > 1) {
		std::atomic<size_t> cursor{0};
		const auto worker = [&]() noexcept {
			for (;;) {
				const size_t u = cursor.fetch_add(1U, std::memory_order_relaxed);
				if (u >= unit_count) {
					return;
				}
				const auto first = static_cast<uint32_t>(u * points_per_unit);
				const auto count = std::min(points_per_unit, total - first);
				evaluate_stochastic_quadrature_range(spec, first, count, results.subspan(first, count));
			}
		};
		const size_t thread_total = std::min<size_t>(static_cast<size_t>(unit_count), hardware_threads);
		std::vector<std::jthread> threads;
		threads.reserve(thread_total - 1);
		for (size_t t = 1; t < thread_total; ++t) {
			threads.emplace_back(worker);
		}
		worker();
	} else {
		for (uint32_t first = 0; first < total; first += points_per_unit) {
			const uint32_t count = std::min(points_per_unit, total - first);
			evaluate_stochastic_quadrature_range(spec, first, count, results.subspan(first, count));
		}
	}
	return true;
}

}
