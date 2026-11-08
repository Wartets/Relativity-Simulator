#pragma once

#include <array>
#include <cmath>

namespace Relativistic::Render {

class BodyOrientation {
public:
	using Quaternion = std::array<double, 4>;
	using Vector = std::array<double, 3>;

	[[nodiscard]] static Quaternion normalized(const Quaternion& q) noexcept {
		const double norm = std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
		if (!(norm > 1e-12)) {
			return {1.0, 0.0, 0.0, 0.0};
		}
		return {q[0] / norm, q[1] / norm, q[2] / norm, q[3] / norm};
	}

	[[nodiscard]] static Vector rotate(const Quaternion& q, const Vector& v) noexcept {
		const double tx = q[2] * v[2] - q[3] * v[1] + q[0] * v[0];
		const double ty = q[3] * v[0] - q[1] * v[2] + q[0] * v[1];
		const double tz = q[1] * v[1] - q[2] * v[0] + q[0] * v[2];
		return {
			v[0] + 2.0 * (q[2] * tz - q[3] * ty),
			v[1] + 2.0 * (q[3] * tx - q[1] * tz),
			v[2] + 2.0 * (q[1] * ty - q[2] * tx)
		};
	}

	[[nodiscard]] static Vector rotate_inverse(const Quaternion& q, const Vector& v) noexcept {
		return rotate(Quaternion{q[0], -q[1], -q[2], -q[3]}, v);
	}
};

}
