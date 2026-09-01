#pragma once

#include "relativistic/uncertainty/interval.hpp"
#include "relativistic/core/tensor.hpp"
#include <vector>
#include <array>
#include <span>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <cstdint>

namespace Relativistic::Uncertainty {

template <typename Scalar = double, size_t Dim = 4>
class Zonotope {
	static_assert(Dim > 0, "Zonotope dimension must be strictly positive");

public:
	static constexpr size_t DIMENSION = Dim;
	using VectorType = std::array<Scalar, Dim>;
	using MatrixType = std::array<std::array<Scalar, Dim>, Dim>;

private:
	VectorType center_{};
	std::vector<VectorType> generators_{};

public:
	constexpr Zonotope() noexcept = default;

	explicit constexpr Zonotope(const VectorType& center) noexcept
		: center_(center), generators_{} {}

	Zonotope(const VectorType& center, std::span<const VectorType> generators)
		: center_(center), generators_(generators.begin(), generators.end()) {}

	Zonotope(const VectorType& center, std::vector<VectorType>&& generators) noexcept
		: center_(center), generators_(std::move(generators)) {}

	[[nodiscard]] static Zonotope from_intervals(const std::array<Interval<Scalar>, Dim>& intervals) {
		VectorType center{};
		std::vector<VectorType> generators;
		generators.reserve(Dim);

		for (size_t i = 0; i < Dim; ++i) {
			center[i] = intervals[i].midpoint();
			const Scalar r = intervals[i].radius();
			if (r > static_cast<Scalar>(0.0)) {
				VectorType gen{};
				gen[i] = r;
				generators.push_back(gen);
			}
		}
		return Zonotope(center, std::move(generators));
	}

	[[nodiscard]] constexpr const VectorType& center() const noexcept { return center_; }
	[[nodiscard]] constexpr VectorType& center() noexcept { return center_; }

	[[nodiscard]] const std::vector<VectorType>& generators() const noexcept { return generators_; }
	[[nodiscard]] std::vector<VectorType>& generators() noexcept { return generators_; }

	[[nodiscard]] size_t generator_count() const noexcept { return generators_.size(); }
	[[nodiscard]] double order() const noexcept {
		return static_cast<double>(generators_.size()) / static_cast<double>(Dim);
	}

	void add_generator(const VectorType& gen) {
		generators_.push_back(gen);
	}

	void clear_generators() noexcept {
		generators_.clear();
	}

	[[nodiscard]] std::array<Interval<Scalar>, Dim> bounding_box() const noexcept {
		std::array<Interval<Scalar>, Dim> box{};
		for (size_t i = 0; i < Dim; ++i) {
			Scalar rad = static_cast<Scalar>(0.0);
			for (const auto& gen : generators_) {
				rad += std::abs(gen[i]);
			}
			box[i] = Interval<Scalar>(center_[i] - rad, center_[i] + rad);
		}
		return box;
	}

	[[nodiscard]] Scalar bounding_box_hypervolume() const noexcept {
		const auto box = bounding_box();
		Scalar vol = static_cast<Scalar>(1.0);
		for (size_t i = 0; i < Dim; ++i) {
			vol *= box[i].width();
		}
		return vol;
	}

	[[nodiscard]] Scalar exact_hypervolume_2d() const noexcept requires (Dim == 2) {
		if (generators_.size() < 2) {
			return static_cast<Scalar>(0.0);
		}
		Scalar sum_det = static_cast<Scalar>(0.0);
		const size_t p = generators_.size();
		for (size_t i = 0; i < p; ++i) {
			for (size_t j = i + 1; j < p; ++j) {
				const Scalar det = generators_[i][0] * generators_[j][1] - generators_[i][1] * generators_[j][0];
				sum_det += std::abs(det);
			}
		}
		return static_cast<Scalar>(4.0) * sum_det;
	}

	[[nodiscard]] Zonotope minkowski_sum(const Zonotope& other) const {
		VectorType new_center{};
		for (size_t i = 0; i < Dim; ++i) {
			new_center[i] = center_[i] + other.center_[i];
		}
		std::vector<VectorType> new_generators = generators_;
		new_generators.insert(new_generators.end(), other.generators_.begin(), other.generators_.end());
		return Zonotope(new_center, std::move(new_generators));
	}

