const double LTB_PI = 3.14159265358979323846LF;
const double SPH_TWO_PI = 6.28318530717958647692LF;

double g_ltb_present = 0.0;

struct SphericalMetric {
	dvec3 g;
	dvec3 dg_t;
	dvec3 dg_r;
	double s;
	double ds_t;
	double ds_r;
	bool regular;
};

struct LtbShell {
	double radius;
	double velocity;
	double curvature;
	bool regular;
};

struct LtbState {
	double radius;
	double velocity;
	double radius_r;
	double velocity_r;
	double radius_rr;
	double curvature;
	double curvature_r;
	bool regular;
};

double ltb_eta_minus_trig(double eta, double s) {
	double e2 = eta * eta;
	if (e2 < 0.04) {
		return eta * e2 / 6.0 * (1.0 + s * e2 / 20.0 * (1.0 + s * e2 / 42.0 * (1.0 + s * e2 / 72.0)));
	}
	if (s > 0.0) {
		return 0.5 * (exp_d(eta) - exp_d(-eta)) - eta;
	}
	dvec2 sc = sincos_wide(eta);
	return eta - sc.x;
}

double ltb_one_minus_trig(double eta, double s) {
	double e2 = eta * eta;
	if (e2 < 0.04) {
		return 0.5 * e2 * (1.0 + s * e2 / 12.0 * (1.0 + s * e2 / 30.0 * (1.0 + s * e2 / 56.0)));
	}
	if (s > 0.0) {
		return 0.5 * (exp_d(eta) + exp_d(-eta)) - 1.0;
	}
	dvec2 sc = sincos_wide(eta);
	return 1.0 - sc.y;
}

double ltb_solve_eta(double y, double s) {
	if (!(y > 0.0)) return 0.0;
	double bound = cbrt_d(6.0 * y);
	double eta = (s > 0.0) ? min(bound, log_d(2.0 * y + 2.0 * bound + 1.0)) : LTB_PI;
	for (int i = 0; i < 40; ++i) {
		double slope = ltb_one_minus_trig(eta, s);
		if (slope < 1.0e-18) break;
		double update = (ltb_eta_minus_trig(eta, s) - y) / slope;
		eta = max(eta - update, 0.0);
		if (abs(update) < 1.0e-14 * max(eta, 1.0e-3)) break;
	}
	return eta;
}

double ltb_omega(double r) {
	double width = max(double(pc.cosmology.ltb_void_width), 1.0e-9);
	double radius = double(pc.cosmology.ltb_void_radius);
	double inner = double(pc.cosmology.ltb_omega_inner);
	double outer = double(pc.cosmology.ltb_omega_outer);
	double step_value = (1.0 - ext_tanh((r - radius) / (2.0 * width))) / (1.0 + ext_tanh(radius / (2.0 * width)));
	return outer + (inner - outer) * step_value;
}

double ltb_omega_derivative(double r) {
	double width = max(double(pc.cosmology.ltb_void_width), 1.0e-9);
	double radius = double(pc.cosmology.ltb_void_radius);
	double t = ext_tanh((r - radius) / (2.0 * width));
	double norm = 1.0 + ext_tanh(radius / (2.0 * width));
	return (double(pc.cosmology.ltb_omega_inner) - double(pc.cosmology.ltb_omega_outer)) * (-(1.0 - t * t) / (2.0 * width)) / norm;
}

double ltb_age(double omega) {
	double h0 = double(pc.cosmology.ltb_hubble0);
	double d = 1.0 - omega;
	if (abs(d) < 1.0e-9) return 2.0 / (3.0 * h0);
	double c = (2.0 - omega) / omega;
	if (d > 0.0) {
		double eta = log_d(max(c, 1.0) + sqrt(max(c * c - 1.0, 0.0)));
		return omega / (2.0 * h0 * d * sqrt(d)) * ltb_eta_minus_trig(eta, 1.0);
	}
	double eta = double(acos(float(clamp(c, -1.0, 1.0))));
	for (int i = 0; i < 2; ++i) {
		dvec2 sc = sincos_wide(eta);
		if (abs(sc.x) > 1.0e-6) eta += (sc.y - c) / sc.x;
	}
	return omega / (2.0 * h0 * (-d) * sqrt(-d)) * ltb_eta_minus_trig(clamp(eta, 0.0, SPH_TWO_PI), -1.0);
}

