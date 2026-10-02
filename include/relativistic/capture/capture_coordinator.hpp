#pragma once

#include "relativistic/capture/camera_path.hpp"
#include "relativistic/capture/capture_progress.hpp"
#include "relativistic/core/engine_log.hpp"
#include "relativistic/io/image_format.hpp"
#include "relativistic/io/image_stream_writers.hpp"
#include "relativistic/io/screenshot_capture_settings.hpp"
#include "relativistic/io/screenshot_exporter.hpp"
#include "relativistic/io/video_capture_settings.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/render/geodesic_compute_pipeline.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

namespace Relativistic::Capture {

inline constexpr uint32_t kMaxCaptureAxisPixels = 65535U;
inline constexpr uint32_t kMaxSourceAxisPixels = 262144U;
inline constexpr uint32_t kMaxSupersampling = 8U;
inline constexpr uint64_t kSourceBandPixelBudget = 1ULL << 21U;
inline constexpr uint32_t kMaxQueuedFrames = 2U;

struct RequestResult {
	bool ok{false};
	std::string message{};
};

struct CaptureTarget {
	uint32_t width{1920};
	uint32_t height{1080};
	uint32_t supersampling{1};
	uint32_t max_ray_steps{0};
	float step_refinement{1.0f};
};

struct ScreenshotRequest {
	CaptureTarget target{};
	std::string output_directory{};
	std::string filename_pattern{};
	IO::ScreenshotFormat format{IO::ScreenshotFormat::PNG};
	IO::ScreenshotOverwritePolicy overwrite_policy{IO::ScreenshotOverwritePolicy::AutoIncrement};
	std::string comment{};
};

struct SequenceRequest {
	CaptureTarget target{};
	IO::VideoSequenceSettings settings{};
	std::string output_directory{};
	std::string session_name{};
	std::string comment{};
	bool use_path{false};
	CameraPath path{};
	bool manual_stepping{false};
};

namespace Detail {

class SrgbTables {
public:
	[[nodiscard]] static const SrgbTables& instance() {
		static const SrgbTables tables;
		return tables;
	}

	[[nodiscard]] float decode(float encoded) const noexcept {
		const float clamped = std::clamp(encoded, 0.0f, 1.0f);
		return decode_[static_cast<size_t>(clamped * static_cast<float>(kDecodeSize - 1) + 0.5f)];
	}

	[[nodiscard]] float encode(float linear) const noexcept {
		const float clamped = std::clamp(linear, 0.0f, 1.0f);
		return encode_[static_cast<size_t>(clamped * static_cast<float>(kEncodeSize - 1) + 0.5f)];
	}

private:
	static constexpr size_t kDecodeSize = 4096;
	static constexpr size_t kEncodeSize = 16384;

	std::vector<float> decode_;
	std::vector<float> encode_;

	SrgbTables() : decode_(kDecodeSize), encode_(kEncodeSize) {
		for (size_t i = 0; i < kDecodeSize; ++i) {
			const double v = static_cast<double>(i) / static_cast<double>(kDecodeSize - 1);
			decode_[i] = static_cast<float>((v <= 0.04045) ? (v / 12.92) : std::pow((v + 0.055) / 1.055, 2.4));
		}
		for (size_t i = 0; i < kEncodeSize; ++i) {
			const double v = static_cast<double>(i) / static_cast<double>(kEncodeSize - 1);
			encode_[i] = static_cast<float>((v <= 0.0031308) ? (12.92 * v) : (1.055 * std::pow(v, 1.0 / 2.4) - 0.055));
		}
	}
};

}

class CaptureCoordinator {
public:
	using ConstantsProvider = std::function<std::optional<Render::GpuCameraPushConstants>()>;
	using BodiesProvider = std::function<std::vector<Render::GpuBodyData>()>;

private:
	struct FrameSample {
		Render::GpuCameraPushConstants constants{};
		std::vector<Render::GpuBodyData> bodies{};
	};

	struct FrameJob {
		std::vector<FrameSample> samples{};
		std::filesystem::path path{};
		uint32_t width{0};
		uint32_t height{0};
		uint32_t supersampling{1};
		IO::ScreenshotFormat format{IO::ScreenshotFormat::PNG};
		std::string comment{};
		float fade{1.0f};
	};

