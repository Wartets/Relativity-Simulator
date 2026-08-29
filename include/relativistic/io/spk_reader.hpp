#pragma once

#include "relativistic/io/ephemeris_types.hpp"
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <vector>
#include <span>
#include <array>
#include <optional>
#include <cmath>
#include <algorithm>

namespace Relativistic::IO {

enum class SpkRecordType : uint32_t {
	ModifiedDifferenceArrays = 1,
	ChebyshevPositionOnly = 2,
	ChebyshevPositionAndVelocity = 3,
	Unknown = 0
};

struct alignas(64) SpkChebyshevSegment {
	uint32_t target_id{0};
	uint32_t center_id{0};
	uint32_t frame_id{1};
	SpkRecordType record_type{SpkRecordType::ChebyshevPositionOnly};
	double initial_epoch_sec{0.0};
	double final_epoch_sec{0.0};
	double initial_epoch_record{0.0};
	double interval_length_sec{0.0};
	size_t coefficients_per_component{0};
	size_t record_count{0};
	std::vector<double> coefficients{};

	[[nodiscard]] bool contains_epoch(double seconds_past_j2000) const noexcept {
		return (seconds_past_j2000 >= initial_epoch_sec) && (seconds_past_j2000 <= final_epoch_sec);
	}

	[[nodiscard]] std::optional<EphemerisStateVector> evaluate(Epoch epoch) const noexcept {
		const double t = epoch.to_j2000_seconds();
		if (!contains_epoch(t)) {
			return std::nullopt;
		}

		if (interval_length_sec <= 0.0 || coefficients_per_component == 0 || record_count == 0) {
			return std::nullopt;
		}

		const double offset = t - initial_epoch_record;
		size_t rec_idx = static_cast<size_t>(std::floor(offset / interval_length_sec));
		if (rec_idx >= record_count) {
			rec_idx = record_count - 1;
		}

		const double rec_start = initial_epoch_record + static_cast<double>(rec_idx) * interval_length_sec;
		const double rec_end = rec_start + interval_length_sec;
		const double s = (2.0 * t - (rec_start + rec_end)) / interval_length_sec;
		const double clamped_s = std::clamp(s, -1.0, 1.0);

		const size_t n_coeff = coefficients_per_component;
		const size_t num_components = (record_type == SpkRecordType::ChebyshevPositionAndVelocity) ? 6 : 3;
		const size_t stride = num_components * n_coeff;
		const size_t rec_offset = rec_idx * stride;

		if (rec_offset + num_components * n_coeff > coefficients.size()) {
			return std::nullopt;
		}

		std::array<double, 64> t_poly{};
		std::array<double, 64> u_poly{};
		if (n_coeff > t_poly.size()) {
			return std::nullopt;
		}

		t_poly[0] = 1.0;
		if (n_coeff > 1) {
			t_poly[1] = clamped_s;
			for (size_t i = 2; i < n_coeff; ++i) {
				t_poly[i] = 2.0 * clamped_s * t_poly[i - 1] - t_poly[i - 2];
			}
		}

		u_poly[0] = 0.0;
		if (n_coeff > 1) {
			u_poly[1] = 1.0;
			if (n_coeff > 2) {
				u_poly[2] = 4.0 * clamped_s;
				for (size_t i = 3; i < n_coeff; ++i) {
					u_poly[i] = 2.0 * t_poly[i - 1] + 2.0 * clamped_s * u_poly[i - 1] - u_poly[i - 2];
				}
			}
		}

		std::array<double, 3> pos{0.0, 0.0, 0.0};
		std::array<double, 3> vel{0.0, 0.0, 0.0};

		const double dt_ds = 2.0 / interval_length_sec;

		for (size_t c = 0; c < 3; ++c) {
			const size_t coeff_start = rec_offset + c * n_coeff;
			double p_val = 0.0;
			double v_val = 0.0;
			for (size_t i = 0; i < n_coeff; ++i) {
				const double a_i = coefficients[coeff_start + i];
				p_val += a_i * t_poly[i];
				v_val += a_i * u_poly[i] * dt_ds;
			}
			pos[c] = p_val * 1000.0;
			vel[c] = v_val * 1000.0;
		}

		if (record_type == SpkRecordType::ChebyshevPositionAndVelocity) {
			for (size_t c = 0; c < 3; ++c) {
				const size_t coeff_start = rec_offset + (c + 3) * n_coeff;
				double v_val = 0.0;
				for (size_t i = 0; i < n_coeff; ++i) {
					v_val += coefficients[coeff_start + i] * t_poly[i];
				}
				vel[c] = v_val * 1000.0;
			}
		}

		return EphemerisStateVector(epoch, pos[0], pos[1], pos[2], vel[0], vel[1], vel[2], target_id, center_id);
	}
};

class SpkKernel {
private:
	std::vector<SpkChebyshevSegment> segments_;

public:
	SpkKernel() = default;

	void add_segment(SpkChebyshevSegment&& segment) {
		segments_.push_back(std::move(segment));
	}

	void add_segment(const SpkChebyshevSegment& segment) {
		segments_.push_back(segment);
	}

	[[nodiscard]] size_t segment_count() const noexcept {
		return segments_.size();
	}

	[[nodiscard]] const std::vector<SpkChebyshevSegment>& segments() const noexcept {
		return segments_;
	}

	[[nodiscard]] std::optional<EphemerisStateVector> evaluate_body(uint32_t target_id, Epoch epoch) const noexcept {
		const double t = epoch.to_j2000_seconds();
		for (auto it = segments_.rbegin(); it != segments_.rend(); ++it) {
			if (it->target_id == target_id && it->contains_epoch(t)) {
				return it->evaluate(epoch);
			}
		}
		return std::nullopt;
	}

	[[nodiscard]] std::optional<EphemerisStateVector> evaluate_relative(
		uint32_t target_id,
		uint32_t observer_center_id,
		Epoch epoch
	) const noexcept {
		if (target_id == observer_center_id) {
			return EphemerisStateVector(epoch, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, target_id, observer_center_id);
		}

		const auto target_state = evaluate_body(target_id, epoch);
		if (!target_state.has_value()) {
			return std::nullopt;
		}

		if (target_state->center_id == observer_center_id) {
			return target_state;
		}

		const auto observer_state = evaluate_body(observer_center_id, epoch);
		if (!observer_state.has_value()) {
			return std::nullopt;
		}

		const double rx = target_state->position(1) - observer_state->position(1);
		const double ry = target_state->position(2) - observer_state->position(2);
		const double rz = target_state->position(3) - observer_state->position(3);

		const double vx = target_state->velocity(1) - observer_state->velocity(1);
		const double vy = target_state->velocity(2) - observer_state->velocity(2);
		const double vz = target_state->velocity(3) - observer_state->velocity(3);

		return EphemerisStateVector(epoch, rx, ry, rz, vx, vy, vz, target_id, observer_center_id);
	}

	[[nodiscard]] static std::optional<SpkKernel> parse_daf_spk_bytes(std::span<const uint8_t> bytes) noexcept {
		if (bytes.size() < 1024) {
			return std::nullopt;
		}

		char locidw[9]{};
		std::memcpy(locidw, bytes.data(), 8);
		if (std::strncmp(locidw, "DAF/SPK", 7) != 0) {
			return std::nullopt;
		}

		SpkKernel kernel;
		return kernel;
	}
};

}
