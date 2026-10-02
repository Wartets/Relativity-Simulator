#pragma once

#include "relativistic/capture/capture_coordinator.hpp"
#include "relativistic/capture/motion_script.hpp"
#include "relativistic/capture/path_preview_builder.hpp"
#include "relativistic/io/capture_studio_settings.hpp"
#include "relativistic/io/image_format.hpp"
#include "relativistic/io/recording_settings.hpp"
#include "relativistic/io/screenshot_capture_settings.hpp"
#include "relativistic/io/screenshot_exporter.hpp"
#include "relativistic/io/user_settings.hpp"
#include "relativistic/io/video_capture_settings.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/ui/capture_widgets.hpp"
#include "relativistic/ui/motion_script_editor.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include "relativistic/ui/viewport_primary_window.hpp"
#include <imgui.h>
#include <algorithm>
#include <array>
#include <bit>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

namespace Relativistic::UI {

namespace CaptureStudioDetail {

struct ResolutionPreset {
	const char* label;
	uint32_t width;
	uint32_t height;
};

inline constexpr std::array<ResolutionPreset, 10> kResolutionPresets{{
	{"720p", 1280, 720},
	{"1080p", 1920, 1080},
	{"1440p", 2560, 1440},
	{"4K", 3840, 2160},
	{"5K", 5120, 2880},
	{"8K", 7680, 4320},
	{"16K", 15360, 8640},
	{"Square 2K", 2048, 2048},
	{"Square 4K", 4096, 4096},
	{"Square 8K", 8192, 8192}
}};

inline constexpr std::array<const char*, 2> kCaptureModeNames{"Deterministic (Frame By Frame)", "Real-Time Recording"};
inline constexpr std::array<const char*, 5> kTriggerNames{"Manual (Until Stopped)", "Fixed Duration", "Fixed Frame Count", "Continuous", "Motion Script Duration"};
inline constexpr std::array<const char*, 3> kAdvanceNames{"Frozen World", "Fixed Ticks Per Frame", "Simulation Seconds Per Video Second"};
inline constexpr std::array<const char*, 2> kPacingNames{"Drop Late Frames", "Duplicate To Fill Gaps"};
inline constexpr std::array<const char*, 9> kCodecNames{"H.264 (libx264)", "H.265 (libx265)", "VP9 (libvpx-vp9)", "ProRes 422 HQ", "Image Sequence Only", "AV1 (SVT-AV1)", "FFV1 (Lossless)", "Motion JPEG", "Animated GIF"};
inline constexpr std::array<const char*, 6> kContainerNames{"MP4", "MKV", "MOV", "WebM", "AVI", "GIF"};
inline constexpr std::array<const char*, 2> kRateControlNames{"Constant Quality (CRF)", "Target Bitrate"};
inline constexpr std::array<const char*, 9> kEncoderSpeedNames{"Ultrafast", "Superfast", "Veryfast", "Faster", "Fast", "Medium", "Slow", "Slower", "Veryslow"};
inline constexpr std::array<const char*, 3> kOverwriteNames{"Auto-Increment Filename", "Overwrite Existing File", "Skip If File Exists"};
inline constexpr std::array<const char*, 7> kRecordingFormatNames{"CSV", "TSV", "JSON (Single Document)", "JSON Lines", "Binary Columns (RCAP)", "Container (Typed Datasets)", "VTK Polyline (ParaView)"};
inline constexpr std::array<const char*, 6> kRecordingPresetNames{"None", "Trajectory", "Camera Kinematics", "Full Kinematics", "Relativistic Physics", "Everything"};

[[nodiscard]] inline ImVec4 phase_color(Capture::CapturePhase phase) noexcept {
	switch (phase) {
		case Capture::CapturePhase::Rendering: return ImVec4(1.0f, 0.8f, 0.3f, 1.0f);
		case Capture::CapturePhase::Assembling: return ImVec4(0.6f, 0.8f, 1.0f, 1.0f);
		case Capture::CapturePhase::Completed: return ImVec4(0.4f, 1.0f, 0.5f, 1.0f);
		case Capture::CapturePhase::Cancelled: return ImVec4(1.0f, 0.65f, 0.3f, 1.0f);
		case Capture::CapturePhase::Failed: return ImVec4(1.0f, 0.4f, 0.35f, 1.0f);
		case Capture::CapturePhase::Idle:
		default: return ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
	}
}

[[nodiscard]] inline const std::array<const char*, IO::kScreenshotFormatCount>& format_names() {
	static const std::array<const char*, IO::kScreenshotFormatCount> names = [] {
		std::array<const char*, IO::kScreenshotFormatCount> table{};
		for (size_t i = 0; i < table.size(); ++i) {
			table[i] = IO::image_format_descriptor(static_cast<IO::ScreenshotFormat>(i)).display_name.data();
		}
		return table;
	}();
	return names;
}

}

class CaptureStudioWindow {
private:
	enum class StudioTab : int {
		None = -1,
		Screenshot = 0,
		Sequence = 1,
		Script = 2,
		Recording = 3,
		Encoding = 4
	};

	static constexpr float kFooterHeight = 196.0f;

	bool is_open_{false};
	Orchestrator::SimulationOrchestrator<1024>& orchestrator_;
	ViewportPrimaryWindow& viewport_;
	IO::UserSettings& user_settings_;
	IO::CaptureStudioSettings settings_;
	Capture::RequestResult last_request_{};
	MotionScriptEditor script_editor_{};
	StudioTab requested_tab_{StudioTab::None};

	Capture::PathPreview preview_{};
	bool preview_enabled_{true};
	bool preview_published_{false};
	bool preview_force_rebuild_{true};
	bool preview_options_dirty_{false};
	bool preview_playing_{false};
	bool preview_active_{false};
	uint32_t preview_samples_{128};
	bool preview_show_markers_{true};
	bool preview_show_labels_{true};
	bool preview_show_samples_{false};
	bool preview_show_frustum_{true};
	double preview_frustum_length_{10.0};
	double preview_speed_{1.0};
	double preview_time_{0.0};
	double published_time_{-1.0};
	int32_t published_highlight_{-2};
	double last_preview_build_{0.0};
	Capture::CameraPose preview_restore_pose_{};

