#pragma once

#include <cstdint>
#include <cmath>
#include <algorithm>

namespace Relativistic::Integrators {

enum class StepControllerMode : uint8_t {
	Standard = 0,
	PI30 = 1,
	PID42 = 2
};

template <typename Scalar = double>
class AdaptiveStepController {
private:
	StepControllerMode mode_{StepControllerMode::Standard};
	Scalar err_prev_{static_cast<Scalar>(1.0)};
	Scalar err_prev2_{static_cast<Scalar>(1.0)};
	bool has_history_{false};

public:
	constexpr AdaptiveStepController() noexcept = default;

	explicit constexpr AdaptiveStepController(StepControllerMode mode) noexcept
		: mode_(mode) {}

	constexpr void set_mode(StepControllerMode mode) noexcept {
		mode_ = mode;
		reset_history();
	}

	[[nodiscard]] constexpr StepControllerMode mode() const noexcept {
		return mode_;
	}

	constexpr void reset_history() noexcept {
		err_prev_ = static_cast<Scalar>(1.0);
		err_prev2_ = static_cast<Scalar>(1.0);
		has_history_ = false;
	}

	[[nodiscard]] Scalar next_factor(Scalar err, Scalar order_p1) noexcept {
		const Scalar safe_err = std::max(err, static_cast<Scalar>(1e-12));
		const Scalar safe_prev = has_history_ ? std::max(err_prev_, static_cast<Scalar>(1e-12)) : safe_err;
		const Scalar safe_prev2 = has_history_ ? std::max(err_prev2_, static_cast<Scalar>(1e-12)) : safe_err;

		Scalar factor;
		switch (mode_) {
			case StepControllerMode::PI30: {
				const Scalar k_integral = static_cast<Scalar>(0.7) / order_p1;
				const Scalar k_proportional = static_cast<Scalar>(0.3) / order_p1;
				factor = std::pow(static_cast<Scalar>(1.0) / safe_err, k_integral)
					* std::pow(safe_prev / safe_err, k_proportional);
				break;
			}
			case StepControllerMode::PID42: {
				const Scalar k_integral = static_cast<Scalar>(0.6) / order_p1;
				const Scalar k_proportional = static_cast<Scalar>(0.4) / order_p1;
				const Scalar k_derivative = static_cast<Scalar>(0.2) / order_p1;
				factor = std::pow(static_cast<Scalar>(1.0) / safe_err, k_integral)
					* std::pow(safe_prev / safe_err, k_proportional)
					* std::pow(safe_prev / safe_prev2, k_derivative);
				break;
			}
			case StepControllerMode::Standard:
			default:
				factor = std::pow(static_cast<Scalar>(1.0) / safe_err, static_cast<Scalar>(1.0) / order_p1);
				break;
		}

		err_prev2_ = err_prev_;
		err_prev_ = safe_err;
		has_history_ = true;

		return factor;
	}
};

}
