#include "support/test_harness.hpp"
#include "relativistic/io/hdf5_serializer.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

using namespace Relativistic::Core;
using namespace Relativistic::IO;
using Relativistic::Testing::TestSuite;

int main() {
	TestSuite suite(
		"hdf5_serializer",
		"Verifies the binary container: a metric tensor series and a worldline survive serialization and deserialization with identical dataset names, dimensions, sizes and values."
	);

	Hdf5Container writer;
	std::vector<MetricTensor<double>> metrics;
	for (size_t index = 0; index < 50; ++index) {
		MetricTensor<double> metric;
		metric.zero();
		metric(0, 0) = -1.0 + static_cast<double>(index) * 0.01;
		metric(1, 1) = 1.0 + static_cast<double>(index) * 0.02;
		metric(2, 2) = 100.0;
		metric(3, 3) = 200.0;
		metrics.push_back(metric);
	}
	writer.write_tensor_series("/spacetime/metric_history", metrics);
	std::vector<FourVector<double>> positions;
	std::vector<FourVector<double>> velocities;
	for (size_t index = 0; index < 100; ++index) {
		const double t = static_cast<double>(index) * 0.1;
		positions.emplace_back(t, t * 10.0, 1.57, t * 0.05);
		velocities.emplace_back(1.0, 10.0, 0.0, 0.05);
	}
	writer.write_worldline("/worldlines/geodesic_001", positions, velocities);

	suite.section("Serialization");
	const auto bytes = writer.serialize();
	suite.check(!bytes.empty(), "the container serializes to a non-empty buffer", "size=" + std::to_string(bytes.size()));
	const auto reader = Hdf5Container::deserialize(bytes);
	suite.check(reader.has_value(), "the serialized buffer deserializes");
	if (!reader.has_value()) {
		return suite.finish();
	}

	suite.section("Metric series");
	const auto metric_dataset = reader->get_dataset("/spacetime/metric_history");
	suite.check(metric_dataset.has_value(), "the metric dataset is present");
	if (metric_dataset.has_value()) {
		suite.check(metric_dataset->dimensions.size() == 3 && metric_dataset->dimensions[0] == 50 && metric_dataset->dimensions[1] == 4 && metric_dataset->dimensions[2] == 4, "the metric dataset has dimensions 50 by 4 by 4");
		const auto span = metric_dataset->as_span<MetricTensor<double>>();
		suite.check(span.size() == 50, "the metric span holds fifty tensors");
		double worst = 0.0;
		for (size_t index = 0; index < std::min<size_t>(span.size(), 50); ++index) {
			for (size_t row = 0; row < 4; ++row) {
				for (size_t column = 0; column < 4; ++column) {
					worst = std::max(worst, std::abs(span[index](row, column) - metrics[index](row, column)));
				}
			}
		}
		suite.check_at_most(worst, 1e-15, "every metric component survives bit-exactly");
	}

	suite.section("Worldline");
	const auto position_dataset = reader->get_dataset("/worldlines/geodesic_001/position");
	suite.check(position_dataset.has_value(), "the worldline position dataset is present");
	if (position_dataset.has_value()) {
		const auto span = position_dataset->as_span<FourVector<double>>();
		suite.check(span.size() == 100, "the position span holds one hundred events");
		double worst = 0.0;
		for (size_t index = 0; index < std::min<size_t>(span.size(), 100); ++index) {
			for (size_t component = 0; component < 4; ++component) {
				worst = std::max(worst, std::abs(span[index](component) - positions[index](component)));
			}
		}
		suite.check_at_most(worst, 1e-15, "every position component survives bit-exactly");
	}
	return suite.finish();
}