	[[nodiscard]] IO::ScreenshotFormat screenshot_format() const noexcept {
		return static_cast<IO::ScreenshotFormat>(std::min<uint32_t>(user_settings_.screenshot_format, IO::kScreenshotFormatCount - 1U));
	}

	[[nodiscard]] Capture::CameraPose current_camera_pose() const noexcept {
		const auto& camera = orchestrator_.camera();
		Capture::CameraPose pose;
		pose.position = camera.position;
		pose.pitch_deg = camera.pitch;
		pose.yaw_deg = camera.yaw;
		pose.roll_deg = camera.roll;
		pose.fov_deg = camera.fov_deg;
		pose.exposure_ev = orchestrator_.parameters().camera_exposure;
		return pose;
	}

	[[nodiscard]] Capture::CaptureTarget resolve_target(const IO::CaptureTargetSettings& source) const noexcept {
		Capture::CaptureTarget target;
		if (source.use_explicit_resolution) {
			target.width = (source.explicit_width + 1U) & ~1U;
			target.height = (source.explicit_height + 1U) & ~1U;
		} else {
			const ImVec2 size = viewport_.content_size();
			const double multiplier = static_cast<double>(source.resolution_multiplier);
			const uint32_t w = static_cast<uint32_t>(std::clamp<long long>(std::llround(static_cast<double>(size.x) * multiplier), 2LL, 1000000LL));
			const uint32_t h = static_cast<uint32_t>(std::clamp<long long>(std::llround(static_cast<double>(size.y) * multiplier), 2LL, 1000000LL));
			target.width = (w + 1U) & ~1U;
			target.height = (h + 1U) & ~1U;
		}
		target.supersampling = source.supersampling;
		target.max_ray_steps = source.max_ray_steps;
		target.step_refinement = source.step_refinement;
		return target;
	}

	[[nodiscard]] bool script_usable() const {
		return settings_.use_script && settings_.script.is_usable();
	}

	[[nodiscard]] uint64_t planned_frame_count() const {
		const auto& video = settings_.video;
		const double fps = static_cast<double>(video.frames_per_second);
		switch (video.trigger) {
			case IO::SequenceCaptureTrigger::FixedDuration:
				return static_cast<uint64_t>(std::max(std::round(static_cast<double>(video.duration_seconds) * fps), 1.0));
			case IO::SequenceCaptureTrigger::FixedFrameCount:
				return std::max<uint64_t>(video.fixed_frame_count, 1ULL);
			case IO::SequenceCaptureTrigger::PathDuration:
				return static_cast<uint64_t>(std::max(std::round((script_usable() ? settings_.script.total_duration() : static_cast<double>(video.duration_seconds)) * fps), 1.0));
			case IO::SequenceCaptureTrigger::Manual:
			case IO::SequenceCaptureTrigger::Continuous:
			default:
				return 0;
		}
	}

	[[nodiscard]] bool script_tracks_bodies() const {
		for (const auto& segment : settings_.script.segments) {
			if (segment.enabled && segment.anchor == Capture::AnchorMode::TrackBody) {
				return true;
			}
			if (segment.orientation.mode == Capture::OrientationMode::LookAtTarget && segment.orientation.target_body >= 0) {
				return true;
			}
		}
		return false;
	}

	void release_preview() {
		if (preview_published_) {
			viewport_.set_path_preview(nullptr);
			preview_published_ = false;
		}
		preview_force_rebuild_ = true;
		published_time_ = -1.0;
		published_highlight_ = -2;
	}

	void update_preview(double dt) {
		auto& coordinator = viewport_.capture_coordinator();
		if (!preview_enabled_ || !settings_.script.is_usable() || coordinator.is_sequence_active()) {
			static_cast<void>(script_editor_.consume_modified());
			release_preview();
			return;
		}

		const double total = settings_.script.total_duration();
		if (preview_playing_) {
			preview_time_ += dt * preview_speed_;
			if (preview_time_ > total) {
				preview_time_ = std::fmod(preview_time_, total);
			}
		}
		if (const auto request = script_editor_.take_cursor_request()) {
			preview_time_ = *request;
		}
		preview_time_ = std::clamp(preview_time_, 0.0, total);

		const int32_t highlighted = script_editor_.selected_slot(settings_.script);
		const double now = ImGui::GetTime();
		const bool edited = script_editor_.consume_modified();
		const bool rebuild = preview_force_rebuild_ || edited || preview_options_dirty_ || (script_tracks_bodies() && (now - last_preview_build_) > 0.2);
		if (!rebuild && preview_time_ == published_time_ && highlighted == published_highlight_ && preview_published_) {
			return;
		}

		const auto lookup = Capture::make_body_position_lookup(orchestrator_);
		if (rebuild) {
			Capture::PathPreviewOptions options;
			options.samples_per_segment = preview_samples_;
			options.frustum_length = preview_frustum_length_;
			options.show_markers = preview_show_markers_;
			options.show_labels = preview_show_labels_;
			options.show_samples = preview_show_samples_;
			options.show_frustum = preview_show_frustum_;
			options.cursor_seconds = preview_time_;
			options.highlighted_segment = highlighted;
			preview_ = Capture::build_path_preview(settings_.script, lookup, options);
			last_preview_build_ = now;
			preview_force_rebuild_ = false;
			preview_options_dirty_ = false;
		} else {
			preview_.cursor = Capture::build_preview_cursor(settings_.script, lookup, preview_time_);
			preview_.highlighted_segment = highlighted;
		}
		viewport_.set_path_preview(&preview_);
		preview_published_ = true;
		published_time_ = preview_time_;
		published_highlight_ = highlighted;
	}