LtbShell ltb_shell(double r, double t) {
	double h0 = double(pc.cosmology.ltb_hubble0);
	double omega = ltb_omega(r);
	double d = 1.0 - omega;
	LtbShell shell;
	shell.radius = 1.0e-9;
	shell.velocity = 0.0;
	shell.curvature = h0 * h0 * d * r * r;
	shell.regular = false;
	double tau = t - (g_ltb_present - ltb_age(omega));
	if (!(tau > 1.0e-9)) return shell;
	double areal;
	if (abs(d) < 1.0e-9) {
		areal = r * cbrt_d(2.25 * h0 * h0 * tau * tau);
	} else if (d > 0.0) {
		double scale = omega / (2.0 * h0 * d * sqrt(d));
		double eta = ltb_solve_eta(tau / scale, 1.0);
		areal = r * omega / (2.0 * d) * ltb_one_minus_trig(eta, 1.0);
	} else {
		double scale = omega / (2.0 * h0 * (-d) * sqrt(-d));
		double eta = ltb_solve_eta(min(tau / scale, LTB_PI), -1.0);
		areal = r * omega / (2.0 * (-d)) * ltb_one_minus_trig(eta, -1.0);
	}
	shell.radius = max(areal, 1.0e-12);
	shell.velocity = sqrt(max(h0 * h0 * omega * r * r * r / shell.radius + shell.curvature, 0.0));
	shell.regular = true;
	return shell;
}

LtbState ltb_state(double r, double t) {
	double h0 = double(pc.cosmology.ltb_hubble0);
	double h = 5.0e-3 * max(r, 1.0);
	double rc = max(r, 2.0 * h);
	LtbShell center = ltb_shell(rc, t);
	LtbShell plus = ltb_shell(rc + h, t);
	LtbShell minus = ltb_shell(rc - h, t);
	LtbShell at_point = (r >= 2.0 * h) ? center : ltb_shell(max(r, 1.0e-9), t);
	LtbState state;
	state.regular = at_point.regular && center.regular && plus.regular && minus.regular;
	state.radius = at_point.radius;
	state.velocity = at_point.velocity;
	state.curvature = at_point.curvature;
	double rs = max(r, 1.0e-9);
	double omega = ltb_omega(rs);
	state.curvature_r = h0 * h0 * (-ltb_omega_derivative(rs) * rs * rs + 2.0 * (1.0 - omega) * rs);
	state.radius_r = (plus.radius - minus.radius) / (2.0 * h);
	state.velocity_r = (plus.velocity - minus.velocity) / (2.0 * h);
	state.radius_rr = (plus.radius - 2.0 * center.radius + minus.radius) / (h * h);
	return state;
}

double ltb_comoving_radius(double areal_target, double t) {
	double r = areal_target;
	for (int i = 0; i < 8; ++i) {
		LtbState state = ltb_state(r, t);
		if (!state.regular || abs(state.radius_r) < 1.0e-9) break;
		double update = (state.radius - areal_target) / state.radius_r;
		r = max(r - update, 1.0e-6);
		if (abs(update) < 1.0e-10 * max(r, 1.0)) break;
	}
	return r;
}

double spherical_mass_at(double v) {
	double m0 = max(pc.metric_mass, 1.0e-4);
	double cubed = m0 * m0 * m0 - 3.0 * double(pc.cosmology.evaporation_rate) * v;
	return max(cbrt_d(max(cubed, 1.0e-12)), 1.0e-4);
}

double spherical_horizon_radius(uint kind, double t) {
	if (kind == 13u) return 2.0 * spherical_mass_at(t);
	if (kind == 14u || kind == 15u || kind == 16u) return 2.0 * max(pc.metric_mass, 1.0e-4);
	return 0.0;
}

