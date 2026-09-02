#pragma once

#include "simd.hpp"
#include "simd_math.hpp"
#include "four_vector_bundle.hpp"
#include <array>
#include <cstddef>

namespace Relativistic::Core {

template <typename T, size_t Width>
struct alignas(64) GeodesicBundle {
	static_assert(std::is_floating_point_v<T>, "GeodesicBundle requires floating-point scalar type");
	static_assert((Width & (Width - 1)) == 0 && Width >= 1, "Width must be a power of two");

	static constexpr size_t BUNDLE_SIZE = Width;

	SimdVec<T, Width> x0;
	SimdVec<T, Width> x1;
	SimdVec<T, Width> x2;
	SimdVec<T, Width> x3;

	SimdVec<T, Width> p0;
	SimdVec<T, Width> p1;
	SimdVec<T, Width> p2;
	SimdVec<T, Width> p3;

	SimdVec<T, Width> affine_parameter;
	SimdVec<T, Width> step_size;
	SimdVec<T, Width> optical_depth;
	SimdVec<T, Width> redshift_factor;

	SimdMask<T, Width> active_mask;
	SimdMask<T, Width> horizon_mask;
	SimdMask<T, Width> celestial_mask;

	constexpr GeodesicBundle() noexcept
		: x0(static_cast<T>(0)), x1(static_cast<T>(0)), x2(static_cast<T>(0)), x3(static_cast<T>(0)),
		  p0(static_cast<T>(0)), p1(static_cast<T>(0)), p2(static_cast<T>(0)), p3(static_cast<T>(0)),
		  affine_parameter(static_cast<T>(0)),
		  step_size(static_cast<T>(0.01)),
		  optical_depth(static_cast<T>(0)),
		  redshift_factor(static_cast<T>(1)),
		  active_mask(true),
		  horizon_mask(false),
		  celestial_mask(false) {}

	constexpr void initialize_all(
		const std::array<T, 4>& init_x,
		const std::array<T, 4>& init_p,
		T init_step_size
	) noexcept {
		x0 = SimdVec<T, Width>(init_x[0]);
		x1 = SimdVec<T, Width>(init_x[1]);
		x2 = SimdVec<T, Width>(init_x[2]);
		x3 = SimdVec<T, Width>(init_x[3]);

		p0 = SimdVec<T, Width>(init_p[0]);
		p1 = SimdVec<T, Width>(init_p[1]);
		p2 = SimdVec<T, Width>(init_p[2]);
		p3 = SimdVec<T, Width>(init_p[3]);

		affine_parameter = SimdVec<T, Width>(static_cast<T>(0));
		step_size = SimdVec<T, Width>(init_step_size);
		optical_depth = SimdVec<T, Width>(static_cast<T>(0));
		redshift_factor = SimdVec<T, Width>(static_cast<T>(1));

		active_mask = SimdMask<T, Width>(true);
		horizon_mask = SimdMask<T, Width>(false);
		celestial_mask = SimdMask<T, Width>(false);
	}

	constexpr void set_ray(
		size_t lane,
		const std::array<T, 4>& pos,
		const std::array<T, 4>& mom,
		T dt
	) noexcept {
		x0[lane] = pos[0];
		x1[lane] = pos[1];
		x2[lane] = pos[2];
		x3[lane] = pos[3];

		p0[lane] = mom[0];
		p1[lane] = mom[1];
		p2[lane] = mom[2];
		p3[lane] = mom[3];

		affine_parameter[lane] = static_cast<T>(0);
		step_size[lane] = dt;
		optical_depth[lane] = static_cast<T>(0);
		redshift_factor[lane] = static_cast<T>(1);

		active_mask[lane] = true;
		horizon_mask[lane] = false;
		celestial_mask[lane] = false;
	}

	[[nodiscard]] constexpr std::array<T, 4> get_position(size_t lane) const noexcept {
		return {x0[lane], x1[lane], x2[lane], x3[lane]};
	}