	void render_target_editor(IO::CaptureTargetSettings& target, IO::ScreenshotFormat format, const char* id) {
		using namespace CaptureWidgets;
		using namespace CaptureStudioDetail;
		ImGui::PushID(id);

		ImGui::Checkbox("Explicit Output Resolution", &target.use_explicit_resolution);
		render_setting_tooltip("When enabled the output size is fixed in pixels; otherwise it is the viewport content area multiplied by the resolution multiplier.");

		if (target.use_explicit_resolution) {
			int width = static_cast<int>(target.explicit_width);
			int height = static_cast<int>(target.explicit_height);
			if (ImGui::InputInt("Width (px)", &width, 16, 256)) {
				target.explicit_width = static_cast<uint32_t>(std::clamp(width, 16, 65535));
			}
			if (ImGui::InputInt("Height (px)", &height, 16, 256)) {
				target.explicit_height = static_cast<uint32_t>(std::clamp(height, 16, 65535));
			}
			for (size_t i = 0; i < kResolutionPresets.size(); ++i) {
				if (i % 5 != 0) {
					ImGui::SameLine();
				}
				ImGui::PushID(static_cast<int>(i));
				if (ImGui::SmallButton(kResolutionPresets[i].label)) {
					target.explicit_width = kResolutionPresets[i].width;
					target.explicit_height = kResolutionPresets[i].height;
				}
				ImGui::PopID();
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Viewport Size")) {
				const ImVec2 size = viewport_.content_size();
				target.explicit_width = static_cast<uint32_t>(std::clamp(static_cast<int>(std::lround(size.x)), 16, 65535));
				target.explicit_height = static_cast<uint32_t>(std::clamp(static_cast<int>(std::lround(size.y)), 16, 65535));
			}
			render_setting_tooltip("Copies the current viewport content size into the explicit resolution.");
		} else {
			float multiplier = target.resolution_multiplier;
			if (ImGui::SliderFloat("Resolution Multiplier", &multiplier, 0.1f, 16.0f, "%.2fx", ImGuiSliderFlags_Logarithmic)) {
				target.resolution_multiplier = multiplier;
			}
			render_setting_tooltip("Scales the viewport content area. Values above 4x are rendered in bands and streamed to disk, so memory stays bounded.");
			const ImVec2 size = viewport_.content_size();
			ImGui::TextDisabled("Viewport content area: %.0f x %.0f px", static_cast<double>(size.x), static_cast<double>(size.y));
		}

		slider_u32("Supersampling (NxN)", target.supersampling, 1U, Capture::kMaxSupersampling);
		render_setting_tooltip("Traces NxN rays per output pixel and averages them in linear light for anti-aliasing.");
		slider_u32("Integration Steps", target.max_ray_steps, 64U, 65536U, ImGuiSliderFlags_Logarithmic);
		render_setting_tooltip("Maximum geodesic integration steps per ray, independent of the live viewport budget.");
		ImGui::SliderFloat("Step Refinement", &target.step_refinement, 1.0f, 32.0f, "%.1fx", ImGuiSliderFlags_Logarithmic);
		render_setting_tooltip("Divides the integration step size and its limits for higher accuracy near the photon sphere and the horizon.");

		const Capture::CaptureTarget resolved = resolve_target(target);
		const uint64_t internal_width = static_cast<uint64_t>(resolved.width) * resolved.supersampling;
		const uint64_t internal_height = static_cast<uint64_t>(resolved.height) * resolved.supersampling;
		ImGui::TextDisabled("Output: %u x %u px | Traced: %llu x %llu px (%.1f Mpx)", resolved.width, resolved.height, static_cast<unsigned long long>(internal_width), static_cast<unsigned long long>(internal_height), static_cast<double>(internal_width * internal_height) / 1.0e6);
		ImGui::TextDisabled("Uncompressed size per frame: %s", format_bytes(IO::estimate_image_file_bytes(format, resolved.width, resolved.height)).c_str());
		const std::string error = Capture::CaptureCoordinator::validate_target(resolved, format);
		if (!error.empty()) {
			wrapped_text(kWarningColor, error);
		}
		ImGui::PopID();
	}

	void render_output_tab() {
		using namespace CaptureWidgets;
		using namespace CaptureStudioDetail;
		ImGui::TextColored(kHeaderColor, "Destination");
		input_text("Output Directory", user_settings_.screenshot_output_directory);
		render_setting_tooltip("Folder where screenshots are written. It is created automatically when missing.");
		input_text("Filename Pattern", user_settings_.screenshot_filename_pattern);
		render_setting_tooltip("Tokens: %metric%, %mass%, %spin%, %width%, %height%, %tick%, plus any strftime token such as %Y %m %d %H %M %S.");
		ImGui::SameLine();
		if (ImGui::SmallButton("Smart Name")) {
			user_settings_.screenshot_filename_pattern = "%metric%_M%mass%_a%spin%_%width%x%height%_%Y%m%d_%H%M%S";
		}
		render_setting_tooltip("Applies a descriptive pattern containing the metric, its parameters, the resolution and a timestamp.");

		int format_index = static_cast<int>(std::min<uint32_t>(user_settings_.screenshot_format, IO::kScreenshotFormatCount - 1U));
		if (ImGui::Combo("File Format", &format_index, format_names().data(), static_cast<int>(IO::kScreenshotFormatCount))) {
			user_settings_.screenshot_format = static_cast<uint32_t>(format_index);
		}
		const IO::ScreenshotFormat format = screenshot_format();
		const auto& descriptor = IO::image_format_descriptor(format);
		ImGui::TextDisabled("Alpha: %s | Float samples: %s | Metadata comment: %s | Max side: %llu px", descriptor.preserves_alpha ? "yes" : "no", descriptor.stores_float_samples ? "yes" : "no", descriptor.supports_comment ? "yes" : "no", static_cast<unsigned long long>(descriptor.max_dimension));

		index_combo("If File Exists", user_settings_.screenshot_overwrite_policy, kOverwriteNames);
		render_setting_tooltip("Rule applied when the target file already exists on disk.");

		ImGui::BeginDisabled(!descriptor.supports_comment);
		ImGui::Checkbox("Embed Watermark / Comment", &user_settings_.screenshot_watermark_enabled);
		if (user_settings_.screenshot_watermark_enabled) {
			input_text("Watermark Text", user_settings_.screenshot_watermark_text);
		}
		ImGui::EndDisabled();
		render_setting_tooltip("Stores the text as metadata in formats that support it (PNG, PNG16, HDR, TIFF).");

		IO::ScreenshotCaptureContext context;
		const Capture::CaptureTarget resolved = resolve_target(settings_.screenshot_target);
		context.metric_name = orchestrator_.active_metric_name();
		context.mass = orchestrator_.parameters().mass;
		context.spin = orchestrator_.parameters().spin;
		context.width = resolved.width;
		context.height = resolved.height;
		context.tick_index = orchestrator_.scheduler().snapshot().tick_index;
		ImGui::TextDisabled("Next file: %s%s", IO::ScreenshotFilenameBuilder::build(user_settings_.screenshot_filename_pattern, context).c_str(), IO::ScreenshotExporter::extension_for_format(format).c_str());

		ImGui::Separator();
		ImGui::TextColored(kHeaderColor, "Quality And Resolution");
		render_target_editor(settings_.screenshot_target, format, "ScreenshotTarget");
	}

