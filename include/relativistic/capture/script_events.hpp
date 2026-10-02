#pragma once

#include "relativistic/capture/motion_script.hpp"
#include "relativistic/orchestrator/command.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace Relativistic::Capture {

struct EventParameterEntry {
	const char* name;
	Orchestrator::ParameterType type;
};

inline constexpr std::array<EventParameterEntry, 35> kEventParameters{{
	{"Central Mass", Orchestrator::ParameterType::Mass},
	{"Spin Parameter", Orchestrator::ParameterType::Spin},
	{"Electric Charge", Orchestrator::ParameterType::Charge},
	{"Cosmological Lambda", Orchestrator::ParameterType::CosmologicalLambda},
	{"Wormhole Throat", Orchestrator::ParameterType::WormholeThroat},
	{"Warp Bubble Velocity", Orchestrator::ParameterType::WarpVelocity},
	{"Camera Exposure", Orchestrator::ParameterType::CameraExposure},
	{"Projection Mode", Orchestrator::ParameterType::ProjectionMode},
	{"Tonemapping Mode", Orchestrator::ParameterType::TonemappingMode},
	{"Sky Star Density", Orchestrator::ParameterType::SkyStarDensity},
	{"Sky Star Brightness", Orchestrator::ParameterType::SkyStarBrightness},
	{"Sky Nebula Intensity", Orchestrator::ParameterType::SkyNebulaIntensity},
	{"Sky Grid Opacity", Orchestrator::ParameterType::SkyGridOpacity},
	{"Sky Rotation", Orchestrator::ParameterType::SkyRotation},
	{"Sky Hue Shift", Orchestrator::ParameterType::SkyHueShift},
	{"Sky Saturation", Orchestrator::ParameterType::SkySaturation},
	{"Sky Galaxy Density", Orchestrator::ParameterType::SkyGalaxyDensity},
	{"Sky Dust Density", Orchestrator::ParameterType::SkyDustDensity},
	{"Sky Cluster Density", Orchestrator::ParameterType::SkyClusterDensity},
	{"Disk Temperature Scale", Orchestrator::ParameterType::DiskTemperatureScale},
	{"Disk Temperature Floor", Orchestrator::ParameterType::DiskTemperatureFloor},
	{"Disk Doppler Beaming Exponent", Orchestrator::ParameterType::DiskDopplerBeamingExponent},
	{"Disk Color Saturation", Orchestrator::ParameterType::DiskColorSaturation},
	{"Post Contrast", Orchestrator::ParameterType::PostContrast},
	{"Post Saturation", Orchestrator::ParameterType::PostSaturation},
	{"Post Lift", Orchestrator::ParameterType::PostLift},
	{"Post Gamma", Orchestrator::ParameterType::PostGamma},
	{"Post Gain", Orchestrator::ParameterType::PostGain},
	{"Post Vignette Strength", Orchestrator::ParameterType::PostVignetteStrength},
	{"Post Highlights", Orchestrator::ParameterType::PostHighlights},
	{"Post Shadows", Orchestrator::ParameterType::PostShadows},
	{"Body Atmosphere Intensity", Orchestrator::ParameterType::BodyAtmosphereGlobalIntensity},
	{"Integration Step Factor", Orchestrator::ParameterType::IntegrationStepFactor},
	{"Far-Field Step Scale", Orchestrator::ParameterType::FarFieldStepScale},
	{"Ambient Temperature", Orchestrator::ParameterType::InteractionAmbientTemperature}
}};

[[nodiscard]] inline size_t event_parameter_index(uint32_t parameter) noexcept {
	for (size_t i = 0; i < kEventParameters.size(); ++i) {
		if (static_cast<uint32_t>(kEventParameters[i].type) == parameter) return i;
	}
	return 0;
}

[[nodiscard]] constexpr bool is_ramp_action(EventAction action) noexcept {
	return action == EventAction::SetParameter || action == EventAction::SetWarp;
}

class ScriptEventDispatcher {
private:
	struct Entry {
		double time;
		size_t index;
	};

	struct ActiveRamp {
		size_t index;
		double start;
	};

	std::vector<Entry> order_{};
	std::vector<ActiveRamp> ramps_{};
	size_t next_{0};

public:
	void reset(const MotionScript& script) {
		order_.clear();
		ramps_.clear();
		next_ = 0;
		order_.reserve(script.events.size());
		for (size_t i = 0; i < script.events.size(); ++i) {
			order_.push_back(Entry{script.resolved_event_time(script.events[i]), i});
		}
		std::stable_sort(order_.begin(), order_.end(), [](const Entry& a, const Entry& b) noexcept { return a.time < b.time; });
	}

	template <typename FireFunction, typename RampFunction>
	void advance(const MotionScript& script, double now, FireFunction&& fire, RampFunction&& ramp) {
		while (next_ < order_.size() && order_[next_].time <= now + 1.0e-9) {
			const Entry entry = order_[next_++];
			if (entry.index >= script.events.size()) continue;
			const ScriptEvent& event = script.events[entry.index];
			if (!event.enabled) continue;
			fire(event, entry.index, entry.time);
			if (is_ramp_action(event.action) && event.duration > 1.0e-9) {
				ramps_.push_back(ActiveRamp{entry.index, entry.time});
			}
		}
		for (auto it = ramps_.begin(); it != ramps_.end();) {
			const ScriptEvent& event = script.events[it->index];
			const double progress = std::clamp((now - it->start) / event.duration, 0.0, 1.0);
			ramp(event, event.value + (event.value_end - event.value) * event.easing.evaluate(progress));
			if (progress >= 1.0) {
				it = ramps_.erase(it);
			} else {
				++it;
			}
		}
	}
};

}
