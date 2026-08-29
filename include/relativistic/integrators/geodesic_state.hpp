#pragma once

#include "relativistic/core/tensor.hpp"

namespace Relativistic::Integrators {

template <typename Scalar = double>
struct alignas(32) GeodesicState {
	Core::FourVector<Scalar> x;
	Core::FourVector<Scalar> u;
};

}
