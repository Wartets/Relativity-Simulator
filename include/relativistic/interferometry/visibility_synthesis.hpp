#pragma once

#include "relativistic/core/constants.hpp"
#include "relativistic/interferometry/vlbi_array.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace Relativistic::Interferometry {

struct IntensityImage {
	uint32_t size{0};
	std::vector<double> pixels{};
	double simulation_pixel_scale_rad{0.0};
	double observer_radius{0.0};

	[[nodiscard]] bool valid() const noexcept {
		return size >= 8 && pixels.size() >= static_cast<size_t>(size) * static_cast<size_t>(size);
	}
};

struct SynthesisControl {
	std::atomic<float> progress{0.0f};
	std::atomic<bool> cancel{false};
};

struct ObservationSettings {
	double frequency_hz{230.0e9};
	double bandwidth_hz{2.0e9};
	double integration_time_s{10.0};
	double source_right_ascension_deg{187.7059};
	double source_declination_deg{12.3911};
	double image_position_angle_deg{0.0};
	double total_flux_jy{0.5};
	double start_mjd{57854.0};
	double start_hour_ut{0.0};
	double duration_hours{24.0};
	double cadence_s{600.0};
	double minimum_elevation_deg{10.0};
	double quantization_efficiency{0.88};
	bool thermal_noise{true};
	uint64_t noise_seed{20170411ULL};
};

struct VisibilitySample {
	uint32_t epoch_index{0};
	uint32_t station_a{0};
	uint32_t station_b{0};
	double time_seconds{0.0};
	double mjd{0.0};
	double u_meters{0.0};
	double v_meters{0.0};
	double u_lambda{0.0};
	double v_lambda{0.0};
	double image_u_lambda{0.0};
	double image_v_lambda{0.0};
	std::complex<double> value{};
	std::complex<double> noiseless_value{};
	double sigma_jy{0.0};
	bool flagged{false};
};

struct ClosurePhaseSample {
	uint32_t epoch_index{0};
	uint32_t station_a{0};
	uint32_t station_b{0};
	uint32_t station_c{0};
	double time_seconds{0.0};
	double mjd{0.0};
	double u1_meters{0.0};
	double v1_meters{0.0};
	double u2_meters{0.0};
	double v2_meters{0.0};
	double phase_rad{0.0};
	double amplitude{0.0};
	double phase_error_rad{0.0};
	double amplitude_error{0.0};
};

struct VlbiDataset {
	bool valid{false};
	bool cancelled{false};
	std::string message{};
	VlbiArray array{};
	ObservationSettings settings{};
	double pixel_scale_rad{0.0};
	double wavelength_m{0.0};
	uint32_t image_size{0};
	std::vector<double> image{};
	std::vector<VisibilitySample> samples{};
	std::vector<ClosurePhaseSample> closures{};
	uint32_t plane_size{0};
	double plane_cell_lambda{0.0};
	std::vector<double> plane_log_amplitude{};
	std::vector<double> profile_baseline_glambda{};
	std::vector<double> profile_amplitude{};
	std::vector<uint32_t> station_visible_epochs{};
	double maximum_baseline_lambda{0.0};
	double minimum_baseline_lambda{0.0};
	double mean_snr{0.0};
	double first_null_baseline_lambda{0.0};
	double estimated_ring_diameter_rad{0.0};
	double nyquist_baseline_lambda{0.0};
	size_t epoch_count{0};
	size_t flagged_count{0};
};