	void render_sequence_tab() {
		using namespace CaptureWidgets;
		using namespace CaptureStudioDetail;
		auto& video = settings_.video;

		ImGui::TextColored(kHeaderColor, "Capture Mode");
		enum_combo("Mode", video.mode, kCaptureModeNames);
		render_setting_tooltip("Deterministic renders every frame offline at the requested quality while the world is advanced under full control. Real-Time records what the viewport currently shows.");
		const bool realtime = video.mode == IO::SequenceCaptureMode::RealTime;

		ImGui::Separator();
		ImGui::TextColored(kHeaderColor, "Camera Control");
		if (script_usable()) {
			ImGui::TextDisabled("Motion script '%s': %zu segment(s), %zu event(s), %.3f s", settings_.script.name.c_str(), settings_.script.active_segments().size(), settings_.script.events.size(), settings_.script.total_duration());
		} else if (settings_.use_script) {
			wrapped_text(kWarningColor, "The motion script is enabled but has no active segment, the live camera will be used.");
		} else {
			ImGui::TextDisabled("The live camera is used. Enable the motion script in the Motion Script tab to automate it.");
		}
		if (ImGui::SmallButton("Open Motion Script Tab")) {
			requested_tab_ = StudioTab::Script;
		}
		if (script_usable() && video.trigger != IO::SequenceCaptureTrigger::PathDuration) {
			ImGui::SameLine();
			if (ImGui::SmallButton("Use Script Duration")) {
				video.trigger = IO::SequenceCaptureTrigger::PathDuration;
			}
		}

		ImGui::Separator();
		ImGui::TextColored(kHeaderColor, "Timing");
		ImGui::SliderFloat("Frame Rate", &video.frames_per_second, 1.0f, 240.0f, "%.2f fps");
		for (const float preset : {24.0f, 25.0f, 30.0f, 50.0f, 60.0f, 120.0f}) {
			char label[16];
			std::snprintf(label, sizeof(label), "%.0f", static_cast<double>(preset));
			ImGui::SameLine();
			if (ImGui::SmallButton(label)) {
				video.frames_per_second = preset;
			}
		}
		enum_combo("Stop Condition", video.trigger, kTriggerNames);
		if (video.trigger == IO::SequenceCaptureTrigger::FixedDuration) {
			ImGui::SliderFloat("Duration", &video.duration_seconds, 0.1f, 3600.0f, "%.2f s", ImGuiSliderFlags_Logarithmic);
		} else if (video.trigger == IO::SequenceCaptureTrigger::FixedFrameCount) {
			ImGui::InputScalar("Frame Count", ImGuiDataType_U64, &video.fixed_frame_count);
			video.fixed_frame_count = std::max<uint64_t>(video.fixed_frame_count, 1ULL);
		} else if (video.trigger == IO::SequenceCaptureTrigger::PathDuration) {
			if (script_usable()) {
				ImGui::TextDisabled("Follows the motion script duration: %.3f s", settings_.script.total_duration());
			} else {
				ImGui::SliderFloat("Fallback Duration", &video.duration_seconds, 0.1f, 3600.0f, "%.2f s", ImGuiSliderFlags_Logarithmic);
				wrapped_text(kWarningColor, "No usable motion script is enabled, the fallback duration is used.");
			}
		}
		const uint64_t frames = planned_frame_count();
		if (frames > 0) {
			ImGui::TextDisabled("Planned: %llu frames | Video length: %.2f s", static_cast<unsigned long long>(frames), static_cast<double>(frames) / static_cast<double>(video.frames_per_second));
		} else {
			ImGui::TextDisabled("Open-ended capture: stop it manually.");
		}

		ImGui::Separator();
		ImGui::TextColored(kHeaderColor, "Output");
		input_text("Sequence Directory", settings_.sequence_directory);
		input_text("Session Name (empty = automatic)", settings_.session_name);
		render_setting_tooltip("Name of the sub-folder created for this capture. strftime tokens are supported; empty generates a timestamped name.");
		int frame_format_index = static_cast<int>(std::min<uint32_t>(static_cast<uint32_t>(video.frame_format), IO::kScreenshotFormatCount - 1U));
		if (ImGui::Combo("Frame Format", &frame_format_index, format_names().data(), static_cast<int>(IO::kScreenshotFormatCount))) {
			video.frame_format = static_cast<IO::ScreenshotFormat>(frame_format_index);
		}
		render_setting_tooltip("Image format of each written frame. Use a lossless 8-bit format for encoding with ffmpeg.");
		ImGui::InputScalar("First Frame Index", ImGuiDataType_U64, &video.start_frame_index);
		slider_u32("Frame Number Padding", video.frame_name_padding, 1U, 12U);

		ImGui::Separator();
		if (realtime) {
			ImGui::TextColored(kHeaderColor, "Real-Time Recording");
			ImGui::SliderFloat("Live Resolution Scale", &video.resolution_scale, 0.1f, 4.0f, "%.2fx", ImGuiSliderFlags_Logarithmic);
			render_setting_tooltip("Locks the viewport render scale while recording so every frame has identical dimensions.");
			enum_combo("Frame Pacing", video.pacing, kPacingNames);
			render_setting_tooltip("Drop skips frames when the renderer is slower than the capture rate; Duplicate repeats the latest frame to keep the video timeline exact.");
			slider_u32("Write Queue Depth", video.realtime_queue_depth, 1U, 64U);
			render_setting_tooltip("Frames allowed to wait for disk writing before new ones are dropped.");
			ImGui::TextDisabled("The simulation keeps running live and the camera can be flown freely while recording.");
		} else {
			ImGui::TextColored(kHeaderColor, "Frame Quality");
			render_target_editor(settings_.sequence_target, video.frame_format, "SequenceTarget");

			ImGui::Separator();
			ImGui::TextColored(kHeaderColor, "Motion Blur And Fades");
			slider_u32("Temporal Samples", video.temporal_samples, 1U, 64U);
			render_setting_tooltip("Renders several sub-frame poses per output frame and averages them in linear light for motion blur.");
			ImGui::BeginDisabled(video.temporal_samples < 2U);
			ImGui::SliderFloat("Shutter Fraction", &video.shutter_fraction, 0.0f, 1.0f, "%.2f");
			render_setting_tooltip("Portion of the frame interval covered by the virtual shutter.");
			ImGui::EndDisabled();
			slider_u32("Fade In Frames", video.fade_in_frames, 0U, 600U);
			slider_u32("Fade Out Frames", video.fade_out_frames, 0U, 600U);

			ImGui::Separator();
			ImGui::TextColored(kHeaderColor, "Stepping Control");
			ImGui::Checkbox("Manual Frame Stepping", &settings_.manual_stepping);
			render_setting_tooltip("Renders a frame only when you request it with the Render Next Frame button, allowing inspection and camera adjustments between frames.");
			ImGui::Checkbox("Pause Simulation During Capture", &video.pause_simulation_during_capture);
			ImGui::Checkbox("Preview Frames In Viewport", &video.preview_in_viewport);
			render_setting_tooltip("Keeps the live viewport rendering while frames are being captured, at the cost of speed.");
		}

		ImGui::Separator();
		ImGui::TextColored(kHeaderColor, "World Advance");
		enum_combo("Advance Mode", video.advance_mode, kAdvanceNames);
		if (video.advance_mode == IO::SequenceAdvanceMode::TicksPerFrame) {
			slider_u32("Ticks Per Frame", video.ticks_per_frame, 0U, 1000U, ImGuiSliderFlags_Logarithmic);
		} else if (video.advance_mode == IO::SequenceAdvanceMode::SimulationRateRatio) {
			drag_double("Simulation Seconds Per Video Second", video.simulation_seconds_per_video_second, 0.01, 0.0, 1.0e9, "%.4f");
			const double tick_dt = orchestrator_.scheduler().tick_dt();
			if (tick_dt > 0.0) {
				ImGui::TextDisabled("Equals %.3f ticks per frame at the current scheduler rate.", video.simulation_seconds_per_video_second / (static_cast<double>(video.frames_per_second) * tick_dt));
			}
		}
		ImGui::Checkbox("Restore Camera After Capture", &video.restore_camera_after_capture);

		if (!realtime && frames > 0) {
			const Capture::CaptureTarget resolved = resolve_target(settings_.sequence_target);
			ImGui::Separator();
			ImGui::TextDisabled("Estimated raw disk usage: %s", format_bytes(IO::estimate_image_file_bytes(video.frame_format, resolved.width, resolved.height) * frames).c_str());
		}
	}

