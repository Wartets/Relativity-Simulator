#pragma once

#include "relativistic/capture/camera_path.hpp"
#include "relativistic/capture/capture_coordinator.hpp"
#include "relativistic/io/capture_studio_settings.hpp"
#include "relativistic/io/image_format.hpp"
#include "relativistic/io/screenshot_capture_settings.hpp"
#include "relativistic/io/screenshot_exporter.hpp"
#include "relativistic/io/user_settings.hpp"
#include "relativistic/io/video_capture_settings.hpp"
#include "relativistic/orchestrator/simulation_orchestrator.hpp"
#include "relativistic/ui/tooltip_utils.hpp"
#include "relativistic/ui/viewport_primary_window.hpp"
#include <imgui.h>
#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

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
inline constexpr std::array<const char*, 5> kTriggerNames{"Manual (Until Stopped)", "Fixed Duration", "Fixed Frame Count", "Continuous", "Camera Path Duration"};
inline constexpr std::array<const char*, 3> kAdvanceNames{"Frozen World", "Fixed Ticks Per Frame", "Simulation Seconds Per Video Second"};
inline constexpr std::array<const char*, 2> kPacingNames{"Drop Late Frames", "Duplicate To Fill Gaps"};
inline constexpr std::array<const char*, 9> kCodecNames{"H.264 (libx264)", "H.265 (libx265)", "VP9 (libvpx-vp9)", "ProRes 422 HQ", "Image Sequence Only", "AV1 (SVT-AV1)", "FFV1 (Lossless)", "Motion JPEG", "Animated GIF"};
inline constexpr std::array<const char*, 6> kContainerNames{"MP4", "MKV", "MOV", "WebM", "AVI", "GIF"};
inline constexpr std::array<const char*, 2> kRateControlNames{"Constant Quality (CRF)", "Target Bitrate"};
inline constexpr std::array<const char*, 9> kEncoderSpeedNames{"Ultrafast", "Superfast", "Veryfast", "Faster", "Fast", "Medium", "Slow", "Slower", "Veryslow"};
inline constexpr std::array<const char*, 7> kPathKindNames{"Keyframes", "Orbit", "Linear Fly-By", "Target Tracking Orbit", "Dolly Zoom (Vertigo)", "Logarithmic Spiral Infall", "Helical Trajectory"};
inline constexpr std::array<const char*, 4> kInterpolationNames{"Step", "Linear", "Smoothstep", "Catmull-Rom Spline"};
inline constexpr std::array<const char*, 5> kEasingNames{"Linear", "Ease In", "Ease Out", "Ease In Out", "Smootherstep"};
inline constexpr std::array<const char*, 3> kOrientationNames{"Keyframed / Fixed Angles", "Look At Target", "Along Travel Direction"};
inline constexpr std::array<const char*, 3> kPathEndNames{"Clamp At End", "Loop", "Ping-Pong"};
inline constexpr std::array<const char*, 3> kOverwriteNames{"Auto-Increment Filename", "Overwrite Existing File", "Skip If File Exists"};

inline const ImVec4 kHeaderColor(0.45f, 0.85f, 1.0f, 1.0f);
inline const ImVec4 kWarningColor(1.0f, 0.55f, 0.35f, 1.0f);

template <size_t N>
inline bool index_combo(const char* label, uint32_t& value, const std::array<const char*, N>& names) {
	int index = std::min(static_cast<int>(value), static_cast<int>(N) - 1);
	if (ImGui::Combo(label, &index, names.data(), static_cast<int>(N))) {
		value = static_cast<uint32_t>(index);
		return true;
	}
	return false;
}

template <typename Enum, size_t N>
inline bool enum_combo(const char* label, Enum& value, const std::array<const char*, N>& names) {
	uint32_t index = static_cast<uint32_t>(value);
	if (index_combo(label, index, names)) {
		value = static_cast<Enum>(index);
		return true;
	}
	return false;
}

