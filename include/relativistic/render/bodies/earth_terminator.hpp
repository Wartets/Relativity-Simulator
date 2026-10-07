#pragma once

#include <algorithm>

namespace Relativistic::Render {

struct EarthTerminator {
	static constexpr float kMinimumHalfWidth = 0.04f;
	static constexpr float kHalfWidthRange = 0.46f;

	[[nodiscard]] static constexpr float half_width(float softness) noexcept {
		return kMinimumHalfWidth + kHalfWidthRange * std::clamp(softness, 0.0f, 1.0f);
	}

	[[nodiscard]] static constexpr float blend_softness(float softness) noexcept {
		return std::min(2.0f * half_width(softness), 1.0f);
	}
};

}
