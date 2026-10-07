#pragma once

#include "relativistic/capture/motion_script_file.hpp"
#include "relativistic/io/capture/capture_settings_io.hpp"
#include "relativistic/io/capture/recording_settings.hpp"
#include "relativistic/io/capture/capture_target_settings.hpp"
#include "relativistic/io/capture/video_capture_settings.hpp"
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

namespace Relativistic::IO {

struct CaptureStudioSettings {
	CaptureTargetSettings screenshot_target{};
	CaptureTargetSettings sequence_target{};
	VideoSequenceSettings video{};
	std::string sequence_directory{"./captures"};
	std::string session_name{};
	bool use_script{false};
	bool manual_stepping{false};
	Capture::MotionScript script{};
	RecordingSettings recording{};

	[[nodiscard]] static std::filesystem::path settings_file_path() {
		return std::filesystem::path("config") / "capture_studio.cfg";
	}

	[[nodiscard]] static std::filesystem::path script_file_path() {
		return std::filesystem::path("config") / "capture_script.cfg";
	}

	[[nodiscard]] static std::filesystem::path legacy_path_file_path() {
		return std::filesystem::path("config") / "capture_path.cfg";
	}

	void sanitize() {
		video.frames_per_second = std::clamp(video.frames_per_second, 1.0f, 240.0f);
		video.duration_seconds = std::clamp(video.duration_seconds, 0.05f, 86400.0f);
		video.fixed_frame_count = std::max<uint64_t>(video.fixed_frame_count, 1ULL);
		video.resolution_scale = std::clamp(video.resolution_scale, 0.1f, 4.0f);
		video.ticks_per_frame = std::clamp<uint32_t>(video.ticks_per_frame, 0U, 100000U);
		video.simulation_seconds_per_video_second = std::clamp(video.simulation_seconds_per_video_second, 0.0, 1.0e9);
		video.temporal_samples = std::clamp<uint32_t>(video.temporal_samples, 1U, 64U);
		video.shutter_fraction = std::clamp(video.shutter_fraction, 0.0f, 1.0f);
		video.frame_name_padding = std::clamp<uint32_t>(video.frame_name_padding, 1U, 12U);
		video.realtime_queue_depth = std::clamp<uint32_t>(video.realtime_queue_depth, 1U, 64U);
		video.crf = std::min<uint32_t>(video.crf, 51U);
		video.target_bitrate_kbps = std::clamp<uint32_t>(video.target_bitrate_kbps, 100U, 2000000U);
		video.encode_frames_per_second = std::clamp(video.encode_frames_per_second, 0.0f, 240.0f);
		video.shutter_phase = std::clamp(video.shutter_phase, -1.0f, 1.0f);
		video.script_time_offset_seconds = std::clamp(video.script_time_offset_seconds, 0.0, 86400.0);
		video.script_speed = std::clamp(video.script_speed, 0.01, 100.0);
		recording.sanitize();
		script.sanitize();
	}

	[[nodiscard]] static CaptureStudioSettings load_or_default() {
		CaptureStudioSettings result{};

		std::ifstream file(settings_file_path());
		if (file.is_open()) {
			std::unordered_map<std::string, std::string> entries;
			std::string line;
			while (std::getline(file, line)) {
				if (!line.empty() && line.back() == '\r') {
					line.pop_back();
				}
				const size_t equals = line.find('=');
				if (equals == std::string::npos) {
					continue;
				}
				entries[line.substr(0, equals)] = line.substr(equals + 1);
			}
			const SettingsReader reader(entries);
			result.screenshot_target.read_settings(reader, "screenshot_");
			result.sequence_target.read_settings(reader, "sequence_");
			result.video.read_settings(reader);
			result.sequence_directory = reader.text("sequence_directory", result.sequence_directory);
			result.session_name = reader.text("session_name", result.session_name);
			result.use_script = reader.flag("use_script", reader.flag("use_camera_path", result.use_script));
			result.manual_stepping = reader.flag("manual_stepping", result.manual_stepping);
			result.recording.read_settings(reader, "recording_");
		}

		bool script_loaded = false;
		std::ifstream script_file(script_file_path());
		if (script_file.is_open()) {
			std::ostringstream buffer;
			buffer << script_file.rdbuf();
			if (auto parsed = Capture::motion_script_from_text(buffer.str()); parsed.has_value()) {
				result.script = std::move(*parsed);
				script_loaded = true;
			}
		}
		if (!script_loaded) {
			std::ifstream legacy_file(legacy_path_file_path());
			if (legacy_file.is_open()) {
				std::ostringstream buffer;
				buffer << legacy_file.rdbuf();
				if (auto parsed = Capture::CameraPath::from_text(buffer.str()); parsed.has_value()) {
					result.script = Capture::MotionScript::from_legacy_path(*parsed);
				}
			}
		}

		result.sanitize();
		return result;
	}

	void save() const {
		std::error_code ec;
		std::filesystem::create_directories(settings_file_path().parent_path(), ec);

		std::ofstream out(settings_file_path(), std::ios::trunc);
		if (out.is_open()) {
			SettingsWriter writer(out);
			screenshot_target.write_settings(writer, "screenshot_");
			sequence_target.write_settings(writer, "sequence_");
			video.write_settings(writer);
			writer.text("sequence_directory", sequence_directory);
			writer.text("session_name", session_name);
			writer.flag("use_script", use_script);
			writer.flag("manual_stepping", manual_stepping);
			recording.write_settings(writer, "recording_");
		}

		static_cast<void>(Capture::save_motion_script(script_file_path(), script));
	}
};

}