inline bool slider_u32(const char* label, uint32_t& value, uint32_t min_value, uint32_t max_value, ImGuiSliderFlags flags = 0) {
	int scratch = static_cast<int>(value);
	if (ImGui::SliderInt(label, &scratch, static_cast<int>(min_value), static_cast<int>(max_value), "%d", flags)) {
		value = static_cast<uint32_t>(scratch);
		return true;
	}
	return false;
}

inline bool drag_double(const char* label, double& value, double speed, double min_value, double max_value, const char* format) {
	return ImGui::DragScalar(label, ImGuiDataType_Double, &value, static_cast<float>(speed), &min_value, &max_value, format);
}

inline bool drag_double3(const char* label, std::array<double, 3>& value, double speed, const char* format) {
	return ImGui::DragScalarN(label, ImGuiDataType_Double, value.data(), 3, static_cast<float>(speed), nullptr, nullptr, format);
}

inline void copy_to_buffer(char* buffer, size_t size, const std::string& value) {
	std::strncpy(buffer, value.c_str(), size - 1);
	buffer[size - 1] = '\0';
}

inline bool text_input(const char* label, char* buffer, size_t size, std::string& target) {
	if (ImGui::InputText(label, buffer, size)) {
		target = buffer;
		return true;
	}
	return false;
}

inline void wrapped_text(const ImVec4& color, const std::string& text) {
	ImGui::PushStyleColor(ImGuiCol_Text, color);
	ImGui::TextWrapped("%s", text.c_str());
	ImGui::PopStyleColor();
}

[[nodiscard]] inline std::string format_bytes(uint64_t bytes) {
	static constexpr std::array<const char*, 5> units{"B", "KiB", "MiB", "GiB", "TiB"};
	double value = static_cast<double>(bytes);
	size_t unit = 0;
	while (value >= 1024.0 && unit + 1 < units.size()) {
		value /= 1024.0;
		++unit;
	}
	char buffer[48];
	std::snprintf(buffer, sizeof(buffer), "%.2f %s", value, units[unit]);
	return buffer;
}