	void render_preview_controls() {
		using namespace CaptureWidgets;
		auto& coordinator = viewport_.capture_coordinator();
		const bool session_running = coordinator.is_sequence_active();
		const auto& script = settings_.script;

		if (ImGui::Checkbox("Show Path Preview In Viewport", &preview_enabled_)) {
			preview_force_rebuild_ = true;
		}
		render_setting_tooltip("Draws the full trajectory, segment boundaries, events and the camera frustum directly on the viewport without rendering a single frame.");
		ImGui::SameLine();
		bool schematic = orchestrator_.parameters().schematic_mode_enabled;
		if (ImGui::Checkbox("Schematic Viewport", &schematic)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SchematicModeEnabled, schematic ? 1.0 : 0.0)));
		}
		render_setting_tooltip("Switches the main viewport to the schematic projection where the preview is drawn without ray tracing, so it updates instantly while editing.");

		bool options_changed = false;
		options_changed |= slider_u32("Samples Per Segment", preview_samples_, 16U, 2048U, ImGuiSliderFlags_Logarithmic);
		options_changed |= ImGui::Checkbox("Markers", &preview_show_markers_);
		ImGui::SameLine();
		options_changed |= ImGui::Checkbox("Labels", &preview_show_labels_);
		ImGui::SameLine();
		options_changed |= ImGui::Checkbox("Sample Points", &preview_show_samples_);
		ImGui::SameLine();
		options_changed |= ImGui::Checkbox("Camera Frustum", &preview_show_frustum_);
		options_changed |= drag_double("Frustum Length", preview_frustum_length_, 0.1, 0.5, 10000.0, "%.2f");
		if (options_changed) {
			preview_options_dirty_ = true;
		}

		ImGui::BeginDisabled(session_running || !script.is_usable());
		const double total = script.total_duration();
		const double zero = 0.0;
		double time = preview_time_;
		if (ImGui::SliderScalar("Preview Time", ImGuiDataType_Double, &time, &zero, &total, "%.3f s")) {
			preview_time_ = time;
		}
		if (ImGui::Button(preview_playing_ ? "Pause Preview" : "Play Preview")) {
			preview_playing_ = !preview_playing_;
		}
		ImGui::SameLine();
		drag_double("Playback Speed", preview_speed_, 0.01, 0.05, 20.0, "%.2fx");
		if (ImGui::Button("Move Live Camera To Preview Pose")) {
			if (!preview_active_) {
				preview_restore_pose_ = current_camera_pose();
				preview_active_ = true;
			}
			const auto sample = script.sample(preview_time_, Capture::make_body_position_lookup(orchestrator_));
			if (sample.valid) {
				coordinator.preview_pose(sample.pose);
			}
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::BeginDisabled(!preview_active_ || session_running);
		if (ImGui::Button("Restore Live Camera")) {
			coordinator.preview_pose(preview_restore_pose_);
			preview_active_ = false;
		}
		ImGui::EndDisabled();
	}

	void render_script_tab() {
		using namespace CaptureWidgets;
		ImGui::Checkbox("Drive The Camera With The Motion Script During Sequence Captures", &settings_.use_script);
		render_setting_tooltip("When enabled, sequence captures follow the script below instead of manual camera input. Script events fire during the capture.");
		if (settings_.use_script && !settings_.script.is_usable()) {
			wrapped_text(kWarningColor, "The script has no active segment and will be ignored until one exists.");
		}
		ImGui::Separator();
		ImGui::TextColored(kHeaderColor, "Preview");
		render_preview_controls();
		ImGui::Separator();
		script_editor_.render(settings_.script, orchestrator_, preview_time_);
	}

	void render_recording_tab() {
		using namespace CaptureWidgets;
		using namespace CaptureStudioDetail;
		auto& recording = settings_.recording;

		ImGui::TextColored(kHeaderColor, "Physical Data Recording");
		ImGui::Checkbox("Record Physical Data During Sequence Captures", &recording.enabled);
		render_setting_tooltip("Samples the selected channels once per captured frame and writes them next to the frames when the sequence finishes.");
		ImGui::TextDisabled("Rows are written at the frame rate of the sequence, reduced by the decimation factor.");

		enum_combo("File Format", recording.format, kRecordingFormatNames);
		input_text("File Name Stem", recording.file_stem);
		slider_u32("Decimation (Keep 1 Of N Frames)", recording.decimation, 1U, 1000U, ImGuiSliderFlags_Logarithmic);
		slider_u32("Numeric Precision (Digits)", recording.precision, 3U, 17U);
		ImGui::Checkbox("Convert To SI Units", &recording.si_units);
		render_setting_tooltip("Scales lengths, times, masses, energies and rates through the active physical constants preset instead of writing simulation units.");
		ImGui::SameLine();
		ImGui::Checkbox("Write Event Log", &recording.write_events);

		ImGui::Separator();
		ImGui::TextColored(kHeaderColor, "Presets");
		for (uint32_t i = 0; i < kRecordingPresetNames.size(); ++i) {
			if (i > 0) {
				ImGui::SameLine();
			}
			ImGui::PushID(static_cast<int>(i));
			if (ImGui::SmallButton(kRecordingPresetNames[i])) {
				recording.apply_preset(i);
			}
			ImGui::PopID();
		}

		ImGui::Separator();
		ImGui::TextColored(kHeaderColor, "Channels");
		const bool vtk = recording.format == IO::RecordingFormat::VtkPolyline;
		const auto& infos = IO::kRecordChannelInfos;
		size_t first = 0;
		size_t group_index = 0;
		while (first < infos.size()) {
			size_t last = first;
			uint64_t group_mask = 0ULL;
			while (last < infos.size() && std::strcmp(infos[last].group, infos[first].group) == 0) {
				group_mask |= IO::record_channel_bit(infos[last].channel);
				++last;
			}
			char header[128];
			std::snprintf(header, sizeof(header), "%s (%d/%zu)###RecordGroup%zu", infos[first].group, std::popcount(recording.channel_mask & group_mask), last - first, group_index);
			if (ImGui::CollapsingHeader(header)) {
				ImGui::PushID(static_cast<int>(group_index));
				if (ImGui::SmallButton("Select All")) {
					recording.channel_mask |= group_mask;
				}
				ImGui::SameLine();
				if (ImGui::SmallButton("Select None")) {
					recording.channel_mask &= ~group_mask;
				}
				for (size_t i = first; i < last; ++i) {
					const uint64_t bit = IO::record_channel_bit(infos[i].channel);
					const bool forced = vtk && (IO::kVtkRequiredRecordChannels & bit) != 0ULL;
					bool value = recording.channel_enabled(infos[i].channel) || forced;
					char label[128];
					if (infos[i].unit[0] != '\0') {
						std::snprintf(label, sizeof(label), "%s [%s]%s", infos[i].name, infos[i].unit, forced ? " (required)" : "");
					} else {
						std::snprintf(label, sizeof(label), "%s%s", infos[i].name, forced ? " (required)" : "");
					}
					ImGui::PushID(static_cast<int>(i));
					ImGui::BeginDisabled(forced);
					if (ImGui::Checkbox(label, &value)) {
						recording.set_channel(infos[i].channel, value);
					}
					ImGui::EndDisabled();
					ImGui::PopID();
				}
				ImGui::PopID();
			}
			first = last;
			++group_index;
		}

		ImGui::Separator();
		ImGui::TextColored(kHeaderColor, "Per-Body Data");
		ImGui::Checkbox("Body Positions", &recording.body_positions);
		ImGui::SameLine();
		ImGui::Checkbox("Body Velocities", &recording.body_velocities);
		ImGui::Checkbox("Distance To Camera", &recording.body_camera_distance);
		ImGui::SameLine();
		ImGui::Checkbox("Body Speeds", &recording.body_speeds);
		ImGui::BeginDisabled(!recording.any_body_channel());
		slider_u32("Maximum Bodies", recording.max_bodies, 1U, 256U);
		ImGui::EndDisabled();

		const uint64_t frames = planned_frame_count();
		const size_t enabled_bodies = std::min<size_t>(orchestrator_.nbody_system().body_count(), recording.max_bodies);
		const size_t body_columns = enabled_bodies * ((recording.body_positions ? 3U : 0U) + (recording.body_velocities ? 3U : 0U) + (recording.body_camera_distance ? 1U : 0U) + (recording.body_speeds ? 1U : 0U));
		const size_t columns = static_cast<size_t>(std::popcount(recording.effective_mask())) + body_columns;
		ImGui::Separator();
		ImGui::TextDisabled("Columns: %zu", columns);
		if (frames > 0) {
			const uint64_t rows = (frames + recording.decimation - 1U) / recording.decimation;
			const bool binary = recording.format == IO::RecordingFormat::BinaryColumns || recording.format == IO::RecordingFormat::Container;
			const uint64_t bytes_per_value = binary ? 8ULL : static_cast<uint64_t>(recording.precision) + 4ULL;
			ImGui::TextDisabled("Planned rows: %llu | Estimated size: %s", static_cast<unsigned long long>(rows), format_bytes(rows * columns * bytes_per_value).c_str());
		}
	}

	void render_encoding_tab() {
		using namespace CaptureWidgets;
		using namespace CaptureStudioDetail;
		auto& video = settings_.video;

		ImGui::TextColored(kHeaderColor, "Codec And Container");
		enum_combo("Video Codec", video.codec, kCodecNames);
		enum_combo("Container", video.container, kContainerNames);
		ImGui::TextDisabled("Effective container: .%s | Pixel format: %s", video.container_extension().c_str(), video.resolved_pixel_format().c_str());

		const bool lossy_rate_codec = video.codec == IO::VideoCodecPreset::H264 || video.codec == IO::VideoCodecPreset::H265 || video.codec == IO::VideoCodecPreset::VP9 || video.codec == IO::VideoCodecPreset::AV1;
		const bool quality_codec = lossy_rate_codec || video.codec == IO::VideoCodecPreset::MJPEG;

		ImGui::BeginDisabled(!lossy_rate_codec);
		enum_combo("Rate Control", video.rate_control, kRateControlNames);
		ImGui::EndDisabled();
		ImGui::BeginDisabled(!quality_codec || (lossy_rate_codec && video.rate_control != IO::VideoRateControl::ConstantQuality));
		slider_u32("Quality (CRF, Lower Is Better)", video.crf, 0U, 51U);
		ImGui::EndDisabled();
		ImGui::BeginDisabled(!lossy_rate_codec || video.rate_control != IO::VideoRateControl::TargetBitrate);
		slider_u32("Target Bitrate (kbps)", video.target_bitrate_kbps, 100U, 400000U, ImGuiSliderFlags_Logarithmic);
		ImGui::EndDisabled();
		ImGui::BeginDisabled(!lossy_rate_codec);
		enum_combo("Encoder Speed", video.encoder_speed, kEncoderSpeedNames);
		ImGui::EndDisabled();

		ImGui::Separator();
		ImGui::TextColored(kHeaderColor, "Stream Options");
		input_text("Pixel Format", video.pixel_format);
		render_setting_tooltip("ffmpeg pixel format such as yuv420p or yuv444p. ProRes, Motion JPEG and GIF select their own format.");
		ImGui::SliderFloat("Output Frame Rate Override", &video.encode_frames_per_second, 0.0f, 240.0f, "%.2f fps");
		render_setting_tooltip("Zero keeps the capture frame rate; any other value resamples the encoded video to that rate.");
		ImGui::Checkbox("Loop Output (GIF)", &video.loop_output);

		ImGui::Separator();
		ImGui::TextColored(kHeaderColor, "Assembly");
		ImGui::Checkbox("Assemble Video Automatically After Capture", &video.assemble_video_after_capture);
		render_setting_tooltip("Runs ffmpeg once all frames are written. Requires ffmpeg to be reachable through the executable below.");
		ImGui::BeginDisabled(!video.assemble_video_after_capture);
		ImGui::Checkbox("Delete Frames After Successful Assembly", &video.delete_frames_after_assembly);
		ImGui::EndDisabled();
		input_text("ffmpeg Executable", video.ffmpeg_executable);

		const std::string extension(IO::image_format_descriptor(video.frame_format).extension);
		const std::string directory = settings_.sequence_directory + "/<session>";
		const std::string command = video.build_ffmpeg_command(directory, extension, directory + "/video");
		ImGui::Separator();
		ImGui::TextColored(kHeaderColor, "Generated Command");
		if (command.empty()) {
			ImGui::TextDisabled("Image sequence only: no encoding command is generated.");
		} else {
			ImGui::TextWrapped("%s", command.c_str());
			if (ImGui::SmallButton("Copy Command")) {
				ImGui::SetClipboardText(command.c_str());
			}
		}
	}

	void render_status_panel() {
		using namespace CaptureWidgets;
		using namespace CaptureStudioDetail;
		auto& coordinator = viewport_.capture_coordinator();
		const auto& progress = coordinator.progress();
		const Capture::CapturePhase phase = progress.phase();
		const bool busy = coordinator.is_busy();

		ImGui::TextColored(phase_color(phase), "%s", Capture::capture_phase_name(phase));
		ImGui::SameLine();
		ImGui::TextDisabled("%s", progress.label().c_str());

		char overlay[96];
		const uint64_t total = progress.frames_total();
		if (total > 0) {
			std::snprintf(overlay, sizeof(overlay), "%llu / %llu frames (%.1f%%)", static_cast<unsigned long long>(progress.frames_done()), static_cast<unsigned long long>(total), progress.fraction() * 100.0);
		} else {
			std::snprintf(overlay, sizeof(overlay), "%llu frames", static_cast<unsigned long long>(progress.frames_done()));
		}
		ImGui::ProgressBar(static_cast<float>(progress.fraction()), ImVec2(-1.0f, 0.0f), overlay);

		if (phase == Capture::CapturePhase::Rendering && progress.bands_total() > 1U) {
			char band_overlay[64];
			std::snprintf(band_overlay, sizeof(band_overlay), "Current frame: band %u / %u", progress.bands_done(), progress.bands_total());
			ImGui::ProgressBar(static_cast<float>(progress.bands_done()) / static_cast<float>(progress.bands_total()), ImVec2(-1.0f, 0.0f), band_overlay);
		}

		ImGui::TextDisabled("Elapsed %s | ETA %s | Dropped %llu | Duplicated %llu", format_duration(progress.elapsed_seconds()).c_str(), busy ? format_duration(progress.eta_seconds()).c_str() : "--", static_cast<unsigned long long>(progress.frames_dropped()), static_cast<unsigned long long>(progress.frames_duplicated()));
		if (settings_.recording.enabled) {
			ImGui::TextDisabled("Recorded data rows: %zu", coordinator.recorded_rows());
		}

		const std::string message = progress.message();
		if (!last_request_.ok && !last_request_.message.empty()) {
			wrapped_text(kWarningColor, last_request_.message);
		} else if (!message.empty()) {
			ImGui::TextDisabled("%s", message.c_str());
		}

		ImGui::BeginDisabled(busy);
		if (ImGui::Button("Capture Screenshot", ImVec2(170.0f, 28.0f))) {
			capture_screenshot_now();
		}
		render_setting_tooltip("Renders one frame offline with the Screenshot tab settings. Disabled while another capture is running.");
		ImGui::SameLine();
		if (ImGui::Button("Start Sequence", ImVec2(150.0f, 28.0f))) {
			start_sequence();
		}
		render_setting_tooltip("Starts a sequence capture with the Sequence, Motion Script, Recording and Video Encoding tab settings.");
		ImGui::EndDisabled();

		ImGui::SameLine();
		ImGui::BeginDisabled(!coordinator.is_sequence_active());
		if (ImGui::Button("Finish Sequence", ImVec2(130.0f, 28.0f))) {
			coordinator.stop_sequence();
		}
		render_setting_tooltip("Stops preparing new frames, lets queued frames finish writing, then finalizes the sequence normally.");
		ImGui::EndDisabled();

		ImGui::SameLine();
		ImGui::BeginDisabled(!busy);
		if (ImGui::Button("Cancel Capture", ImVec2(130.0f, 28.0f))) {
			coordinator.cancel_all();
		}
		render_setting_tooltip("Aborts the current capture immediately and discards partially written files.");
		ImGui::EndDisabled();

		if (coordinator.is_manual_stepping()) {
			if (ImGui::Button("Render Next Frame", ImVec2(170.0f, 26.0f))) {
				coordinator.request_manual_frame();
			}
			ImGui::SameLine();
			ImGui::TextDisabled("Pending requests: %u", coordinator.manual_frames_pending());
		}
	}

	[[nodiscard]] ImGuiTabItemFlags tab_flags(StudioTab tab) noexcept {
		if (requested_tab_ == tab) {
			return ImGuiTabItemFlags_SetSelected;
		}
		return ImGuiTabItemFlags_None;
	}

