#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace Relativistic::Capture {

enum class PathPreviewMarkerKind : uint32_t {
	Start = 0,
	End = 1,
	SegmentBoundary = 2,
	Event = 3
};

inline constexpr std::array<std::array<uint8_t, 3>, 8> kPathPreviewPalette{{
	{80, 200, 255},
	{255, 170, 70},
	{130, 235, 120},
	{235, 110, 200},
	{250, 230, 90},
	{150, 130, 255},
	{90, 235, 200},
	{255, 120, 110}
}};

struct PathPreviewVertex {
	std::array<double, 3> position{0.0, 0.0, 0.0};
	uint32_t segment{0};
};

struct PathPreviewMarker {
	std::array<double, 3> position{0.0, 0.0, 0.0};
	PathPreviewMarkerKind kind{PathPreviewMarkerKind::SegmentBoundary};
	std::string label{};
};

struct PathPreviewCursor {
	bool valid{false};
	std::array<double, 3> position{0.0, 0.0, 0.0};
	std::array<double, 3> forward{1.0, 0.0, 0.0};
	std::array<double, 3> right{0.0, -1.0, 0.0};
	std::array<double, 3> up{0.0, 0.0, 1.0};
	double fov_rad{1.0471975511965976};
};

struct PathPreview {
	std::vector<PathPreviewVertex> vertices{};
	std::vector<PathPreviewMarker> markers{};
	PathPreviewCursor cursor{};
	double frustum_length{10.0};
	int32_t highlighted_segment{-1};
	bool show_markers{true};
	bool show_labels{true};
	bool show_samples{false};
	bool show_frustum{true};
	bool show_direction{true};

	[[nodiscard]] bool empty() const noexcept {
		return vertices.empty() && !cursor.valid;
	}
};

}
