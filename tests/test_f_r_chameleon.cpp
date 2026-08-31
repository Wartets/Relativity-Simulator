#include "relativistic/modified_gravity/f_r_gravity.hpp"
#include <cassert>
#include <cmath>

int main() {
	using namespace Relativistic::ModifiedGravity;

	HuSawickiParams<double> hs_params;
	hs_params.f_r0 = 1e-11;
	hs_params.n_index = 1.0;
	hs_params.r_background0 = 1e-52;

	FRChameleonModel<double> model(hs_params);

	const double rho_dense = 5500.0;
	const double rho_cosmic = 1e-26;

	const double m_dense = model.scalaron_effective_mass(rho_dense);
	const double m_cosmic = model.scalaron_effective_mass(rho_cosmic);

	assert(m_dense > m_cosmic * 1e6);

	const double m_earth = 5.972e24;
	const double r_earth = 6.371e6;

	const double thin_shell_earth = model.thin_shell_factor(m_earth, r_earth, rho_cosmic, rho_dense);
	const double g_eff_earth = model.effective_gravitational_coupling(m_earth, r_earth, rho_cosmic, rho_dense);
	const double g_std = model.gravitational_constant();

	assert(thin_shell_earth < 0.1);
	assert(std::abs(g_eff_earth - g_std) / g_std < 0.05);

	const double m_cloud = 1e15;
	const double r_cloud = 1e9;
	const double thin_shell_cloud = model.thin_shell_factor(m_cloud, r_cloud, rho_cosmic, 1e-20);
	const double g_eff_cloud = model.effective_gravitational_coupling(m_cloud, r_cloud, rho_cosmic, 1e-20);

	assert(thin_shell_cloud == 1.0);
	assert(std::abs(g_eff_cloud - (4.0 / 3.0) * g_std) / g_std < 1e-6);

	return 0;
}