public:
	explicit CaptureStudioWindow(Orchestrator::SimulationOrchestrator<1024>& orchestrator, ViewportPrimaryWindow& viewport, IO::UserSettings& user_settings)
		: orchestrator_(orchestrator),
		  viewport_(viewport),
		  user_settings_(user_settings),
		  settings_(IO::CaptureStudioSettings::load_or_default()) {}

	[[nodiscard]] bool& open_state() noexcept {
		return is_open_;
	}

	void save_settings() const {
		settings_.save();
	}

	void capture_screenshot_now() {
		Capture::ScreenshotRequest request;
		request.target = resolve_target(settings_.screenshot_target);
		request.output_directory = user_settings_.screenshot_output_directory;
		request.filename_pattern = user_settings_.screenshot_filename_pattern;
		request.format = screenshot_format();
		request.overwrite_policy = static_cast<IO::ScreenshotOverwritePolicy>(std::min<uint32_t>(user_settings_.screenshot_overwrite_policy, 2U));
		request.comment = user_settings_.screenshot_watermark_enabled ? user_settings_.screenshot_watermark_text : std::string{};
		last_request_ = viewport_.capture_coordinator().request_screenshot(request);
	}

	void start_sequence() {
		settings_.script.sanitize();
		settings_.recording.sanitize();
		Capture::SequenceRequest request;
		request.target = resolve_target(settings_.sequence_target);
		request.settings = settings_.video;
		request.output_directory = settings_.sequence_directory;
		request.session_name = settings_.session_name.empty() ? std::string{} : IO::ScreenshotExporter::expand_filename_pattern(settings_.session_name);
		request.comment = user_settings_.screenshot_watermark_enabled ? user_settings_.screenshot_watermark_text : std::string{};
		request.use_script = script_usable();
		request.script = settings_.script;
		request.recording = settings_.recording;
		request.manual_stepping = settings_.manual_stepping && settings_.video.mode == IO::SequenceCaptureMode::Deterministic;
		last_request_ = viewport_.capture_coordinator().start_sequence(request);
		if (last_request_.ok) {
			preview_active_ = false;
			preview_playing_ = false;
		}
	}

	void render() {
		if (!is_open_) {
			release_preview();
			return;
		}

		update_preview(static_cast<double>(ImGui::GetIO().DeltaTime));

		ImGui::SetNextWindowPos(viewport_.window_center(), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(700.0f, 800.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSizeConstraints(ImVec2(560.0f, 480.0f), ImVec2(FLT_MAX, FLT_MAX));
		if (!ImGui::Begin("Capture Studio", &is_open_)) {
			ImGui::End();
			return;
		}

		ImGui::BeginChild("CaptureStudioTabs", ImVec2(0.0f, -kFooterHeight), false);
		if (ImGui::BeginTabBar("CaptureStudioTabBar")) {
			if (ImGui::BeginTabItem("Screenshot", nullptr, tab_flags(StudioTab::Screenshot))) {
				render_output_tab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Sequence", nullptr, tab_flags(StudioTab::Sequence))) {
				render_sequence_tab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Motion Script", nullptr, tab_flags(StudioTab::Script))) {
				render_script_tab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Data Recording", nullptr, tab_flags(StudioTab::Recording))) {
				render_recording_tab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Video Encoding", nullptr, tab_flags(StudioTab::Encoding))) {
				render_encoding_tab();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
		requested_tab_ = StudioTab::None;
		ImGui::EndChild();

		ImGui::Separator();
		render_status_panel();
		ImGui::End();
	}
};

}