	struct SequenceSession {
		bool active{false};
		bool realtime{false};
		bool stopping{false};
		bool cancelled{false};
		SequenceRequest request{};
		std::filesystem::path directory{};
		std::string extension{};
		uint64_t frames_total{0};
		uint64_t frames_prepared{0};
		double ticks_per_frame{0.0};
		double tick_accumulator{0.0};
		double session_time{0.0};
		double frame_accumulator{0.0};
		bool dimensions_locked{false};
		uint32_t locked_width{0};
		uint32_t locked_height{0};
		uint32_t manual_requests{0};
		bool paused_by_session{false};
		Orchestrator::CameraState saved_camera{};
		double saved_exposure{0.0};
		double saved_fov{60.0};
		uint32_t saved_camera_mode{0};
	};

	Orchestrator::SimulationOrchestrator<1024>& orchestrator_;
	Render::GeodesicComputePipeline& pipeline_;
	ConstantsProvider constants_provider_{};
	BodiesProvider bodies_provider_{};
	CaptureProgress progress_{};
	SequenceSession session_{};
	std::atomic<uint32_t> pending_tasks_{0};
	std::atomic<bool> sequence_failed_{false};
	std::mutex queue_mutex_;
	std::condition_variable_any queue_cv_;
	std::deque<std::function<void()>> tasks_;
	std::jthread worker_;

	void worker_loop(std::stop_token stop) {
		for (;;) {
			std::function<void()> task;
			{
				std::unique_lock<std::mutex> lock(queue_mutex_);
				if (!queue_cv_.wait(lock, stop, [this]() { return !tasks_.empty(); })) {
					return;
				}
				task = std::move(tasks_.front());
				tasks_.pop_front();
			}
			try {
				task();
			} catch (const std::exception& ex) {
				record_failure(std::string("Capture task failed: ") + ex.what());
			} catch (...) {
				record_failure("Capture task failed with an unknown error.");
			}
			pending_tasks_.fetch_sub(1U, std::memory_order_acq_rel);
		}
	}

	void enqueue(std::function<void()> task) {
		{
			std::lock_guard<std::mutex> lock(queue_mutex_);
			pending_tasks_.fetch_add(1U, std::memory_order_acq_rel);
			tasks_.push_back(std::move(task));
		}
		queue_cv_.notify_one();
	}

	void record_failure(const std::string& message) {
		Core::log_error(message);
		progress_.set_message(message);
		sequence_failed_.store(true, std::memory_order_release);
	}

	static void prepare_constants(Render::GpuCameraPushConstants& constants, const CaptureTarget& target) noexcept {
		constants.interlace_mode = 0U;
		constants.interlace_phase = 0U;
		constants.render_flags &= ~(Render::RenderFlags::USE_LOD_SYSTEM | Render::RenderFlags::ADAPTIVE_TILE_PREPASS);
		if (target.max_ray_steps > 0U) {
			constants.max_integration_steps = std::clamp(target.max_ray_steps, 64U, 65536U);
		}
		const double refinement = std::clamp(static_cast<double>(target.step_refinement), 1.0, 32.0);
		constants.step_size_factor /= refinement;
		constants.min_step_size /= refinement;
		constants.max_step_size /= refinement;
	}

	static void accumulate_source_band(std::span<const Render::GpuPixelOutput> source, uint32_t width, uint32_t rows, uint32_t k, std::vector<float>& accumulation) {
		const auto& tables = Detail::SrgbTables::instance();
		const size_t source_width = static_cast<size_t>(width) * k;
		for (uint32_t oy = 0; oy < rows; ++oy) {
			float* destination = accumulation.data() + static_cast<size_t>(oy) * width * 3U;
			for (uint32_t ky = 0; ky < k; ++ky) {
				const Render::GpuPixelOutput* row = source.data() + (static_cast<size_t>(oy) * k + ky) * source_width;
				for (uint32_t ox = 0; ox < width; ++ox) {
					const Render::GpuPixelOutput* pixel = row + static_cast<size_t>(ox) * k;
					float r = 0.0f;
					float g = 0.0f;
					float b = 0.0f;
					for (uint32_t kx = 0; kx < k; ++kx) {
						r += tables.decode(pixel[kx].r);
						g += tables.decode(pixel[kx].g);
						b += tables.decode(pixel[kx].b);
					}
					destination[static_cast<size_t>(ox) * 3U + 0U] += r;
					destination[static_cast<size_t>(ox) * 3U + 1U] += g;
					destination[static_cast<size_t>(ox) * 3U + 2U] += b;
				}
			}
		}
	}

	static void resolve_accumulation(const std::vector<float>& accumulation, float weight, std::vector<Render::GpuPixelOutput>& output) {
		const auto& tables = Detail::SrgbTables::instance();
		for (size_t i = 0; i < output.size(); ++i) {
			output[i] = Render::GpuPixelOutput{
				.r = tables.encode(accumulation[i * 3U + 0U] * weight),
				.g = tables.encode(accumulation[i * 3U + 1U] * weight),
				.b = tables.encode(accumulation[i * 3U + 2U] * weight),
				.a = 1.0f
			};
		}
	}

