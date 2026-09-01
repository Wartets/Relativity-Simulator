#pragma once

#include "relativistic/uncertainty/interval.hpp"
#include "relativistic/uncertainty/zonotope.hpp"
#include "relativistic/core/constants.hpp"
#include <vector>
#include <array>
#include <cmath>
#include <numbers>

namespace Relativistic::Uncertainty {

struct WrappingBenchmarkResult {
	double interval_final_hypervolume{0.0};
	double zonotope_final_hypervolume{0.0};
	double volume_reduction_factor{1.0};
	size_t step_count{0};
};

class OrbitalWrappingBenchmark {
public:
	[[nodiscard]] static WrappingBenchmarkResult run_orbital_comparison_2d(
		double radius,
		double initial_uncertainty,
		double orbital_revolutions = 100.0,
		size_t steps_per_revolution = 64
	) noexcept {
		const size_t total_steps = static_cast<size_t>(orbital_revolutions * static_cast<double>(steps_per_revolution));
		const double d_theta = (2.0 * std::numbers::pi) / static_cast<double>(steps_per_revolution);
		const double cos_dt = std::cos(d_theta);
		const double sin_dt = std::sin(d_theta);

		const std::array<std::array<double, 2>, 2> rot_mat{{
			{cos_dt, -sin_dt},
			{sin_dt, cos_dt}
		}};

		std::array<Interval<double>, 2> intv_box{
			Interval<double>(radius - initial_uncertainty, radius + initial_uncertainty),
			Interval<double>(-initial_uncertainty, initial_uncertainty)
		};

		Zonotope<double, 2> zono = Zonotope<double, 2>::from_intervals(intv_box);

		for (size_t step = 0; step < total_steps; ++step) {
			const Interval<double> next_x = intv_box[0] * cos_dt - intv_box[1] * sin_dt;
			const Interval<double> next_y = intv_box[0] * sin_dt + intv_box[1] * cos_dt;
			intv_box[0] = next_x;
			intv_box[1] = next_y;

			zono = zono.linear_transform(rot_mat);
			if (zono.generator_count() > 16) {
				zono.reduce_girard(8);
			}
		}

		const double intv_vol = intv_box[0].width() * intv_box[1].width();
		const double zono_vol = zono.bounding_box_hypervolume();
		const double reduction = (zono_vol > 0.0) ? (intv_vol / zono_vol) : 1.0;

		return WrappingBenchmarkResult{
			.interval_final_hypervolume = intv_vol,
			.zonotope_final_hypervolume = zono_vol,
			.volume_reduction_factor = reduction,
			.step_count = total_steps
		};
	}
};

}
