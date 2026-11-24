#pragma once

#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <optional>
#include <algorithm>

namespace Relativistic::Orchestrator {

enum class CommandType : uint32_t {
	None = 0,
	Pause = 1,
	Resume = 2,
	Step = 3,
	Warp = 4,
	Reset = 5,
	SetParam = 6,
	SetTickRate = 7,
	Status = 8,
	Shutdown = 9,
	CameraMove = 10,
	CameraRotate = 11,
	CameraSetFov = 12,
	CameraSetSpeed = 13,
	CameraReset = 14,
	SetMetric = 15,
	LoadScenario = 16,
	SaveScenario = 17,
	SetIntegrator = 18,
	TriggerExport = 19,
	SetResolutionScale = 20,
	SetRenderSteps = 21,
	SetPerformancePreset = 22,
	SetCameraMode = 23,
	SetVisualOverlay = 24,
	CaptureScreenshot = 25,
	CameraTeleport = 26,
	CameraLookAt = 27,
	CameraSnap = 28,
	BulkBodyAction = 29,
	ModifyBody = 30,
	ResetPostProcessing = 31,
	Help = 32,
	TriggerCaptureSequence = 33,
	ConfigureCluster = 34,
	DiagnosticSnapshot = 35,
	ResetUnits = 36
};

[[nodiscard]] constexpr const char* command_type_name(CommandType type) noexcept {
	switch (type) {
		case CommandType::None: return "None";
		case CommandType::Pause: return "Pause";
		case CommandType::Resume: return "Resume";
		case CommandType::Step: return "Step";
		case CommandType::Warp: return "Warp";
		case CommandType::Reset: return "Reset";
		case CommandType::SetParam: return "SetParam";
		case CommandType::SetTickRate: return "SetTickRate";
		case CommandType::Status: return "Status";
		case CommandType::Shutdown: return "Shutdown";
		case CommandType::CameraMove: return "CameraMove";
		case CommandType::CameraRotate: return "CameraRotate";
		case CommandType::CameraSetFov: return "CameraSetFov";
		case CommandType::CameraSetSpeed: return "CameraSetSpeed";
		case CommandType::CameraReset: return "CameraReset";
		case CommandType::SetMetric: return "SetMetric";
		case CommandType::LoadScenario: return "LoadScenario";
		case CommandType::SaveScenario: return "SaveScenario";
		case CommandType::SetIntegrator: return "SetIntegrator";
		case CommandType::TriggerExport: return "TriggerExport";
		case CommandType::SetResolutionScale: return "SetResolutionScale";
		case CommandType::SetRenderSteps: return "SetRenderSteps";
		case CommandType::SetPerformancePreset: return "SetPerformancePreset";
		case CommandType::SetCameraMode: return "SetCameraMode";
		case CommandType::SetVisualOverlay: return "SetVisualOverlay";
		case CommandType::CaptureScreenshot: return "CaptureScreenshot";
		case CommandType::CameraTeleport: return "CameraTeleport";
		case CommandType::CameraLookAt: return "CameraLookAt";
		case CommandType::CameraSnap: return "CameraSnap";
		case CommandType::BulkBodyAction: return "BulkBodyAction";
		case CommandType::ModifyBody: return "ModifyBody";
		case CommandType::ResetPostProcessing: return "ResetPostProcessing";
		case CommandType::Help: return "Help";
		case CommandType::TriggerCaptureSequence: return "TriggerCaptureSequence";
		case CommandType::ConfigureCluster: return "ConfigureCluster";
		case CommandType::DiagnosticSnapshot: return "DiagnosticSnapshot";
		case CommandType::ResetUnits: return "ResetUnits";
	}
	return "Unknown";
}

enum class ParameterType : uint32_t {
	Unknown = 0,
	Mass = 1,
	Spin = 2,
	Charge = 3,
	CosmologicalLambda = 4,
	WormholeThroat = 5,
	WarpVelocity = 6,
	TickRate = 7,
	ProjectionMode = 8,
	TimeFlowMode = 9,
	CameraSpeed = 10,
	CameraFov = 11,
	CameraExposure = 12,
	TonemappingMode = 13,
	IntegrationRtol = 14,
	IntegrationAtol = 15,
	IntegrationMinStep = 16,
	IntegrationMaxStep = 17,
	ResolutionScale = 18,
	MaxRaySteps = 19,
	PerformancePreset = 20,
	CameraMode = 21,
	VisualOverlays = 22,
	Custom = 23,
	SkyStarDensity = 24,
	SkyStarBrightness = 25,
	SkyNebulaIntensity = 26,
	SkyGridOpacity = 27,
	SkyRotation = 28,
	SkyHueShift = 29,
	SkySaturation = 30,
	SkyBackgroundR = 31,
	SkyBackgroundG = 32,
	SkyBackgroundB = 33,
	WorkDistributionMode = 34,
	ForceTextureReallocation = 35,
	RollingAverageFrameCount = 36,
	RenderDistanceScale = 37,
	LodEnabled = 38,
	LodDistanceThreshold = 39,
	LodReducedSteps = 40,
	UseGpuCompute = 41,
	MotionQualityMode = 42,
	MotionQualityScale = 43,
	StepControllerMode = 44,
	SpaceSkippingEnabled = 45,
	SpaceSkipRadiusScale = 46,
	SchematicModeEnabled = 47,
	SchematicAllowSimulation = 48,
	PoleGuardPrecisionScale = 49,
	PostContrast = 50,
	PostSaturation = 51,
	PostLift = 52,
	PostGamma = 53,
	PostGain = 54,
	PostVignetteStrength = 55,
	PostHighlights = 56,
	PostShadows = 57,
	ConstantsPresetSelect = 58,
	ConstantSimC = 59,
	ConstantSimG = 60,
	ConstantSimH = 61,
	ConstantSimKB = 62,
	ConstantSimNA = 63,
	ConstantSimKe = 64,
	ConstantSimKcd = 65,
	InteractionElectricityEnabled = 66,
	InteractionMagnetismEnabled = 67,
	InteractionVacuumPermittivity = 68,
	InteractionVacuumPermeability = 69,
	InteractionCollisionsEnabled = 70,
	InteractionCollisionResponseModel = 71,
	InteractionCollisionConsiderRotation = 72,
	InteractionCollisionConsiderFriction = 73,
	InteractionCollisionRestitutionMultiplier = 74,
	InteractionThermodynamicsEnabled = 75,
	InteractionAmbientTemperature = 76,
	InteractionRadiativeCouplingScale = 77,
	InteractionFragmentationEnabled = 78,
	InteractionMinimumFragmentMass = 79,
	InteractionFragmentationMaxFragments = 80,
	InteractionCollisionEnergyToIntegrityLoss = 81,
	InteractionAnnihilationEnabled = 82,
	InteractionAnnihilationContactScale = 83,
	InteractionAnnihilationRequireOppositeCharge = 84,
	InteractionCollisionStiffnessScale = 85,
	InteractionCollisionPositionCorrectionFactor = 86,
	InteractionFragmentationTidalStressEnabled = 87,
	InteractionFragmentationTidalStressToIntegrityLoss = 88,
	FarFieldStepScale = 89,
	InterlaceRenderingEnabled = 90,
	DynamicResolutionEnabled = 91,
	DynamicResolutionTargetFps = 92,
	AdaptiveTilePrepassEnabled = 93,
	BodyRenderLodPixelThreshold = 94,
	BodyShadowsEnabled = 95,
	BodyAtmosphereGlobalIntensity = 96,
	BodyRenderLowPowerMode = 97,
	BodiesOnlyRenderMode = 98,
	BodyDiskOcclusionEnabled = 99,
	BodyRenderPointPixelThreshold = 100,
	BodyNoiseOctaves = 101,
	SkyStarBrightnessVariation = 102,
	SkyStarSizeVariation = 103,
	SkyStarColorVariation = 104,
	SkyStarTemperatureBias = 105,
	SkyProceduralSeed = 106,
	SkyGalaxyDensity = 107,
	SkyGalaxyBrightness = 108,
	SkyGalaxySizeScale = 109,
	SkyDustDensity = 110,
	SkyDustIntensity = 111,
	SkyDustScale = 112,
	SkyClusterDensity = 113,
	SkyClusterBrightness = 114,
	SkyClusterSizeScale = 115,
	DiskTemperatureScale = 116,
	DiskTemperatureFloor = 117,
	DiskDopplerBeamingExponent = 118,
	DiskColorSaturation = 119,
	SkyBackgroundSource = 120,
	SkyPanoramaId = 121,
	SkyPanoramaQuality = 122,
	IntegrationStepFactor = 123,
	LightSourceMode = 124,
	LightAttenuationMode = 125,
	LightSourceBodyId = 126,
	LightIntensity = 127,
	LightColorR = 128,
	LightColorG = 129,
	LightColorB = 130,
	LightAmbient = 131,
	LightTerminatorSoftness = 132,
	LightSpecularScale = 133,
	LightDirectionAzimuth = 134,
	LightDirectionElevation = 135,
	LightPositionX = 136,
	LightPositionY = 137,
	LightPositionZ = 138,
	LightReferenceDistance = 139,
	BodyEmissionLightingEnabled = 140,
	BodyEmissionLightingGain = 141,
	BodyEmissionLightingReferenceDistance = 142,
	CameraCollisionEnabled = 143,
	CameraCollisionClearance = 144,
	CameraMotionLiveRefresh = 145,
	WarpBubbleRadius = 146,
	WarpWallSharpness = 147,
	WormholeTidalPotential = 148,
	TevesCriticalAcceleration = 149,
	JetEnabled = 150,
	JetLength = 151,
	JetOpeningAngle = 152,
	JetLorentzFactor = 153,
	JetMagneticField = 154,
	JetPrecessionAngle = 155,
	DiskInnerRadius = 156,
	DiskOuterRadius = 157,
	DarkMatterEnabled = 158,
	DarkMatterLensing = 159,
	DarkMatterVisualization = 160,
	DarkMatterVizMode = 161,
	DarkMatterAffectsBodies = 162,
	CosmologyVoidRadius = 163,
	CosmologyVoidWidth = 164,
	CosmologyHubble0 = 165,
	CosmologyEvaporationEnabled = 166,
	CosmologyEvaporationTimeAccel = 167,
	SurfaceWalkTargetBodyId = 168,
	SkyStarsEnabled = 169,
	SkyGalaxiesEnabled = 170,
	SkyDustEnabled = 171,
	SkyClustersEnabled = 172,
	UnitDistanceSelect = 173,
	UnitVelocitySelect = 174,
	UnitMassSelect = 175,
	UnitEnergySelect = 176,
	UnitTemperatureSelect = 177,
	UnitAngleSelect = 178,
	UnitChargeSelect = 179,
	UnitCurrentSelect = 180,
	UnitTimeSelect = 181,
	UnitAccelerationSelect = 182,
	UnitDensitySelect = 183,
	UnitPressureSelect = 184,
	UnitPowerSelect = 185,
	UnitFrequencySelect = 186,
	UnitForceSelect = 187,
	UnitMagneticSelect = 188,
	UnitVoltageSelect = 189,
	UnitFrameRateSelect = 190,
	CoordDefaultSelect = 191
};

struct Command {
	CommandType type{CommandType::None};
	ParameterType param_type{ParameterType::Unknown};
	uint64_t step_count{0};
	double numeric_value{0.0};
	double vec_values[4]{0.0, 0.0, 0.0, 0.0};
	char custom_param_name[64]{};
	char text_payload[256]{};
	uint64_t target_tick{0};

