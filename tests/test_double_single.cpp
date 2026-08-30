#include "relativistic/render/double_single.hpp"
#include <cassert>
#include <iostream>
#include <cmath>

int main() {
	using namespace Relativistic::Render;

	const double val_a = 1.23456789012345;
	const double val_b = 9.87654321098765;

	const DoubleSingle ds_a(val_a);
	const DoubleSingle ds_b(val_b);

	const double recon_a = static_cast<double>(ds_a);
	const double recon_b = static_cast<double>(ds_b);

	assert(std::abs(recon_a - val_a) < 1e-13);
	assert(std::abs(recon_b - val_b) < 1e-13);

	const DoubleSingle ds_sum = ds_a + ds_b;
	const double sum_d = static_cast<double>(ds_sum);
	const double target_sum = val_a + val_b;
	assert(std::abs((sum_d - target_sum) / target_sum) < 1e-13);

	const DoubleSingle ds_diff = ds_b - ds_a;
	const double diff_d = static_cast<double>(ds_diff);
	const double target_diff = val_b - val_a;
	assert(std::abs((diff_d - target_diff) / target_diff) < 1e-13);

	const DoubleSingle ds_prod = ds_a * ds_b;
	const double prod_d = static_cast<double>(ds_prod);
	const double target_prod = val_a * val_b;
	assert(std::abs((prod_d - target_prod) / target_prod) < 1e-13);

	const DoubleSingle ds_div_val = ds_b / ds_a;
	const double div_d = static_cast<double>(ds_div_val);
	const double target_div = val_b / val_a;
	assert(std::abs((div_d - target_div) / target_div) < 1e-13);

	const double val_c = 137.035999084;
	const DoubleSingle ds_c(val_c);
	const DoubleSingle ds_sqrt_c = ds_sqrt(ds_c);
	const double sqrt_d = static_cast<double>(ds_sqrt_c);
	const double target_sqrt = std::sqrt(val_c);
	assert(std::abs((sqrt_d - target_sqrt) / target_sqrt) < 1e-13);

	const DoubleSingle ds_sin_a = ds_sin(ds_a);
	const double sin_d = static_cast<double>(ds_sin_a);
	const double target_sin = std::sin(val_a);
	assert(std::abs((sin_d - target_sin) / target_sin) < 1e-13);

	std::cout << "test_double_single passed successfully.\n";
	return 0;
}