namespace Detail {

inline void fft_inplace(std::vector<std::complex<double>>& a) {
	const size_t n = a.size();
	for (size_t i = 1, j = 0; i < n; ++i) {
		size_t bit = n >> 1;
		for (; j & bit; bit >>= 1) {
			j ^= bit;
		}
		j ^= bit;
		if (i < j) {
			std::swap(a[i], a[j]);
		}
	}
	for (size_t length = 2; length <= n; length <<= 1) {
		const double angle = -2.0 * std::numbers::pi_v<double> / static_cast<double>(length);
		const std::complex<double> step(std::cos(angle), std::sin(angle));
		for (size_t i = 0; i < n; i += length) {
			std::complex<double> twiddle(1.0, 0.0);
			for (size_t j = 0; j < length / 2; ++j) {
				const std::complex<double> even = a[i + j];
				const std::complex<double> odd = a[i + j + length / 2] * twiddle;
				a[i + j] = even + odd;
				a[i + j + length / 2] = even - odd;
				twiddle *= step;
			}
		}
	}
}

inline void fft2d_inplace(std::vector<std::complex<double>>& data, size_t n) {
	std::vector<std::complex<double>> line(n);
	for (size_t row = 0; row < n; ++row) {
		std::copy(data.begin() + static_cast<std::ptrdiff_t>(row * n), data.begin() + static_cast<std::ptrdiff_t>((row + 1) * n), line.begin());
		fft_inplace(line);
		std::copy(line.begin(), line.end(), data.begin() + static_cast<std::ptrdiff_t>(row * n));
	}
	for (size_t column = 0; column < n; ++column) {
		for (size_t row = 0; row < n; ++row) {
			line[row] = data[row * n + column];
		}
		fft_inplace(line);
		for (size_t row = 0; row < n; ++row) {
			data[row * n + column] = line[row];
		}
	}
}

class GaussianNoise {
public:
	explicit GaussianNoise(uint64_t seed) noexcept : state_(seed ^ 0x9E3779B97F4A7C15ULL) {}

	[[nodiscard]] std::pair<double, double> next_pair() noexcept {
		const double u1 = std::max(next_uniform(), 1e-300);
		const double u2 = next_uniform();
		const double radius = std::sqrt(-2.0 * std::log(u1));
		const double angle = 2.0 * std::numbers::pi_v<double> * u2;
		return {radius * std::cos(angle), radius * std::sin(angle)};
	}

private:
	[[nodiscard]] uint64_t next_u64() noexcept {
		state_ += 0x9E3779B97F4A7C15ULL;
		uint64_t z = state_;
		z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
		z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
		return z ^ (z >> 31);
	}

	[[nodiscard]] double next_uniform() noexcept {
		return static_cast<double>(next_u64() >> 11) * (1.0 / 9007199254740992.0);
	}

	uint64_t state_;
};

template <typename Function>
void parallel_for_each(size_t count, Function&& function, SynthesisControl* control, float progress_begin, float progress_end) {
	if (count == 0) return;
	const size_t hardware = std::max<size_t>(1, std::thread::hardware_concurrency());
	const size_t workers = std::min(hardware, count);
	std::atomic<size_t> next{0};
	std::atomic<size_t> done{0};
	constexpr size_t chunk = 16;
	const auto body = [&]() {
		for (;;) {
			if (control != nullptr && control->cancel.load(std::memory_order_relaxed)) return;
			const size_t begin = next.fetch_add(chunk, std::memory_order_relaxed);
			if (begin >= count) return;
			const size_t end = std::min(begin + chunk, count);
			for (size_t i = begin; i < end; ++i) {
				function(i);
			}
			const size_t finished = done.fetch_add(end - begin, std::memory_order_relaxed) + (end - begin);
			if (control != nullptr) {
				control->progress.store(progress_begin + (progress_end - progress_begin) * static_cast<float>(finished) / static_cast<float>(count), std::memory_order_relaxed);
			}
		}
	};
	std::vector<std::jthread> threads;
	threads.reserve(workers);
	for (size_t i = 1; i < workers; ++i) {
		threads.emplace_back(body);
	}
	body();
}

[[nodiscard]] inline double dot3(const std::array<double, 3>& a, const std::array<double, 3>& b) noexcept {
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

}

class VisibilitySynthesizer {
public:
	static constexpr uint32_t kMaximumImageSize = 512;

