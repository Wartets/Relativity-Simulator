#pragma once

#include "relativistic/distributed/compute_payloads.hpp"
#include <cstdint>
#include <deque>
#include <memory>
#include <span>
#include <utility>
#include <vector>

namespace Relativistic::Distributed {

inline constexpr size_t kNBodyRetainedJobs = 4;

void evaluate_nbody_rows(
	std::span<const NBodyBodyState> bodies,
	uint32_t first_row,
	uint32_t row_count,
	uint32_t phase,
	double gravitational_constant,
	double speed_of_light,
	std::span<NBodyRowResult> output
) noexcept;

[[nodiscard]] bool evaluate_nbody_accelerations_local(
	std::span<const NBodyBodyState> bodies,
	const NBodyEvaluationOptions& options,
	std::span<NBodyRowResult> output
);

[[nodiscard]] bool validate_ensemble_spec(const GeodesicEnsembleSpec& spec) noexcept;

[[nodiscard]] GeodesicEnsemblePartial integrate_geodesic_ensemble(
	const GeodesicEnsembleSpec& spec,
	uint64_t first_sample,
	uint32_t sample_count
) noexcept;

void merge_ensemble_partials(GeodesicEnsemblePartial& accumulator, const GeodesicEnsemblePartial& addition) noexcept;

[[nodiscard]] bool run_geodesic_ensemble_local(
	const GeodesicEnsembleSpec& spec,
	uint32_t samples_per_unit,
	GeodesicEnsemblePartial& result
);

[[nodiscard]] bool validate_stochastic_spec(const StochasticEvaluationSpec& spec) noexcept;

void evaluate_stochastic_quadrature_range(
	const StochasticEvaluationSpec& spec,
	uint32_t first_point,
	uint32_t point_count,
	std::span<StochasticQuadratureResult> output
) noexcept;

[[nodiscard]] bool run_stochastic_pce_quadrature_local(
	const StochasticEvaluationSpec& spec,
	uint32_t points_per_unit,
	std::span<StochasticQuadratureResult> results
);

class NBodyStateStore {
public:
	using SharedState = std::shared_ptr<const std::vector<NBodyBodyState>>;

	enum class IngestResult : uint8_t {
		Accepted = 0,
		Completed = 1,
		Invalid = 2
	};

private:
	struct Pending {
		uint32_t job_id{0};
		uint32_t total{0};
		uint32_t received_count{0};
		std::vector<NBodyBodyState> bodies{};
		std::vector<uint8_t> received{};
	};

	std::deque<Pending> pending_{};
	std::deque<std::pair<uint32_t, SharedState>> published_{};

public:
	[[nodiscard]] IngestResult ingest(std::span<const uint8_t> payload, uint32_t& completed_job);
	[[nodiscard]] SharedState find(uint32_t job_id) const;
	void clear() noexcept;
};

}