	[[nodiscard]] static std::vector<Render::GpuPixelOutput> resample_nearest(
		const std::vector<Render::GpuPixelOutput>& source,
		uint32_t source_width,
		uint32_t source_height,
		uint32_t target_width,
		uint32_t target_height
	) {
		std::vector<Render::GpuPixelOutput> result(static_cast<size_t>(target_width) * target_height);
		for (uint32_t y = 0; y < target_height; ++y) {
			const uint32_t sy = std::min<uint32_t>(static_cast<uint32_t>((static_cast<uint64_t>(y) * source_height) / target_height), source_height - 1U);
			for (uint32_t x = 0; x < target_width; ++x) {
				const uint32_t sx = std::min<uint32_t>(static_cast<uint32_t>((static_cast<uint64_t>(x) * source_width) / target_width), source_width - 1U);
				result[static_cast<size_t>(y) * target_width + x] = source[static_cast<size_t>(sy) * source_width + sx];
			}
		}
		return result;
	}

	[[nodiscard]] bool render_frame_job(const FrameJob& job) {
		const std::atomic<bool>* cancel = progress_.cancel_flag();
		const auto fail = [this](const std::string& message) {
			progress_.set_message(message);
			return false;
		};

		auto writer = IO::make_image_stream_writer(job.format);
		if (!writer->begin(job.path, job.width, job.height, job.comment)) {
			return fail("The output file could not be created: " + job.path.string());
		}

		const uint32_t k = job.supersampling;
		const uint32_t band_rows = rows_per_band(job.width, job.height, k);
		const uint32_t band_count = (job.height + band_rows - 1U) / band_rows;
		progress_.set_bands(0, band_count);

		const size_t sample_count = job.samples.size();
		const bool needs_resolve = (k > 1U) || (sample_count > 1U) || (job.fade < 0.9999f);
		const float weight = job.fade / static_cast<float>(static_cast<size_t>(k) * k * sample_count);

		std::vector<Render::GpuPixelOutput> source_band;
		std::vector<Render::GpuPixelOutput> output_band;
		std::vector<float> accumulation;

		const auto render_sample = [&](const FrameSample& sample, uint32_t first_row, uint32_t rows, std::vector<Render::GpuPixelOutput>& destination) {
			Render::GpuCameraPushConstants constants = sample.constants;
			constants.screen_width = job.width * k;
			constants.screen_height = job.height * k;
			constants.body_count = static_cast<uint32_t>(sample.bodies.size());
			return pipeline_.render_capture_band(constants, sample.bodies, first_row, rows, destination, cancel);
		};

		uint32_t bands_done = 0;
		for (uint32_t row = 0; row < job.height; row += band_rows) {
			if (cancel != nullptr && cancel->load(std::memory_order_acquire)) {
				writer->abort();
				return false;
			}
			const uint32_t rows = std::min(band_rows, job.height - row);
			output_band.resize(static_cast<size_t>(job.width) * rows);

			if (!needs_resolve) {
				if (!render_sample(job.samples.front(), row, rows, output_band)) {
					writer->abort();
					return fail("A render band failed or was interrupted.");
				}
			} else {
				accumulation.assign(static_cast<size_t>(job.width) * rows * 3U, 0.0f);
				for (const FrameSample& sample : job.samples) {
					if (!render_sample(sample, row * k, rows * k, source_band)) {
						writer->abort();
						return fail("A render band failed or was interrupted.");
					}
					accumulate_source_band(source_band, job.width, rows, k, accumulation);
				}
				resolve_accumulation(accumulation, weight, output_band);
			}

			if (!writer->write_rows(output_band)) {
				writer->abort();
				return fail("Writing image rows to disk failed: " + job.path.string());
			}
			progress_.set_bands(++bands_done, band_count);
		}
		if (!writer->finish()) {
			return fail("Finalizing the image file failed: " + job.path.string());
		}
		return true;
	}

	void run_frame_job(const std::shared_ptr<FrameJob>& job, bool single) {
		if (render_frame_job(*job)) {
			progress_.add_frame();
			if (single) {
				progress_.finish(CapturePhase::Completed, "Saved " + job->path.string());
			}
			return;
		}
		if (progress_.cancel_requested()) {
			if (single) {
				progress_.finish(CapturePhase::Cancelled, "Capture cancelled.");
			}
			return;
		}
		if (single) {
			progress_.finish(CapturePhase::Failed, progress_.message());
		} else {
			sequence_failed_.store(true, std::memory_order_release);
		}
	}

