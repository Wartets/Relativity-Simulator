#include "relativistic/io/hdf5_serializer.hpp"
#include <iostream>
#include <cassert>

int main() {
	using namespace Relativistic::Core;
	using namespace Relativistic::IO;

	Hdf5Container container_out;

	std::vector<MetricTensor<double>> metric_series;
	for (size_t i = 0; i < 50; ++i) {
		MetricTensor<double> g;
		g.zero();
		g(0, 0) = -1.0 + static_cast<double>(i) * 0.01;
		g(1, 1) = 1.0 + static_cast<double>(i) * 0.02;
		g(2, 2) = 100.0;
		g(3, 3) = 200.0;
		metric_series.push_back(g);
	}
	container_out.write_tensor_series("/spacetime/metric_history", metric_series);

	std::vector<FourVector<double>> positions;
	std::vector<FourVector<double>> velocities;
	for (size_t i = 0; i < 100; ++i) {
		const double t = static_cast<double>(i) * 0.1;
		positions.emplace_back(t, t * 10.0, 1.57, t * 0.05);
		velocities.emplace_back(1.0, 10.0, 0.0, 0.05);
	}
	container_out.write_worldline("/worldlines/geodesic_001", positions, velocities);

	const auto binary_data = container_out.serialize();
	assert(!binary_data.empty());

	const auto container_in = Hdf5Container::deserialize(binary_data);
	assert(container_in.has_value());

	const auto metric_ds = container_in->get_dataset("/spacetime/metric_history");
	assert(metric_ds.has_value());
	assert(metric_ds->dimensions.size() == 3);
	assert(metric_ds->dimensions[0] == 50);
	assert(metric_ds->dimensions[1] == 4);
	assert(metric_ds->dimensions[2] == 4);

	const auto metric_span = metric_ds->as_span<MetricTensor<double>>();
	assert(metric_span.size() == 50);
	for (size_t i = 0; i < 50; ++i) {
		assert(std::abs(metric_span[i](0, 0) - metric_series[i](0, 0)) < 1e-15);
		assert(std::abs(metric_span[i](1, 1) - metric_series[i](1, 1)) < 1e-15);
	}

	const auto pos_ds = container_in->get_dataset("/worldlines/geodesic_001/position");
	assert(pos_ds.has_value());
	const auto pos_span = pos_ds->as_span<FourVector<double>>();
	assert(pos_span.size() == 100);
	for (size_t i = 0; i < 100; ++i) {
		assert(std::abs(pos_span[i](0) - positions[i](0)) < 1e-15);
		assert(std::abs(pos_span[i](1) - positions[i](1)) < 1e-15);
	}

	return 0;
}
