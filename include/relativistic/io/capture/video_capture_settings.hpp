#pragma once

#include "relativistic/io/capture/capture_settings_io.hpp"
#include "relativistic/io/image/image_format.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>

namespace Relativistic::IO {

enum class VideoContainer : uint32_t {
	MP4 = 0,
	MKV = 1,
	MOV = 2,
	WebM = 3,
	AVI = 4,
	GIF = 5
};

enum class VideoCodecPreset : uint32_t {
	H264 = 0,
	H265 = 1,
	VP9 = 2,
	ProRes = 3,
	PngSequence = 4,
	AV1 = 5,
	FFV1 = 6,
	MJPEG = 7,
	GIF = 8
};

enum class SequenceCaptureTrigger : uint32_t {
	Manual = 0,
	FixedDuration = 1,
	FixedFrameCount = 2,
	Continuous = 3,
	PathDuration = 4
};

enum class SequenceCaptureMode : uint32_t {
	Deterministic = 0,
	RealTime = 1
};

enum class SequenceAdvanceMode : uint32_t {
	FrozenWorld = 0,
	TicksPerFrame = 1,
	SimulationRateRatio = 2
};

enum class RealTimePacing : uint32_t {
	DropLate = 0,
	DuplicateToFill = 1
};

enum class VideoRateControl : uint32_t {
	ConstantQuality = 0,
	TargetBitrate = 1
};

enum class VideoEncoderSpeed : uint32_t {
	Ultrafast = 0,
	Superfast = 1,
	Veryfast = 2,
	Faster = 3,
	Fast = 4,
	Medium = 5,
	Slow = 6,
	Slower = 7,
	Veryslow = 8
};

struct VideoSequenceSettings {
	float frames_per_second{30.0f};
	float duration_seconds{5.0f};
	uint64_t fixed_frame_count{300};
	float resolution_scale{1.0f};
	ScreenshotFormat frame_format{ScreenshotFormat::PNG};
	SequenceCaptureMode mode{SequenceCaptureMode::Deterministic};
	SequenceCaptureTrigger trigger{SequenceCaptureTrigger::FixedDuration};
	SequenceAdvanceMode advance_mode{SequenceAdvanceMode::FrozenWorld};
	bool pause_simulation_during_capture{true};
	uint32_t ticks_per_frame{1};
	double simulation_seconds_per_video_second{1.0};
	uint32_t temporal_samples{1};
	float shutter_fraction{0.5f};
	uint32_t fade_in_frames{0};
	uint32_t fade_out_frames{0};
	uint64_t start_frame_index{0};
	uint32_t frame_name_padding{6};
	bool preview_in_viewport{false};
	bool restore_camera_after_capture{true};
	RealTimePacing pacing{RealTimePacing::DropLate};
	uint32_t realtime_queue_depth{8};
	VideoCodecPreset codec{VideoCodecPreset::H264};
	VideoContainer container{VideoContainer::MP4};
	VideoRateControl rate_control{VideoRateControl::ConstantQuality};
	VideoEncoderSpeed encoder_speed{VideoEncoderSpeed::Medium};
	uint32_t crf{18};
	uint32_t target_bitrate_kbps{20000};
	std::string pixel_format{"yuv420p"};
	float encode_frames_per_second{0.0f};
	bool loop_output{false};
	bool assemble_video_after_capture{false};
	bool delete_frames_after_assembly{false};
	std::string ffmpeg_executable{"ffmpeg"};

	[[nodiscard]] std::string codec_name() const {
		switch (codec) {
			case VideoCodecPreset::H264: return "libx264";
			case VideoCodecPreset::H265: return "libx265";
			case VideoCodecPreset::VP9: return "libvpx-vp9";
			case VideoCodecPreset::ProRes: return "prores_ks";
			case VideoCodecPreset::PngSequence: return "png";
			case VideoCodecPreset::AV1: return "libsvtav1";
			case VideoCodecPreset::FFV1: return "ffv1";
			case VideoCodecPreset::MJPEG: return "mjpeg";
			case VideoCodecPreset::GIF: return "gif";
			default: return "libx264";
		}
	}