	void apply_pose(const CameraPose& pose) noexcept {
		auto& camera = orchestrator_.camera();
		camera.position = pose.position;
		camera.pitch = std::clamp(pose.pitch_deg, -89.0, 89.0);
		camera.yaw = pose.yaw_deg;
		camera.roll = pose.roll_deg;
		camera.fov_deg = std::clamp(pose.fov_deg, 5.0, 175.0);
		camera.velocity = {0.0, 0.0, 0.0};
		camera.synchronize_spherical();
		camera.orbit_distance = camera.radius;
		auto& parameters = orchestrator_.parameters();
		parameters.camera_fov_deg = camera.fov_deg;
		parameters.camera_exposure = std::clamp(pose.exposure_ev, -6.0, 6.0);
		orchestrator_.notify_state_changed();
	}

	void step_one_tick() noexcept {
		auto& scheduler = orchestrator_.scheduler();
		scheduler.request_steps(1);
		if (scheduler.advance_tick()) {
			orchestrator_.advance_simulation(scheduler.tick_dt() * scheduler.warp_factor());
		}
	}

	[[nodiscard]] bool prepare_deterministic_frame() {
		const SequenceRequest& request = session_.request;
		const IO::VideoSequenceSettings& settings = request.settings;
		const uint64_t index = session_.frames_prepared;
		const double fps = std::max(static_cast<double>(settings.frames_per_second), 1.0e-3);
		const double frame_dt = 1.0 / fps;
		const uint32_t sample_count = std::max(settings.temporal_samples, 1U);

		double base_time = static_cast<double>(index) * frame_dt;
		if (settings.trigger == IO::SequenceCaptureTrigger::PathDuration && request.use_path && session_.frames_total > 1) {
			base_time = request.path.effective_duration() * static_cast<double>(index) / static_cast<double>(session_.frames_total - 1U);
		}

		auto job = std::make_shared<FrameJob>();
		job->samples.reserve(sample_count);
		for (uint32_t s = 0; s < sample_count; ++s) {
			const double offset = ((static_cast<double>(s) + 0.5) / static_cast<double>(sample_count) - 0.5) * static_cast<double>(settings.shutter_fraction) * frame_dt;
			session_.tick_accumulator += session_.ticks_per_frame / static_cast<double>(sample_count);
			while (session_.tick_accumulator >= 1.0) {
				step_one_tick();
				session_.tick_accumulator -= 1.0;
			}
			if (request.use_path) {
				std::array<double, 3> dynamic_target{0.0, 0.0, 0.0};
				if (request.path.tracked_body_id >= 0) {
					const auto& bodies = orchestrator_.nbody_system().bodies();
					for (const auto& b : bodies) {
						if (static_cast<int32_t>(b.id) == request.path.tracked_body_id) {
							dynamic_target = b.position;
							break;
						}
					}
				}
				apply_pose(request.path.evaluate(std::max(base_time + offset, 0.0), dynamic_target));
			}
			auto constants = constants_provider_ ? constants_provider_() : std::nullopt;
			if (!constants.has_value()) {
				record_failure("The renderer has no valid frame to derive the capture state from.");
				return false;
			}
			FrameSample sample;
			sample.constants = *constants;
			prepare_constants(sample.constants, request.target);
			if (bodies_provider_) {
				sample.bodies = bodies_provider_();
			}
			job->samples.push_back(std::move(sample));
		}

		float fade = 1.0f;
		if (session_.frames_total > 0) {
			if (index < settings.fade_in_frames) {
				fade = std::min(fade, static_cast<float>(index + 1U) / static_cast<float>(settings.fade_in_frames + 1U));
			}
			const uint64_t remaining = session_.frames_total - index;
			if (remaining <= settings.fade_out_frames) {
				fade = std::min(fade, static_cast<float>(remaining) / static_cast<float>(settings.fade_out_frames + 1U));
			}
		}

		job->path = session_.directory / settings.frame_file_name(settings.start_frame_index + index, session_.extension);
		job->width = request.target.width;
		job->height = request.target.height;
		job->supersampling = request.target.supersampling;
		job->format = settings.frame_format;
		job->comment = request.comment;
		job->fade = fade;
		++session_.frames_prepared;
		enqueue([this, job]() { run_frame_job(job, false); });
		return true;
	}

	void update_deterministic() {
		const bool limit_reached = session_.frames_total > 0 && session_.frames_prepared >= session_.frames_total;
		const bool manual_ready = !session_.request.manual_stepping || session_.manual_requests > 0U;
		if (session_.stopping || limit_reached || !manual_ready || pending_tasks_.load(std::memory_order_acquire) >= kMaxQueuedFrames) {
			return;
		}
		if (session_.request.manual_stepping) {
			--session_.manual_requests;
		}
		if (!prepare_deterministic_frame()) {
			session_.stopping = true;
		}
	}

