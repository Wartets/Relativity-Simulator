#pragma once

#include "relativistic/distributed/network_transport.hpp"
#include "relativistic/distributed/packet_protocol.hpp"
#include "relativistic/render/gpu_types.hpp"
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

namespace Relativistic::Distributed {

static_assert(std::is_trivially_copyable_v<Render::GpuBodyData>);

inline constexpr uint32_t kMaximumCatalogBodies = 4096;
inline constexpr size_t kCatalogRetainedFrames = 4;

#pragma pack(push, 1)

struct BodyCatalogChunkHeader {
	uint64_t frame_index{0};
	uint32_t total_bodies{0};
	uint32_t first_index{0};
	uint32_t chunk_bodies{0};
	uint32_t reserved{0};
};

#pragma pack(pop)

static_assert(sizeof(BodyCatalogChunkHeader) == 24);

inline constexpr size_t kBodiesPerCatalogChunk =
	(kMaximumTransportPacketBytes - sizeof(PacketEnvelope) - sizeof(BodyCatalogChunkHeader)) / sizeof(Render::GpuBodyData);

[[nodiscard]] size_t serialize_body_catalog_chunk(
	uint32_t frame_key,
	std::span<const Render::GpuBodyData> bodies,
	uint32_t first_index,
	uint32_t count,
	std::span<uint8_t> output
) noexcept;

class BodyCatalogStore {
public:
	using BodyList = std::vector<Render::GpuBodyData>;
	using SharedBodies = std::shared_ptr<const BodyList>;

	enum class IngestResult : uint8_t {
		Accepted = 0,
		Completed = 1,
		Duplicate = 2,
		Invalid = 3
	};

private:
	struct Pending {
		uint32_t frame_key{0};
		uint32_t total{0};
		uint32_t received_count{0};
		BodyList bodies{};
		std::vector<uint8_t> received{};
	};

	std::deque<Pending> pending_{};
	std::deque<std::pair<uint32_t, SharedBodies>> published_{};
	SharedBodies latest_{};

public:
	[[nodiscard]] IngestResult ingest(std::span<const uint8_t> payload, uint32_t& completed_frame, BodyList& completed);

	void publish(uint32_t frame_key, SharedBodies bodies);

	[[nodiscard]] SharedBodies find(uint32_t frame_key) const;

	[[nodiscard]] SharedBodies latest() const {
		return latest_;
	}

	void clear() noexcept;
};

}