SphericalMetric spherical_metric_at(uint kind, double t, double r) {
	SphericalMetric m;
	m.g = dvec3(-1.0, 0.0, 1.0);
	m.dg_t = dvec3(0.0);
	m.dg_r = dvec3(0.0);
	m.s = max(abs(r), 1.0e-9);
	m.ds_t = 0.0;
	m.ds_r = 1.0;
	m.regular = true;
	if (kind == 10u) {
		LtbState state = ltb_state(m.s, t);
		double one = max(1.0 + state.curvature, 1.0e-9);
		m.regular = state.regular;
		m.g = dvec3(-1.0, 0.0, state.radius_r * state.radius_r / one);
		m.dg_t = dvec3(0.0, 0.0, 2.0 * state.radius_r * state.velocity_r / one);
		m.dg_r = dvec3(0.0, 0.0, 2.0 * state.radius_r * state.radius_rr / one - state.radius_r * state.radius_r * state.curvature_r / (one * one));
		m.s = state.radius;
		m.ds_t = state.velocity;
		m.ds_r = state.radius_r;
		return m;
	}
	if (kind == 16u) {
		double coordinate_radius = m.s;
		teves_spherical_terms(coordinate_radius, m.g, m.dg_r, m.s, m.ds_r);
		return m;
	}
	double rr = m.s;
	double mass0 = max(pc.metric_mass, 1.0e-4);
	if (kind == 13u) {
		double mass = spherical_mass_at(t);
		double rate = -double(pc.cosmology.evaporation_rate) / (mass * mass);
		m.g = dvec3(-(1.0 - 2.0 * mass / rr), 1.0, 0.0);
		m.dg_t = dvec3(2.0 * rate / rr, 0.0, 0.0);
		m.dg_r = dvec3(-2.0 * mass / (rr * rr), 0.0, 0.0);
	} else if (kind == 14u) {
		double rs = 2.0 * mass0;
		m.g = dvec3(-(1.0 - rs / rr), 1.0, 0.0);
		m.dg_r = dvec3(-rs / (rr * rr), 0.0, 0.0);
	} else {
		double rs = 2.0 * mass0;
		m.g = dvec3(-(1.0 - rs / rr), sqrt(rs / rr), 1.0);
		m.dg_r = dvec3(-rs / (rr * rr), -0.5 * sqrt(rs) / (rr * sqrt(rr)), 0.0);
	}
	return m;
}

double spherical_dot(dvec3 g, dvec2 a, dvec2 b) {
	return g.x * a.x * b.x + g.y * (a.x * b.y + a.y * b.x) + g.z * a.y * b.y;
}

void spherical_frame(SphericalMetric m, out dvec2 u, out dvec2 e) {
	double gtt = min(m.g.x, -1.0e-12);
	double ut = 1.0 / sqrt(-gtt);
	double er = 1.0 / sqrt(max(m.g.z - m.g.y * m.g.y / gtt, 1.0e-12));
	u = dvec2(ut, 0.0);
	e = dvec2(-m.g.y * er / gtt, er);
}

void spherical_plane_dirs(double psi, dvec3 e_radial, dvec3 e_tangent, out dvec3 radial_hat, out dvec3 tangent_hat) {
	double reduced = psi - SPH_TWO_PI * floor(psi / SPH_TWO_PI + 0.5);
	dvec2 sc = sincos_wide(reduced);
	radial_hat = sc.y * e_radial + sc.x * e_tangent;
	tangent_hat = -sc.x * e_radial + sc.y * e_tangent;
}

void spherical_rhs(uint kind, double t, double r, dvec2 k, double l, out dvec2 dk, out double dpsi, out bool regular) {
	SphericalMetric m = spherical_metric_at(kind, t, r);
	regular = m.regular;
	double det = m.g.x * m.g.z - m.g.y * m.g.y;
	if (abs(det) < 1.0e-30) det = (det < 0.0) ? -1.0e-30 : 1.0e-30;
	dvec3 inv = dvec3(m.g.z, -m.g.y, m.g.x) / det;
	dvec3 d[2];
	d[0] = m.dg_t;
	d[1] = m.dg_r;
	double kv[2];
	kv[0] = k.x;
	kv[1] = k.y;
	double ds[2];
	ds[0] = m.ds_t;
	ds[1] = m.ds_r;
	double s = max(m.s, 1.0e-12);
	double centrifugal = l * l / (s * s * s);
	double acc[2];
	for (int a = 0; a < 2; ++a) {
		double sum = 0.0;
		for (int b = 0; b < 2; ++b) {
			for (int c = 0; c < 2; ++c) {
				double gamma_sum = 0.0;
				for (int e = 0; e < 2; ++e) {
					gamma_sum += inv[a + e] * (d[b][e + c] + d[c][e + b] - d[e][b + c]);
				}
				sum -= 0.5 * gamma_sum * kv[b] * kv[c];
			}
			sum += centrifugal * inv[a + b] * ds[b];
		}
		acc[a] = sum;
	}
	dk = dvec2(acc[0], acc[1]);
	dpsi = l / (s * s);
}