	void submit_realtime_frame(uint64_t copies) {
		std::vector<Render::GpuPixelOutput> pixels;
		uint32_t width = 0;
		uint32_t height = 0;
		pipeline_.copy_framebuffer(pixels, width, height);
		if (width == 0U || height == 0U || pixels.size() != static_cast<size_t>(width) * static_cast<size_t>(height)) {
			return;
		}
		if (!session_.dimensions_locked) {
			session_.locked_width = width;
			session_.locked_height = height;
			session_.dimensions_locked = true;
		}
		if (width != session_.locked_width || height != session_.locked_height) {
			pixels = resample_nearest(pixels, width, height, session_.locked_width, session_.locked_height);
		}
		const auto shared = std::make_shared<const std::vector<Render::GpuPixelOutput>>(std::move(pixels));
		const IO::VideoSequenceSettings& settings = session_.request.settings;
		for (uint64_t c = 0; c < copies; ++c) {
			if (pending_tasks_.load(std::memory_order_acquire) >= std::max(settings.realtime_queue_depth, 1U)) {
				progress_.add_dropped(1);
				continue;
			}
			const std::filesystem::path path = session_.directory / settings.frame_file_name(settings.start_frame_index + session_.frames_prepared, session_.extension);
			++session_.frames_prepared;
			if (c > 0) {
				progress_.add_duplicated(1);
			}
			const uint32_t locked_width = session_.locked_width;
			const uint32_t locked_height = session_.locked_height;
			const IO::ScreenshotFormat format = settings.frame_format;
			const std::string comment = session_.request.comment;
			enqueue([this, shared, path, locked_width, locked_height, format, comment]() {
				if (IO::write_image_file(path, *shared, locked_width, locked_height, format, comment)) {
					progress_.add_frame();
				} else {
					record_failure("Writing a real-time frame failed: " + path.string());
				}
			});
		}
	}

	void update_realtime(double dt) {
		const IO::VideoSequenceSettings& settings = session_.request.settings;
		session_.session_time += dt;
		if (session_.request.use_path) {
			std::array<double, 3> dynamic_target{0.0, 0.0, 0.0};
			if (session_.request.path.tracked_body_id >= 0) {
				const auto& bodies = orchestrator_.nbody_system().bodies();
				for (const auto& b : bodies) {
					if (static_cast<int32_t>(b.id) == session_.request.path.tracked_body_id) {
						dynamic_target = b.position;
						break;
					}
				}
			}
			apply_pose(session_.request.path.evaluate(session_.session_time, dynamic_target));
		}
		const bool limit_reached = session_.frames_total > 0 && session_.frames_prepared >= session_.frames_total;
		if (session_.stopping || limit_reached) {
			return;
		}
		const double interval = 1.0 / std::max(static_cast<double>(settings.frames_per_second), 1.0e-3);
		session_.frame_accumulator += dt;
		if (session_.frame_accumulator < interval) {
			return;
		}
		const uint64_t missed = std::max<uint64_t>(static_cast<uint64_t>(std::floor(session_.frame_accumulator / interval)), 1U);
		session_.frame_accumulator -= static_cast<double>(missed) * interval;
		const bool duplicate = settings.pacing == IO::RealTimePacing::DuplicateToFill;
		if (!duplicate && missed > 1U) {
			progress_.add_dropped(missed - 1U);
		}
		submit_realtime_frame(duplicate ? missed : 1U);
	}

	void restore_session_state() noexcept {
		auto& parameters = orchestrator_.parameters();
		if (session_.request.settings.restore_camera_after_capture || session_.request.use_path) {
			orchestrator_.camera() = session_.saved_camera;
			parameters.camera_fov_deg = session_.saved_fov;
			parameters.camera_exposure = session_.saved_exposure;
		}
		parameters.camera_mode = session_.saved_camera_mode;
		if (session_.paused_by_session) {
			orchestrator_.scheduler().resume();
		}
		orchestrator_.notify_state_changed();
	}

