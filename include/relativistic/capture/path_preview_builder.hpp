#pragma once

#include "relativistic/capture/motion_script.hpp"
#include "relativistic/capture/path_preview.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include <algorithm>
#include <cstdint>
#include <mutex>
#include <numbers>
#include <optional>

namespace Relativistic::Capture {

struct PathPreviewOptions {
	uint32_t samples_per_segment{128};
	double frustum_length{10.0};
	bool show_markers{true};
	bool show_labels{true};
	bool show_samples{false};
	bool show_frustum{true};
	bool include_events{true};
	std::optional<double> cursor_seconds{};
	int32_t highlighted_segment{-1};
};

[[nodiscard]] inline BodyPositionLookup make_body_position_lookup(Orchestrator::SimulationOrchestrator<1024>& orchestrator) {
	return [&orchestrator](int32_t id) -> std::optional<Vec3> {
		std::lock_guard<std::recursive_mutex> lock(orchestrator.nbody_system().bodies_mutex());
		for (const auto& body : orchestrator.nbody_system().bodies()) {
			if (static_cast<int32_t>(body.id) == id) {
				return body.position;
			}
		}
		return std::nullopt;
	};
}

[[nodiscard]] inline PathPreviewCursor build_preview_cursor(const MotionScript& script, const BodyPositionLookup& lookup, double seconds) {
	PathPreviewCursor cursor;
	const ScriptSample sample = script.sample(seconds, lookup);
	if (!sample.valid) {
		return cursor;
	}
	Orchestrator::CameraState state;
	state.pitch = sample.pose.pitch_deg;
	state.yaw = sample.pose.yaw_deg;
	state.roll = sample.pose.roll_deg;
	const auto basis = state.orientation_basis();
	cursor.valid = true;
	cursor.position = sample.pose.position;
	cursor.forward = basis.forward;
	cursor.right = basis.right;
	cursor.up = basis.up;
	cursor.fov_rad = std::clamp(sample.pose.fov_deg * (std::numbers::pi_v<double> / 180.0), 0.02, 3.0);
	return cursor;
}

[[nodiscard]] inline PathPreview build_path_preview(const MotionScript& script, const BodyPositionLookup& lookup, const PathPreviewOptions& options) {
	PathPreview preview;
	preview.frustum_length = options.frustum_length;
	preview.highlighted_segment = options.highlighted_segment;
	preview.show_markers = options.show_markers;
	preview.show_labels = options.show_labels;
	preview.show_samples = options.show_samples;
	preview.show_frustum = options.show_frustum;

	const std::vector<size_t> active = script.active_segments();
	if (active.empty()) {
		return preview;
	}

	const uint32_t samples = std::clamp<uint32_t>(options.samples_per_segment, 4U, 4096U);
	preview.vertices.reserve(active.size() * (static_cast<size_t>(samples) + 1U));

	for (size_t slot = 0; slot < active.size(); ++slot) {
		const size_t segment_index = active[slot];
		const ScriptSegment& segment = script.segments[segment_index];
		const double start_time = script.segment_start_time(segment_index);
		const Vec3 anchor = script.evaluate_segment_anchor(segment_index, lookup);
		const size_t first_vertex = preview.vertices.size();

		for (uint32_t i = 0; i <= samples; ++i) {
			const double linear = static_cast<double>(i) / static_cast<double>(samples);
			const double progress = segment.time_easing.evaluate(linear);
			const double local_seconds = linear * segment.duration;
			const Vec3 raw = script.evaluate_segment_raw_position(segment_index, progress, local_seconds, start_time + local_seconds);
			preview.vertices.push_back(PathPreviewVertex{ScriptMath::add(anchor, raw), static_cast<uint32_t>(slot)});
		}

		if (slot > 0) {
			preview.markers.push_back(PathPreviewMarker{preview.vertices[first_vertex].position, PathPreviewMarkerKind::SegmentBoundary, segment.name});
		}
	}

	preview.markers.push_back(PathPreviewMarker{preview.vertices.front().position, PathPreviewMarkerKind::Start, "Start"});
	preview.markers.push_back(PathPreviewMarker{preview.vertices.back().position, PathPreviewMarkerKind::End, "End"});

	if (options.include_events) {
		for (const ScriptEvent& event : script.events) {
			if (!event.enabled) {
				continue;
			}
			const ScriptSample sample = script.sample(script.resolved_event_time(event), lookup);
			if (sample.valid) {
				preview.markers.push_back(PathPreviewMarker{sample.pose.position, PathPreviewMarkerKind::Event, event.display_label()});
			}
		}
	}

	if (options.cursor_seconds.has_value()) {
		preview.cursor = build_preview_cursor(script, lookup, *options.cursor_seconds);
	}
	return preview;
}

}