	[[nodiscard]] static std::string_view encoder_speed_name(VideoEncoderSpeed speed) noexcept {
		switch (speed) {
			case VideoEncoderSpeed::Ultrafast: return "ultrafast";
			case VideoEncoderSpeed::Superfast: return "superfast";
			case VideoEncoderSpeed::Veryfast: return "veryfast";
			case VideoEncoderSpeed::Faster: return "faster";
			case VideoEncoderSpeed::Fast: return "fast";
			case VideoEncoderSpeed::Slow: return "slow";
			case VideoEncoderSpeed::Slower: return "slower";
			case VideoEncoderSpeed::Veryslow: return "veryslow";
			case VideoEncoderSpeed::Medium:
			default: return "medium";
		}
	}

	[[nodiscard]] VideoContainer resolved_container() const noexcept {
		if (codec == VideoCodecPreset::GIF) {
			return VideoContainer::GIF;
		}
		if (codec == VideoCodecPreset::ProRes) {
			return VideoContainer::MOV;
		}
		if (container == VideoContainer::GIF) {
			return VideoContainer::MKV;
		}
		if (container == VideoContainer::WebM && codec != VideoCodecPreset::VP9 && codec != VideoCodecPreset::AV1) {
			return VideoContainer::MKV;
		}
		if (codec == VideoCodecPreset::FFV1 && (container == VideoContainer::MP4 || container == VideoContainer::MOV)) {
			return VideoContainer::MKV;
		}
		return container;
	}

	[[nodiscard]] std::string container_extension() const {
		switch (resolved_container()) {
			case VideoContainer::MP4: return "mp4";
			case VideoContainer::MKV: return "mkv";
			case VideoContainer::MOV: return "mov";
			case VideoContainer::WebM: return "webm";
			case VideoContainer::AVI: return "avi";
			case VideoContainer::GIF: return "gif";
			default: return "mp4";
		}
	}

	[[nodiscard]] std::string resolved_pixel_format() const {
		if (codec == VideoCodecPreset::ProRes) {
			return "yuv422p10le";
		}
		if (codec == VideoCodecPreset::MJPEG) {
			return "yuvj420p";
		}
		return pixel_format.empty() ? std::string("yuv420p") : pixel_format;
	}

	[[nodiscard]] std::string frame_file_name(uint64_t index, std::string_view extension) const {
		std::string digits = std::to_string(index);
		const size_t padding = std::clamp<size_t>(frame_name_padding, 1U, 12U);
		if (digits.size() < padding) {
			digits.insert(0, padding - digits.size(), '0');
		}
		return "frame_" + digits + "." + std::string(extension);
	}

	[[nodiscard]] std::string frame_input_pattern(std::string_view extension) const {
		return "frame_%0" + std::to_string(std::clamp<uint32_t>(frame_name_padding, 1U, 12U)) + "d." + std::string(extension);
	}