	[[nodiscard]] constexpr std::array<T, 4> get_momentum(size_t lane) const noexcept {
		return {p0[lane], p1[lane], p2[lane], p3[lane]};
	}

	[[nodiscard]] constexpr bool is_lane_active(size_t lane) const noexcept {
		return active_mask[lane];
	}

	[[nodiscard]] constexpr bool is_any_active() const noexcept {
		return active_mask.any();
	}

	[[nodiscard]] constexpr bool is_all_active() const noexcept {
		return active_mask.all();
	}

	[[nodiscard]] constexpr size_t active_count() const noexcept {
		return active_mask.count();
	}

	constexpr void deactivate_lane(size_t lane) noexcept {
		active_mask[lane] = false;
	}

	constexpr void mask_deactivate(const SimdMask<T, Width>& mask) noexcept {
		active_mask &= (!mask);
	}

	constexpr void mask_mark_horizon(const SimdMask<T, Width>& mask) noexcept {
		horizon_mask |= (mask && active_mask);
		active_mask &= (!mask);
	}

	constexpr void mask_mark_celestial(const SimdMask<T, Width>& mask) noexcept {
		celestial_mask |= (mask && active_mask);
		active_mask &= (!mask);
	}

	[[nodiscard]] constexpr SimdVec<T, Width> compute_minkowski_norm_squared(T speed_of_light = static_cast<T>(1)) const noexcept {
		return minkowski_norm_squared(p0, p1, p2, p3, speed_of_light);
	}

	constexpr void set_step_size(const SimdVec<T, Width>& dt) noexcept {
		step_size = dt;
	}

	constexpr void set_step_size_scalar(T dt) noexcept {
		step_size = SimdVec<T, Width>(dt);
	}