bool spherical_rk4(uint kind, inout double t, inout double r, inout double psi, inout dvec2 k, double l, double h) {
	dvec2 x0 = dvec2(t, r);
	dvec2 dk1;
	dvec2 dk2;
	dvec2 dk3;
	dvec2 dk4;
	double dp1;
	double dp2;
	double dp3;
	double dp4;
	bool ok1;
	bool ok2;
	bool ok3;
	bool ok4;
	spherical_rhs(kind, x0.x, x0.y, k, l, dk1, dp1, ok1);
	dvec2 x2 = x0 + 0.5 * h * k;
	dvec2 k2 = k + 0.5 * h * dk1;
	spherical_rhs(kind, x2.x, x2.y, k2, l, dk2, dp2, ok2);
	dvec2 x3 = x0 + 0.5 * h * k2;
	dvec2 k3 = k + 0.5 * h * dk2;
	spherical_rhs(kind, x3.x, x3.y, k3, l, dk3, dp3, ok3);
	dvec2 x4 = x0 + h * k3;
	dvec2 k4 = k + h * dk3;
	spherical_rhs(kind, x4.x, x4.y, k4, l, dk4, dp4, ok4);
	double sixth = h / 6.0;
	dvec2 advanced = x0 + sixth * (k + 2.0 * k2 + 2.0 * k3 + k4);
	k += sixth * (dk1 + 2.0 * dk2 + 2.0 * dk3 + dk4);
	psi += sixth * (dp1 + 2.0 * dp2 + 2.0 * dp3 + dp4);
	t = advanced.x;
	r = advanced.y;
	return ok1 && ok2 && ok3 && ok4;
}