	void write_manifest() const {
		const IO::VideoSequenceSettings& settings = session_.request.settings;
		std::ofstream info(session_.directory / "sequence_info.txt", std::ios::trunc);
		if (info.is_open()) {
			info << "mode=" << (session_.realtime ? "real-time" : "deterministic") << '\n';
			info << "frames_submitted=" << session_.frames_prepared << '\n';
			info << "frames_written=" << progress_.frames_done() << '\n';
			info << "frames_dropped=" << progress_.frames_dropped() << '\n';
			info << "frames_duplicated=" << progress_.frames_duplicated() << '\n';
			info << "frames_per_second=" << settings.frames_per_second << '\n';
			info << "frame_format=" << IO::image_format_descriptor(settings.frame_format).display_name << '\n';
			info << "start_frame_index=" << settings.start_frame_index << '\n';
			if (!session_.realtime) {
				info << "resolution=" << session_.request.target.width << 'x' << session_.request.target.height << '\n';
				info << "supersampling=" << session_.request.target.supersampling << '\n';
				info << "temporal_samples=" << settings.temporal_samples << '\n';
			}
			const std::string command = assembly_command();
			if (!command.empty()) {
				info << "ffmpeg=" << command << '\n';
			}
		}
		if (session_.request.use_path) {
			std::ofstream path_file(session_.directory / "camera_path.txt", std::ios::trunc);
			if (path_file.is_open()) {
				path_file << session_.request.path.to_text();
			}
		}
	}

	[[nodiscard]] std::string assembly_command() const {
		return session_.request.settings.build_ffmpeg_command(
			session_.directory.string(),
			session_.extension,
			(session_.directory / "video").string()
		);
	}

	void finalize_session() {
		const CapturePhase outcome = session_.cancelled ? CapturePhase::Cancelled
			: (sequence_failed_.load(std::memory_order_acquire) ? CapturePhase::Failed : CapturePhase::Completed);
		restore_session_state();
		write_manifest();
		const std::string summary = std::to_string(progress_.frames_done()) + " frame(s) written to " + session_.directory.string();
		const IO::VideoSequenceSettings& settings = session_.request.settings;
		const std::string command = assembly_command();
		const bool assemble = outcome == CapturePhase::Completed && settings.assemble_video_after_capture && !command.empty() && progress_.frames_done() > 0;
		session_.active = false;
		if (!assemble) {
			progress_.finish(outcome, outcome == CapturePhase::Failed ? progress_.message() : summary);
			return;
		}
		progress_.set_phase(CapturePhase::Assembling);
		const std::filesystem::path directory = session_.directory;
		const std::string extension = session_.extension;
		const bool delete_frames = settings.delete_frames_after_assembly;
		enqueue([this, command, directory, extension, delete_frames, summary]() {
#if defined(_WIN32)
			const std::string invocation = "cmd.exe /C \"" + command + "\"";
#else
			const std::string invocation = command;
#endif
			const int status = std::system(invocation.c_str());
			if (status != 0) {
				progress_.finish(CapturePhase::Failed, "ffmpeg returned a non-zero status; the frames were kept in " + directory.string());
				return;
			}
			if (delete_frames) {
				std::error_code ec;
				for (const auto& entry : std::filesystem::directory_iterator(directory, ec)) {
					const std::string name = entry.path().filename().string();
					if (name.rfind("frame_", 0) == 0 && entry.path().extension() == ("." + extension)) {
						std::filesystem::remove(entry.path(), ec);
					}
				}
			}
			progress_.finish(CapturePhase::Completed, summary + " and assembled into a video.");
		});
	}

public:
	CaptureCoordinator(Orchestrator::SimulationOrchestrator<1024>& orchestrator, Render::GeodesicComputePipeline& pipeline)
		: orchestrator_(orchestrator), pipeline_(pipeline) {
		worker_ = std::jthread([this](std::stop_token stop) { worker_loop(stop); });
	}

	~CaptureCoordinator() {
		shutdown();
	}

	CaptureCoordinator(const CaptureCoordinator&) = delete;
	CaptureCoordinator& operator=(const CaptureCoordinator&) = delete;

	void bind_frame_sources(ConstantsProvider constants_provider, BodiesProvider bodies_provider) {
		constants_provider_ = std::move(constants_provider);
		bodies_provider_ = std::move(bodies_provider);
	}

	void shutdown() {
		progress_.request_cancel();
		worker_.request_stop();
		queue_cv_.notify_all();
		if (worker_.joinable()) {
			worker_.join();
		}
	}

	[[nodiscard]] static uint32_t rows_per_band(uint32_t width, uint32_t height, uint32_t supersampling) noexcept {
		const uint64_t row_cost = std::max<uint64_t>(static_cast<uint64_t>(width) * supersampling * supersampling, 1U);
		return static_cast<uint32_t>(std::clamp<uint64_t>(kSourceBandPixelBudget / row_cost, 1U, std::max<uint32_t>(height, 1U)));
	}