	[[nodiscard]] Zonotope linear_transform(const MatrixType& matrix) const {
		VectorType new_center{};
		for (size_t i = 0; i < Dim; ++i) {
			Scalar sum = static_cast<Scalar>(0.0);
			for (size_t j = 0; j < Dim; ++j) {
				sum += matrix[i][j] * center_[j];
			}
			new_center[i] = sum;
		}

		std::vector<VectorType> new_generators;
		new_generators.reserve(generators_.size());

		for (const auto& gen : generators_) {
			VectorType new_gen{};
			for (size_t i = 0; i < Dim; ++i) {
				Scalar sum = static_cast<Scalar>(0.0);
				for (size_t j = 0; j < Dim; ++j) {
					sum += matrix[i][j] * gen[j];
				}
				new_gen[i] = sum;
			}
			new_generators.push_back(new_gen);
		}

		return Zonotope(new_center, std::move(new_generators));
	}

	[[nodiscard]] Zonotope scale(Scalar factor) const {
		VectorType new_center{};
		for (size_t i = 0; i < Dim; ++i) {
			new_center[i] = center_[i] * factor;
		}
		std::vector<VectorType> new_generators;
		new_generators.reserve(generators_.size());
		for (const auto& gen : generators_) {
			VectorType new_gen{};
			for (size_t i = 0; i < Dim; ++i) {
				new_gen[i] = gen[i] * factor;
			}
			new_generators.push_back(new_gen);
		}
		return Zonotope(new_center, std::move(new_generators));
	}

	[[nodiscard]] Zonotope translate(const VectorType& offset) const {
		VectorType new_center{};
		for (size_t i = 0; i < Dim; ++i) {
			new_center[i] = center_[i] + offset[i];
		}
		return Zonotope(new_center, generators_);
	}

	void reduce_girard(size_t max_order) {
		const size_t max_generators = max_order * Dim;
		if (generators_.size() <= max_generators || max_generators < Dim) {
			return;
		}

		struct GenScore {
			size_t index;
			Scalar score;
		};

		std::vector<GenScore> scores;
		scores.reserve(generators_.size());

		for (size_t i = 0; i < generators_.size(); ++i) {
			Scalar l1_norm = static_cast<Scalar>(0.0);
			Scalar l_inf_norm = static_cast<Scalar>(0.0);
			for (size_t d = 0; d < Dim; ++d) {
				const Scalar abs_val = std::abs(generators_[i][d]);
				l1_norm += abs_val;
				if (abs_val > l_inf_norm) {
					l_inf_norm = abs_val;
				}
			}
			scores.push_back(GenScore{i, l1_norm - l_inf_norm});
		}

		std::sort(scores.begin(), scores.end(), [](const GenScore& a, const GenScore& b) {
			return a.score > b.score;
		});

		const size_t num_keep = max_generators - Dim;
		std::vector<VectorType> kept_generators;
		kept_generators.reserve(max_generators);

		for (size_t i = 0; i < num_keep; ++i) {
			kept_generators.push_back(generators_[scores[i].index]);
		}

		VectorType box_radius{};
		for (size_t i = num_keep; i < scores.size(); ++i) {
			const auto& gen = generators_[scores[i].index];
			for (size_t d = 0; d < Dim; ++d) {
				box_radius[d] += std::abs(gen[d]);
			}
		}

		for (size_t d = 0; d < Dim; ++d) {
			if (box_radius[d] > static_cast<Scalar>(0.0)) {
				VectorType box_gen{};
				box_gen[d] = box_radius[d];
				kept_generators.push_back(box_gen);
			}
		}

		generators_ = std::move(kept_generators);
	}

	[[nodiscard]] bool contains_point(const VectorType& point) const noexcept {
		const auto box = bounding_box();
		for (size_t i = 0; i < Dim; ++i) {
			if (!box[i].contains(point[i])) {
				return false;
			}
		}
		return true;
	}

	[[nodiscard]] Zonotope operator+(const Zonotope& rhs) const {
		return minkowski_sum(rhs);
	}

	[[nodiscard]] Zonotope operator*(Scalar scalar) const {
		return scale(scalar);
	}
};

template <typename Scalar, size_t Dim>
[[nodiscard]] inline Zonotope<Scalar, Dim> operator*(Scalar scalar, const Zonotope<Scalar, Dim>& z) {
	return z.scale(scalar);
}

}