void trace_spherical_spacetime(dvec3 ray_dir, out dvec3 accum_out, out uint status_out, out uint iters_out, out double redshift_out) {
	uint kind = pc.metric_type;
	double mass = max(pc.metric_mass, 1.0e-4);
	double obs_r = max(pc.observer_position.y, 1.0e-6);
	double obs_th = clamp(pc.observer_position.z, 0.001, PI_D - 0.001);
	double obs_ph = pc.observer_position.w;
	dvec3 obs_cart = spherical_to_cartesian(obs_r, obs_th, obs_ph);
	dvec3 e_radial = obs_cart / max(length(obs_cart), 1.0e-12);
	dvec3 direction = ray_dir / max(length(ray_dir), 1.0e-12);
	double n_radial = dot(direction, e_radial);
	dvec3 lateral = direction - n_radial * e_radial;
	double n_tangent = length(lateral);
	dvec3 e_tangent;
	if (n_tangent > 1.0e-12) {
		e_tangent = lateral / n_tangent;
	} else {
		dvec3 helper = (abs(e_radial.z) < 0.9) ? dvec3(0.0, 0.0, 1.0) : dvec3(1.0, 0.0, 0.0);
		e_tangent = normalize(cross(helper, e_radial));
		n_tangent = 0.0;
	}
	dvec3 plane_normal = cross(e_radial, e_tangent);

	double t = 0.0;
	double r = obs_r;
	g_ltb_present = 0.0;
	if (kind == 10u) {
		g_ltb_present = max(ltb_age(double(pc.cosmology.ltb_omega_inner)), ltb_age(double(pc.cosmology.ltb_omega_outer)));
		t = g_ltb_present;
		r = ltb_comoving_radius(obs_r, t);
	} else if (kind == 16u) {
		r = teves_coordinate_radius(obs_r);
	}

	SphericalMetric start = spherical_metric_at(kind, t, r);
	dvec2 start_u;
	dvec2 start_e;
	spherical_frame(start, start_u, start_e);
	dvec2 k = -start_u + n_radial * start_e;
	double l = start.s * n_tangent;
	double psi = 0.0;

	bool has_disk = (kind >= 13u) && pc.primary_disk.enabled > 0.5;
	double disk_inner = 6.0 * mass * double(max(pc.primary_disk.inner_radius_scale, 1.0));
	double disk_outer = max(double(pc.primary_disk.outer_radius_mass_units) * mass, disk_inner * 1.05);
	bool bodies_traced = ((pc.render_flags & (1u << 12)) != 0u) && pc.body_count > 0u && ((pc.render_flags & (1u << 17)) == 0u);
	bool dark_active = pc.dark_matter.halo_count > 0u && pc.dark_matter.flags != 0u;
	bool need_cart = bodies_traced || has_disk || dark_active;

	dvec3 accum = dvec3(0.0);
	double throughput = 1.0;
	uint status = 0u;
	uint iters = 0u;
	double redshift_rec = 1.0;
	bool exited = false;
	dvec3 prev_cart = obs_cart;

	for (uint step = 0u; step < pc.max_integration_steps && throughput > 0.01; ++step) {
		iters = step + 1u;
		SphericalMetric current = spherical_metric_at(kind, t, r);
		if (!current.regular) {
			status = 1u;
			throughput = 0.0;
			break;
		}
		double horizon = spherical_horizon_radius(kind, t);
		if (horizon > 0.0 && r <= horizon * 1.0001) {
			status = 1u;
			throughput = 0.0;
			break;
		}
		if (current.s >= pc.escape_radius) {
			exited = true;
			break;
		}

		double dt_mag;
		if (horizon > 0.0) {
			double r_scale = max(r - horizon, 0.02 * mass);
			double far_factor = 1.0 + (pc.far_field_step_scale - 1.0) * clamp((r - 20.0 * horizon) / max(80.0 * horizon, 1.0e-9), 0.0, 1.0);
			dt_mag = clamp(pc.step_size_factor * sqrt(r * r_scale) * far_factor, pc.min_step_size, pc.max_step_size * pc.far_field_step_scale);
		} else {
			double hubble = abs(current.ds_t) / max(current.s, 1.0e-9);
			double scale = min(current.s, 0.1 / max(hubble, 1.0e-6));
			dt_mag = clamp(pc.step_size_factor * scale, pc.min_step_size, pc.max_step_size * pc.far_field_step_scale);
		}
		if (dark_active) {
			dt_mag = min(dt_mag, dark_matter_step_limit(prev_cart));
		}

		dvec2 k_before = k;
		bool regular = spherical_rk4(kind, t, r, psi, k, l, dt_mag);
		if (!regular || any(isnan(k)) || any(isinf(k)) || isnan(r) || isinf(r) || isnan(psi) || isinf(psi)) {
			status = 1u;
			throughput = 0.0;
			break;
		}
		if (r < 0.0) {
			r = -r;
			k.y = -k.y;
			psi += LTB_PI;
		}

		if (need_cart) {
			SphericalMetric after = spherical_metric_at(kind, t, r);
			dvec3 radial_hat;
			dvec3 tangent_hat;
			spherical_plane_dirs(psi, e_radial, e_tangent, radial_hat, tangent_hat);
			dvec3 cart = after.s * radial_hat;

			if (dark_active) {
				dvec3 middle = 0.5 * (prev_cart + cart);
				accum += throughput * dvec3(dark_matter_emission(middle, length(cart - prev_cart)));
				dvec2 frame_u;
				dvec2 frame_e;
				spherical_frame(after, frame_u, frame_e);
				double omega = spherical_dot(after.g, k, frame_u);
				double radial_part = spherical_dot(after.g, k, frame_e);
				double tangent_part = l / max(after.s, 1.0e-12);
				double norm = length(dvec2(radial_part, tangent_part));
				if (norm > 1.0e-12) {
					dvec3 look = (radial_part * radial_hat + tangent_part * tangent_hat) / norm;
					dvec3 kicked = dark_matter_kick(middle, look, dt_mag);
					dvec2 plane = dvec2(dot(kicked, radial_hat), dot(kicked, tangent_hat));
					double plane_norm = length(plane);
					if (plane_norm > 1.0e-12) {
						double new_radial = omega * plane.x / plane_norm;
						double new_tangent = omega * plane.y / plane_norm;
						k = -omega * frame_u + new_radial * frame_e;
						l = after.s * new_tangent;
					}
				}
			}

			if (bodies_traced) {
				dvec3 segment = cart - prev_cart;
				double segment_length = length(segment);
				if (segment_length > 1.0e-12) {
					vec3 body_color;
					uint body_status;
					double body_dist;
					if (intersect_bodies(prev_cart, segment / segment_length, segment_length, body_color, body_status, body_dist)) {
						accum += throughput * dvec3(body_color);
						status |= 16u;
						throughput = 0.0;
						break;
					}
				}
			}

			if (has_disk && prev_cart.z * cart.z <= 0.0) {
				double energy = max(abs(current.g.x * k_before.x + current.g.y * k_before.y), 1.0e-12);
				double l_over_e = -l * plane_normal.z / energy;
				if (pc.hydro_disk.model == 2u) {
					vec3 torus_rgb;
					double torus_opacity;
					double torus_redshift;
					torus_integrate_segment(prev_cart, cart, l_over_e, mass, 0.0, torus_rgb, torus_opacity, torus_redshift);
					if (torus_opacity > 1.0e-6) {
						status |= 4u;
						redshift_rec = torus_redshift;
						accum += throughput * dvec3(torus_rgb);
						throughput *= (1.0 - torus_opacity);
					}
				} else {
					double z_span = abs(prev_cart.z) + abs(cart.z);
					double s_cross = (z_span > 1.0e-15) ? abs(prev_cart.z) / z_span : 0.5;
					dvec3 cross_pos = prev_cart + s_cross * (cart - prev_cart);
					double r_cross = length(cross_pos.xy);
					if (r_cross >= disk_inner && r_cross <= disk_outer && r_cross > horizon * 1.02) {
						double azimuth = double(atan(float(cross_pos.y), float(cross_pos.x)));
						double g_doppler = disk_redshift(mass, 0.0, r_cross, l_over_e);
						double nt_base = (pc.hydro_disk.model == 1u) ? nt_base_temperature(r_cross, mass, 0.0) : -1.0;
						vec3 shaded_rgb;
						double shaded_opacity;
						disk_shade_with_base(pc.primary_disk, mass, 0.0, disk_inner, disk_outer, r_cross, azimuth, g_doppler, pc.time, 1.0, nt_base, shaded_rgb, shaded_opacity);
						status |= 4u;
						redshift_rec = g_doppler;
						accum += throughput * dvec3(shaded_rgb);
						throughput *= (1.0 - shaded_opacity);
					}
				}
			}
			prev_cart = cart;
		}
	}

	if (exited) {
		SphericalMetric final_metric = spherical_metric_at(kind, t, r);
		dvec2 final_u;
		dvec2 final_e;
		spherical_frame(final_metric, final_u, final_e);
		double omega = spherical_dot(final_metric.g, k, final_u);
		double radial_part = spherical_dot(final_metric.g, k, final_e);
		double tangent_part = l / max(final_metric.s, 1.0e-12);
		double norm = max(length(dvec2(radial_part, tangent_part)), 1.0e-12);
		dvec3 radial_hat;
		dvec3 tangent_hat;
		spherical_plane_dirs(psi, e_radial, e_tangent, radial_hat, tangent_hat);
		dvec3 sky_dir = (radial_part * radial_hat + tangent_part * tangent_hat) / norm;
		accum += throughput * dvec3(compute_sky_radiance(sky_dir, pc.sky_rotation_rad));
		status |= 2u;
		if (kind == 10u) {
			redshift_rec = 1.0 / max(omega, 1.0e-6);
		}
	}

	accum_out = accum;
	status_out = status;
	iters_out = iters;
	redshift_out = redshift_rec;
}