	[[nodiscard]] static VlbiDataset synthesize(
		const IntensityImage& image,
		const VlbiArray& array,
		const ObservationSettings& settings,
		double pixel_scale_rad,
		SynthesisControl* control = nullptr
	) {
		VlbiDataset dataset;
		dataset.array = array;
		dataset.settings = settings;
		dataset.pixel_scale_rad = pixel_scale_rad;
		if (control != nullptr) {
			control->progress.store(0.0f);
		}

		if (!image.valid() || image.size > kMaximumImageSize) {
			dataset.message = "The captured intensity image is missing or exceeds the supported resolution.";
			return dataset;
		}
		if (!(pixel_scale_rad > 0.0) || !std::isfinite(pixel_scale_rad)) {
			dataset.message = "The angular pixel scale is invalid. Verify the source mass, distance and camera field of view.";
			return dataset;
		}
		if (!(settings.total_flux_jy > 0.0) || !(settings.frequency_hz > 0.0) || !(settings.cadence_s > 0.0) || !(settings.duration_hours > 0.0)) {
			dataset.message = "The observation settings contain non-positive flux, frequency, cadence or duration.";
			return dataset;
		}

		std::vector<size_t> active;
		for (size_t i = 0; i < array.stations.size(); ++i) {
			if (array.stations[i].enabled) {
				active.push_back(i);
			}
		}
		if (active.size() < 2) {
			dataset.message = "At least two enabled stations are required.";
			return dataset;
		}

		const uint32_t n = image.size;
		std::vector<double> normalized(image.pixels.begin(), image.pixels.begin() + static_cast<std::ptrdiff_t>(static_cast<size_t>(n) * n));
		double total = 0.0;
		for (double& value : normalized) {
			value = std::max(value, 0.0);
			total += value;
		}
		if (!(total > 0.0)) {
			dataset.message = "The captured image has no positive intensity.";
			return dataset;
		}
		for (double& value : normalized) {
			value /= total;
		}
		dataset.image_size = n;
		dataset.image.resize(normalized.size());
		for (size_t i = 0; i < normalized.size(); ++i) {
			dataset.image[i] = normalized[i] * settings.total_flux_jy;
		}

		build_plane(normalized, n, pixel_scale_rad, dataset);
		if (control != nullptr) {
			control->progress.store(0.08f);
		}

		constexpr double c = Core::PhysicalConstants<double>::SPEED_OF_LIGHT;
		constexpr double pi = std::numbers::pi_v<double>;
		constexpr double degrees = pi / 180.0;
		const double wavelength = c / settings.frequency_hz;
		dataset.wavelength_m = wavelength;

		const double ra = settings.source_right_ascension_deg * degrees;
		const double dec = settings.source_declination_deg * degrees;
		const std::array<double, 3> source{std::cos(dec) * std::cos(ra), std::cos(dec) * std::sin(ra), std::sin(dec)};
		const std::array<double, 3> east{-std::sin(ra), std::cos(ra), 0.0};
		const std::array<double, 3> north{-std::sin(dec) * std::cos(ra), -std::sin(dec) * std::sin(ra), std::cos(dec)};
		const double position_angle = settings.image_position_angle_deg * degrees;
		const double right_l = -std::cos(position_angle);
		const double right_m = std::sin(position_angle);
		const double up_l = std::sin(position_angle);
		const double up_m = std::cos(position_angle);
		const double minimum_elevation = settings.minimum_elevation_deg * degrees;

		const size_t epoch_count = static_cast<size_t>(std::floor(settings.duration_hours * 3600.0 / settings.cadence_s)) + 1;
		dataset.epoch_count = epoch_count;
		dataset.station_visible_epochs.assign(array.stations.size(), 0U);

		std::vector<std::vector<std::array<double, 3>>> positions(epoch_count, std::vector<std::array<double, 3>>(active.size()));
		std::vector<std::vector<uint8_t>> visible(epoch_count, std::vector<uint8_t>(active.size(), 0U));
		std::vector<double> epoch_mjd(epoch_count);
		std::vector<double> epoch_time(epoch_count);

		for (size_t k = 0; k < epoch_count; ++k) {
			const double elapsed = static_cast<double>(k) * settings.cadence_s;
			const double mjd = settings.start_mjd + settings.start_hour_ut / 24.0 + elapsed / 86400.0;
			const double days = mjd + 2400000.5 - 2451545.0;
			const double fractional = 0.7790572732640 + 1.00273781191135448 * days;
			const double era = 2.0 * pi * (fractional - std::floor(fractional));
			epoch_mjd[k] = mjd;
			epoch_time[k] = settings.start_hour_ut * 3600.0 + elapsed;
			for (size_t a = 0; a < active.size(); ++a) {
				const VlbiStation& station = array.stations[active[a]];
				positions[k][a] = station.inertial_position(era, elapsed);
				const bool sees_source = source_visible(station, positions[k][a], source, minimum_elevation);
				visible[k][a] = sees_source ? 1U : 0U;
				if (sees_source) {
					++dataset.station_visible_epochs[active[a]];
				}
			}
		}

		const size_t station_count = active.size();
		std::vector<std::vector<int32_t>> sample_lookup(epoch_count, std::vector<int32_t>(station_count * station_count, -1));
		for (size_t k = 0; k < epoch_count; ++k) {
			for (size_t a = 0; a < station_count; ++a) {
				if (visible[k][a] == 0U) continue;
				for (size_t b = a + 1; b < station_count; ++b) {
					if (visible[k][b] == 0U) continue;
					const std::array<double, 3> baseline{positions[k][b][0] - positions[k][a][0], positions[k][b][1] - positions[k][a][1], positions[k][b][2] - positions[k][a][2]};
					VisibilitySample sample;
					sample.epoch_index = static_cast<uint32_t>(k);
					sample.station_a = static_cast<uint32_t>(active[a]);
					sample.station_b = static_cast<uint32_t>(active[b]);
					sample.time_seconds = epoch_time[k];
					sample.mjd = epoch_mjd[k];
					sample.u_meters = Detail::dot3(baseline, east);
					sample.v_meters = Detail::dot3(baseline, north);
					sample.u_lambda = sample.u_meters / wavelength;
					sample.v_lambda = sample.v_meters / wavelength;
					sample.image_u_lambda = sample.u_lambda * right_l + sample.v_lambda * right_m;
					sample.image_v_lambda = sample.u_lambda * up_l + sample.v_lambda * up_m;
					const double cycles_u = pixel_scale_rad * sample.image_u_lambda;
					const double cycles_v = pixel_scale_rad * sample.image_v_lambda;
					sample.flagged = (std::abs(cycles_u) >= 0.5) || (std::abs(cycles_v) >= 0.5);
					sample_lookup[k][a * station_count + b] = static_cast<int32_t>(dataset.samples.size());
					dataset.samples.push_back(sample);
				}
			}
		}

		if (dataset.samples.empty()) {
			dataset.message = "No baseline sees the source during the requested observing window.";
			return dataset;
		}

		Detail::parallel_for_each(dataset.samples.size(), [&](size_t index) {
			VisibilitySample& sample = dataset.samples[index];
			if (sample.flagged) return;
			const double cycles_u = pixel_scale_rad * sample.image_u_lambda;
			const double cycles_v = pixel_scale_rad * sample.image_v_lambda;
			sample.noiseless_value = settings.total_flux_jy * evaluate_visibility(normalized, n, cycles_u, cycles_v);
			sample.value = sample.noiseless_value;
		}, control, 0.08f, 0.9f);

		if (control != nullptr && control->cancel.load(std::memory_order_relaxed)) {
			dataset.cancelled = true;
			dataset.message = "Synthesis cancelled.";
			return dataset;
		}

		Detail::GaussianNoise noise(settings.noise_seed);
		const double bandwidth_time = 2.0 * std::max(settings.bandwidth_hz, 1.0) * std::max(settings.integration_time_s, 1e-6);
		double snr_sum = 0.0;
		size_t snr_count = 0;
		double minimum_baseline = 1e300;
		double maximum_baseline = 0.0;
		for (VisibilitySample& sample : dataset.samples) {
			if (sample.flagged) {
				++dataset.flagged_count;
				continue;
			}
			const double sefd_a = array.stations[sample.station_a].sefd_jy;
			const double sefd_b = array.stations[sample.station_b].sefd_jy;
			sample.sigma_jy = settings.thermal_noise ? std::sqrt(sefd_a * sefd_b / bandwidth_time) / std::max(settings.quantization_efficiency, 1e-3) : 0.0;
			if (settings.thermal_noise) {
				const auto gaussian = noise.next_pair();
				sample.value += std::complex<double>(sample.sigma_jy * gaussian.first, sample.sigma_jy * gaussian.second);
			}
			const double baseline = std::hypot(sample.u_lambda, sample.v_lambda);
			minimum_baseline = std::min(minimum_baseline, baseline);
			maximum_baseline = std::max(maximum_baseline, baseline);
			if (sample.sigma_jy > 0.0) {
				snr_sum += std::abs(sample.value) / sample.sigma_jy;
				++snr_count;
			}
		}
		dataset.minimum_baseline_lambda = (minimum_baseline < 1e299) ? minimum_baseline : 0.0;
		dataset.maximum_baseline_lambda = maximum_baseline;
		dataset.mean_snr = (snr_count > 0) ? (snr_sum / static_cast<double>(snr_count)) : 0.0;

		for (size_t k = 0; k < epoch_count; ++k) {
			for (size_t a = 0; a < station_count; ++a) {
				for (size_t b = a + 1; b < station_count; ++b) {
					const int32_t ab = sample_lookup[k][a * station_count + b];
					if (ab < 0 || dataset.samples[static_cast<size_t>(ab)].flagged) continue;
					for (size_t d = b + 1; d < station_count; ++d) {
						const int32_t bc = sample_lookup[k][b * station_count + d];
						const int32_t ac = sample_lookup[k][a * station_count + d];
						if (bc < 0 || ac < 0) continue;
						const VisibilitySample& s_ab = dataset.samples[static_cast<size_t>(ab)];
						const VisibilitySample& s_bc = dataset.samples[static_cast<size_t>(bc)];
						const VisibilitySample& s_ac = dataset.samples[static_cast<size_t>(ac)];
						if (s_bc.flagged || s_ac.flagged) continue;
						const std::complex<double> bispectrum = s_ab.value * s_bc.value * std::conj(s_ac.value);
						const double amp_ab = std::max(std::abs(s_ab.value), 1e-30);
						const double amp_bc = std::max(std::abs(s_bc.value), 1e-30);
						const double amp_ac = std::max(std::abs(s_ac.value), 1e-30);
						const double relative = std::sqrt(std::pow(s_ab.sigma_jy / amp_ab, 2.0) + std::pow(s_bc.sigma_jy / amp_bc, 2.0) + std::pow(s_ac.sigma_jy / amp_ac, 2.0));
						ClosurePhaseSample closure;
						closure.epoch_index = static_cast<uint32_t>(k);
						closure.station_a = static_cast<uint32_t>(active[a]);
						closure.station_b = static_cast<uint32_t>(active[b]);
						closure.station_c = static_cast<uint32_t>(active[d]);
						closure.time_seconds = epoch_time[k];
						closure.mjd = epoch_mjd[k];
						closure.u1_meters = s_ab.u_meters;
						closure.v1_meters = s_ab.v_meters;
						closure.u2_meters = s_bc.u_meters;
						closure.v2_meters = s_bc.v_meters;
						closure.phase_rad = std::arg(bispectrum);
						closure.amplitude = std::abs(bispectrum);
						closure.phase_error_rad = std::min(relative, pi);
						closure.amplitude_error = closure.amplitude * relative;
						dataset.closures.push_back(closure);
					}
				}
			}
		}

		dataset.valid = (dataset.samples.size() > dataset.flagged_count);
		dataset.message = dataset.valid
			? ("Synthesized " + std::to_string(dataset.samples.size() - dataset.flagged_count) + " visibilities and " + std::to_string(dataset.closures.size()) + " closure phases.")
			: "Every baseline exceeds the Nyquist limit of the image grid. Increase the image resolution or reduce the field of view.";
		if (control != nullptr) {
			control->progress.store(1.0f);
		}
		return dataset;
	}

private:
	[[nodiscard]] static bool source_visible(const VlbiStation& station, const std::array<double, 3>& position, const std::array<double, 3>& source, double minimum_elevation) noexcept {
		const double radius = std::sqrt(Detail::dot3(position, position));
		if (radius <= 0.0) return false;
		const double along = Detail::dot3(position, source);
		if (station.kind == StationKind::Ground) {
			return (along / radius) >= std::sin(minimum_elevation);
		}
		if (along >= 0.0) return true;
		return (radius * radius - along * along) > kEarthEquatorialRadiusMeters * kEarthEquatorialRadiusMeters;
	}

