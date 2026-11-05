#pragma once

#include <cstdint>

namespace Relativistic::Distributed {

struct ClusterFrameTelemetry {
	uint64_t frame_index{0};
	uint32_t tiles_total{0};
	uint32_t tiles_remote{0};
	uint32_t tiles_local{0};
	uint32_t tiles_reclaimed{0};
	uint32_t workers_active{0};
	uint64_t bytes_received{0};
	double dispatch_ms{0.0};
	double local_render_ms{0.0};
	double decode_ms{0.0};
	double frame_ms{0.0};
	double round_trip_mean_ms{0.0};
	bool complete{false};
	bool offline{false};
};

}