	void step_rk4_schwarzschild(
		T mass,
		T speed_of_light = static_cast<T>(1),
		T g_constant = static_cast<T>(1)
	) noexcept {
		const T r_s = static_cast<T>(2) * g_constant * mass / (speed_of_light * speed_of_light);
		const SimdVec<T, Width> v_rs(r_s);
		const SimdVec<T, Width> v_c2(speed_of_light * speed_of_light);
		const SimdVec<T, Width> v_two(static_cast<T>(2));
		const SimdVec<T, Width> v_one(static_cast<T>(1));
		const SimdVec<T, Width> v_half(static_cast<T>(0.5));
		const SimdVec<T, Width> v_sixth(static_cast<T>(1.0 / 6.0));

		const auto eval_derivs = [&](
			const SimdVec<T, Width>& rx0, const SimdVec<T, Width>& rx1,
			const SimdVec<T, Width>& rx2, const SimdVec<T, Width>& rx3,
			const SimdVec<T, Width>& rp0, const SimdVec<T, Width>& rp1,
			const SimdVec<T, Width>& rp2, const SimdVec<T, Width>& rp3,
			SimdVec<T, Width>& dx0, SimdVec<T, Width>& dx1,
			SimdVec<T, Width>& dx2, SimdVec<T, Width>& dx3,
			SimdVec<T, Width>& dp0, SimdVec<T, Width>& dp1,
			SimdVec<T, Width>& dp2, SimdVec<T, Width>& dp3
		) noexcept {
			static_cast<void>(rx0);
			static_cast<void>(rx3);
			dx0 = rp0;
			dx1 = rp1;
			dx2 = rp2;
			dx3 = rp3;

			const auto r = max(rx1, v_rs * static_cast<T>(1.0001));
			const auto theta = rx2;
			const auto sincos_t = simd_sincos(theta);
			const auto sin_t = sincos_t.sin_val;
			const auto cos_t = sincos_t.cos_val;
			const auto sin2_t = sin_t * sin_t;

			const auto factor = v_one - v_rs / r;
			constexpr T eps_val = std::is_same_v<T, float> ? static_cast<T>(1e-6f) : static_cast<T>(1e-9);
			const auto safe_factor = max(factor, SimdVec<T, Width>(eps_val));
			const auto r2 = r * r;
			const auto inv_r = v_one / r;

			const auto g001 = v_rs / (v_two * r2 * safe_factor);
			const auto g100 = (v_c2 * v_rs * safe_factor) / (v_two * r2);
			const auto g111 = -g001;
			const auto g122 = -r * safe_factor;
			const auto g133 = -r * safe_factor * sin2_t;
			const auto g212 = inv_r;
			const auto g233 = -sin_t * cos_t;
			const auto g313 = inv_r;
			const auto safe_sin_t = select(abs(sin_t) < SimdVec<T, Width>(eps_val), SimdVec<T, Width>(eps_val), sin_t);
			const auto g323 = cos_t / safe_sin_t;
			const auto p_t = v_one / safe_factor;

			dp0 = SimdVec<T, Width>(static_cast<T>(0));
			dp1 = -(g100 * p_t * p_t + g111 * rp1 * rp1 + g122 * rp2 * rp2 + g133 * rp3 * rp3);
			dp2 = -(v_two * g212 * rp1 * rp2 + g233 * rp3 * rp3);
			dp3 = -v_two * (g313 * rp1 * rp3 + g323 * rp2 * rp3);
		};

		SimdVec<T, Width> k1_dx0, k1_dx1, k1_dx2, k1_dx3;
		SimdVec<T, Width> k1_dp0, k1_dp1, k1_dp2, k1_dp3;
		eval_derivs(x0, x1, x2, x3, p0, p1, p2, p3,
		            k1_dx0, k1_dx1, k1_dx2, k1_dx3, k1_dp0, k1_dp1, k1_dp2, k1_dp3);

		const auto dt = step_size;
		const auto half_dt = dt * v_half;

		const auto s2_x0 = fma(half_dt, k1_dx0, x0);
		const auto s2_x1 = fma(half_dt, k1_dx1, x1);
		const auto s2_x2 = fma(half_dt, k1_dx2, x2);
		const auto s2_x3 = fma(half_dt, k1_dx3, x3);
		const auto s2_p0 = fma(half_dt, k1_dp0, p0);
		const auto s2_p1 = fma(half_dt, k1_dp1, p1);
		const auto s2_p2 = fma(half_dt, k1_dp2, p2);
		const auto s2_p3 = fma(half_dt, k1_dp3, p3);

		SimdVec<T, Width> k2_dx0, k2_dx1, k2_dx2, k2_dx3;
		SimdVec<T, Width> k2_dp0, k2_dp1, k2_dp2, k2_dp3;
		eval_derivs(s2_x0, s2_x1, s2_x2, s2_x3, s2_p0, s2_p1, s2_p2, s2_p3,
		            k2_dx0, k2_dx1, k2_dx2, k2_dx3, k2_dp0, k2_dp1, k2_dp2, k2_dp3);

		const auto s3_x0 = fma(half_dt, k2_dx0, x0);
		const auto s3_x1 = fma(half_dt, k2_dx1, x1);
		const auto s3_x2 = fma(half_dt, k2_dx2, x2);
		const auto s3_x3 = fma(half_dt, k2_dx3, x3);
		const auto s3_p0 = fma(half_dt, k2_dp0, p0);
		const auto s3_p1 = fma(half_dt, k2_dp1, p1);
		const auto s3_p2 = fma(half_dt, k2_dp2, p2);
		const auto s3_p3 = fma(half_dt, k2_dp3, p3);

		SimdVec<T, Width> k3_dx0, k3_dx1, k3_dx2, k3_dx3;
		SimdVec<T, Width> k3_dp0, k3_dp1, k3_dp2, k3_dp3;
		eval_derivs(s3_x0, s3_x1, s3_x2, s3_x3, s3_p0, s3_p1, s3_p2, s3_p3,
		            k3_dx0, k3_dx1, k3_dx2, k3_dx3, k3_dp0, k3_dp1, k3_dp2, k3_dp3);

		const auto s4_x0 = fma(dt, k3_dx0, x0);
		const auto s4_x1 = fma(dt, k3_dx1, x1);
		const auto s4_x2 = fma(dt, k3_dx2, x2);
		const auto s4_x3 = fma(dt, k3_dx3, x3);
		const auto s4_p0 = fma(dt, k3_dp0, p0);
		const auto s4_p1 = fma(dt, k3_dp1, p1);
		const auto s4_p2 = fma(dt, k3_dp2, p2);
		const auto s4_p3 = fma(dt, k3_dp3, p3);

		SimdVec<T, Width> k4_dx0, k4_dx1, k4_dx2, k4_dx3;
		SimdVec<T, Width> k4_dp0, k4_dp1, k4_dp2, k4_dp3;
		eval_derivs(s4_x0, s4_x1, s4_x2, s4_x3, s4_p0, s4_p1, s4_p2, s4_p3,
		            k4_dx0, k4_dx1, k4_dx2, k4_dx3, k4_dp0, k4_dp1, k4_dp2, k4_dp3);

		const auto update_rk4 = [&](const SimdVec<T, Width>& orig,
		                            const SimdVec<T, Width>& k1, const SimdVec<T, Width>& k2,
		                            const SimdVec<T, Width>& k3, const SimdVec<T, Width>& k4) noexcept {
			return orig + (dt * v_sixth) * (k1 + v_two * k2 + v_two * k3 + k4);
		};

		const auto next_x0 = update_rk4(x0, k1_dx0, k2_dx0, k3_dx0, k4_dx0);
		const auto next_x1 = update_rk4(x1, k1_dx1, k2_dx1, k3_dx1, k4_dx1);
		const auto next_x2 = update_rk4(x2, k1_dx2, k2_dx2, k3_dx2, k4_dx2);
		const auto next_x3 = update_rk4(x3, k1_dx3, k2_dx3, k3_dx3, k4_dx3);

		const auto next_p0 = update_rk4(p0, k1_dp0, k2_dp0, k3_dp0, k4_dp0);
		const auto next_p1 = update_rk4(p1, k1_dp1, k2_dp1, k3_dp1, k4_dp1);
		const auto next_p2 = update_rk4(p2, k1_dp2, k2_dp2, k3_dp2, k4_dp2);
		const auto next_p3 = update_rk4(p3, k1_dp3, k2_dp3, k3_dp3, k4_dp3);

		masked_assign(active_mask, x0, next_x0);
		masked_assign(active_mask, x1, next_x1);
		masked_assign(active_mask, x2, next_x2);
		masked_assign(active_mask, x3, next_x3);

		masked_assign(active_mask, p0, next_p0);
		masked_assign(active_mask, p1, next_p1);
		masked_assign(active_mask, p2, next_p2);
		masked_assign(active_mask, p3, next_p3);

		masked_assign(active_mask, affine_parameter, affine_parameter + dt);

		const auto horizon_hit = (x1 <= (v_rs * static_cast<T>(1.001)));
		mask_mark_horizon(horizon_hit);

		const auto celestial_escape = (x1 >= SimdVec<T, Width>(static_cast<T>(50.0) * r_s));
		mask_mark_celestial(celestial_escape);
	}
};

using GeodesicBundle4d = GeodesicBundle<double, 4>;
using GeodesicBundle8d = GeodesicBundle<double, 8>;
using GeodesicBundle8f = GeodesicBundle<float, 8>;
using GeodesicBundle16f = GeodesicBundle<float, 16>;

static_assert(sizeof(GeodesicBundle4d) == 512);
static_assert(sizeof(GeodesicBundle8d) == 960);
static_assert(std::is_trivially_copyable_v<GeodesicBundle4d>);
static_assert(std::is_trivially_copyable_v<GeodesicBundle8d>);

}