[[nodiscard]] inline std::string format_duration(double seconds) {
	if (!(seconds >= 0.0) || seconds > 1.0e9) {
		return "--";
	}
	const uint64_t total = static_cast<uint64_t>(seconds + 0.5);
	char buffer[48];
	std::snprintf(buffer, sizeof(buffer), "%lluh %02llum %02llus", static_cast<unsigned long long>(total / 3600ULL), static_cast<unsigned long long>((total / 60ULL) % 60ULL), static_cast<unsigned long long>(total % 60ULL));
	return buffer;
}

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
	static constexpr float kFooterHeight = 196.0f;
	static constexpr size_t kPathTextCapacity = 65536;

	bool is_open_{false};
	Orchestrator::SimulationOrchestrator<1024>& orchestrator_;
	ViewportPrimaryWindow& viewport_;
	IO::UserSettings& user_settings_;
	IO::CaptureStudioSettings settings_;
	Capture::RequestResult last_request_{};
	bool buffers_synced_{false};
	char directory_buffer_[256]{};
	char pattern_buffer_[128]{};
	char watermark_buffer_[128]{};
	char sequence_directory_buffer_[256]{};
	char session_buffer_[96]{};
	char ffmpeg_buffer_[192]{};
	char pixel_format_buffer_[32]{};
	char path_file_buffer_[256]{"config/camera_path.txt"};
	std::vector<char> path_text_buffer_ = std::vector<char>(kPathTextCapacity, '\0');
	std::string path_message_{};
	int selected_keyframe_{-1};
	double keyframe_spacing_{2.0};
	double preview_time_{0.0};
	bool preview_active_{false};
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

	[[nodiscard]] uint64_t planned_frame_count() const noexcept {
		const auto& video = settings_.video;
		const double fps = static_cast<double>(video.frames_per_second);
		const bool path_usable = settings_.use_camera_path && settings_.path.is_usable();
		switch (video.trigger) {
			case IO::SequenceCaptureTrigger::FixedDuration:
				return static_cast<uint64_t>(std::max(std::round(static_cast<double>(video.duration_seconds) * fps), 1.0));
			case IO::SequenceCaptureTrigger::FixedFrameCount:
				return std::max<uint64_t>(video.fixed_frame_count, 1ULL);
			case IO::SequenceCaptureTrigger::PathDuration:
				return static_cast<uint64_t>(std::max(std::round((path_usable ? settings_.path.effective_duration() : static_cast<double>(video.duration_seconds)) * fps), 1.0));
			case IO::SequenceCaptureTrigger::Manual:
			case IO::SequenceCaptureTrigger::Continuous:
			default:
				return 0;
		}
	}

	void sync_buffers() {
		using CaptureStudioDetail::copy_to_buffer;
		copy_to_buffer(directory_buffer_, sizeof(directory_buffer_), user_settings_.screenshot_output_directory);
		copy_to_buffer(pattern_buffer_, sizeof(pattern_buffer_), user_settings_.screenshot_filename_pattern);
		copy_to_buffer(watermark_buffer_, sizeof(watermark_buffer_), user_settings_.screenshot_watermark_text);
		copy_to_buffer(sequence_directory_buffer_, sizeof(sequence_directory_buffer_), settings_.sequence_directory);
		copy_to_buffer(session_buffer_, sizeof(session_buffer_), settings_.session_name);
		copy_to_buffer(ffmpeg_buffer_, sizeof(ffmpeg_buffer_), settings_.video.ffmpeg_executable);
		copy_to_buffer(pixel_format_buffer_, sizeof(pixel_format_buffer_), settings_.video.pixel_format);
		buffers_synced_ = true;
	}

	void render_target_editor(IO::CaptureTargetSettings& target, IO::ScreenshotFormat format, const char* id) {
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
		using namespace CaptureStudioDetail;
		ImGui::TextColored(kHeaderColor, "Destination");
		text_input("Output Directory", directory_buffer_, sizeof(directory_buffer_), user_settings_.screenshot_output_directory);
		render_setting_tooltip("Folder where screenshots are written. It is created automatically when missing.");
		text_input("Filename Pattern", pattern_buffer_, sizeof(pattern_buffer_), user_settings_.screenshot_filename_pattern);
		render_setting_tooltip("Tokens: %metric%, %mass%, %spin%, %width%, %height%, %tick%, plus any strftime token such as %Y %m %d %H %M %S.");
		ImGui::SameLine();
		if (ImGui::SmallButton("Smart Name")) {
			user_settings_.screenshot_filename_pattern = "%metric%_M%mass%_a%spin%_%width%x%height%_%Y%m%d_%H%M%S";
			copy_to_buffer(pattern_buffer_, sizeof(pattern_buffer_), user_settings_.screenshot_filename_pattern);
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
			text_input("Watermark Text", watermark_buffer_, sizeof(watermark_buffer_), user_settings_.screenshot_watermark_text);
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
		using namespace CaptureStudioDetail;
		auto& video = settings_.video;

		ImGui::TextColored(kHeaderColor, "Capture Mode");
		enum_combo("Mode", video.mode, kCaptureModeNames);
		render_setting_tooltip("Deterministic renders every frame offline at the requested quality while the world is advanced under full control. Real-Time records what the viewport currently shows.");
		const bool realtime = video.mode == IO::SequenceCaptureMode::RealTime;

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
			if (settings_.use_camera_path && settings_.path.is_usable()) {
				ImGui::TextDisabled("Follows the camera path duration: %.3f s", settings_.path.effective_duration());
			} else {
				ImGui::SliderFloat("Fallback Duration", &video.duration_seconds, 0.1f, 3600.0f, "%.2f s", ImGuiSliderFlags_Logarithmic);
				wrapped_text(kWarningColor, "No usable camera path is enabled, the fallback duration is used.");
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
		text_input("Sequence Directory", sequence_directory_buffer_, sizeof(sequence_directory_buffer_), settings_.sequence_directory);
		text_input("Session Name (empty = automatic)", session_buffer_, sizeof(session_buffer_), settings_.session_name);
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

	void render_keyframe_editor(Capture::CameraPath& path) {
		using namespace CaptureStudioDetail;
		auto& keys = path.keyframes;
		selected_keyframe_ = std::clamp(selected_keyframe_, -1, static_cast<int>(keys.size()) - 1);

		ImGui::BeginChild("CaptureKeyframeList", ImVec2(0.0f, 120.0f), true);
		for (size_t i = 0; i < keys.size(); ++i) {
			char label[192];
			const auto& pose = keys[i].pose;
			std::snprintf(label, sizeof(label), "%02zu | t=%.3f s | pos=(%.2f, %.2f, %.2f) | pitch=%.1f yaw=%.1f | fov=%.1f", i + 1, keys[i].time_seconds, pose.position[0], pose.position[1], pose.position[2], pose.pitch_deg, pose.yaw_deg, pose.fov_deg);
			if (ImGui::Selectable(label, selected_keyframe_ == static_cast<int>(i))) {
				selected_keyframe_ = static_cast<int>(i);
			}
		}
		if (keys.empty()) {
			ImGui::TextDisabled("No keyframes. Move the camera and add one.");
		}
		ImGui::EndChild();

		const auto reselect = [&](const Capture::CameraKeyframe& reference) {
			selected_keyframe_ = -1;
			for (size_t i = 0; i < keys.size(); ++i) {
				if (keys[i].time_seconds == reference.time_seconds && keys[i].pose.position == reference.pose.position) {
					selected_keyframe_ = static_cast<int>(i);
					break;
				}
			}
		};

		drag_double("Spacing For New Keyframes (s)", keyframe_spacing_, 0.05, 0.01, 3600.0, "%.2f");
		if (ImGui::Button("Add From Current Camera")) {
			const double time = keys.empty() ? 0.0 : keys.back().time_seconds + keyframe_spacing_;
			const Capture::CameraPose pose = current_camera_pose();
			path.add_keyframe(pose, time);
			Capture::CameraKeyframe reference;
			reference.time_seconds = time;
			reference.pose = pose;
			reselect(reference);
		}
		render_setting_tooltip("Stores the current camera pose as a new keyframe after the last one.");
		ImGui::SameLine();
		ImGui::BeginDisabled(selected_keyframe_ < 0);
		if (ImGui::Button("Update Selected From Camera")) {
			keys[static_cast<size_t>(selected_keyframe_)].pose = current_camera_pose();
		}
		ImGui::SameLine();
		if (ImGui::Button("Duplicate")) {
			Capture::CameraKeyframe copy = keys[static_cast<size_t>(selected_keyframe_)];
			copy.time_seconds += keyframe_spacing_;
			path.keyframes.push_back(copy);
			path.sort_keyframes();
			reselect(copy);
		}
		ImGui::SameLine();
		if (ImGui::Button("Delete")) {
			keys.erase(keys.begin() + selected_keyframe_);
			selected_keyframe_ = std::min(selected_keyframe_, static_cast<int>(keys.size()) - 1);
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button("Clear All")) {
			keys.clear();
			selected_keyframe_ = -1;
		}
		ImGui::SameLine();
		if (ImGui::Button("Apply Default Interpolation To All")) {
			for (auto& key : keys) {
				key.interpolation = path.interpolation;
			}
		}

		if (selected_keyframe_ >= 0) {
			auto& key = keys[static_cast<size_t>(selected_keyframe_)];
			ImGui::Separator();
			const double previous_time = key.time_seconds;
			if (drag_double("Time (s)", key.time_seconds, 0.02, 0.0, 86400.0, "%.3f") && key.time_seconds != previous_time) {
				const Capture::CameraKeyframe reference = key;
				path.sort_keyframes();
				reselect(reference);
			}
			if (selected_keyframe_ >= 0) {
				auto& edited = keys[static_cast<size_t>(selected_keyframe_)];
				drag_double3("Position", edited.pose.position, 0.1, "%.4f");
				drag_double("Pitch (deg)", edited.pose.pitch_deg, 0.25, -89.0, 89.0, "%.2f");
				drag_double("Yaw (deg)", edited.pose.yaw_deg, 0.25, -36000.0, 36000.0, "%.2f");
				drag_double("Roll (deg)", edited.pose.roll_deg, 0.25, -36000.0, 36000.0, "%.2f");
				drag_double("Field Of View (deg)", edited.pose.fov_deg, 0.1, 5.0, 175.0, "%.2f");
				drag_double("Exposure (EV)", edited.pose.exposure_ev, 0.02, -6.0, 6.0, "%.2f");
				enum_combo("Interpolation To Next", edited.interpolation, kInterpolationNames);
			}
		}
	}

	void save_path_file(const Capture::CameraPath& path) {
		const std::filesystem::path target(path_file_buffer_);
		std::error_code ec;
		if (target.has_parent_path()) {
			std::filesystem::create_directories(target.parent_path(), ec);
		}
		std::ofstream out(target, std::ios::trunc);
		if (!out.is_open()) {
			path_message_ = "The path file could not be written: " + target.string();
			return;
		}
		out << path.to_text();
		path_message_ = "Camera path saved to " + target.string();
	}

	void load_path_file(Capture::CameraPath& path) {
		std::ifstream in{std::filesystem::path(path_file_buffer_)};
		if (!in.is_open()) {
			path_message_ = std::string("The path file could not be opened: ") + path_file_buffer_;
			return;
		}
		std::ostringstream buffer;
		buffer << in.rdbuf();
		auto parsed = Capture::CameraPath::from_text(buffer.str());
		if (!parsed.has_value()) {
			path_message_ = "The file does not contain a valid camera path.";
			return;
		}
		path = std::move(*parsed);
		selected_keyframe_ = -1;
		path_message_ = "Camera path loaded.";
	}

	void render_path_tab() {
		using namespace CaptureStudioDetail;
		auto& path = settings_.path;
		auto& coordinator = viewport_.capture_coordinator();

		ImGui::Checkbox("Drive The Camera With This Path During Sequence Captures", &settings_.use_camera_path);
		render_setting_tooltip("When enabled, sequence captures move the camera along the path below instead of following manual input.");
		if (settings_.use_camera_path && !path.is_usable()) {
			wrapped_text(kWarningColor, "The keyframe path is empty and will be ignored until at least one keyframe exists.");
		}

		ImGui::Separator();
		ImGui::TextColored(kHeaderColor, "Path Definition");
		enum_combo("Path Type", path.kind, kPathKindNames);
		const bool keyframed = path.kind == Capture::PathKind::Keyframes;
		if (keyframed) {
			ImGui::TextDisabled("Duration follows the last keyframe: %.3f s", path.effective_duration());
		} else {
			drag_double("Duration (s)", path.duration_seconds, 0.05, 0.01, 86400.0, "%.3f");
		}
		enum_combo("Default Keyframe Interpolation", path.interpolation, kInterpolationNames);
		enum_combo("Time Easing", path.easing, kEasingNames);
		enum_combo("End Behavior", path.end_behavior, kPathEndNames);
		enum_combo("Orientation Mode", path.orientation, kOrientationNames);

		if (path.orientation == Capture::PathOrientation::LookAtTarget) {
			drag_double3("Look-At Target", path.look_target, 0.1, "%.4f");
			if (ImGui::SmallButton("Target: Central Object")) {
				path.look_target = {0.0, 0.0, 0.0};
			}
		}

		if (!keyframed) {
			drag_double("Roll (deg)", path.roll_deg, 0.25, -36000.0, 36000.0, "%.2f");
			if (path.orientation == Capture::PathOrientation::Keyframed) {
				drag_double("Fixed Pitch (deg)", path.fixed_pitch_yaw[0], 0.25, -89.0, 89.0, "%.2f");
				drag_double("Fixed Yaw (deg)", path.fixed_pitch_yaw[1], 0.25, -36000.0, 36000.0, "%.2f");
			}
			drag_double("Field Of View Start (deg)", path.fov_start_deg, 0.1, 5.0, 175.0, "%.2f");
			drag_double("Field Of View End (deg)", path.fov_end_deg, 0.1, 5.0, 175.0, "%.2f");
			drag_double("Exposure Start (EV)", path.exposure_start_ev, 0.02, -6.0, 6.0, "%.2f");
			drag_double("Exposure End (EV)", path.exposure_end_ev, 0.02, -6.0, 6.0, "%.2f");
		}

		ImGui::Separator();
		if (path.kind == Capture::PathKind::Orbit || path.kind == Capture::PathKind::TargetTrackingOrbit) {
			ImGui::TextColored(kHeaderColor, path.kind == Capture::PathKind::TargetTrackingOrbit ? "Target Tracking Orbit" : "Orbit");
			if (path.kind == Capture::PathKind::TargetTrackingOrbit) {
				const auto& bodies = orchestrator_.nbody_system().bodies();
				std::vector<std::string> body_items;
				body_items.push_back("Origin / Central Source");
				int selected_body_idx = (path.tracked_body_id < 0) ? 0 : 0;
				for (size_t bi = 0; bi < bodies.size(); ++bi) {
					body_items.push_back("Body #" + std::to_string(bodies[bi].id) + " (M=" + std::to_string(bodies[bi].mass).substr(0, 4) + ")");
					if (static_cast<int32_t>(bodies[bi].id) == path.tracked_body_id) {
						selected_body_idx = static_cast<int>(bi + 1);
					}
				}
				std::vector<const char*> item_ptrs;
				item_ptrs.reserve(body_items.size());
				for (const auto& s : body_items) item_ptrs.push_back(s.c_str());
				if (ImGui::Combo("Tracked Celestial Body", &selected_body_idx, item_ptrs.data(), static_cast<int>(item_ptrs.size()))) {
					if (selected_body_idx == 0) {
						path.tracked_body_id = -1;
					} else {
						path.tracked_body_id = static_cast<int32_t>(bodies[static_cast<size_t>(selected_body_idx - 1)].id);
					}
				}
			} else {
				drag_double3("Orbit Center", path.orbit.center, 0.1, "%.4f");
			}
			drag_double("Radius Start", path.orbit.radius_start, 0.1, 0.001, 1.0e9, "%.3f");
			drag_double("Radius End", path.orbit.radius_end, 0.1, 0.001, 1.0e9, "%.3f");
			drag_double("Elevation Start (deg)", path.orbit.elevation_start_deg, 0.1, -89.0, 89.0, "%.2f");
			drag_double("Elevation End (deg)", path.orbit.elevation_end_deg, 0.1, -89.0, 89.0, "%.2f");
			drag_double("Azimuth Start (deg)", path.orbit.azimuth_start_deg, 0.25, -36000.0, 36000.0, "%.2f");
			drag_double("Revolutions", path.orbit.revolutions, 0.01, -1000.0, 1000.0, "%.3f");
			if (ImGui::SmallButton("Radius Start From Camera")) {
				const auto& camera = orchestrator_.camera();
				path.orbit.radius_start = std::max(std::sqrt(camera.position[0] * camera.position[0] + camera.position[1] * camera.position[1] + camera.position[2] * camera.position[2]), 0.001);
			}
		} else if (path.kind == Capture::PathKind::LogarithmicSpiral) {
			ImGui::TextColored(kHeaderColor, "Logarithmic Spiral Infall");
			drag_double3("Spiral Center", path.spiral.center, 0.1, "%.4f");
			drag_double("Radius Start", path.spiral.radius_start, 0.1, 0.01, 1.0e9, "%.3f");
			drag_double("Radius End (Infall)", path.spiral.radius_end, 0.1, 0.001, 1.0e9, "%.3f");
			drag_double("Height Start", path.spiral.height_start, 0.1, -1.0e6, 1.0e6, "%.3f");
			drag_double("Height End", path.spiral.height_end, 0.1, -1.0e6, 1.0e6, "%.3f");
			drag_double("Revolutions", path.spiral.revolutions, 0.05, 0.1, 100.0, "%.3f");
			drag_double("Expansion / Acceleration Rate", path.spiral.expansion_rate, 0.02, 0.1, 10.0, "%.2f");
			if (ImGui::SmallButton("Start From Camera")) {
				const auto& camera = orchestrator_.camera();
				path.spiral.radius_start = std::max(std::sqrt(camera.position[0] * camera.position[0] + camera.position[1] * camera.position[1]), 0.01);
				path.spiral.height_start = camera.position[2];
			}
		} else if (path.kind == Capture::PathKind::Helical) {
			ImGui::TextColored(kHeaderColor, "Helical Trajectory");
			drag_double3("Helical Start Axis", path.helical.start, 0.1, "%.4f");
			drag_double3("Helical End Axis", path.helical.end, 0.1, "%.4f");
			drag_double("Helix Radius", path.helical.radius, 0.1, 0.01, 1.0e6, "%.3f");
			drag_double("Revolutions", path.helical.revolutions, 0.05, 0.1, 100.0, "%.3f");
			drag_double("Phase Offset (deg)", path.helical.phase_deg, 0.25, -360.0, 360.0, "%.2f");
		} else if (path.kind == Capture::PathKind::DollyZoom) {
			ImGui::TextColored(kHeaderColor, "Dolly Zoom (Vertigo Effect)");
			drag_double3("Start Position", path.dolly_zoom.start_position, 0.1, "%.4f");
			drag_double3("End Position", path.dolly_zoom.end_position, 0.1, "%.4f");
			drag_double3("Fixed Focus Target", path.dolly_zoom.target, 0.1, "%.4f");
			drag_double("Start FOV (deg)", path.dolly_zoom.fov_start_deg, 0.1, 5.0, 170.0, "%.2f");
			drag_double("End FOV (deg)", path.dolly_zoom.fov_end_deg, 0.1, 5.0, 170.0, "%.2f");
			if (ImGui::SmallButton("Start From Camera")) {
				path.dolly_zoom.start_position = orchestrator_.camera().position;
				path.dolly_zoom.fov_start_deg = orchestrator_.camera().fov_deg;
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("End At Camera")) {
				path.dolly_zoom.end_position = orchestrator_.camera().position;
				path.dolly_zoom.fov_end_deg = orchestrator_.camera().fov_deg;
			}
		} else if (path.kind == Capture::PathKind::LinearFlyBy) {
			ImGui::TextColored(kHeaderColor, "Linear Fly-By");
			drag_double3("Start Position", path.fly_by.start, 0.1, "%.4f");
			drag_double3("End Position", path.fly_by.end, 0.1, "%.4f");
			if (ImGui::SmallButton("Start From Camera")) {
				path.fly_by.start = orchestrator_.camera().position;
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("End At Camera")) {
				path.fly_by.end = orchestrator_.camera().position;
			}
		} else {
			ImGui::TextColored(kHeaderColor, "Keyframes");
			render_keyframe_editor(path);
		}

		ImGui::Separator();
		ImGui::TextColored(kHeaderColor, "Preview");
		const double total = path.effective_duration();
		const double zero = 0.0;
		const bool session_running = coordinator.is_sequence_active();
		ImGui::BeginDisabled(session_running || !path.is_usable());
		if (ImGui::SliderScalar("Preview Time", ImGuiDataType_Double, &preview_time_, &zero, &total, "%.3f s")) {
			if (!preview_active_) {
				preview_restore_pose_ = current_camera_pose();
				preview_active_ = true;
			}
			coordinator.preview_pose(path.evaluate(preview_time_));
		}
		render_setting_tooltip("Moves the live camera along the path so the trajectory can be inspected before capturing.");
		ImGui::EndDisabled();
		ImGui::BeginDisabled(!preview_active_ || session_running);
		if (ImGui::Button("Restore Camera After Preview")) {
			coordinator.preview_pose(preview_restore_pose_);
			preview_active_ = false;
		}
		ImGui::EndDisabled();

		ImGui::Separator();
		ImGui::TextColored(kHeaderColor, "Files And Text");
		ImGui::InputText("Path File", path_file_buffer_, sizeof(path_file_buffer_));
		if (ImGui::Button("Save Path File")) {
			save_path_file(path);
		}
		ImGui::SameLine();
		if (ImGui::Button("Load Path File")) {
			load_path_file(path);
		}
		ImGui::InputTextMultiline("##CapturePathText", path_text_buffer_.data(), path_text_buffer_.size(), ImVec2(-1.0f, 110.0f));
		if (ImGui::Button("Export To Text")) {
			copy_to_buffer(path_text_buffer_.data(), path_text_buffer_.size(), path.to_text());
		}
		ImGui::SameLine();
		if (ImGui::Button("Import From Text")) {
			auto parsed = Capture::CameraPath::from_text(std::string(path_text_buffer_.data()));
			if (parsed.has_value()) {
				path = std::move(*parsed);
				selected_keyframe_ = -1;
				path_message_ = "Camera path imported from text.";
			} else {
				path_message_ = "The text does not contain a valid camera path.";
			}
		}
		if (!path_message_.empty()) {
			ImGui::TextDisabled("%s", path_message_.c_str());
		}
	}

	void render_encoding_tab() {
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
		text_input("Pixel Format", pixel_format_buffer_, sizeof(pixel_format_buffer_), video.pixel_format);
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
		text_input("ffmpeg Executable", ffmpeg_buffer_, sizeof(ffmpeg_buffer_), video.ffmpeg_executable);

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
		render_setting_tooltip("Renders one frame offline with the Output tab settings. Disabled while another capture is running.");
		ImGui::SameLine();
		if (ImGui::Button("Start Sequence", ImVec2(150.0f, 28.0f))) {
			start_sequence();
		}
		render_setting_tooltip("Starts a sequence capture with the Sequence, Camera Path and Encoding tab settings.");
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
		Capture::SequenceRequest request;
		request.target = resolve_target(settings_.sequence_target);
		request.settings = settings_.video;
		request.output_directory = settings_.sequence_directory;
		request.session_name = settings_.session_name.empty() ? std::string{} : IO::ScreenshotExporter::expand_filename_pattern(settings_.session_name);
		request.comment = user_settings_.screenshot_watermark_enabled ? user_settings_.screenshot_watermark_text : std::string{};
		request.use_path = settings_.use_camera_path && settings_.path.is_usable();
		request.path = settings_.path;
		request.manual_stepping = settings_.manual_stepping && settings_.video.mode == IO::SequenceCaptureMode::Deterministic;
		last_request_ = viewport_.capture_coordinator().start_sequence(request);
		if (last_request_.ok) {
			preview_active_ = false;
		}
	}

	void render() {
		if (!is_open_) {
			return;
		}
		if (!buffers_synced_) {
			sync_buffers();
		}

		ImGui::SetNextWindowPos(viewport_.window_center(), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(660.0f, 760.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSizeConstraints(ImVec2(540.0f, 460.0f), ImVec2(FLT_MAX, FLT_MAX));
		if (!ImGui::Begin("Capture Studio", &is_open_)) {
			ImGui::End();
			return;
		}

		ImGui::BeginChild("CaptureStudioTabs", ImVec2(0.0f, -kFooterHeight), false);
		if (ImGui::BeginTabBar("CaptureStudioTabBar")) {
			if (ImGui::BeginTabItem("Screenshot")) {
				render_output_tab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Sequence")) {
				render_sequence_tab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Camera Path")) {
				render_path_tab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Video Encoding")) {
				render_encoding_tab();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
		ImGui::EndChild();

		ImGui::Separator();
		render_status_panel();
		ImGui::End();
	}
};

}