	[[nodiscard]] std::string build_ffmpeg_command(
		const std::string& input_directory,
		const std::string& input_pattern,
		uint64_t start_number,
		const std::string& output_path_without_extension
	) const {
		if (codec == VideoCodecPreset::PngSequence) {
			return {};
		}
		char number_buffer[32];
		std::snprintf(number_buffer, sizeof(number_buffer), "%.3f", static_cast<double>(frames_per_second));
		std::string exe = ffmpeg_executable.empty() ? std::string("ffmpeg") : ffmpeg_executable;
		if (exe.find(' ') != std::string::npos && (exe.front() != '"' || exe.back() != '"')) {
			exe = "\"" + exe + "\"";
		}
		std::string cmd = exe;
		cmd += " -y -hide_banner -loglevel warning -framerate ";
		cmd += number_buffer;
		cmd += " -start_number " + std::to_string(start_number);
		const std::string input_full = (std::filesystem::path(input_directory) / input_pattern).string();
		cmd += " -i \"" + input_full + "\"";
		if (codec == VideoCodecPreset::GIF) {
			cmd += " -vf \"split[s0][s1];[s0]palettegen=stats_mode=diff[p];[s1][p]paletteuse=dither=bayer:bayer_scale=3\"";
		} else {
			cmd += " -vf \"pad=ceil(iw/2)*2:ceil(ih/2)*2\"";
		}
		cmd += " -c:v " + codec_name();
		const uint32_t speed_index = static_cast<uint32_t>(encoder_speed);
		switch (codec) {
			case VideoCodecPreset::H264:
			case VideoCodecPreset::H265:
				cmd += " -preset " + std::string(encoder_speed_name(encoder_speed));
				cmd += (rate_control == VideoRateControl::ConstantQuality) ? (" -crf " + std::to_string(crf)) : (" -b:v " + std::to_string(target_bitrate_kbps) + "k");
				if (codec == VideoCodecPreset::H264) {
					cmd += " -pix_fmt " + resolved_pixel_format();
				}
				break;
			case VideoCodecPreset::VP9:
				cmd += (rate_control == VideoRateControl::ConstantQuality) ? (" -crf " + std::to_string(crf) + " -b:v 0") : (" -b:v " + std::to_string(target_bitrate_kbps) + "k");
				cmd += " -deadline " + std::string(speed_index <= 2 ? "realtime" : (speed_index <= 5 ? "good" : "best"));
				cmd += " -pix_fmt " + resolved_pixel_format();
				break;
			case VideoCodecPreset::AV1:
				cmd += (rate_control == VideoRateControl::ConstantQuality) ? (" -crf " + std::to_string(crf)) : (" -b:v " + std::to_string(target_bitrate_kbps) + "k");
				cmd += " -preset " + std::to_string(std::clamp<uint32_t>(speed_index, 0U, 12U));
				cmd += " -pix_fmt " + resolved_pixel_format();
				break;
			case VideoCodecPreset::ProRes:
				cmd += " -profile:v 3 -pix_fmt " + resolved_pixel_format();
				break;
			case VideoCodecPreset::FFV1:
				cmd += " -level 3 -pix_fmt " + resolved_pixel_format();
				break;
			case VideoCodecPreset::MJPEG:
				cmd += " -q:v " + std::to_string(std::clamp<uint32_t>((crf * 31U) / 51U + 1U, 1U, 31U));
				cmd += " -pix_fmt " + resolved_pixel_format();
				break;
			case VideoCodecPreset::GIF:
				if (loop_output) {
					cmd += " -loop 0";
				}
				break;
			default:
				break;
		}
		if (encode_frames_per_second > 0.0f) {
			char out_fps_buf[32];
			std::snprintf(out_fps_buf, sizeof(out_fps_buf), "%.3f", static_cast<double>(encode_frames_per_second));
			cmd += " -r ";
			cmd += out_fps_buf;
		}
		const std::string ext = container_extension();
		cmd += " \"" + output_path_without_extension + "." + ext + "\"";
		return cmd;
	}

	[[nodiscard]] std::string build_ffmpeg_command(
		const std::string& input_directory,
		const std::string& extension,
		const std::string& output_path_without_extension
	) const {
		return build_ffmpeg_command(
			input_directory,
			frame_input_pattern(extension),
			start_frame_index,
			output_path_without_extension
		);
	}

	void write_settings(SettingsWriter& writer) const {
		writer.real("frames_per_second", frames_per_second);
		writer.real("duration_seconds", duration_seconds);
		writer.unsigned_value("fixed_frame_count", fixed_frame_count);
		writer.real("resolution_scale", resolution_scale);
		writer.enumeration("frame_format", frame_format);
		writer.enumeration("mode", mode);
		writer.enumeration("trigger", trigger);
		writer.enumeration("advance_mode", advance_mode);
		writer.flag("pause_simulation_during_capture", pause_simulation_during_capture);
		writer.unsigned_value("ticks_per_frame", ticks_per_frame);
		writer.real("simulation_seconds_per_video_second", simulation_seconds_per_video_second);
		writer.unsigned_value("temporal_samples", temporal_samples);
		writer.real("shutter_fraction", shutter_fraction);
		writer.unsigned_value("fade_in_frames", fade_in_frames);
		writer.unsigned_value("fade_out_frames", fade_out_frames);
		writer.unsigned_value("start_frame_index", start_frame_index);
		writer.unsigned_value("frame_name_padding", frame_name_padding);
		writer.flag("preview_in_viewport", preview_in_viewport);
		writer.flag("restore_camera_after_capture", restore_camera_after_capture);
		writer.enumeration("pacing", pacing);
		writer.unsigned_value("realtime_queue_depth", realtime_queue_depth);
		writer.enumeration("codec", codec);
		writer.enumeration("container", container);
		writer.enumeration("rate_control", rate_control);
		writer.enumeration("encoder_speed", encoder_speed);
		writer.unsigned_value("crf", crf);
		writer.unsigned_value("target_bitrate_kbps", target_bitrate_kbps);
		writer.text("pixel_format", pixel_format);
		writer.real("encode_frames_per_second", encode_frames_per_second);
		writer.flag("loop_output", loop_output);
		writer.flag("assemble_video_after_capture", assemble_video_after_capture);
		writer.flag("delete_frames_after_assembly", delete_frames_after_assembly);
		writer.text("ffmpeg_executable", ffmpeg_executable);
	}