	[[nodiscard]] static std::string validate_target(const CaptureTarget& target, IO::ScreenshotFormat format) {
		if (target.width == 0U || target.height == 0U) {
			return "The capture resolution must be strictly positive.";
		}
		if (target.width > kMaxCaptureAxisPixels || target.height > kMaxCaptureAxisPixels) {
			return "Each output axis is limited to " + std::to_string(kMaxCaptureAxisPixels) + " pixels.";
		}
		if (target.supersampling == 0U || target.supersampling > kMaxSupersampling) {
			return "Supersampling must be between 1 and " + std::to_string(kMaxSupersampling) + ".";
		}
		if (static_cast<uint64_t>(target.width) * target.supersampling > kMaxSourceAxisPixels || static_cast<uint64_t>(target.height) * target.supersampling > kMaxSourceAxisPixels) {
			return "The supersampled internal resolution exceeds " + std::to_string(kMaxSourceAxisPixels) + " pixels per axis.";
		}
		const auto error = IO::validate_image_dimensions(format, target.width, target.height);
		return error.value_or(std::string{});
	}

	[[nodiscard]] CaptureProgress& progress() noexcept { return progress_; }
	[[nodiscard]] const CaptureProgress& progress() const noexcept { return progress_; }

	[[nodiscard]] bool is_busy() const noexcept {
		return session_.active || pending_tasks_.load(std::memory_order_acquire) > 0U;
	}

	[[nodiscard]] bool is_sequence_active() const noexcept { return session_.active; }
	[[nodiscard]] bool is_manual_stepping() const noexcept { return session_.active && !session_.realtime && session_.request.manual_stepping; }
	[[nodiscard]] uint32_t manual_frames_pending() const noexcept { return session_.manual_requests; }
	[[nodiscard]] bool locks_live_resolution() const noexcept { return session_.active && session_.realtime; }
	[[nodiscard]] bool drives_camera() const noexcept { return session_.active && session_.request.use_path; }
	[[nodiscard]] bool suppresses_live_render() const noexcept { return session_.active && !session_.realtime && !session_.request.settings.preview_in_viewport; }

	[[nodiscard]] float live_resolution_multiplier() const noexcept {
		return locks_live_resolution() ? std::clamp(session_.request.settings.resolution_scale, 0.1f, 4.0f) : 1.0f;
	}

	void request_manual_frame() noexcept {
		if (is_manual_stepping() && session_.manual_requests < 64U) {
			++session_.manual_requests;
		}
	}

	void preview_pose(const CameraPose& pose) noexcept {
		apply_pose(pose);
	}

	void stop_sequence() noexcept {
		if (session_.active) {
			session_.stopping = true;
		}
	}

	void cancel_all() noexcept {
		progress_.request_cancel();
		if (session_.active) {
			session_.stopping = true;
			session_.cancelled = true;
		}
	}

	[[nodiscard]] RequestResult request_screenshot(const ScreenshotRequest& request) {
		if (is_busy()) {
			return {false, "A capture is already in progress."};
		}
		const std::string error = validate_target(request.target, request.format);
		if (!error.empty()) {
			return {false, error};
		}
		auto constants = constants_provider_ ? constants_provider_() : std::nullopt;
		if (!constants.has_value()) {
			return {false, "No frame has been rendered yet; wait for the first frame and retry."};
		}

		IO::ScreenshotCaptureContext context;
		context.metric_name = orchestrator_.active_metric_name();
		context.mass = orchestrator_.parameters().mass;
		context.spin = orchestrator_.parameters().spin;
		context.width = request.target.width;
		context.height = request.target.height;
		context.tick_index = orchestrator_.scheduler().snapshot().tick_index;
		const std::string stem = IO::ScreenshotFilenameBuilder::build(request.filename_pattern, context);
		const auto resolved = IO::ScreenshotExporter::resolve_output_path(request.output_directory, stem, request.format, request.overwrite_policy);
		if (!resolved.has_value()) {
			return {false, "The output file already exists or the output directory is not writable."};
		}

		auto job = std::make_shared<FrameJob>();
		FrameSample sample;
		sample.constants = *constants;
		prepare_constants(sample.constants, request.target);
		if (bodies_provider_) {
			sample.bodies = bodies_provider_();
		}
		job->samples.push_back(std::move(sample));
		job->path = *resolved;
		job->width = request.target.width;
		job->height = request.target.height;
		job->supersampling = request.target.supersampling;
		job->format = request.format;
		job->comment = request.comment;

		sequence_failed_.store(false, std::memory_order_release);
		progress_.begin("Screenshot", 1);
		enqueue([this, job]() { run_frame_job(job, true); });
		return {true, resolved->string()};
	}