	[[nodiscard]] static std::complex<double> evaluate_visibility(const std::vector<double>& pixels, uint32_t n, double cycles_u, double cycles_v) noexcept {
		constexpr double two_pi = 2.0 * std::numbers::pi_v<double>;
		std::array<double, kMaximumImageSize> column_cos{};
		std::array<double, kMaximumImageSize> column_sin{};
		const double center = 0.5 * static_cast<double>(n - 1);
		for (uint32_t i = 0; i < n; ++i) {
			const double angle = -two_pi * cycles_u * (static_cast<double>(i) - center);
			column_cos[i] = std::cos(angle);
			column_sin[i] = std::sin(angle);
		}
		double real = 0.0;
		double imaginary = 0.0;
		for (uint32_t j = 0; j < n; ++j) {
			const double* row = pixels.data() + static_cast<size_t>(j) * n;
			double row_real = 0.0;
			double row_imaginary = 0.0;
			for (uint32_t i = 0; i < n; ++i) {
				row_real += row[i] * column_cos[i];
				row_imaginary += row[i] * column_sin[i];
			}
			const double angle = -two_pi * cycles_v * (center - static_cast<double>(j));
			const double row_cos = std::cos(angle);
			const double row_sin = std::sin(angle);
			real += row_real * row_cos - row_imaginary * row_sin;
			imaginary += row_real * row_sin + row_imaginary * row_cos;
		}
		return {real, imaginary};
	}

