#pragma once

#include <cstdint>
#include <string>

namespace Relativistic::IO {

enum class VideoContainer : uint32_t {
	MP4 = 0,
	MKV = 1,
	MOV = 2,
	WebM = 3
};

enum class VideoCodecPreset : uint32_t {
	H264 = 0,
	H265 = 1,
	VP9 = 2,
	ProRes = 3,
	PngSequence = 4
};

enum class SequenceCaptureTrigger : uint32_t {
	Manual = 0,
	FixedDuration = 1,
	FixedFrameCount = 2,
	Continuous = 3
};

struct VideoSequenceSettings {
	float frames_per_second{30.0f};
	float duration_seconds{5.0f};
	float resolution_scale{1.0f};
	VideoCodecPreset codec{VideoCodecPreset::H264};
	VideoContainer container{VideoContainer::MP4};
	uint32_t crf{18};
	std::string pixel_format{"yuv420p"};
	bool loop_output{false};
	bool pause_simulation_during_capture{false};
	SequenceCaptureTrigger trigger{SequenceCaptureTrigger::FixedDuration};

	[[nodiscard]] std::string codec_name() const noexcept {
		switch (codec) {
			case VideoCodecPreset::H264: return "libx264";
			case VideoCodecPreset::H265: return "libx265";
			case VideoCodecPreset::VP9: return "libvpx-vp9";
			case VideoCodecPreset::ProRes: return "prores_ks";
			case VideoCodecPreset::PngSequence: return "png";
			default: return "libx264";
		}
	}

	[[nodiscard]] std::string container_extension() const noexcept {
		switch (container) {
			case VideoContainer::MP4: return "mp4";
			case VideoContainer::MKV: return "mkv";
			case VideoContainer::MOV: return "mov";
			case VideoContainer::WebM: return "webm";
			default: return "mp4";
		}
	}

	[[nodiscard]] std::string build_ffmpeg_command(
		const std::string& input_directory,
		const std::string& input_extension,
		const std::string& output_path
	) const {
		std::string cmd = "ffmpeg -y -framerate " + std::to_string(static_cast<int>(frames_per_second));
		cmd += " -pattern_type glob -i \"" + input_directory + "/*." + input_extension + "\"";
		cmd += " -c:v " + codec_name();
		if (codec != VideoCodecPreset::PngSequence) {
			cmd += " -crf " + std::to_string(crf);
			cmd += " -pix_fmt " + pixel_format;
		}
		if (loop_output) {
			cmd += " -loop 0";
		}
		cmd += " \"" + output_path + "." + container_extension() + "\"";
		return cmd;
	}
};

}