	[[nodiscard]] RequestResult start_sequence(const SequenceRequest& request) {
		if (is_busy()) {
			return {false, "A capture is already in progress."};
		}
		const IO::VideoSequenceSettings& settings = request.settings;
		const bool realtime = (settings.mode == IO::SequenceCaptureMode::RealTime);
		if (!realtime) {
			const std::string error = validate_target(request.target, settings.frame_format);
			if (!error.empty()) {
				return {false, error};
			}
		}

		uint64_t total_frames = 0;
		switch (settings.trigger) {
			case IO::SequenceCaptureTrigger::FixedDuration:
				total_frames = static_cast<uint64_t>(std::max(std::round(settings.duration_seconds * settings.frames_per_second), 1.0f));
				break;
			case IO::SequenceCaptureTrigger::FixedFrameCount:
				total_frames = std::max<uint64_t>(settings.fixed_frame_count, 1ULL);
				break;
			case IO::SequenceCaptureTrigger::PathDuration:
				if (request.use_path) {
					total_frames = static_cast<uint64_t>(std::max(std::round(request.path.effective_duration() * static_cast<double>(settings.frames_per_second)), 1.0));
				} else {
					total_frames = static_cast<uint64_t>(std::max(std::round(settings.duration_seconds * settings.frames_per_second), 1.0f));
				}
				break;
			case IO::SequenceCaptureTrigger::Manual:
			case IO::SequenceCaptureTrigger::Continuous:
			default:
				total_frames = 0;
				break;
		}

		const std::string stem = request.session_name.empty() ? IO::ScreenshotExporter::expand_filename_pattern("sequence_%Y%m%d_%H%M%S") : request.session_name;
		std::filesystem::path dir = std::filesystem::path(request.output_directory) / stem;
		std::error_code ec;
		std::filesystem::create_directories(dir, ec);
		if (ec) {
			return {false, "Failed to create output directory: " + dir.string()};
		}

		session_ = SequenceSession{};
		session_.active = true;
		session_.realtime = realtime;
		session_.stopping = false;
		session_.cancelled = false;
		session_.request = request;
		session_.directory = std::move(dir);
		session_.extension = image_format_descriptor(settings.frame_format).extension;
		session_.frames_total = total_frames;
		session_.frames_prepared = 0;
		session_.session_time = 0.0;
		session_.frame_accumulator = 0.0;
		session_.tick_accumulator = 0.0;
		session_.dimensions_locked = false;
		session_.locked_width = 0;
		session_.locked_height = 0;
		session_.manual_requests = 0;

		switch (settings.advance_mode) {
			case IO::SequenceAdvanceMode::TicksPerFrame:
				session_.ticks_per_frame = static_cast<double>(settings.ticks_per_frame);
				break;
			case IO::SequenceAdvanceMode::SimulationRateRatio: {
				const double dt_tick = orchestrator_.scheduler().tick_dt();
				const double fps = std::max(static_cast<double>(settings.frames_per_second), 1.0e-3);
				session_.ticks_per_frame = (dt_tick > 0.0) ? (settings.simulation_seconds_per_video_second / (fps * dt_tick)) : 0.0;
				break;
			}
			case IO::SequenceAdvanceMode::FrozenWorld:
			default:
				session_.ticks_per_frame = 0.0;
				break;
		}

		auto& camera = orchestrator_.camera();
		auto& parameters = orchestrator_.parameters();
		session_.saved_camera = camera;
		session_.saved_fov = parameters.camera_fov_deg;
		session_.saved_exposure = parameters.camera_exposure;
		session_.saved_camera_mode = parameters.camera_mode;

		if (!realtime && settings.pause_simulation_during_capture) {
			session_.paused_by_session = !orchestrator_.scheduler().is_paused();
			orchestrator_.scheduler().pause();
		}

		if (request.use_path) {
			apply_pose(request.path.evaluate(0.0));
		}

		sequence_failed_.store(false, std::memory_order_release);
		progress_.begin(realtime ? "Real-Time Capture" : "Sequence Capture", total_frames);
		return {true, session_.directory.string()};
	}

	void update(double dt) {
		if (!session_.active) {
			return;
		}
		if (progress_.cancel_requested()) {
			session_.stopping = true;
			session_.cancelled = true;
		}
		if (!session_.stopping) {
			if (session_.realtime) {
				update_realtime(dt);
			} else {
				update_deterministic();
			}
		}

		const bool limit_reached = session_.frames_total > 0 && session_.frames_prepared >= session_.frames_total;
		if (session_.stopping || limit_reached) {
			if (pending_tasks_.load(std::memory_order_acquire) == 0U) {
				finalize_session();
			}
		}
	}
};

}