	void read_settings(const SettingsReader& reader) {
		frames_per_second = static_cast<float>(reader.real("frames_per_second", frames_per_second));
		duration_seconds = static_cast<float>(reader.real("duration_seconds", duration_seconds));
		fixed_frame_count = reader.wide_value("fixed_frame_count", fixed_frame_count);
		resolution_scale = static_cast<float>(reader.real("resolution_scale", resolution_scale));
		frame_format = reader.enumeration("frame_format", frame_format, ScreenshotFormat::PAM);
		mode = reader.enumeration("mode", mode, SequenceCaptureMode::RealTime);
		trigger = reader.enumeration("trigger", trigger, SequenceCaptureTrigger::PathDuration);
		advance_mode = reader.enumeration("advance_mode", advance_mode, SequenceAdvanceMode::SimulationRateRatio);
		pause_simulation_during_capture = reader.flag("pause_simulation_during_capture", pause_simulation_during_capture);
		ticks_per_frame = reader.unsigned_value("ticks_per_frame", ticks_per_frame);
		simulation_seconds_per_video_second = reader.real("simulation_seconds_per_video_second", simulation_seconds_per_video_second);
		temporal_samples = reader.unsigned_value("temporal_samples", temporal_samples);
		shutter_fraction = static_cast<float>(reader.real("shutter_fraction", shutter_fraction));
		fade_in_frames = reader.unsigned_value("fade_in_frames", fade_in_frames);
		fade_out_frames = reader.unsigned_value("fade_out_frames", fade_out_frames);
		start_frame_index = reader.wide_value("start_frame_index", start_frame_index);
		frame_name_padding = reader.unsigned_value("frame_name_padding", frame_name_padding);
		preview_in_viewport = reader.flag("preview_in_viewport", preview_in_viewport);
		restore_camera_after_capture = reader.flag("restore_camera_after_capture", restore_camera_after_capture);
		pacing = reader.enumeration("pacing", pacing, RealTimePacing::DuplicateToFill);
		realtime_queue_depth = reader.unsigned_value("realtime_queue_depth", realtime_queue_depth);
		codec = reader.enumeration("codec", codec, VideoCodecPreset::GIF);
		container = reader.enumeration("container", container, VideoContainer::GIF);
		rate_control = reader.enumeration("rate_control", rate_control, VideoRateControl::TargetBitrate);
		encoder_speed = reader.enumeration("encoder_speed", encoder_speed, VideoEncoderSpeed::Veryslow);
		crf = reader.unsigned_value("crf", crf);
		target_bitrate_kbps = reader.unsigned_value("target_bitrate_kbps", target_bitrate_kbps);
		pixel_format = reader.text("pixel_format", pixel_format);
		encode_frames_per_second = static_cast<float>(reader.real("encode_frames_per_second", encode_frames_per_second));
		loop_output = reader.flag("loop_output", loop_output);
		assemble_video_after_capture = reader.flag("assemble_video_after_capture", assemble_video_after_capture);
		delete_frames_after_assembly = reader.flag("delete_frames_after_assembly", delete_frames_after_assembly);
		ffmpeg_executable = reader.text("ffmpeg_executable", ffmpeg_executable);
	}
};

}
