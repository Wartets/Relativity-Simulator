void teves_scalar_profile(double r, out double phi, out double dphi) {
	double mass = max(pc.metric_mass, 1.0e-4);
	double critical_acceleration = max(pc.teves_critical_acceleration, 1.0e-12);
	double transition_radius = sqrt(mass / critical_acceleration);
	double amplitude = 0.5 * sqrt(mass * critical_acceleration);
	double x = max(r, 1.0e-9) / transition_radius;
	double root = sqrt(1.0 + 4.0 * x * x);
	double bracket = -4.0 * x / (root + 1.0) + 2.0 * log_d(2.0 * x + root);
	phi = amplitude * bracket;
	dphi = amplitude * 4.0 / ((root + 1.0) * transition_radius);
}

void teves_spherical_terms(double r, out dvec3 g, out dvec3 dg_r, out double s, out double ds_r) {
	double horizon = 2.0 * max(pc.metric_mass, 1.0e-4);
	double radius = max(r, 1.0e-9);
	double phi;
	double dphi;
	teves_scalar_profile(radius, phi, dphi);
	double factor = max(1.0 - horizon / radius, 1.0e-12);
	double dfactor = horizon / (radius * radius);
	double conformal_plus = exp_d(2.0 * phi);
	double conformal_minus = 1.0 / conformal_plus;
	g = dvec3(-conformal_plus * factor, 0.0, conformal_minus / factor);
	dg_r = dvec3(
		-conformal_plus * (2.0 * dphi * factor + dfactor),
		0.0,
		conformal_minus * (-2.0 * dphi / factor - dfactor / (factor * factor))
	);
	double shrink = sqrt(conformal_minus);
	s = radius * shrink;
	ds_r = shrink * (1.0 - radius * dphi);
}

double teves_coordinate_radius(double areal) {
	double lower = 2.0 * max(pc.metric_mass, 1.0e-4) * 1.0002;
	double coordinate = max(areal, lower);
	for (int iteration = 0; iteration < 12; ++iteration) {
		dvec3 g;
		dvec3 dg;
		double s;
		double ds;
		teves_spherical_terms(coordinate, g, dg, s, ds);
		if (!(ds > 1.0e-9)) break;
		double update = (s - areal) / ds;
		coordinate = max(coordinate - update, lower);
		if (abs(update) < 1.0e-12 * max(coordinate, 1.0)) break;
	}
	return coordinate;
}