	[[nodiscard]] static constexpr Command make_pause() noexcept {
		Command cmd{};
		cmd.type = CommandType::Pause;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_resume() noexcept {
		Command cmd{};
		cmd.type = CommandType::Resume;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_step(uint64_t n = 1) noexcept {
		Command cmd{};
		cmd.type = CommandType::Step;
		cmd.step_count = n;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_warp(double factor) noexcept {
		Command cmd{};
		cmd.type = CommandType::Warp;
		cmd.numeric_value = factor;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_reset() noexcept {
		Command cmd{};
		cmd.type = CommandType::Reset;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_set_param(ParameterType param, double val) noexcept {
		Command cmd{};
		cmd.type = CommandType::SetParam;
		cmd.param_type = param;
		cmd.numeric_value = val;
		return cmd;
	}

	[[nodiscard]] static Command make_set_custom_param(std::string_view name, double val) noexcept {
		Command cmd{};
		cmd.type = CommandType::SetParam;
		cmd.param_type = ParameterType::Custom;
		cmd.numeric_value = val;
		const size_t len = std::min(name.size(), sizeof(cmd.custom_param_name) - 1);
		std::memcpy(cmd.custom_param_name, name.data(), len);
		cmd.custom_param_name[len] = '\0';
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_set_tickrate(double rate_hz) noexcept {
		Command cmd{};
		cmd.type = CommandType::SetTickRate;
		cmd.numeric_value = rate_hz;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_status() noexcept {
		Command cmd{};
		cmd.type = CommandType::Status;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_shutdown() noexcept {
		Command cmd{};
		cmd.type = CommandType::Shutdown;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_camera_move(double dx, double dy, double dz) noexcept {
		Command cmd{};
		cmd.type = CommandType::CameraMove;
		cmd.vec_values[0] = dx;
		cmd.vec_values[1] = dy;
		cmd.vec_values[2] = dz;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_camera_rotate(double pitch, double yaw, double roll = 0.0) noexcept {
		Command cmd{};
		cmd.type = CommandType::CameraRotate;
		cmd.vec_values[0] = pitch;
		cmd.vec_values[1] = yaw;
		cmd.vec_values[2] = roll;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_camera_set_fov(double fov_deg) noexcept {
		Command cmd{};
		cmd.type = CommandType::CameraSetFov;
		cmd.numeric_value = fov_deg;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_camera_set_speed(double speed) noexcept {
		Command cmd{};
		cmd.type = CommandType::CameraSetSpeed;
		cmd.numeric_value = speed;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_camera_reset() noexcept {
		Command cmd{};
		cmd.type = CommandType::CameraReset;
		return cmd;
	}

	[[nodiscard]] static Command make_set_metric(std::string_view metric_name) noexcept {
		Command cmd{};
		cmd.type = CommandType::SetMetric;
		const size_t len = std::min(metric_name.size(), sizeof(cmd.text_payload) - 1);
		std::memcpy(cmd.text_payload, metric_name.data(), len);
		cmd.text_payload[len] = '\0';
		return cmd;
	}

	[[nodiscard]] static Command make_load_scenario(std::string_view scenario_path) noexcept {
		Command cmd{};
		cmd.type = CommandType::LoadScenario;
		const size_t len = std::min(scenario_path.size(), sizeof(cmd.text_payload) - 1);
		std::memcpy(cmd.text_payload, scenario_path.data(), len);
		cmd.text_payload[len] = '\0';
		return cmd;
	}

	[[nodiscard]] static Command make_save_scenario(std::string_view scenario_path) noexcept {
		Command cmd{};
		cmd.type = CommandType::SaveScenario;
		const size_t len = std::min(scenario_path.size(), sizeof(cmd.text_payload) - 1);
		std::memcpy(cmd.text_payload, scenario_path.data(), len);
		cmd.text_payload[len] = '\0';
		return cmd;
	}

	[[nodiscard]] static Command make_set_integrator(std::string_view scheme_name) noexcept {
		Command cmd{};
		cmd.type = CommandType::SetIntegrator;
		const size_t len = std::min(scheme_name.size(), sizeof(cmd.text_payload) - 1);
		std::memcpy(cmd.text_payload, scheme_name.data(), len);
		cmd.text_payload[len] = '\0';
		return cmd;
	}

	[[nodiscard]] static Command make_trigger_export(std::string_view format_name) noexcept {
		Command cmd{};
		cmd.type = CommandType::TriggerExport;
		const size_t len = std::min(format_name.size(), sizeof(cmd.text_payload) - 1);
		std::memcpy(cmd.text_payload, format_name.data(), len);
		cmd.text_payload[len] = '\0';
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_set_resolution_scale(double scale) noexcept {
		Command cmd{};
		cmd.type = CommandType::SetResolutionScale;
		cmd.numeric_value = scale;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_set_render_steps(uint64_t steps) noexcept {
		Command cmd{};
		cmd.type = CommandType::SetRenderSteps;
		cmd.step_count = steps;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_set_performance_preset(uint32_t preset) noexcept {
		Command cmd{};
		cmd.type = CommandType::SetPerformancePreset;
		cmd.step_count = preset;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_set_camera_mode(uint32_t mode) noexcept {
		Command cmd{};
		cmd.type = CommandType::SetCameraMode;
		cmd.step_count = mode;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_set_visual_overlay(uint32_t overlay_flag, bool enable) noexcept {
		Command cmd{};
		cmd.type = CommandType::SetVisualOverlay;
		cmd.step_count = overlay_flag;
		cmd.numeric_value = enable ? 1.0 : 0.0;
		return cmd;
	}

	[[nodiscard]] static Command make_capture_screenshot(std::string_view filename = {}) noexcept {
		Command cmd{};
		cmd.type = CommandType::CaptureScreenshot;
		if (!filename.empty()) {
			const size_t len = std::min(filename.size(), sizeof(cmd.text_payload) - 1);
			std::memcpy(cmd.text_payload, filename.data(), len);
			cmd.text_payload[len] = '\0';
		}
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_camera_teleport(double x, double y, double z) noexcept {
		Command cmd{};
		cmd.type = CommandType::CameraTeleport;
		cmd.vec_values[0] = x;
		cmd.vec_values[1] = y;
		cmd.vec_values[2] = z;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_camera_lookat(double x, double y, double z) noexcept {
		Command cmd{};
		cmd.type = CommandType::CameraLookAt;
		cmd.vec_values[0] = x;
		cmd.vec_values[1] = y;
		cmd.vec_values[2] = z;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_camera_snap(uint32_t snap_mode, double distance = 0.0) noexcept {
		Command cmd{};
		cmd.type = CommandType::CameraSnap;
		cmd.step_count = snap_mode;
		cmd.numeric_value = distance;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_bulk_body_action(uint32_t action_kind, double param = 0.0) noexcept {
		Command cmd{};
		cmd.type = CommandType::BulkBodyAction;
		cmd.step_count = action_kind;
		cmd.numeric_value = param;
		return cmd;
	}

	[[nodiscard]] static Command make_add_body(double mass, double radius, double x, double y, double z, std::string_view name = {}) noexcept {
		Command cmd{};
		cmd.type = CommandType::ModifyBody;
		cmd.step_count = 0;
		cmd.numeric_value = mass;
		cmd.vec_values[0] = x;
		cmd.vec_values[1] = y;
		cmd.vec_values[2] = z;
		cmd.vec_values[3] = radius;
		if (!name.empty()) {
			const size_t len = std::min(name.size(), sizeof(cmd.text_payload) - 1);
			std::memcpy(cmd.text_payload, name.data(), len);
			cmd.text_payload[len] = '\0';
		}
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_post_reset() noexcept {
		Command cmd{};
		cmd.type = CommandType::ResetPostProcessing;
		return cmd;
	}

	[[nodiscard]] static Command make_help(std::string_view topic = {}, uint64_t page = 1) noexcept {
		Command cmd{};
		cmd.type = CommandType::Help;
		cmd.step_count = (page > 0) ? page : 1;
		if (!topic.empty()) {
			const size_t len = std::min(topic.size(), sizeof(cmd.text_payload) - 1);
			std::memcpy(cmd.text_payload, topic.data(), len);
			cmd.text_payload[len] = '\0';
		}
		return cmd;
	}

	[[nodiscard]] static Command make_diagnostic_snapshot(std::string_view reason = {}) noexcept {
		Command cmd{};
		cmd.type = CommandType::DiagnosticSnapshot;
		if (!reason.empty()) {
			const size_t len = std::min(reason.size(), sizeof(cmd.text_payload) - 1);
			std::memcpy(cmd.text_payload, reason.data(), len);
			cmd.text_payload[len] = '\0';
		}
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_reset_units() noexcept {
		Command cmd{};
		cmd.type = CommandType::ResetUnits;
		return cmd;
	}

	[[nodiscard]] static constexpr Command make_trigger_capture_sequence(bool start) noexcept {
		Command cmd{};
		cmd.type = CommandType::TriggerCaptureSequence;
		cmd.numeric_value = start ? 1.0 : 0.0;
		return cmd;
	}
};

struct CommandResult {
	bool success{false};
	char message[128]{};
};

class CommandParser {
public:
	[[nodiscard]] static std::optional<Command> parse(std::string_view line, CommandResult* result_out = nullptr) noexcept {
		auto trim_sv = [](std::string_view s) noexcept -> std::string_view {
			while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n')) {
				s.remove_prefix(1);
			}
			while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n')) {
				s.remove_suffix(1);
			}
			return s;
		};

		auto iequals_sv = [](std::string_view a, std::string_view b) noexcept -> bool {
			if (a.size() != b.size()) return false;
			for (size_t i = 0; i < a.size(); ++i) {
				const char ca = (a[i] >= 'A' && a[i] <= 'Z') ? static_cast<char>(a[i] + ('a' - 'A')) : a[i];
				const char cb = (b[i] >= 'A' && b[i] <= 'Z') ? static_cast<char>(b[i] + ('a' - 'A')) : b[i];
				if (ca != cb) return false;
			}
			return true;
		};

		auto parse_u64 = [](std::string_view s, uint64_t& out) noexcept -> bool {
			if (s.empty()) return false;
			uint64_t v = 0;
			for (char c : s) {
				if (c < '0' || c > '9') return false;
				v = v * 10 + static_cast<uint64_t>(c - '0');
			}
			out = v;
			return true;
		};

		auto parse_dbl = [](std::string_view s, double& out) noexcept -> bool {
			if (s.empty()) return false;
			char buf[64];
			if (s.size() >= sizeof(buf)) return false;
			std::memcpy(buf, s.data(), s.size());
			buf[s.size()] = '\0';
			char* end_ptr = nullptr;
			const double v = std::strtod(buf, &end_ptr);
			if (end_ptr != buf + s.size()) return false;
			out = v;
			return true;
		};

		auto set_msg = [](CommandResult* r, bool s, std::string_view m) noexcept {
			if (!r) return;
			r->success = s;
			const size_t len = std::min(m.size(), sizeof(r->message) - 1);
			std::memcpy(r->message, m.data(), len);
			r->message[len] = '\0';
		};

		std::string_view remaining = trim_sv(line);
		if (remaining.empty()) {
			set_msg(result_out, false, "Empty command line");
			return std::nullopt;
		}

		auto extract_token = [&](std::string_view& rem) noexcept -> std::string_view {
			rem = trim_sv(rem);
			if (rem.empty()) return {};
			size_t end = 0;
			while (end < rem.size() && rem[end] != ' ' && rem[end] != '\t' && rem[end] != '\r' && rem[end] != '\n') {
				++end;
			}
			const std::string_view token = rem.substr(0, end);
			rem = trim_sv(rem.substr(end));
			return token;
		};

		const std::string_view token1 = extract_token(remaining);

		if (iequals_sv(token1, "help") || token1 == "?" || iequals_sv(token1, "topics") || iequals_sv(token1, "pages")) {
			std::string_view query = (iequals_sv(token1, "topics") || iequals_sv(token1, "pages")) ? std::string_view("topics") : extract_token(remaining);
			uint64_t page = 1;
			if (parse_u64(query, page)) {
				set_msg(result_out, true, "Help page requested");
				return Command::make_help({}, page);
			}
			set_msg(result_out, true, "Help topic requested");
			return Command::make_help(query, 1);
		}

		if (iequals_sv(token1, "quicksave")) {
			set_msg(result_out, true, "Quick-saving simulation state");
			return Command::make_save_scenario("scenarios/quicksave.yaml");
		}

		if (iequals_sv(token1, "quickload")) {
			set_msg(result_out, true, "Quick-loading simulation state");
			return Command::make_load_scenario("scenarios/quicksave.yaml");
		}

		if (iequals_sv(token1, "snapshot")) {
			const std::string_view reason = remaining;
			set_msg(result_out, true, "Diagnostic snapshot requested");
			return Command::make_diagnostic_snapshot(reason);
		}

		if (iequals_sv(token1, "pause")) {
			set_msg(result_out, true, "Simulation paused");
			return Command::make_pause();
		}

		if (iequals_sv(token1, "resume")) {
			set_msg(result_out, true, "Simulation resumed");
			return Command::make_resume();
		}

		if (iequals_sv(token1, "step")) {
			const std::string_view token2 = extract_token(remaining);
			uint64_t n = 1;
			if (!token2.empty()) {
				if (!parse_u64(token2, n) || n == 0) {
					set_msg(result_out, false, "Invalid step count argument");
					return std::nullopt;
				}
			}
			set_msg(result_out, true, "Step command queued");
			return Command::make_step(n);
		}

		if (iequals_sv(token1, "warp")) {
			const std::string_view token2 = extract_token(remaining);
			if (token2.empty()) {
				set_msg(result_out, false, "Missing warp factor argument");
				return std::nullopt;
			}
			double factor = 0.0;
			if (!parse_dbl(token2, factor) || factor <= 0.0) {
				set_msg(result_out, false, "Invalid warp factor");
				return std::nullopt;
			}
			set_msg(result_out, true, "Warp factor updated");
			return Command::make_warp(factor);
		}

		if (iequals_sv(token1, "reset")) {
			set_msg(result_out, true, "Simulation reset");
			return Command::make_reset();
		}

		if (iequals_sv(token1, "tickrate")) {
			const std::string_view token2 = extract_token(remaining);
			if (token2.empty()) {
				set_msg(result_out, false, "Missing tickrate argument");
				return std::nullopt;
			}
			double rate = 0.0;
			if (!parse_dbl(token2, rate) || rate < 10.0 || rate > 1000.0) {
				set_msg(result_out, false, "Invalid tickrate");
				return std::nullopt;
			}
			set_msg(result_out, true, "Tick rate updated");
			return Command::make_set_tickrate(rate);
		}

		if (iequals_sv(token1, "status")) {
			set_msg(result_out, true, "Status query accepted");
			return Command::make_status();
		}

		if (iequals_sv(token1, "shutdown") || iequals_sv(token1, "quit") || iequals_sv(token1, "exit")) {
			set_msg(result_out, true, "Shutdown command registered");
			return Command::make_shutdown();
		}

		if (iequals_sv(token1, "camera")) {
			const std::string_view sub = extract_token(remaining);
			if (iequals_sv(sub, "reset")) {
				set_msg(result_out, true, "Camera reset");
				return Command::make_camera_reset();
			}
			if (iequals_sv(sub, "fov")) {
				const std::string_view val_tok = extract_token(remaining);
				double fov_val = 0.0;
				if (!parse_dbl(val_tok, fov_val) || fov_val < 5.0 || fov_val > 175.0) {
					set_msg(result_out, false, "Invalid FOV (range: 5 to 175 degrees)");
					return std::nullopt;
				}
				set_msg(result_out, true, "Camera FOV set");
				return Command::make_camera_set_fov(fov_val);
			}
			if (iequals_sv(sub, "speed")) {
				const std::string_view val_tok = extract_token(remaining);
				double spd_val = 0.0;
				if (!parse_dbl(val_tok, spd_val) || spd_val <= 0.0) {
					set_msg(result_out, false, "Invalid camera speed");
					return std::nullopt;
				}
				set_msg(result_out, true, "Camera speed set");
				return Command::make_camera_set_speed(spd_val);
			}
			if (iequals_sv(sub, "move")) {
				double dx = 0.0, dy = 0.0, dz = 0.0;
				const auto tx = extract_token(remaining);
				const auto ty = extract_token(remaining);
				const auto tz = extract_token(remaining);
				if (!parse_dbl(tx, dx) || !parse_dbl(ty, dy) || !parse_dbl(tz, dz)) {
					set_msg(result_out, false, "Invalid move delta (expected dx dy dz)");
					return std::nullopt;
				}
				set_msg(result_out, true, "Camera translation applied");
				return Command::make_camera_move(dx, dy, dz);
			}
			if (iequals_sv(sub, "rotate")) {
				double dp = 0.0, dy = 0.0, dr = 0.0;
				const auto tp = extract_token(remaining);
				const auto ty = extract_token(remaining);
				const auto tr = extract_token(remaining);
				if (!parse_dbl(tp, dp) || !parse_dbl(ty, dy)) {
					set_msg(result_out, false, "Invalid rotation delta (expected pitch yaw [roll])");
					return std::nullopt;
				}
				if (!tr.empty()) static_cast<void>(parse_dbl(tr, dr));
				set_msg(result_out, true, "Camera rotation applied");
				return Command::make_camera_rotate(dp, dy, dr);
			}
			if (iequals_sv(sub, "teleport") || iequals_sv(sub, "pos") || iequals_sv(sub, "position")) {
				double px = 0.0, py = 0.0, pz = 0.0;
				const auto tx = extract_token(remaining);
				const auto ty = extract_token(remaining);
				const auto tz = extract_token(remaining);
				if (!parse_dbl(tx, px) || !parse_dbl(ty, py) || !parse_dbl(tz, pz)) {
					set_msg(result_out, false, "Invalid position (expected x y z)");
					return std::nullopt;
				}
				set_msg(result_out, true, "Camera teleport queued");
				return Command::make_camera_teleport(px, py, pz);
			}
			if (iequals_sv(sub, "lookat") || iequals_sv(sub, "look_at") || iequals_sv(sub, "aim")) {
				double tx = 0.0, ty = 0.0, tz = 0.0;
				const auto sx = extract_token(remaining);
				const auto sy = extract_token(remaining);
				const auto sz = extract_token(remaining);
				if (!parse_dbl(sx, tx) || !parse_dbl(sy, ty) || !parse_dbl(sz, tz)) {
					set_msg(result_out, false, "Invalid target (expected x y z)");
					return std::nullopt;
				}
				set_msg(result_out, true, "Camera orientation toward target queued");
				return Command::make_camera_lookat(tx, ty, tz);
			}
			if (iequals_sv(sub, "snap")) {
				const auto mode_token = extract_token(remaining);
				if (iequals_sv(mode_token, "isco")) {
					set_msg(result_out, true, "Camera snap to ISCO orbit queued");
					return Command::make_camera_snap(0);
				}
				double dist = 0.0;
				const auto dist_token = extract_token(remaining);
				if (!dist_token.empty()) static_cast<void>(parse_dbl(dist_token, dist));
				set_msg(result_out, true, "Camera snap to equatorial view queued");
				return Command::make_camera_snap(1, dist);
			}
			if (iequals_sv(sub, "mode")) {
				const auto mode_tok = extract_token(remaining);
				uint64_t mode_val = 0;
				if (iequals_sv(mode_tok, "free") || iequals_sv(mode_tok, "fly") || iequals_sv(mode_tok, "6dof")) mode_val = 0;
				else if (iequals_sv(mode_tok, "orbit")) mode_val = 1;
				else if (iequals_sv(mode_tok, "spherical")) mode_val = 2;
				else if (iequals_sv(mode_tok, "rocket")) mode_val = 3;
				else if (iequals_sv(mode_tok, "walk")) mode_val = 4;
				else if (iequals_sv(mode_tok, "planet")) mode_val = 5;
				else if (!parse_u64(mode_tok, mode_val)) {
					set_msg(result_out, false, "Invalid camera mode (expected 0..5 or name)");
					return std::nullopt;
				}
				set_msg(result_out, true, "Camera mode updated");
				return Command::make_set_camera_mode(static_cast<uint32_t>(mode_val));
			}
			if (iequals_sv(sub, "collision")) {
				const auto state_tok = extract_token(remaining);
				const bool en = iequals_sv(state_tok, "on") || iequals_sv(state_tok, "1") || iequals_sv(state_tok, "true");
				set_msg(result_out, true, "Camera collision updated");
				return Command::make_set_param(ParameterType::CameraCollisionEnabled, en ? 1.0 : 0.0);
			}
			if (iequals_sv(sub, "clearance")) {
				const auto val_tok = extract_token(remaining);
				double v = 0.0;
				if (!parse_dbl(val_tok, v) || v < 0.0) {
					set_msg(result_out, false, "Invalid clearance value");
					return std::nullopt;
				}
				set_msg(result_out, true, "Camera collision clearance updated");
				return Command::make_set_param(ParameterType::CameraCollisionClearance, v);
			}
			if (iequals_sv(sub, "motion_refresh")) {
				const auto state_tok = extract_token(remaining);
				const bool en = iequals_sv(state_tok, "on") || iequals_sv(state_tok, "1") || iequals_sv(state_tok, "true");
				set_msg(result_out, true, "Camera motion live refresh updated");
				return Command::make_set_param(ParameterType::CameraMotionLiveRefresh, en ? 1.0 : 0.0);
			}
			if (iequals_sv(sub, "walk")) {
				const auto wsub = extract_token(remaining);
				if (iequals_sv(wsub, "start")) {
					set_msg(result_out, true, "Entering surface walk mode");
					return Command::make_set_camera_mode(4);
				}
				if (iequals_sv(wsub, "stop")) {
					set_msg(result_out, true, "Exiting surface walk mode");
					return Command::make_set_camera_mode(0);
				}
				if (iequals_sv(wsub, "target")) {
					const auto id_tok = extract_token(remaining);
					uint64_t tid = 0;
					if (!parse_u64(id_tok, tid)) { set_msg(result_out, false, "Invalid walk target ID"); return std::nullopt; }
					set_msg(result_out, true, "Walk target body set");
					return Command::make_set_param(ParameterType::SurfaceWalkTargetBodyId, static_cast<double>(tid));
				}
			}
			if (iequals_sv(sub, "planet")) {
				set_msg(result_out, true, "Switching to planet orbit navigation mode");
				return Command::make_set_camera_mode(5);
			}
		}

		if (iequals_sv(token1, "metric")) {
			const std::string_view name = extract_token(remaining);
			if (name.empty()) {
				set_msg(result_out, false, "Missing metric name");
				return std::nullopt;
			}
			set_msg(result_out, true, "Metric changed");
			return Command::make_set_metric(name);
		}

		if (iequals_sv(token1, "load")) {
			const std::string_view path = remaining;
			if (path.empty()) {
				set_msg(result_out, false, "Missing scenario path");
				return std::nullopt;
			}
			set_msg(result_out, true, "Loading scenario");
			return Command::make_load_scenario(path);
		}

		if (iequals_sv(token1, "save")) {
			const std::string_view path = remaining;
			if (path.empty()) {
				set_msg(result_out, false, "Missing scenario save path");
				return std::nullopt;
			}
			set_msg(result_out, true, "Saving scenario");
			return Command::make_save_scenario(path);
		}

		if (iequals_sv(token1, "integrator")) {
			const std::string_view name = extract_token(remaining);
			if (name.empty()) {
				set_msg(result_out, false, "Missing integrator name");
				return std::nullopt;
			}
			set_msg(result_out, true, "Integrator updated");
			return Command::make_set_integrator(name);
		}

		if (iequals_sv(token1, "export")) {
			const std::string_view fmt = extract_token(remaining);
			set_msg(result_out, true, "Export triggered");
			return Command::make_trigger_export(fmt.empty() ? "all" : fmt);
		}

		if (iequals_sv(token1, "body") || iequals_sv(token1, "bodies")) {
			const std::string_view action = extract_token(remaining);
			if (iequals_sv(action, "invert") || iequals_sv(action, "invert_velocities") || iequals_sv(action, "invert_vel")) {
				set_msg(result_out, true, "Inverting all body velocities");
				return Command::make_bulk_body_action(0);
			}
			if (iequals_sv(action, "scatter")) {
				set_msg(result_out, true, "Scattering body positions");
				return Command::make_bulk_body_action(1);
			}
			if (iequals_sv(action, "snap") || iequals_sv(action, "snap_grid")) {
				double cell = 1.0;
				const auto ctok = extract_token(remaining);
				if (!ctok.empty()) static_cast<void>(parse_dbl(ctok, cell));
				set_msg(result_out, true, "Snapping bodies to grid");
				return Command::make_bulk_body_action(2, cell);
			}
			if (iequals_sv(action, "cull")) {
				double radius = 1000.0;
				const auto rtok = extract_token(remaining);
				if (!rtok.empty()) static_cast<void>(parse_dbl(rtok, radius));
				set_msg(result_out, true, "Culling bodies outside radius");
				return Command::make_bulk_body_action(3, radius);
			}
			if (iequals_sv(action, "equalize") || iequals_sv(action, "equalize_masses")) {
				set_msg(result_out, true, "Equalizing body masses");
				return Command::make_bulk_body_action(4);
			}
			if (iequals_sv(action, "average") || iequals_sv(action, "average_masses")) {
				set_msg(result_out, true, "Averaging body masses");
				return Command::make_bulk_body_action(5);
			}
			if (iequals_sv(action, "zero_spin") || iequals_sv(action, "zero_spins")) {
				set_msg(result_out, true, "Zeroing all body spin angular momenta");
				return Command::make_bulk_body_action(6);
			}
			if (iequals_sv(action, "clear")) {
				set_msg(result_out, true, "Clearing all bodies from catalog");
				return Command::make_bulk_body_action(7);
			}
			if (iequals_sv(action, "remove") || iequals_sv(action, "del") || iequals_sv(action, "delete")) {
				const auto id_tok = extract_token(remaining);
				uint64_t body_id = 0;
				if (!parse_u64(id_tok, body_id)) {
					set_msg(result_out, false, "Invalid body id to remove");
					return std::nullopt;
				}
				set_msg(result_out, true, "Removing body");
				return Command::make_bulk_body_action(8, static_cast<double>(body_id));
			}
			if (iequals_sv(action, "add")) {
				double b_mass = 1.0, b_radius = 1.0, bx = 0.0, by = 0.0, bz = 0.0;
				const auto tm = extract_token(remaining);
				const auto tr = extract_token(remaining);
				const auto tx = extract_token(remaining);
				const auto ty = extract_token(remaining);
				const auto tz = extract_token(remaining);
				if (!parse_dbl(tm, b_mass) || !parse_dbl(tr, b_radius) || !parse_dbl(tx, bx) || !parse_dbl(ty, by) || !parse_dbl(tz, bz)) {
					set_msg(result_out, false, "Invalid body add parameters (expected mass radius x y z [name])");
					return std::nullopt;
				}
				const auto name_tok = extract_token(remaining);
				set_msg(result_out, true, "Adding body to catalog");
				return Command::make_add_body(b_mass, b_radius, bx, by, bz, name_tok);
			}
		}

		if (iequals_sv(token1, "post") || iequals_sv(token1, "grade") || iequals_sv(token1, "color_grading")) {
			const std::string_view sub = extract_token(remaining);
			if (iequals_sv(sub, "reset")) {
				set_msg(result_out, true, "Color grading reset");
				return Command::make_post_reset();
			}
			const auto val_tok = extract_token(remaining);
			double val = 0.0;
			if (!parse_dbl(val_tok, val)) {
				set_msg(result_out, false, "Invalid post-processing parameter value");
				return std::nullopt;
			}
			ParameterType pt = ParameterType::Custom;
			if (iequals_sv(sub, "contrast")) pt = ParameterType::PostContrast;
			else if (iequals_sv(sub, "saturation")) pt = ParameterType::PostSaturation;
			else if (iequals_sv(sub, "lift")) pt = ParameterType::PostLift;
			else if (iequals_sv(sub, "gamma")) pt = ParameterType::PostGamma;
			else if (iequals_sv(sub, "gain")) pt = ParameterType::PostGain;
			else if (iequals_sv(sub, "vignette")) pt = ParameterType::PostVignetteStrength;
			else if (iequals_sv(sub, "highlights")) pt = ParameterType::PostHighlights;
			else if (iequals_sv(sub, "shadows")) pt = ParameterType::PostShadows;
			if (pt != ParameterType::Custom) {
				set_msg(result_out, true, "Post-processing parameter updated");
				return Command::make_set_param(pt, val);
			}
		}

		if (iequals_sv(token1, "light")) {
			const std::string_view sub = extract_token(remaining);
			if (iequals_sv(sub, "intensity")) {
				const auto vtok = extract_token(remaining);
				double v = 0.0;
				if (!parse_dbl(vtok, v)) { set_msg(result_out, false, "Invalid light intensity"); return std::nullopt; }
				set_msg(result_out, true, "Light intensity set");
				return Command::make_set_param(ParameterType::LightIntensity, v);
			}
			if (iequals_sv(sub, "ambient")) {
				const auto vtok = extract_token(remaining);
				double v = 0.0;
				if (!parse_dbl(vtok, v)) { set_msg(result_out, false, "Invalid light ambient"); return std::nullopt; }
				set_msg(result_out, true, "Light ambient set");
				return Command::make_set_param(ParameterType::LightAmbient, v);
			}
			if (iequals_sv(sub, "softness")) {
				const auto vtok = extract_token(remaining);
				double v = 0.0;
				if (!parse_dbl(vtok, v)) { set_msg(result_out, false, "Invalid light softness"); return std::nullopt; }
				set_msg(result_out, true, "Light softness set");
				return Command::make_set_param(ParameterType::LightTerminatorSoftness, v);
			}
			if (iequals_sv(sub, "specular")) {
				const auto vtok = extract_token(remaining);
				double v = 0.0;
				if (!parse_dbl(vtok, v)) { set_msg(result_out, false, "Invalid light specular"); return std::nullopt; }
				set_msg(result_out, true, "Light specular set");
				return Command::make_set_param(ParameterType::LightSpecularScale, v);
			}
			if (iequals_sv(sub, "emission")) {
				const auto vtok = extract_token(remaining);
				const bool en = iequals_sv(vtok, "on") || iequals_sv(vtok, "1") || iequals_sv(vtok, "true");
				set_msg(result_out, true, "Body emission lighting set");
				return Command::make_set_param(ParameterType::BodyEmissionLightingEnabled, en ? 1.0 : 0.0);
			}
			if (iequals_sv(sub, "emission_gain")) {
				const auto vtok = extract_token(remaining);
				double v = 0.0;
				if (!parse_dbl(vtok, v)) { set_msg(result_out, false, "Invalid emission gain"); return std::nullopt; }
				set_msg(result_out, true, "Body emission lighting gain set");
				return Command::make_set_param(ParameterType::BodyEmissionLightingGain, v);
			}
			if (iequals_sv(sub, "emission_ref")) {
				const auto vtok = extract_token(remaining);
				double v = 0.0;
				if (!parse_dbl(vtok, v)) { set_msg(result_out, false, "Invalid emission reference"); return std::nullopt; }
				set_msg(result_out, true, "Body emission lighting reference set");
				return Command::make_set_param(ParameterType::BodyEmissionLightingReferenceDistance, v);
			}
			if (iequals_sv(sub, "attenuation")) {
				const auto vtok = extract_token(remaining);
				double v = (iequals_sv(vtok, "inverse_square") || iequals_sv(vtok, "1")) ? 1.0 : 0.0;
				set_msg(result_out, true, "Light attenuation set");
				return Command::make_set_param(ParameterType::LightAttenuationMode, v);
			}
			if (iequals_sv(sub, "ref_dist")) {
				const auto vtok = extract_token(remaining);
				double v = 0.0;
				if (!parse_dbl(vtok, v)) { set_msg(result_out, false, "Invalid reference distance"); return std::nullopt; }
				set_msg(result_out, true, "Light reference distance set");
				return Command::make_set_param(ParameterType::LightReferenceDistance, v);
			}
			if (iequals_sv(sub, "direction")) {
				const auto atok = extract_token(remaining);
				const auto etok = extract_token(remaining);
				double a = 0.0, e = 0.0;
				if (!parse_dbl(atok, a) || !parse_dbl(etok, e)) { set_msg(result_out, false, "Invalid direction angles (expected azimuth elevation)"); return std::nullopt; }
				set_msg(result_out, true, "Light direction angles set");
				return Command::make_set_param(ParameterType::LightDirectionAzimuth, a);
			}
			if (iequals_sv(sub, "body")) {
				const auto idtok = extract_token(remaining);
				uint64_t bid = 0;
				if (!parse_u64(idtok, bid)) { set_msg(result_out, false, "Invalid light source body ID"); return std::nullopt; }
				set_msg(result_out, true, "Light source body ID set");
				return Command::make_set_param(ParameterType::LightSourceBodyId, static_cast<double>(bid));
			}
			if (iequals_sv(sub, "mode")) {
				const auto vtok = extract_token(remaining);
				double v = 0.0;
				if (iequals_sv(vtok, "central")) v = 0.0;
				else if (iequals_sv(vtok, "headlight")) v = 1.0;
				else if (iequals_sv(vtok, "direction")) v = 2.0;
				else if (iequals_sv(vtok, "point")) v = 3.0;
				else if (iequals_sv(vtok, "nearest")) v = 4.0;
				else if (iequals_sv(vtok, "body")) v = 5.0;
				else if (iequals_sv(vtok, "unlit")) v = 6.0;
				else static_cast<void>(parse_dbl(vtok, v));
				set_msg(result_out, true, "Light mode set");
				return Command::make_set_param(ParameterType::LightSourceMode, v);
			}
		}

		if (iequals_sv(token1, "disk")) {
			const std::string_view sub = extract_token(remaining);
			const auto vtok = extract_token(remaining);
			double v = 0.0;
			if (!parse_dbl(vtok, v)) { set_msg(result_out, false, "Invalid disk parameter value"); return std::nullopt; }
			if (iequals_sv(sub, "temp_scale")) { set_msg(result_out, true, "Disk temperature scale set"); return Command::make_set_param(ParameterType::DiskTemperatureScale, v); }
			if (iequals_sv(sub, "temp_floor")) { set_msg(result_out, true, "Disk temperature floor set"); return Command::make_set_param(ParameterType::DiskTemperatureFloor, v); }
			if (iequals_sv(sub, "beaming")) { set_msg(result_out, true, "Disk Doppler beaming exponent set"); return Command::make_set_param(ParameterType::DiskDopplerBeamingExponent, v); }
			if (iequals_sv(sub, "saturation")) { set_msg(result_out, true, "Disk color saturation set"); return Command::make_set_param(ParameterType::DiskColorSaturation, v); }
			if (iequals_sv(sub, "inner")) { set_msg(result_out, true, "Disk inner radius set"); return Command::make_set_param(ParameterType::DiskInnerRadius, v); }
			if (iequals_sv(sub, "outer")) { set_msg(result_out, true, "Disk outer radius set"); return Command::make_set_param(ParameterType::DiskOuterRadius, v); }
		}

		if (iequals_sv(token1, "jet")) {
			const std::string_view sub = extract_token(remaining);
			if (iequals_sv(sub, "on") || iequals_sv(sub, "1") || iequals_sv(sub, "true")) { set_msg(result_out, true, "Jet enabled"); return Command::make_set_param(ParameterType::JetEnabled, 1.0); }
			if (iequals_sv(sub, "off") || iequals_sv(sub, "0") || iequals_sv(sub, "false")) { set_msg(result_out, true, "Jet disabled"); return Command::make_set_param(ParameterType::JetEnabled, 0.0); }
			const auto vtok = extract_token(remaining);
			double v = 0.0;
			if (!parse_dbl(vtok, v)) { set_msg(result_out, false, "Invalid jet value"); return std::nullopt; }
			if (iequals_sv(sub, "length")) { set_msg(result_out, true, "Jet length set"); return Command::make_set_param(ParameterType::JetLength, v); }
			if (iequals_sv(sub, "angle")) { set_msg(result_out, true, "Jet opening angle set"); return Command::make_set_param(ParameterType::JetOpeningAngle, v); }
			if (iequals_sv(sub, "lorentz")) { set_msg(result_out, true, "Jet Lorentz factor set"); return Command::make_set_param(ParameterType::JetLorentzFactor, v); }
			if (iequals_sv(sub, "bfield")) { set_msg(result_out, true, "Jet magnetic field set"); return Command::make_set_param(ParameterType::JetMagneticField, v); }
			if (iequals_sv(sub, "precession")) { set_msg(result_out, true, "Jet precession angle set"); return Command::make_set_param(ParameterType::JetPrecessionAngle, v); }
		}

		if (iequals_sv(token1, "darkmatter") || iequals_sv(token1, "dm")) {
			const std::string_view sub = extract_token(remaining);
			if (iequals_sv(sub, "on") || iequals_sv(sub, "1") || iequals_sv(sub, "true")) { set_msg(result_out, true, "Dark matter enabled"); return Command::make_set_param(ParameterType::DarkMatterEnabled, 1.0); }
			if (iequals_sv(sub, "off") || iequals_sv(sub, "0") || iequals_sv(sub, "false")) { set_msg(result_out, true, "Dark matter disabled"); return Command::make_set_param(ParameterType::DarkMatterEnabled, 0.0); }
			if (iequals_sv(sub, "lensing")) {
				const auto st = extract_token(remaining);
				const bool en = iequals_sv(st, "on") || iequals_sv(st, "1") || iequals_sv(st, "true");
				set_msg(result_out, true, "Dark matter lensing updated");
				return Command::make_set_param(ParameterType::DarkMatterLensing, en ? 1.0 : 0.0);
			}
			if (iequals_sv(sub, "viz")) {
				const auto st = extract_token(remaining);
				const bool en = iequals_sv(st, "on") || iequals_sv(st, "1") || iequals_sv(st, "true");
				set_msg(result_out, true, "Dark matter visualization updated");
				return Command::make_set_param(ParameterType::DarkMatterVisualization, en ? 1.0 : 0.0);
			}
			if (iequals_sv(sub, "viz_mode")) {
				const auto mt = extract_token(remaining);
				double m = 0.0;
				if (iequals_sv(mt, "density")) m = 0.0;
				else if (iequals_sv(mt, "potential")) m = 1.0;
				else if (iequals_sv(mt, "force")) m = 2.0;
				else if (iequals_sv(mt, "distortion")) m = 3.0;
				else static_cast<void>(parse_dbl(mt, m));
				set_msg(result_out, true, "Dark matter visualization mode set");
				return Command::make_set_param(ParameterType::DarkMatterVizMode, m);
			}
			if (iequals_sv(sub, "bodies")) {
				const auto st = extract_token(remaining);
				const bool en = iequals_sv(st, "on") || iequals_sv(st, "1") || iequals_sv(st, "true");
				set_msg(result_out, true, "Dark matter body interaction updated");
				return Command::make_set_param(ParameterType::DarkMatterAffectsBodies, en ? 1.0 : 0.0);
			}
		}

		if (iequals_sv(token1, "cosmology") || iequals_sv(token1, "cosmo")) {
			const std::string_view sub = extract_token(remaining);
			if (iequals_sv(sub, "evaporation")) {
				const auto st = extract_token(remaining);
				const bool en = iequals_sv(st, "on") || iequals_sv(st, "1") || iequals_sv(st, "true");
				set_msg(result_out, true, "Black hole evaporation updated");
				return Command::make_set_param(ParameterType::CosmologyEvaporationEnabled, en ? 1.0 : 0.0);
			}
			const auto vtok = extract_token(remaining);
			double v = 0.0;
			if (!parse_dbl(vtok, v)) { set_msg(result_out, false, "Invalid cosmology parameter value"); return std::nullopt; }
			if (iequals_sv(sub, "void_radius")) { set_msg(result_out, true, "Cosmic void radius set"); return Command::make_set_param(ParameterType::CosmologyVoidRadius, v); }
			if (iequals_sv(sub, "void_width")) { set_msg(result_out, true, "Cosmic void width set"); return Command::make_set_param(ParameterType::CosmologyVoidWidth, v); }
			if (iequals_sv(sub, "hubble")) { set_msg(result_out, true, "Hubble constant H0 set"); return Command::make_set_param(ParameterType::CosmologyHubble0, v); }
			if (iequals_sv(sub, "evap_accel")) { set_msg(result_out, true, "Evaporation time acceleration set"); return Command::make_set_param(ParameterType::CosmologyEvaporationTimeAccel, v); }
		}

		if (iequals_sv(token1, "unit") || iequals_sv(token1, "units")) {
			const std::string_view sub = extract_token(remaining);
			if (iequals_sv(sub, "reset")) {
				set_msg(result_out, true, "Display units reset to SI defaults");
				return Command::make_reset_units();
			}
			const auto utok = extract_token(remaining);
			double u = 0.0;
			if (!parse_dbl(utok, u)) {
				if (iequals_sv(utok, "meters") || iequals_sv(utok, "m") || iequals_sv(utok, "si")) u = 0.0;
				else if (iequals_sv(utok, "km") || iequals_sv(utok, "kilometers")) u = 1.0;
				else if (iequals_sv(utok, "au")) u = 5.0;
				else if (iequals_sv(utok, "ly")) u = 6.0;
				else if (iequals_sv(utok, "pc")) u = 7.0;
				else if (iequals_sv(utok, "solar") || iequals_sv(utok, "msun")) u = 4.0;
				else if (iequals_sv(utok, "c")) u = 4.0;
			}
			if (iequals_sv(sub, "distance")) { set_msg(result_out, true, "Distance unit updated"); return Command::make_set_param(ParameterType::UnitDistanceSelect, u); }
			if (iequals_sv(sub, "velocity") || iequals_sv(sub, "speed")) { set_msg(result_out, true, "Velocity unit updated"); return Command::make_set_param(ParameterType::UnitVelocitySelect, u); }
			if (iequals_sv(sub, "mass")) { set_msg(result_out, true, "Mass unit updated"); return Command::make_set_param(ParameterType::UnitMassSelect, u); }
			if (iequals_sv(sub, "energy")) { set_msg(result_out, true, "Energy unit updated"); return Command::make_set_param(ParameterType::UnitEnergySelect, u); }
			if (iequals_sv(sub, "temp") || iequals_sv(sub, "temperature")) { set_msg(result_out, true, "Temperature unit updated"); return Command::make_set_param(ParameterType::UnitTemperatureSelect, u); }
			if (iequals_sv(sub, "angle")) { set_msg(result_out, true, "Angle unit updated"); return Command::make_set_param(ParameterType::UnitAngleSelect, u); }
			if (iequals_sv(sub, "charge")) { set_msg(result_out, true, "Charge unit updated"); return Command::make_set_param(ParameterType::UnitChargeSelect, u); }
			if (iequals_sv(sub, "current")) { set_msg(result_out, true, "Current unit updated"); return Command::make_set_param(ParameterType::UnitCurrentSelect, u); }
			if (iequals_sv(sub, "time")) { set_msg(result_out, true, "Time unit updated"); return Command::make_set_param(ParameterType::UnitTimeSelect, u); }
			if (iequals_sv(sub, "accel") || iequals_sv(sub, "acceleration")) { set_msg(result_out, true, "Acceleration unit updated"); return Command::make_set_param(ParameterType::UnitAccelerationSelect, u); }
			if (iequals_sv(sub, "density")) { set_msg(result_out, true, "Density unit updated"); return Command::make_set_param(ParameterType::UnitDensitySelect, u); }
			if (iequals_sv(sub, "pressure")) { set_msg(result_out, true, "Pressure unit updated"); return Command::make_set_param(ParameterType::UnitPressureSelect, u); }
			if (iequals_sv(sub, "power")) { set_msg(result_out, true, "Power unit updated"); return Command::make_set_param(ParameterType::UnitPowerSelect, u); }
			if (iequals_sv(sub, "force")) { set_msg(result_out, true, "Force unit updated"); return Command::make_set_param(ParameterType::UnitForceSelect, u); }
			if (iequals_sv(sub, "magnetic")) { set_msg(result_out, true, "Magnetic field unit updated"); return Command::make_set_param(ParameterType::UnitMagneticSelect, u); }
			if (iequals_sv(sub, "voltage")) { set_msg(result_out, true, "Voltage unit updated"); return Command::make_set_param(ParameterType::UnitVoltageSelect, u); }
			if (iequals_sv(sub, "fps")) { set_msg(result_out, true, "Frame rate unit updated"); return Command::make_set_param(ParameterType::UnitFrameRateSelect, u); }
		}

		if (iequals_sv(token1, "coord") || iequals_sv(token1, "coords")) {
			const std::string_view sub = extract_token(remaining);
			const auto ctok = extract_token(remaining);
			double c = 0.0;
			if (!parse_dbl(ctok, c)) {
				if (iequals_sv(ctok, "cartesian") || iequals_sv(ctok, "cart")) c = 0.0;
				else if (iequals_sv(ctok, "spherical") || iequals_sv(ctok, "sph")) c = 1.0;
				else if (iequals_sv(ctok, "cylindrical") || iequals_sv(ctok, "cyl")) c = 2.0;
				else if (iequals_sv(ctok, "boyer_lindquist") || iequals_sv(ctok, "bl")) c = 3.0;
				else if (iequals_sv(ctok, "tortoise")) c = 4.0;
				else if (iequals_sv(ctok, "isotropic")) c = 5.0;
				else if (iequals_sv(ctok, "kruskal")) c = 6.0;
				else if (iequals_sv(ctok, "painleve")) c = 7.0;
				else if (iequals_sv(ctok, "eddington")) c = 8.0;
			}
			if (iequals_sv(sub, "default")) {
				set_msg(result_out, true, "Default coordinate system updated");
				return Command::make_set_param(ParameterType::CoordDefaultSelect, c);
			}
		}

		if (iequals_sv(token1, "sky")) {
			const std::string_view sub = extract_token(remaining);
			if (iequals_sv(sub, "on") || iequals_sv(sub, "enable")) {
				set_msg(result_out, true, "Sky rendering enabled");
				return Command::make_set_param(ParameterType::SkyStarDensity, 91.064);
			}
			if (iequals_sv(sub, "off") || iequals_sv(sub, "disable")) {
				set_msg(result_out, true, "Sky rendering disabled");
				return Command::make_set_param(ParameterType::SkyStarDensity, 0.0);
			}
			if (iequals_sv(sub, "density")) {
				const auto vtok = extract_token(remaining);
				double v = 0.0;
				if (!parse_dbl(vtok, v)) { set_msg(result_out, false, "Invalid sky star density"); return std::nullopt; }
				set_msg(result_out, true, "Sky star density set");
				return Command::make_set_param(ParameterType::SkyStarDensity, v);
			}
			if (iequals_sv(sub, "brightness")) {
				const auto vtok = extract_token(remaining);
				double v = 0.0;
				if (!parse_dbl(vtok, v)) { set_msg(result_out, false, "Invalid sky star brightness"); return std::nullopt; }
				set_msg(result_out, true, "Sky star brightness set");
				return Command::make_set_param(ParameterType::SkyStarBrightness, v);
			}
			if (iequals_sv(sub, "nebula")) {
				const auto vtok = extract_token(remaining);
				double v = 0.0;
				if (!parse_dbl(vtok, v)) { set_msg(result_out, false, "Invalid nebula intensity"); return std::nullopt; }
				set_msg(result_out, true, "Nebula intensity set");
				return Command::make_set_param(ParameterType::SkyNebulaIntensity, v);
			}
			if (iequals_sv(sub, "grid")) {
				const auto vtok = extract_token(remaining);
				double v = 0.0;
				if (!parse_dbl(vtok, v)) { set_msg(result_out, false, "Invalid grid opacity"); return std::nullopt; }
				set_msg(result_out, true, "Sky grid opacity set");
				return Command::make_set_param(ParameterType::SkyGridOpacity, v);
			}
			if (iequals_sv(sub, "rotation")) {
				const auto vtok = extract_token(remaining);
				double v = 0.0;
				if (!parse_dbl(vtok, v)) { set_msg(result_out, false, "Invalid sky rotation"); return std::nullopt; }
				set_msg(result_out, true, "Sky rotation set");
				return Command::make_set_param(ParameterType::SkyRotation, v);
			}
			if (iequals_sv(sub, "seed")) {
				const auto vtok = extract_token(remaining);
				double v = 0.0;
				if (!parse_dbl(vtok, v)) { set_msg(result_out, false, "Invalid sky procedural seed"); return std::nullopt; }
				set_msg(result_out, true, "Sky procedural seed set");
				return Command::make_set_param(ParameterType::SkyProceduralSeed, v);
			}
		}

		if (iequals_sv(token1, "screenshot") || iequals_sv(token1, "capture")) {
			const std::string_view path = remaining;
			set_msg(result_out, true, "Screenshot capture triggered");
			return Command::make_capture_screenshot(path);
		}

		if (iequals_sv(token1, "mass")) {
			const auto vtok = extract_token(remaining);
			double v = 0.0;
			if (!parse_dbl(vtok, v)) { set_msg(result_out, false, "Invalid mass value"); return std::nullopt; }
			set_msg(result_out, true, "Central mass set");
			return Command::make_set_param(ParameterType::Mass, v);
		}
		if (iequals_sv(token1, "spin")) {
			const auto vtok = extract_token(remaining);
			double v = 0.0;
			if (!parse_dbl(vtok, v)) { set_msg(result_out, false, "Invalid spin value"); return std::nullopt; }
			set_msg(result_out, true, "Central spin set");
			return Command::make_set_param(ParameterType::Spin, v);
		}
		if (iequals_sv(token1, "charge")) {
			const auto vtok = extract_token(remaining);
			double v = 0.0;
			if (!parse_dbl(vtok, v)) { set_msg(result_out, false, "Invalid charge value"); return std::nullopt; }
			set_msg(result_out, true, "Central charge set");
			return Command::make_set_param(ParameterType::Charge, v);
		}
		if (iequals_sv(token1, "lambda")) {
			const auto vtok = extract_token(remaining);
			double v = 0.0;
			if (!parse_dbl(vtok, v)) { set_msg(result_out, false, "Invalid lambda value"); return std::nullopt; }
			set_msg(result_out, true, "Cosmological constant set");
			return Command::make_set_param(ParameterType::CosmologicalLambda, v);
		}
		if (iequals_sv(token1, "fov")) {
			const auto vtok = extract_token(remaining);
			double v = 0.0;
			if (!parse_dbl(vtok, v) || v < 5.0 || v > 175.0) { set_msg(result_out, false, "Invalid FOV value"); return std::nullopt; }
			set_msg(result_out, true, "FOV set");
			return Command::make_camera_set_fov(v);
		}
		if (iequals_sv(token1, "speed")) {
			const auto vtok = extract_token(remaining);
			double v = 0.0;
			if (!parse_dbl(vtok, v) || v <= 0.0) { set_msg(result_out, false, "Invalid speed value"); return std::nullopt; }
			set_msg(result_out, true, "Speed set");
			return Command::make_camera_set_speed(v);
		}
		if (iequals_sv(token1, "exposure")) {
			const auto vtok = extract_token(remaining);
			double v = 0.0;
			if (!parse_dbl(vtok, v)) { set_msg(result_out, false, "Invalid exposure value"); return std::nullopt; }
			set_msg(result_out, true, "Exposure set");
			return Command::make_set_param(ParameterType::CameraExposure, v);
		}
		if (iequals_sv(token1, "preset")) {
			const auto vtok = extract_token(remaining);
			uint64_t v = 0;
			if (iequals_sv(vtok, "potato")) v = 0;
			else if (iequals_sv(vtok, "perf") || iequals_sv(vtok, "performance")) v = 1;
			else if (iequals_sv(vtok, "balanced")) v = 2;
			else if (iequals_sv(vtok, "high")) v = 3;
			else if (iequals_sv(vtok, "ultra")) v = 4;
			else if (iequals_sv(vtok, "extreme")) v = 5;
			else if (!parse_u64(vtok, v) || v > 6) { set_msg(result_out, false, "Invalid performance preset"); return std::nullopt; }
			set_msg(result_out, true, "Performance preset set");
			return Command::make_set_performance_preset(static_cast<uint32_t>(v));
		}
		if (iequals_sv(token1, "scale") || iequals_sv(token1, "resolution")) {
			const auto vtok = extract_token(remaining);
			double v = 0.0;
			if (!parse_dbl(vtok, v) || v < 0.1 || v > 4.0) { set_msg(result_out, false, "Invalid resolution scale"); return std::nullopt; }
			set_msg(result_out, true, "Resolution scale set");
			return Command::make_set_resolution_scale(v);
		}
		if (iequals_sv(token1, "steps")) {
			const auto vtok = extract_token(remaining);
			uint64_t v = 0;
			if (!parse_u64(vtok, v) || v < 64 || v > 65536) { set_msg(result_out, false, "Invalid steps limit"); return std::nullopt; }
			set_msg(result_out, true, "Ray steps limit set");
			return Command::make_set_render_steps(v);
		}
		if (iequals_sv(token1, "gpu")) {
			const auto vtok = extract_token(remaining);
			const bool en = iequals_sv(vtok, "on") || iequals_sv(vtok, "1") || iequals_sv(vtok, "true");
			set_msg(result_out, true, "GPU compute path toggled");
			return Command::make_set_param(ParameterType::UseGpuCompute, en ? 1.0 : 0.0);
		}
		if (iequals_sv(token1, "lod")) {
			const auto vtok = extract_token(remaining);
			const bool en = iequals_sv(vtok, "on") || iequals_sv(vtok, "1") || iequals_sv(vtok, "true");
			set_msg(result_out, true, "LOD system toggled");
			return Command::make_set_param(ParameterType::LodEnabled, en ? 1.0 : 0.0);
		}
		if (iequals_sv(token1, "skip")) {
			const auto vtok = extract_token(remaining);
			const bool en = iequals_sv(vtok, "on") || iequals_sv(vtok, "1") || iequals_sv(vtok, "true");
			set_msg(result_out, true, "Space-skipping toggled");
			return Command::make_set_param(ParameterType::SpaceSkippingEnabled, en ? 1.0 : 0.0);
		}
		if (iequals_sv(token1, "shadows")) {
			const auto vtok = extract_token(remaining);
			const bool en = iequals_sv(vtok, "on") || iequals_sv(vtok, "1") || iequals_sv(vtok, "true");
			set_msg(result_out, true, "Body shadows toggled");
			return Command::make_set_param(ParameterType::BodyShadowsEnabled, en ? 1.0 : 0.0);
		}
		if (iequals_sv(token1, "atmosphere")) {
			const auto vtok = extract_token(remaining);
			double v = 1.0;
			if (iequals_sv(vtok, "off") || iequals_sv(vtok, "0")) v = 0.0;
			else if (iequals_sv(vtok, "on") || iequals_sv(vtok, "1")) v = 1.0;
			else static_cast<void>(parse_dbl(vtok, v));
			set_msg(result_out, true, "Atmosphere intensity set");
			return Command::make_set_param(ParameterType::BodyAtmosphereGlobalIntensity, v);
		}
		if (iequals_sv(token1, "bodies_only")) {
			const auto vtok = extract_token(remaining);
			const bool en = iequals_sv(vtok, "on") || iequals_sv(vtok, "1") || iequals_sv(vtok, "true");
			set_msg(result_out, true, "Bodies-only mode toggled");
			return Command::make_set_param(ParameterType::BodiesOnlyRenderMode, en ? 1.0 : 0.0);
		}
		if (iequals_sv(token1, "disk_occlusion")) {
			const auto vtok = extract_token(remaining);
			const bool en = iequals_sv(vtok, "on") || iequals_sv(vtok, "1") || iequals_sv(vtok, "true");
			set_msg(result_out, true, "Disk occlusion toggled");
			return Command::make_set_param(ParameterType::BodyDiskOcclusionEnabled, en ? 1.0 : 0.0);
		}
		if (iequals_sv(token1, "schematic")) {
			const auto vtok = extract_token(remaining);
			const bool en = iequals_sv(vtok, "on") || iequals_sv(vtok, "1") || iequals_sv(vtok, "true");
			set_msg(result_out, true, "Schematic mode toggled");
			return Command::make_set_param(ParameterType::SchematicModeEnabled, en ? 1.0 : 0.0);
		}

		if (iequals_sv(token1, "set")) {
			const std::string_view token2 = extract_token(remaining);
			if (token2.empty()) {
				set_msg(result_out, false, "Missing parameter name in set command");
				return std::nullopt;
			}
			const std::string_view token3 = extract_token(remaining);
			if (token3.empty()) {
				set_msg(result_out, false, "Missing value in set command");
				return std::nullopt;
			}
			double val = 0.0;
			if (!parse_dbl(token3, val)) {
				set_msg(result_out, false, "Invalid numeric value in set command");
				return std::nullopt;
			}

			ParameterType ptype = ParameterType::Custom;
			if (iequals_sv(token2, "mass")) ptype = ParameterType::Mass;
			else if (iequals_sv(token2, "spin")) ptype = ParameterType::Spin;
			else if (iequals_sv(token2, "charge")) ptype = ParameterType::Charge;
			else if (iequals_sv(token2, "lambda")) ptype = ParameterType::CosmologicalLambda;
			else if (iequals_sv(token2, "throat")) ptype = ParameterType::WormholeThroat;
			else if (iequals_sv(token2, "warp_velocity") || iequals_sv(token2, "warp_vel")) ptype = ParameterType::WarpVelocity;
			else if (iequals_sv(token2, "projection") || iequals_sv(token2, "proj")) ptype = ParameterType::ProjectionMode;
			else if (iequals_sv(token2, "timeflow") || iequals_sv(token2, "time_flow")) ptype = ParameterType::TimeFlowMode;
			else if (iequals_sv(token2, "cameran_speed") || iequals_sv(token2, "speed") || iequals_sv(token2, "camera_speed")) ptype = ParameterType::CameraSpeed;
			else if (iequals_sv(token2, "fov") || iequals_sv(token2, "camera_fov")) ptype = ParameterType::CameraFov;
			else if (iequals_sv(token2, "exposure") || iequals_sv(token2, "camera_exposure")) ptype = ParameterType::CameraExposure;
			else if (iequals_sv(token2, "tonemapper") || iequals_sv(token2, "tonemap") || iequals_sv(token2, "tonemapping")) ptype = ParameterType::TonemappingMode;
			else if (iequals_sv(token2, "rtol") || iequals_sv(token2, "integration_rtol")) ptype = ParameterType::IntegrationRtol;
			else if (iequals_sv(token2, "atol") || iequals_sv(token2, "integration_atol")) ptype = ParameterType::IntegrationAtol;
			else if (iequals_sv(token2, "min_step") || iequals_sv(token2, "integration_min_step")) ptype = ParameterType::IntegrationMinStep;
			else if (iequals_sv(token2, "max_step") || iequals_sv(token2, "integration_max_step")) ptype = ParameterType::IntegrationMaxStep;
			else if (iequals_sv(token2, "step_factor") || iequals_sv(token2, "step_scale") || iequals_sv(token2, "integration_step_factor")) ptype = ParameterType::IntegrationStepFactor;
			else if (iequals_sv(token2, "render_scale") || iequals_sv(token2, "scale") || iequals_sv(token2, "resolution_scale")) ptype = ParameterType::ResolutionScale;
			else if (iequals_sv(token2, "ray_steps") || iequals_sv(token2, "steps_limit") || iequals_sv(token2, "max_ray_steps") || iequals_sv(token2, "steps")) ptype = ParameterType::MaxRaySteps;
			else if (iequals_sv(token2, "performance") || iequals_sv(token2, "perf") || iequals_sv(token2, "performance_preset")) ptype = ParameterType::PerformancePreset;
			else if (iequals_sv(token2, "camera_mode") || iequals_sv(token2, "cam_mode")) ptype = ParameterType::CameraMode;
			else if (iequals_sv(token2, "contrast") || iequals_sv(token2, "post_contrast")) ptype = ParameterType::PostContrast;
			else if (iequals_sv(token2, "saturation") || iequals_sv(token2, "post_saturation")) ptype = ParameterType::PostSaturation;
			else if (iequals_sv(token2, "lift") || iequals_sv(token2, "post_lift")) ptype = ParameterType::PostLift;
			else if (iequals_sv(token2, "gamma") || iequals_sv(token2, "post_gamma")) ptype = ParameterType::PostGamma;
			else if (iequals_sv(token2, "gain") || iequals_sv(token2, "post_gain")) ptype = ParameterType::PostGain;
			else if (iequals_sv(token2, "vignette") || iequals_sv(token2, "post_vignette") || iequals_sv(token2, "post_vignette_strength")) ptype = ParameterType::PostVignetteStrength;
			else if (iequals_sv(token2, "highlights") || iequals_sv(token2, "post_highlights")) ptype = ParameterType::PostHighlights;
			else if (iequals_sv(token2, "shadows") || iequals_sv(token2, "post_shadows")) ptype = ParameterType::PostShadows;
			else if (iequals_sv(token2, "gpu") || iequals_sv(token2, "use_gpu") || iequals_sv(token2, "use_gpu_compute")) ptype = ParameterType::UseGpuCompute;
			else if (iequals_sv(token2, "lod") || iequals_sv(token2, "lod_enabled")) ptype = ParameterType::LodEnabled;
			else if (iequals_sv(token2, "lod_distance") || iequals_sv(token2, "lod_distance_threshold")) ptype = ParameterType::LodDistanceThreshold;
			else if (iequals_sv(token2, "lod_steps") || iequals_sv(token2, "lod_reduced_steps")) ptype = ParameterType::LodReducedSteps;
			else if (iequals_sv(token2, "sky_star_density") || iequals_sv(token2, "star_density")) ptype = ParameterType::SkyStarDensity;
			else if (iequals_sv(token2, "sky_star_brightness") || iequals_sv(token2, "star_brightness")) ptype = ParameterType::SkyStarBrightness;
			else if (iequals_sv(token2, "sky_nebula") || iequals_sv(token2, "nebula_intensity")) ptype = ParameterType::SkyNebulaIntensity;
			else if (iequals_sv(token2, "sky_grid_opacity") || iequals_sv(token2, "grid_opacity")) ptype = ParameterType::SkyGridOpacity;
			else if (iequals_sv(token2, "sky_rotation")) ptype = ParameterType::SkyRotation;
			else if (iequals_sv(token2, "sky_hue")) ptype = ParameterType::SkyHueShift;
			else if (iequals_sv(token2, "sky_saturation")) ptype = ParameterType::SkySaturation;
			else if (iequals_sv(token2, "sky_bg_r")) ptype = ParameterType::SkyBackgroundR;
			else if (iequals_sv(token2, "sky_bg_g")) ptype = ParameterType::SkyBackgroundG;
			else if (iequals_sv(token2, "sky_bg_b")) ptype = ParameterType::SkyBackgroundB;
			else if (iequals_sv(token2, "sky_seed") || iequals_sv(token2, "procedural_seed")) ptype = ParameterType::SkyProceduralSeed;
			else if (iequals_sv(token2, "sky_galaxy_density")) ptype = ParameterType::SkyGalaxyDensity;
			else if (iequals_sv(token2, "sky_galaxy_brightness")) ptype = ParameterType::SkyGalaxyBrightness;
			else if (iequals_sv(token2, "sky_galaxy_scale")) ptype = ParameterType::SkyGalaxySizeScale;
			else if (iequals_sv(token2, "sky_dust_density")) ptype = ParameterType::SkyDustDensity;
			else if (iequals_sv(token2, "sky_dust_intensity")) ptype = ParameterType::SkyDustIntensity;
			else if (iequals_sv(token2, "sky_dust_scale")) ptype = ParameterType::SkyDustScale;
			else if (iequals_sv(token2, "sky_cluster_density")) ptype = ParameterType::SkyClusterDensity;
			else if (iequals_sv(token2, "sky_cluster_brightness")) ptype = ParameterType::SkyClusterBrightness;
			else if (iequals_sv(token2, "sky_cluster_scale")) ptype = ParameterType::SkyClusterSizeScale;
			else if (iequals_sv(token2, "work_distribution") || iequals_sv(token2, "tiling")) ptype = ParameterType::WorkDistributionMode;
			else if (iequals_sv(token2, "force_realloc") || iequals_sv(token2, "realloc_texture")) ptype = ParameterType::ForceTextureReallocation;
			else if (iequals_sv(token2, "space_skip") || iequals_sv(token2, "space_skip_enabled")) ptype = ParameterType::SpaceSkippingEnabled;
			else if (iequals_sv(token2, "space_skip_radius")) ptype = ParameterType::SpaceSkipRadiusScale;
			else if (iequals_sv(token2, "pole_precision") || iequals_sv(token2, "pole_guard")) ptype = ParameterType::PoleGuardPrecisionScale;
			else if (iequals_sv(token2, "far_field_step") || iequals_sv(token2, "far_field_scale")) ptype = ParameterType::FarFieldStepScale;
			else if (iequals_sv(token2, "render_distance") || iequals_sv(token2, "render_distance_scale")) ptype = ParameterType::RenderDistanceScale;
			else if (iequals_sv(token2, "schematic") || iequals_sv(token2, "schematic_mode")) ptype = ParameterType::SchematicModeEnabled;
			else if (iequals_sv(token2, "schematic_sim") || iequals_sv(token2, "schematic_allow_sim")) ptype = ParameterType::SchematicAllowSimulation;
			else if (iequals_sv(token2, "interlace") || iequals_sv(token2, "interlace_rendering")) ptype = ParameterType::InterlaceRenderingEnabled;
			else if (iequals_sv(token2, "dynamic_res") || iequals_sv(token2, "dynamic_resolution")) ptype = ParameterType::DynamicResolutionEnabled;
			else if (iequals_sv(token2, "dynamic_fps") || iequals_sv(token2, "target_fps")) ptype = ParameterType::DynamicResolutionTargetFps;
			else if (iequals_sv(token2, "adaptive_tile") || iequals_sv(token2, "adaptive_tile_prepass")) ptype = ParameterType::AdaptiveTilePrepassEnabled;
			else if (iequals_sv(token2, "body_lod_threshold") || iequals_sv(token2, "body_lod_pixels")) ptype = ParameterType::BodyRenderLodPixelThreshold;
			else if (iequals_sv(token2, "body_point_threshold") || iequals_sv(token2, "body_point_pixels")) ptype = ParameterType::BodyRenderPointPixelThreshold;
			else if (iequals_sv(token2, "body_shadows") || iequals_sv(token2, "shadows")) ptype = ParameterType::BodyShadowsEnabled;
			else if (iequals_sv(token2, "atmosphere") || iequals_sv(token2, "atmosphere_intensity")) ptype = ParameterType::BodyAtmosphereGlobalIntensity;
			else if (iequals_sv(token2, "body_low_power") || iequals_sv(token2, "low_power_bodies")) ptype = ParameterType::BodyRenderLowPowerMode;
			else if (iequals_sv(token2, "bodies_only") || iequals_sv(token2, "bodies_only_mode")) ptype = ParameterType::BodiesOnlyRenderMode;
			else if (iequals_sv(token2, "disk_occlusion") || iequals_sv(token2, "body_disk_occlusion")) ptype = ParameterType::BodyDiskOcclusionEnabled;
			else if (iequals_sv(token2, "body_noise_octaves") || iequals_sv(token2, "body_noise")) ptype = ParameterType::BodyNoiseOctaves;
			else if (iequals_sv(token2, "disk_temp_scale") || iequals_sv(token2, "disk_temperature_scale")) ptype = ParameterType::DiskTemperatureScale;
			else if (iequals_sv(token2, "disk_temp_floor") || iequals_sv(token2, "disk_temperature_floor")) ptype = ParameterType::DiskTemperatureFloor;
			else if (iequals_sv(token2, "disk_beaming") || iequals_sv(token2, "disk_beaming_exponent")) ptype = ParameterType::DiskDopplerBeamingExponent;
			else if (iequals_sv(token2, "disk_saturation") || iequals_sv(token2, "disk_color_saturation")) ptype = ParameterType::DiskColorSaturation;
			else if (iequals_sv(token2, "light_mode")) ptype = ParameterType::LightSourceMode;
			else if (iequals_sv(token2, "light_attenuation")) ptype = ParameterType::LightAttenuationMode;
			else if (iequals_sv(token2, "light_body") || iequals_sv(token2, "light_source_body")) ptype = ParameterType::LightSourceBodyId;
			else if (iequals_sv(token2, "light_intensity")) ptype = ParameterType::LightIntensity;
			else if (iequals_sv(token2, "light_ambient")) ptype = ParameterType::LightAmbient;
			else if (iequals_sv(token2, "light_softness") || iequals_sv(token2, "light_terminator_softness")) ptype = ParameterType::LightTerminatorSoftness;
			else if (iequals_sv(token2, "light_specular") || iequals_sv(token2, "light_specular_scale")) ptype = ParameterType::LightSpecularScale;
			else if (iequals_sv(token2, "light_azimuth") || iequals_sv(token2, "light_direction_azimuth")) ptype = ParameterType::LightDirectionAzimuth;
			else if (iequals_sv(token2, "light_elevation") || iequals_sv(token2, "light_direction_elevation")) ptype = ParameterType::LightDirectionElevation;
			else if (iequals_sv(token2, "light_reference") || iequals_sv(token2, "light_reference_distance")) ptype = ParameterType::LightReferenceDistance;
			else if (iequals_sv(token2, "emission_lighting")) ptype = ParameterType::BodyEmissionLightingEnabled;
			else if (iequals_sv(token2, "emission_lighting_gain") || iequals_sv(token2, "emission_gain")) ptype = ParameterType::BodyEmissionLightingGain;
			else if (iequals_sv(token2, "emission_lighting_reference") || iequals_sv(token2, "emission_reference")) ptype = ParameterType::BodyEmissionLightingReferenceDistance;
			else if (iequals_sv(token2, "camera_collision") || iequals_sv(token2, "collision")) ptype = ParameterType::CameraCollisionEnabled;
			else if (iequals_sv(token2, "camera_collision_clearance") || iequals_sv(token2, "collision_clearance")) ptype = ParameterType::CameraCollisionClearance;
			else if (iequals_sv(token2, "camera_motion_refresh") || iequals_sv(token2, "motion_refresh")) ptype = ParameterType::CameraMotionLiveRefresh;
			else if (iequals_sv(token2, "warp_radius") || iequals_sv(token2, "bubble_radius")) ptype = ParameterType::WarpBubbleRadius;
			else if (iequals_sv(token2, "warp_sharpness") || iequals_sv(token2, "wall_sharpness")) ptype = ParameterType::WarpWallSharpness;
			else if (iequals_sv(token2, "tidal_potential") || iequals_sv(token2, "wormhole_tidal")) ptype = ParameterType::WormholeTidalPotential;
			else if (iequals_sv(token2, "teves_a0") || iequals_sv(token2, "critical_acceleration")) ptype = ParameterType::TevesCriticalAcceleration;
			else if (iequals_sv(token2, "constants_preset") || iequals_sv(token2, "const_preset")) ptype = ParameterType::ConstantsPresetSelect;
			else if (iequals_sv(token2, "constant_c") || iequals_sv(token2, "const_c")) ptype = ParameterType::ConstantSimC;
			else if (iequals_sv(token2, "constant_g") || iequals_sv(token2, "const_g")) ptype = ParameterType::ConstantSimG;
			else if (iequals_sv(token2, "constant_h") || iequals_sv(token2, "const_h")) ptype = ParameterType::ConstantSimH;
			else if (iequals_sv(token2, "constant_kb") || iequals_sv(token2, "const_kb")) ptype = ParameterType::ConstantSimKB;
			else if (iequals_sv(token2, "constant_na") || iequals_sv(token2, "const_na")) ptype = ParameterType::ConstantSimNA;
			else if (iequals_sv(token2, "constant_ke") || iequals_sv(token2, "const_ke")) ptype = ParameterType::ConstantSimKe;
			else if (iequals_sv(token2, "constant_kcd") || iequals_sv(token2, "const_kcd")) ptype = ParameterType::ConstantSimKcd;
			else if (iequals_sv(token2, "interaction_electricity") || iequals_sv(token2, "electricity")) ptype = ParameterType::InteractionElectricityEnabled;
			else if (iequals_sv(token2, "interaction_magnetism") || iequals_sv(token2, "magnetism")) ptype = ParameterType::InteractionMagnetismEnabled;
			else if (iequals_sv(token2, "interaction_collisions") || iequals_sv(token2, "collisions")) ptype = ParameterType::InteractionCollisionsEnabled;
			else if (iequals_sv(token2, "interaction_thermo") || iequals_sv(token2, "thermodynamics")) ptype = ParameterType::InteractionThermodynamicsEnabled;
			else if (iequals_sv(token2, "interaction_frag") || iequals_sv(token2, "fragmentation")) ptype = ParameterType::InteractionFragmentationEnabled;
			else if (iequals_sv(token2, "interaction_annihilation") || iequals_sv(token2, "annihilation")) ptype = ParameterType::InteractionAnnihilationEnabled;
			else if (iequals_sv(token2, "jet") || iequals_sv(token2, "jet_enabled")) ptype = ParameterType::JetEnabled;
			else if (iequals_sv(token2, "jet_length")) ptype = ParameterType::JetLength;
			else if (iequals_sv(token2, "jet_angle") || iequals_sv(token2, "jet_opening_angle")) ptype = ParameterType::JetOpeningAngle;
			else if (iequals_sv(token2, "jet_lorentz")) ptype = ParameterType::JetLorentzFactor;
			else if (iequals_sv(token2, "jet_bfield") || iequals_sv(token2, "jet_magnetic_field")) ptype = ParameterType::JetMagneticField;
			else if (iequals_sv(token2, "dark_matter") || iequals_sv(token2, "darkmatter")) ptype = ParameterType::DarkMatterEnabled;
			else if (iequals_sv(token2, "dm_lensing")) ptype = ParameterType::DarkMatterLensing;
			else if (iequals_sv(token2, "dm_viz") || iequals_sv(token2, "dm_visualization")) ptype = ParameterType::DarkMatterVisualization;
			else if (iequals_sv(token2, "cosmo_void_radius") || iequals_sv(token2, "void_radius")) ptype = ParameterType::CosmologyVoidRadius;
			else if (iequals_sv(token2, "cosmo_void_width") || iequals_sv(token2, "void_width")) ptype = ParameterType::CosmologyVoidWidth;
			else if (iequals_sv(token2, "hubble") || iequals_sv(token2, "hubble0")) ptype = ParameterType::CosmologyHubble0;
			else if (iequals_sv(token2, "evaporation") || iequals_sv(token2, "hawking")) ptype = ParameterType::CosmologyEvaporationEnabled;
			else if (iequals_sv(token2, "tickrate")) {
				if (val < 10.0 || val > 1000.0) {
					set_msg(result_out, false, "Invalid tickrate (must be between 10.0 and 1000.0 Hz)");
					return std::nullopt;
				}
				ptype = ParameterType::TickRate;
			}

			Command cmd = Command::make_set_param(ptype, val);
			if (ptype == ParameterType::Custom) {
				const size_t len = std::min(token2.size(), sizeof(cmd.custom_param_name) - 1);
				std::memcpy(cmd.custom_param_name, token2.data(), len);
				cmd.custom_param_name[len] = '\0';
			}
			set_msg(result_out, true, "Parameter updated");
			return cmd;
		}

		set_msg(result_out, false, "Unrecognized command");
		return std::nullopt;
	}
};

}