	static void build_plane(const std::vector<double>& normalized, uint32_t n, double pixel_scale_rad, VlbiDataset& dataset) {
		size_t padded = 1;
		while (padded < static_cast<size_t>(n) * 2) {
			padded <<= 1;
		}
		std::vector<std::complex<double>> grid(padded * padded, std::complex<double>(0.0, 0.0));
		for (size_t j = 0; j < n; ++j) {
			for (size_t i = 0; i < n; ++i) {
				grid[j * padded + i] = normalized[j * n + i];
			}
		}
		Detail::fft2d_inplace(grid, padded);

		dataset.plane_size = static_cast<uint32_t>(padded);
		dataset.plane_cell_lambda = 1.0 / (static_cast<double>(padded) * pixel_scale_rad);
		dataset.plane_log_amplitude.assign(padded * padded, -6.0);
		const size_t half = padded / 2;
		const size_t bins = half;
		std::vector<double> bin_sum(bins, 0.0);
		std::vector<double> bin_count(bins, 0.0);
		for (size_t row = 0; row < padded; ++row) {
			const size_t source_row = (row + half) % padded;
			for (size_t column = 0; column < padded; ++column) {
				const size_t source_column = (column + half) % padded;
				const double amplitude = std::abs(grid[source_row * padded + source_column]);
				dataset.plane_log_amplitude[row * padded + column] = std::log10(std::max(amplitude, 1e-6));
				const double du = static_cast<double>(column) - static_cast<double>(half);
				const double dv = static_cast<double>(row) - static_cast<double>(half);
				const size_t radius = static_cast<size_t>(std::lround(std::sqrt(du * du + dv * dv)));
				if (radius < bins) {
					bin_sum[radius] += amplitude;
					bin_count[radius] += 1.0;
				}
			}
		}
		dataset.profile_baseline_glambda.reserve(bins);
		dataset.profile_amplitude.reserve(bins);
		for (size_t r = 0; r < bins; ++r) {
			if (bin_count[r] <= 0.0) continue;
			dataset.profile_baseline_glambda.push_back(static_cast<double>(r) * dataset.plane_cell_lambda * 1e-9);
			dataset.profile_amplitude.push_back(bin_sum[r] / bin_count[r]);
		}

		dataset.nyquist_baseline_lambda = 0.5 / pixel_scale_rad;
		const auto& amplitude = dataset.profile_amplitude;
		for (size_t i = 2; i + 1 < amplitude.size(); ++i) {
			if (amplitude[i] < amplitude[i - 1] && amplitude[i] <= amplitude[i + 1] && amplitude[i] < 0.6 * amplitude[0]) {
				const double curvature = amplitude[i - 1] - 2.0 * amplitude[i] + amplitude[i + 1];
				const double offset = (std::abs(curvature) > 1e-30) ? std::clamp(0.5 * (amplitude[i - 1] - amplitude[i + 1]) / curvature, -0.5, 0.5) : 0.0;
				dataset.first_null_baseline_lambda = dataset.profile_baseline_glambda[i] * 1e9 + offset * dataset.plane_cell_lambda;
				if (dataset.first_null_baseline_lambda > 0.0) {
					dataset.estimated_ring_diameter_rad = 0.7655 / dataset.first_null_baseline_lambda;
				}
				break;
			}
		}
	}
};

}
