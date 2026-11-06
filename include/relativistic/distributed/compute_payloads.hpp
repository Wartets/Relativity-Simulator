#pragma once

#include "relativistic/distributed/packet_protocol.hpp"
#include <cstddef>
#include <cstdint>

namespace Relativistic::Distributed {

enum class ComputeMessageKind : uint32_t {
	StateChunk = 1,
	BlockTask = 2,
	BlockResult = 3
};

inline constexpr uint32_t kNBodyPhaseNewtonian = 0;
inline constexpr uint32_t kNBodyPhasePostNewtonian = 1;
inline constexpr uint32_t kNBodyBodiesPerChunk = 512;
inline constexpr uint32_t kNBodyMaximumBodies = 65536;
inline constexpr uint32_t kNBodyMaximumRowsPerUnit = 4096;
inline constexpr uint32_t kEnsembleMaximumSamplesPerUnit = 1U << 20U;
inline constexpr uint32_t kEnsembleMaximumSteps = 1U << 20U;

struct ComputeMessageHeader {
	uint32_t kind{0};
	uint32_t job_id{0};
	uint32_t unit_index{0};
	uint32_t reserved{0};
};

struct NBodyBodyState {
	double position[3]{0.0, 0.0, 0.0};
	double velocity[3]{0.0, 0.0, 0.0};
	double mass{0.0};
	double acceleration[3]{0.0, 0.0, 0.0};
	double potential{0.0};
};

struct NBodyRowResult {
	double acceleration[3]{0.0, 0.0, 0.0};
	double potential{0.0};
};

struct NBodyStateChunkHeader {
	ComputeMessageHeader base{};
	uint32_t total_bodies{0};
	uint32_t first_index{0};
	uint32_t chunk_bodies{0};
	uint32_t reserved{0};
};

struct NBodyBlockTask {
	ComputeMessageHeader base{};
	uint32_t first_row{0};
	uint32_t row_count{0};
	uint32_t total_bodies{0};
	uint32_t phase{0};
	double gravitational_constant{1.0};
	double speed_of_light{1.0};
};

struct NBodyEvaluationOptions {
	double gravitational_constant{1.0};
	double speed_of_light{1.0};
	bool post_newtonian{true};
	uint32_t rows_per_unit{256};
};

struct GeodesicEnsembleSpec {
	double mass{1.0};
	double observer_radius{50.0};
	double escape_radius{200.0};
	double launch_angle_rad{0.1};
	double angular_sigma_rad{0.0};
	double step_size_factor{0.45};
	double min_step_size{0.0008};
	double max_step_size{38.0};
	uint64_t seed{0};
	uint64_t total_samples{0};
	uint32_t max_steps{4096};
	uint32_t reserved{0};
};

struct GeodesicEnsembleTask {
	ComputeMessageHeader base{};
	GeodesicEnsembleSpec spec{};
	uint64_t first_sample{0};
	uint32_t sample_count{0};
	uint32_t reserved{0};
};

struct GeodesicEnsemblePartial {
	uint64_t samples{0};
	uint64_t absorbed{0};
	uint64_t escaped{0};
	uint64_t saturated{0};
	double mean_minimum_radius{0.0};
	double m2_minimum_radius{0.0};
	double mean_steps{0.0};
	double m2_steps{0.0};
	double bounds_min[3]{0.0, 0.0, 0.0};
	double bounds_max[3]{0.0, 0.0, 0.0};
	uint32_t has_bounds{0};
	uint32_t reserved{0};
};

static_assert(sizeof(ComputeMessageHeader) == 16);
static_assert(sizeof(NBodyBodyState) == 88);
static_assert(sizeof(NBodyRowResult) == 32);
static_assert(sizeof(NBodyStateChunkHeader) == 32);
static_assert(sizeof(NBodyBlockTask) == 48);
static_assert(sizeof(GeodesicEnsembleSpec) == 88);
static_assert(sizeof(GeodesicEnsembleTask) == 120);
static_assert(sizeof(GeodesicEnsemblePartial) == 120);
struct StochasticEvaluationSpec {
	uint32_t dimension{1};
	uint32_t polynomial_order{3};
	uint32_t total_quadrature_points{0};
	uint32_t evaluation_mode{0};
	double parameter_means[4]{1.0, 0.0, 0.0, 0.0};
	double parameter_sigmas[4]{0.1, 0.0, 0.0, 0.0};
	double integration_step{0.05};
	uint32_t max_steps{2048};
	uint32_t reserved{0};
};

struct StochasticQuadratureTask {
	ComputeMessageHeader base{};
	StochasticEvaluationSpec spec{};
	uint32_t first_point{0};
	uint32_t point_count{0};
	uint32_t reserved{0};
};

struct StochasticQuadratureResult {
	uint32_t point_index{0};
	uint32_t status{0};
	double response_value{0.0};
	double secondary_value{0.0};
};

static_assert(sizeof(StochasticEvaluationSpec) == 96);
static_assert(sizeof(StochasticQuadratureTask) == 128);
static_assert(sizeof(StochasticQuadratureResult) == 24);
static_assert(DistributedPayload<NBodyBodyState>);
static_assert(DistributedPayload<NBodyBlockTask>);
static_assert(DistributedPayload<GeodesicEnsembleTask>);
static_assert(DistributedPayload<GeodesicEnsemblePartial>);
static_assert(DistributedPayload<StochasticQuadratureTask>);
static_assert(DistributedPayload<StochasticQuadratureResult>);

}
