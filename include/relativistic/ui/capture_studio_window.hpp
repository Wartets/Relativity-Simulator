#pragma once

#include "relativistic/capture/capture_coordinator.hpp"
#include "relativistic/capture/motion_script.hpp"
#include "relativistic/capture/path_preview_builder.hpp"
#include "relativistic/io/capture/capture_studio_settings.hpp"
#include "relativistic/io/image/image_format.hpp"
#include "relativistic/io/capture/recording_settings.hpp"
#include "relativistic/io/capture/screenshot_capture_settings.hpp"
#include "relativistic/io/capture/screenshot_exporter.hpp"
#include "relativistic/io/user_settings.hpp"
#include "relativistic/io/capture/video_capture_settings.hpp"
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
public:
	enum class StudioTab : int {
		None = -1,
		Screenshot = 0,
		Sequence = 1,
		Script = 2,
		Recording = 3,
		Encoding = 4
	};

private:
	static constexpr float kDefaultFooterUnits = 15.5f;
	static constexpr float kMinimumFooterUnits = 8.0f;
	static constexpr float kMinimumTabsUnits = 14.0f;

	float footer_height_{0.0f};

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
	bool preview_show_direction_{true};
	bool preview_include_modifiers_{true};
	bool preview_show_reference_{true};
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
			if ((segment.orientation.mode == Capture::OrientationMode::LookAtTarget || segment.orientation.mode == Capture::OrientationMode::TargetPath) && (segment.orientation.target_body >= 0 || segment.orientation.target_body == Capture::kNearestBodyReference)) {
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
			options.show_direction = preview_show_direction_;
			options.include_modifiers = preview_include_modifiers_;
			options.show_reference = preview_show_reference_;
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
			render_setting_tooltip("Horizontal resolution of the output image or sequence frames in pixels.");
			if (ImGui::InputInt("Height (px)", &height, 16, 256)) {
				target.explicit_height = static_cast<uint32_t>(std::clamp(height, 16, 65535));
			}
			render_setting_tooltip("Vertical resolution of the output image or sequence frames in pixels.");
			FlowLayout resolution_flow;
			for (size_t i = 0; i < kResolutionPresets.size(); ++i) {
				const bool selected = target.explicit_width == kResolutionPresets[i].width && target.explicit_height == kResolutionPresets[i].height;
				ImGui::PushID(static_cast<int>(i));
				if (resolution_flow.small_button(kResolutionPresets[i].label, selected)) {
					target.explicit_width = kResolutionPresets[i].width;
					target.explicit_height = kResolutionPresets[i].height;
				}
				render_setting_tooltip("Sets the explicit resolution to this standard preset.");
				ImGui::PopID();
			}
			if (resolution_flow.small_button("Viewport Size")) {
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
		if (begin_stat_table("##TargetStats", 3)) {
			stat_cell("Output", std::to_string(resolved.width) + " x " + std::to_string(resolved.height) + " px");
			stat_cell("Traced", std::to_string(internal_width) + " x " + std::to_string(internal_height) + " px (" + format_number(static_cast<double>(internal_width * internal_height) / 1.0e6, 1) + " Mpx)");
			stat_cell("Size Per Frame", format_bytes(IO::estimate_image_file_bytes(format, resolved.width, resolved.height)));
			end_stat_table();
		}
		const std::string error = Capture::CaptureCoordinator::validate_target(resolved, format);
		if (!error.empty()) {
			wrapped_text(kWarningColor, error);
		}
		ImGui::PopID();
	}

	void render_output_tab() {
		using namespace CaptureWidgets;
		using namespace CaptureStudioDetail;
		section_header("Destination");
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
		render_setting_tooltip("Image file format and bit depth used to save the screenshot.");
		const IO::ScreenshotFormat format = screenshot_format();
		const auto& descriptor = IO::image_format_descriptor(format);
		if (begin_stat_table("##FormatStats", 4)) {
			stat_cell("Alpha", descriptor.preserves_alpha ? "Yes" : "No", descriptor.preserves_alpha ? kAccentColor : kMutedColor);
			stat_cell("Float Samples", descriptor.stores_float_samples ? "Yes" : "No", descriptor.stores_float_samples ? kAccentColor : kMutedColor);
			stat_cell("Metadata Comment", descriptor.supports_comment ? "Yes" : "No", descriptor.supports_comment ? kAccentColor : kMutedColor);
			stat_cell("Max Side", std::to_string(descriptor.max_dimension) + " px");
			end_stat_table();
		}

		index_combo("If File Exists", user_settings_.screenshot_overwrite_policy, kOverwriteNames);
		render_setting_tooltip("Rule applied when the target file already exists on disk.");

		ImGui::BeginDisabled(!descriptor.supports_comment);
		ImGui::Checkbox("Embed Watermark / Comment", &user_settings_.screenshot_watermark_enabled);
		render_setting_tooltip("Stores the text as metadata in formats that support it (PNG, PNG16, HDR, TIFF).");
		if (user_settings_.screenshot_watermark_enabled) {
			input_text("Watermark Text", user_settings_.screenshot_watermark_text);
			render_setting_tooltip("Custom metadata or comment string embedded in the saved image file header.");
		}
		ImGui::EndDisabled();

		IO::ScreenshotCaptureContext context;
		const Capture::CaptureTarget resolved = resolve_target(settings_.screenshot_target);
		context.metric_name = orchestrator_.active_metric_name();
		context.mass = orchestrator_.parameters().mass;
		context.spin = orchestrator_.parameters().spin;
		context.width = resolved.width;
		context.height = resolved.height;
		context.tick_index = orchestrator_.scheduler().snapshot().tick_index;
		ImGui::TextDisabled("Next file: %s%s", IO::ScreenshotFilenameBuilder::build(user_settings_.screenshot_filename_pattern, context).c_str(), IO::ScreenshotExporter::extension_for_format(format).c_str());

		section_header("Quality And Resolution");
		render_target_editor(settings_.screenshot_target, format, "ScreenshotTarget");
	}

	void render_sequence_tab() {
		using namespace CaptureWidgets;
		using namespace CaptureStudioDetail;
		auto& video = settings_.video;

		section_header("Capture Mode");
		enum_combo("Mode", video.mode, kCaptureModeNames);
		render_setting_tooltip("Deterministic renders every frame offline at the requested quality while the world is advanced under full control. Real-Time records what the viewport currently shows.");
		const bool realtime = video.mode == IO::SequenceCaptureMode::RealTime;

		section_header("Camera Control");
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
		render_setting_tooltip("Switches directly to the Motion Script tab to edit or configure the camera trajectory.");
		if (script_usable() && video.trigger != IO::SequenceCaptureTrigger::PathDuration) {
			ImGui::SameLine();
			if (ImGui::SmallButton("Use Script Duration")) {
				video.trigger = IO::SequenceCaptureTrigger::PathDuration;
			}
			render_setting_tooltip("Sets the capture stop condition to match the total duration of the motion script.");
		}

		section_header("Timing");
		ImGui::SliderFloat("Frame Rate", &video.frames_per_second, 1.0f, 240.0f, "%.2f fps");
		render_setting_tooltip("Number of frames per second of video. It also sets the simulation time step between frames in deterministic mode.");
		FlowLayout rate_flow;
		for (const float preset : {24.0f, 25.0f, 30.0f, 50.0f, 60.0f, 120.0f}) {
			char label[24];
			std::snprintf(label, sizeof(label), "%.0f fps", static_cast<double>(preset));
			if (rate_flow.small_button(label, std::abs(video.frames_per_second - preset) < 0.01f)) {
				video.frames_per_second = preset;
			}
			render_setting_tooltip("Sets the capture and video frame rate to this standard value.");
		}
		enum_combo("Stop Condition", video.trigger, kTriggerNames);
		render_setting_tooltip("Defines when the capture ends: by hand, after a duration, after a frame count, never, or when the motion script finishes.");
		if (video.trigger == IO::SequenceCaptureTrigger::FixedDuration) {
			ImGui::SliderFloat("Duration", &video.duration_seconds, 0.1f, 3600.0f, "%.2f s", ImGuiSliderFlags_Logarithmic);
			render_setting_tooltip("Length of the video. The number of frames is this value multiplied by the frame rate.");
		} else if (video.trigger == IO::SequenceCaptureTrigger::FixedFrameCount) {
			ImGui::InputScalar("Frame Count", ImGuiDataType_U64, &video.fixed_frame_count);
			render_setting_tooltip("Exact number of frames to render before the sequence finishes.");
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

		section_header("Output");
		input_text("Sequence Directory", settings_.sequence_directory);
		render_setting_tooltip("Parent folder in which every capture session creates its own sub-folder.");
		input_text("Session Name (empty = automatic)", settings_.session_name);
		render_setting_tooltip("Name of the sub-folder created for this capture. strftime tokens are supported; empty generates a timestamped name.");
		int frame_format_index = static_cast<int>(std::min<uint32_t>(static_cast<uint32_t>(video.frame_format), IO::kScreenshotFormatCount - 1U));
		if (ImGui::Combo("Frame Format", &frame_format_index, format_names().data(), static_cast<int>(IO::kScreenshotFormatCount))) {
			video.frame_format = static_cast<IO::ScreenshotFormat>(frame_format_index);
		}
		render_setting_tooltip("Image format of each written frame. Use a lossless 8-bit format for encoding with ffmpeg.");
		ImGui::InputScalar("First Frame Index", ImGuiDataType_U64, &video.start_frame_index);
		render_setting_tooltip("Number given to the first written frame, useful to continue a previous sequence.");
		slider_u32("Frame Number Padding", video.frame_name_padding, 1U, 12U);
		render_setting_tooltip("Number of digits in frame file names, padded with zeros so frames sort correctly.");

		ImGui::Separator();
		if (realtime) {
			section_header("Real-Time Recording");
			ImGui::SliderFloat("Live Resolution Scale", &video.resolution_scale, 0.1f, 4.0f, "%.2fx", ImGuiSliderFlags_Logarithmic);
			render_setting_tooltip("Locks the viewport render scale while recording so every frame has identical dimensions.");
			enum_combo("Frame Pacing", video.pacing, kPacingNames);
			render_setting_tooltip("Drop skips frames when the renderer is slower than the capture rate; Duplicate repeats the latest frame to keep the video timeline exact.");
			slider_u32("Write Queue Depth", video.realtime_queue_depth, 1U, 64U);
			render_setting_tooltip("Frames allowed to wait for disk writing before new ones are dropped.");
			ImGui::TextDisabled("The simulation keeps running live and the camera can be flown freely while recording.");
		} else {
			section_header("Frame Quality");
			render_target_editor(settings_.sequence_target, video.frame_format, "SequenceTarget");

			section_header("Motion Blur And Fades");
			slider_u32("Temporal Samples", video.temporal_samples, 1U, 64U);
			render_setting_tooltip("Renders several sub-frame poses per output frame and averages them in linear light for motion blur.");
			ImGui::BeginDisabled(video.temporal_samples < 2U);
			ImGui::SliderFloat("Shutter Fraction", &video.shutter_fraction, 0.0f, 1.0f, "%.2f");
			render_setting_tooltip("Portion of the frame interval covered by the virtual shutter.");
			ImGui::EndDisabled();
			slider_u32("Fade In Frames", video.fade_in_frames, 0U, 600U);
			render_setting_tooltip("Number of opening frames that fade up from black.");
			slider_u32("Fade Out Frames", video.fade_out_frames, 0U, 600U);
			render_setting_tooltip("Number of closing frames that fade down to black. Requires a fixed frame count, duration or script duration.");

			section_header("Stepping Control");
			ImGui::Checkbox("Manual Frame Stepping", &settings_.manual_stepping);
			render_setting_tooltip("Renders a frame only when you request it with the Render Next Frame button, allowing inspection and camera adjustments between frames.");
			ImGui::Checkbox("Pause Simulation During Capture", &video.pause_simulation_during_capture);
			render_setting_tooltip("Stops the live simulation clock while frames are rendered; the capture then advances the world itself according to World Advance.");
			ImGui::Checkbox("Preview Frames In Viewport", &video.preview_in_viewport);
			render_setting_tooltip("Keeps the live viewport rendering while frames are being captured, at the cost of speed.");
		}

		section_header("World Advance");
		enum_combo("Advance Mode", video.advance_mode, kAdvanceNames);
		render_setting_tooltip("Frozen World keeps bodies still, Fixed Ticks advances a constant number of ticks per frame, and Simulation Seconds Per Video Second makes the video run at a chosen speed. A motion script simulation rate multiplier scales the advance further.");
		if (video.advance_mode == IO::SequenceAdvanceMode::TicksPerFrame) {
			slider_u32("Ticks Per Frame", video.ticks_per_frame, 0U, 1000U, ImGuiSliderFlags_Logarithmic);
		render_setting_tooltip("Scheduler ticks integrated between two frames. With temporal samples above 1 the ticks are spread over the sub-frame samples.");
		} else if (video.advance_mode == IO::SequenceAdvanceMode::SimulationRateRatio) {
			drag_double("Simulation Seconds Per Video Second", video.simulation_seconds_per_video_second, 0.01, 0.0, 1.0e9, "%.4f");
			render_setting_tooltip("Ratio of physical simulation seconds that elapse during one second of captured video playback.");
			const double tick_dt = orchestrator_.scheduler().tick_dt();
			if (tick_dt > 0.0) {
				ImGui::TextDisabled("Equals %.3f ticks per frame at the current scheduler rate.", video.simulation_seconds_per_video_second / (static_cast<double>(video.frames_per_second) * tick_dt));
			}
		}
		ImGui::Checkbox("Restore Camera After Capture", &video.restore_camera_after_capture);
		render_setting_tooltip("Puts the live camera, field of view and exposure back where they were before the capture started. A motion script always restores them.");

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

		FlowLayout preview_flow;
		preview_flow.next(FlowLayout::checkbox_width("Show Path In Viewport"));
		if (ImGui::Checkbox("Show Path In Viewport", &preview_enabled_)) {
			preview_force_rebuild_ = true;
		}
		render_setting_tooltip("Draws the full trajectory, segment boundaries, events and the camera frustum directly on the viewport without rendering a single frame.");
		preview_flow.next(FlowLayout::checkbox_width("Schematic Viewport"));
		bool schematic = orchestrator_.parameters().schematic_mode_enabled;
		if (ImGui::Checkbox("Schematic Viewport", &schematic)) {
			static_cast<void>(orchestrator_.enqueue_command(Orchestrator::Command::make_set_param(Orchestrator::ParameterType::SchematicModeEnabled, schematic ? 1.0 : 0.0)));
		}
		render_setting_tooltip("Switches the main viewport to the schematic projection where the preview is drawn without ray tracing, so it updates instantly while editing.");
		preview_flow.next(FlowLayout::button_width("Preview Options"));
		if (ImGui::Button("Preview Options")) {
			ImGui::OpenPopup("##PreviewOptionsPopup");
		}
		render_setting_tooltip("Configures trajectory visualization details, markers, labels, samples, frustum and reference paths in the viewport.");
		if (ImGui::BeginPopup("##PreviewOptionsPopup")) {
			bool options_changed = false;
			if (begin_property_grid("##PreviewOptionsGrid")) {
				options_changed |= property_u32("Samples Per Segment", preview_samples_, 16U, 2048U, ImGuiSliderFlags_Logarithmic, "Number of points used to draw each segment.");
				options_changed |= property_check("Markers", preview_show_markers_, "Start, end, segment boundary and event markers.");
				options_changed |= property_check("Labels", preview_show_labels_, "Names next to the markers.");
				options_changed |= property_check("Sample Points", preview_show_samples_, "Dots at every sampled point of the path.");
				options_changed |= property_check("Direction Arrows", preview_show_direction_, "Arrows indicating the travel direction along the path.");
				options_changed |= property_check("Shake And Transitions", preview_include_modifiers_, "Draws the path the camera really follows, including camera shake displacement and position blends between segments, and adds a marker where each transition ends.");
				options_changed |= property_check("Reference Path", preview_show_reference_, "Keeps a faint line showing the clean path without shake and transitions so their effect is easy to judge.");
				options_changed |= property_check("Camera Frustum", preview_show_frustum_, "Frustum drawn at the preview time.");
				options_changed |= property_drag("Frustum Length", preview_frustum_length_, 0.1, 0.5, 10000.0, "%.2f", "Depth of the drawn frustum in world units.");
				end_property_grid();
			}
			if (options_changed) {
				preview_options_dirty_ = true;
			}
			ImGui::EndPopup();
		}

		ImGui::BeginDisabled(session_running || !script.is_usable());
		if (ImGui::Button(preview_playing_ ? "Pause" : "Play", ImVec2(ImGui::GetFontSize() * 4.5f, 0.0f))) {
			preview_playing_ = !preview_playing_;
		}
		render_setting_tooltip("Plays or pauses the real-time preview animation along the motion script trajectory.");
		ImGui::SameLine();
		const double total = script.total_duration();
		const double zero = 0.0;
		double time = preview_time_;
		ImGui::SetNextItemWidth(std::max(ImGui::GetContentRegionAvail().x - ImGui::GetFontSize() * 11.0f, ImGui::GetFontSize() * 9.0f));
		if (ImGui::SliderScalar("##PreviewTime", ImGuiDataType_Double, &time, &zero, &total, "%.3f s")) {
			preview_time_ = time;
		}
		render_setting_tooltip("Preview time position along the script.");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(ImGui::GetFontSize() * 5.5f);
		drag_double("##PreviewSpeed", preview_speed_, 0.01, 0.05, 20.0, "%.2fx");
		render_setting_tooltip("Playback speed of the preview.");
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
		render_setting_tooltip("Positions and aligns the interactive viewport camera to match the motion script at the current preview time.");
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::BeginDisabled(!preview_active_ || session_running);
		if (ImGui::Button("Restore Live Camera")) {
			coordinator.preview_pose(preview_restore_pose_);
			preview_active_ = false;
		}
		render_setting_tooltip("Restores the interactive viewport camera to its pose prior to moving to the preview pose.");
		ImGui::EndDisabled();
	}

	void render_script_tab() {
		using namespace CaptureWidgets;
		ImGui::Checkbox("Drive The Camera With This Script During Sequence Captures", &settings_.use_script);
		render_setting_tooltip("When enabled, sequence captures follow the script below instead of manual camera input. Script events fire during the capture.");
		if (settings_.use_script && !settings_.script.is_usable()) {
			wrapped_text(kWarningColor, "The script has no active segment and will be ignored until one exists.");
		}
		ImGui::SeparatorText("Viewport Preview");
		render_preview_controls();
		ImGui::SeparatorText("Motion Script");
		script_editor_.render(settings_.script, orchestrator_, preview_time_);
	}

	void render_recording_tab() {
		using namespace CaptureWidgets;
		using namespace CaptureStudioDetail;
		auto& recording = settings_.recording;

		section_header("Physical Data Recording");
		ImGui::Checkbox("Record Physical Data During Sequence Captures", &recording.enabled);
		render_setting_tooltip("Samples the selected channels once per captured frame and writes them next to the frames when the sequence finishes.");
		ImGui::TextDisabled("Rows are written at the frame rate of the sequence, reduced by the decimation factor.");

		enum_combo("File Format", recording.format, kRecordingFormatNames);
		render_setting_tooltip("Storage format of the recorded data. The VTK polyline format forces the position, time and redshift channels.");
		input_text("File Name Stem", recording.file_stem);
		render_setting_tooltip("Base name of the recording files. Characters other than letters, digits, dash and underscore are replaced.");
		slider_u32("Decimation (Keep 1 Of N Frames)", recording.decimation, 1U, 1000U, ImGuiSliderFlags_Logarithmic);
		render_setting_tooltip("Writes one row every N captured frames. Velocity and acceleration are still derived from every frame.");
		slider_u32("Numeric Precision (Digits)", recording.precision, 3U, 17U);
		render_setting_tooltip("Significant digits written in text formats. 17 digits reproduce a 64-bit value exactly.");
		ImGui::Checkbox("Convert To SI Units", &recording.si_units);
		render_setting_tooltip("Scales lengths, times, masses, energies and rates through the active physical constants preset instead of writing simulation units.");
		ImGui::SameLine();
		ImGui::Checkbox("Write Event Log", &recording.write_events);
		render_setting_tooltip("Generates a separate text log listing all script events triggered during sequence capture.");

		section_header("Presets");
		FlowLayout recording_flow;
		for (uint32_t i = 0; i < kRecordingPresetNames.size(); ++i) {
			ImGui::PushID(static_cast<int>(i));
			if (recording_flow.small_button(kRecordingPresetNames[i])) {
				recording.apply_preset(i);
			}
			render_setting_tooltip("Applies this predefined channel selection preset for physical data recording.");
			ImGui::PopID();
		}

		section_header("Channels");
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
				render_setting_tooltip("Enables all channels in this group.");
				ImGui::SameLine();
				if (ImGui::SmallButton("Select None")) {
					recording.channel_mask &= ~group_mask;
				}
				render_setting_tooltip("Disables all channels in this group.");
				const int channel_columns = std::clamp(static_cast<int>(ImGui::GetContentRegionAvail().x / (ImGui::GetFontSize() * 16.0f)), 1, 4);
				const bool channel_table = ImGui::BeginTable("##ChannelTable", channel_columns, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoSavedSettings);
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
					if (channel_table) {
						ImGui::TableNextColumn();
					}
					ImGui::PushID(static_cast<int>(i));
					ImGui::BeginDisabled(forced);
					if (ImGui::Checkbox(label, &value)) {
						recording.set_channel(infos[i].channel, value);
					}
					render_setting_tooltip(forced ? "This channel is required by the selected export format and cannot be disabled." : "Toggles recording for this physical channel.");
					ImGui::EndDisabled();
					ImGui::PopID();
				}
				if (channel_table) {
					ImGui::EndTable();
				}
				ImGui::PopID();
			}
			first = last;
			++group_index;
		}

		section_header("Per-Body Data");
		ImGui::Checkbox("Body Positions", &recording.body_positions);
		render_setting_tooltip("Records the 3D position coordinates (x, y, z) for each active N-body.");
		ImGui::SameLine();
		ImGui::Checkbox("Body Velocities", &recording.body_velocities);
		render_setting_tooltip("Records the 3D velocity vector components (vx, vy, vz) for each active N-body.");
		ImGui::Checkbox("Distance To Camera", &recording.body_camera_distance);
		render_setting_tooltip("Records the Euclidean distance from the camera to each active N-body.");
		ImGui::SameLine();
		ImGui::Checkbox("Body Speeds", &recording.body_speeds);
		render_setting_tooltip("Records the scalar velocity magnitude for each active N-body.");
		ImGui::BeginDisabled(!recording.any_body_channel());
		slider_u32("Maximum Bodies", recording.max_bodies, 1U, 256U);
		render_setting_tooltip("Caps the maximum number of N-body entities included in the per-body channel columns.");
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

		section_header("Codec And Container");
		enum_combo("Video Codec", video.codec, kCodecNames);
		render_setting_tooltip("Encoder used by ffmpeg. H.264 is the most compatible, H.265 and AV1 are more compact, ProRes, FFV1 and Motion JPEG suit editing, and Image Sequence Only skips encoding.");
		enum_combo("Container", video.container, kContainerNames);
		render_setting_tooltip("File wrapper of the encoded video. Some codecs force a compatible container, as shown in the effective container line below.");
		ImGui::TextDisabled("Effective container: .%s | Pixel format: %s", video.container_extension().c_str(), video.resolved_pixel_format().c_str());

		const bool lossy_rate_codec = video.codec == IO::VideoCodecPreset::H264 || video.codec == IO::VideoCodecPreset::H265 || video.codec == IO::VideoCodecPreset::VP9 || video.codec == IO::VideoCodecPreset::AV1;
		const bool quality_codec = lossy_rate_codec || video.codec == IO::VideoCodecPreset::MJPEG;

		ImGui::BeginDisabled(!lossy_rate_codec);
		enum_combo("Rate Control", video.rate_control, kRateControlNames);
		render_setting_tooltip("Selects whether the encoder targets a constant quality factor (CRF) or an average bitrate.");
		ImGui::EndDisabled();
		ImGui::BeginDisabled(!quality_codec || (lossy_rate_codec && video.rate_control != IO::VideoRateControl::ConstantQuality));
		slider_u32("Quality (CRF, Lower Is Better)", video.crf, 0U, 51U);
		render_setting_tooltip("Constant quality factor. Around 18 is visually lossless for H.264 and 0 is lossless; higher values reduce file size.");
		ImGui::EndDisabled();
		ImGui::BeginDisabled(!lossy_rate_codec || video.rate_control != IO::VideoRateControl::TargetBitrate);
		slider_u32("Target Bitrate (kbps)", video.target_bitrate_kbps, 100U, 400000U, ImGuiSliderFlags_Logarithmic);
		render_setting_tooltip("Average bitrate aimed at by the encoder when Target Bitrate rate control is selected.");
		ImGui::EndDisabled();
		ImGui::BeginDisabled(!lossy_rate_codec);
		enum_combo("Encoder Speed", video.encoder_speed, kEncoderSpeedNames);
		render_setting_tooltip("Slower presets spend more time to compress better at the same quality.");
		ImGui::EndDisabled();

		section_header("Stream Options");
		input_text("Pixel Format", video.pixel_format);
		render_setting_tooltip("ffmpeg pixel format such as yuv420p or yuv444p. ProRes, Motion JPEG and GIF select their own format.");
		ImGui::SliderFloat("Output Frame Rate Override", &video.encode_frames_per_second, 0.0f, 240.0f, "%.2f fps");
		render_setting_tooltip("Zero keeps the capture frame rate; any other value resamples the encoded video to that rate.");
		ImGui::Checkbox("Loop Output (GIF)", &video.loop_output);
		render_setting_tooltip("Makes an animated GIF restart automatically when it reaches the end.");

		section_header("Assembly");
		ImGui::Checkbox("Assemble Video Automatically After Capture", &video.assemble_video_after_capture);
		render_setting_tooltip("Runs ffmpeg once all frames are written. Requires ffmpeg to be reachable through the executable below.");
		ImGui::BeginDisabled(!video.assemble_video_after_capture);
		ImGui::Checkbox("Delete Frames After Successful Assembly", &video.delete_frames_after_assembly);
		render_setting_tooltip("Removes the intermediate frame images once ffmpeg reports success. Frames are kept if encoding fails.");
		ImGui::EndDisabled();
		input_text("ffmpeg Executable", video.ffmpeg_executable);
		render_setting_tooltip("Path or command name for the ffmpeg executable used for video encoding.");

		const std::string extension(IO::image_format_descriptor(video.frame_format).extension);
		const std::string directory = settings_.sequence_directory + "/<session>";
		const std::string command = video.build_ffmpeg_command(directory, extension, directory + "/video");
		section_header("Generated Command");
		if (command.empty()) {
			ImGui::TextDisabled("Image sequence only: no encoding command is generated.");
		} else {
			ImGui::TextWrapped("%s", command.c_str());
			if (ImGui::SmallButton("Copy Command")) {
				ImGui::SetClipboardText(command.c_str());
			}
			render_setting_tooltip("Copies the full ffmpeg command line to the system clipboard.");
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
		ImGui::ProgressBar(static_cast<float>(progress.fraction()), ImVec2(-FLT_MIN, ImGui::GetFontSize() * 1.4f), overlay);

		if (phase == Capture::CapturePhase::Rendering && progress.bands_total() > 1U) {
			char band_overlay[64];
			std::snprintf(band_overlay, sizeof(band_overlay), "Current frame: band %u / %u", progress.bands_done(), progress.bands_total());
			ImGui::ProgressBar(static_cast<float>(progress.bands_done()) / static_cast<float>(progress.bands_total()), ImVec2(-FLT_MIN, ImGui::GetFontSize() * 1.0f), band_overlay);
		}

		if (begin_stat_table("##CaptureStats", settings_.recording.enabled ? 5 : 4)) {
			stat_cell("Elapsed", format_duration(progress.elapsed_seconds()), kMutedColor);
			stat_cell("ETA", busy ? format_duration(progress.eta_seconds()) : std::string("--"), kMutedColor);
			stat_cell("Dropped", std::to_string(progress.frames_dropped()), progress.frames_dropped() > 0 ? kWarningColor : kMutedColor);
			stat_cell("Duplicated", std::to_string(progress.frames_duplicated()), kMutedColor);
			if (settings_.recording.enabled) {
				stat_cell("Data Rows", std::to_string(coordinator.recorded_rows()), kAccentColor);
			}
			end_stat_table();
		}

		const std::string message = progress.message();
		if (!last_request_.ok && !last_request_.message.empty()) {
			wrapped_text(kWarningColor, last_request_.message);
		} else if (!message.empty()) {
			ImGui::TextDisabled("%s", message.c_str());
		}

		FlowLayout actions;
		const float action_width = ImGui::GetFontSize() * 9.0f;
		ImGui::BeginDisabled(busy);
		if (actions.button("Capture Screenshot", action_width)) {
			capture_screenshot_now();
		}
		render_setting_tooltip("Renders one frame offline with the Screenshot tab settings. Disabled while another capture is running.");
		if (actions.button("Start Sequence", action_width)) {
			start_sequence();
		}
		render_setting_tooltip("Starts a sequence capture with the Sequence, Motion Script, Recording and Video Encoding tab settings.");
		ImGui::EndDisabled();

		ImGui::BeginDisabled(!coordinator.is_sequence_active());
		if (actions.button("Finish Sequence", action_width)) {
			coordinator.stop_sequence();
		}
		render_setting_tooltip("Stops preparing new frames, lets queued frames finish writing, then finalizes the sequence normally.");
		ImGui::EndDisabled();

		ImGui::BeginDisabled(!busy);
		if (actions.button("Cancel Capture", action_width)) {
			coordinator.cancel_all();
		}
		render_setting_tooltip("Aborts the current capture immediately and discards partially written files.");
		ImGui::EndDisabled();

		if (coordinator.is_manual_stepping()) {
			if (actions.button("Render Next Frame", action_width)) {
				coordinator.request_manual_frame();
			}
			render_setting_tooltip("Renders and records the next frame in manual stepping mode.");
			ImGui::SameLine();
			ImGui::TextDisabled("Pending requests: %u", coordinator.manual_frames_pending());
		}
	}

	template <typename Body>
	void render_tab(const char* label, StudioTab tab, Body&& body) {
		if (!ImGui::BeginTabItem(label, nullptr, tab_flags(tab))) {
			return;
		}
		const float unit = ImGui::GetFontSize();
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(unit * 0.7f, unit * 0.6f));
		const bool visible = ImGui::BeginChild(label, ImVec2(0.0f, 0.0f), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_None);
		ImGui::PopStyleVar();
		if (visible) {
			const CaptureWidgets::FormScope form;
			body();
		}
		ImGui::EndChild();
		ImGui::EndTabItem();
	}

	void render_summary_bar() {
		using namespace CaptureWidgets;
		const float unit = ui_unit();
		const auto& video = settings_.video;
		const bool realtime = video.mode == IO::SequenceCaptureMode::RealTime;
		const Capture::CaptureTarget still = resolve_target(settings_.screenshot_target);
		const Capture::CaptureTarget clip = resolve_target(settings_.sequence_target);
		const uint64_t frames = planned_frame_count();
		const double fps = static_cast<double>(video.frames_per_second);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(unit * 0.7f, unit * 0.45f));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, unit * 0.35f);
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.13f, 0.20f, 0.85f));
		const bool visible = ImGui::BeginChild("##StudioSummary", ImVec2(0.0f, 0.0f), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar);
		ImGui::PopStyleColor();
		ImGui::PopStyleVar(2);
		if (visible) {
			ImGui::PushTextWrapPos(0.0f);
			if (begin_stat_table("##SummaryStats", 5)) {
				stat_cell("Still Output", std::to_string(still.width) + " x " + std::to_string(still.height) + " px");
				stat_cell("Sequence Output", realtime ? std::string("Live viewport") : (std::to_string(clip.width) + " x " + std::to_string(clip.height) + " px"));
				stat_cell("Frame Rate", format_number(fps, 2) + " fps");
				stat_cell("Planned", frames > 0 ? (std::to_string(frames) + " frames / " + format_number(static_cast<double>(frames) / fps, 2) + " s") : std::string("Open-ended"));
				stat_cell("Camera", script_usable() ? std::string("Motion script") : std::string("Live camera"), script_usable() ? kAccentColor : kMutedColor);
				end_stat_table();
			}
			const std::string still_error = Capture::CaptureCoordinator::validate_target(still, screenshot_format());
			if (!still_error.empty()) {
				wrapped_text(kWarningColor, "Screenshot: " + still_error);
			}
			if (!realtime) {
				const std::string clip_error = Capture::CaptureCoordinator::validate_target(clip, video.frame_format);
				if (!clip_error.empty()) {
					wrapped_text(kWarningColor, "Sequence: " + clip_error);
				}
			}
			ImGui::PopTextWrapPos();
		}
		ImGui::EndChild();
	}

	void render_footer() {
		const float unit = ImGui::GetFontSize();
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(unit * 0.7f, unit * 0.5f));
		const bool visible = ImGui::BeginChild("CaptureStudioFooter", ImVec2(0.0f, 0.0f), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_None);
		ImGui::PopStyleVar();
		if (visible) {
			const CaptureWidgets::FormScope form;
			render_status_panel();
		}
		ImGui::EndChild();
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

	void open_tab(StudioTab tab) noexcept {
		is_open_ = true;
		requested_tab_ = tab;
	}

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

		const float base_unit = ImGui::GetFontSize();
		const ImVec2 work_size = ImGui::GetMainViewport()->WorkSize;
		const ImVec2 initial_size(
			std::clamp(base_unit * 52.0f, 520.0f, std::max(work_size.x * 0.85f, 520.0f)),
			std::clamp(base_unit * 60.0f, 480.0f, std::max(work_size.y * 0.9f, 480.0f))
		);
		ImGui::SetNextWindowPos(viewport_.window_center(), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(initial_size, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSizeConstraints(ImVec2(base_unit * 34.0f, base_unit * 36.0f), ImVec2(FLT_MAX, FLT_MAX));
		if (!ImGui::Begin("Capture Studio", &is_open_)) {
			ImGui::End();
			return;
		}

		render_summary_bar();

		const float unit = ImGui::GetFontSize();
		const float region_height = ImGui::GetContentRegionAvail().y;
		const float splitter_height = std::max(unit * 0.5f, 6.0f);
		const float spacing = ImGui::GetStyle().ItemSpacing.y;
		const float minimum_footer = unit * kMinimumFooterUnits;
		const float maximum_footer = std::max(region_height - unit * kMinimumTabsUnits - splitter_height - spacing * 2.0f, minimum_footer);
		const float default_footer = std::clamp(unit * kDefaultFooterUnits, minimum_footer, maximum_footer);
		if (footer_height_ <= 0.0f) {
			footer_height_ = default_footer;
		}
		footer_height_ = std::clamp(footer_height_, minimum_footer, maximum_footer);
		const float tabs_height = std::max(region_height - footer_height_ - splitter_height - spacing * 2.0f, unit * 4.0f);

		if (ImGui::BeginChild("CaptureStudioTabs", ImVec2(0.0f, tabs_height), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
			if (ImGui::BeginTabBar("CaptureStudioTabBar", ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_TabListPopupButton)) {
				render_tab("Screenshot", StudioTab::Screenshot, [this] { render_output_tab(); });
				render_tab("Sequence", StudioTab::Sequence, [this] { render_sequence_tab(); });
				render_tab("Motion Script", StudioTab::Script, [this] { render_script_tab(); });
				render_tab("Data Recording", StudioTab::Recording, [this] { render_recording_tab(); });
				render_tab("Video Encoding", StudioTab::Encoding, [this] { render_encoding_tab(); });
				ImGui::EndTabBar();
			}
		}
		ImGui::EndChild();
		requested_tab_ = StudioTab::None;

		CaptureWidgets::horizontal_splitter("##StudioFooterSplitter", footer_height_, minimum_footer, maximum_footer, default_footer);
		render_footer();
		ImGui::End();
	}
};

}
