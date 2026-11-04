#include "relativistic/distributed/body_catalog.hpp"
#include <algorithm>
#include <cstring>

namespace Relativistic::Distributed {

size_t serialize_body_catalog_chunk(
	uint32_t frame_key,
	std::span<const Render::GpuBodyData> bodies,
	uint32_t first_index,
	uint32_t count,
	std::span<uint8_t> output
) noexcept {
	const size_t body_bytes = static_cast<size_t>(count) * sizeof(Render::GpuBodyData);
	const size_t total_bytes = sizeof(BodyCatalogChunkHeader) + body_bytes;
	if (count > kBodiesPerCatalogChunk || bodies.size() > kMaximumCatalogBodies) {
		return 0;
	}
	if (static_cast<size_t>(first_index) + count > bodies.size() || output.size() < total_bytes) {
		return 0;
	}
	BodyCatalogChunkHeader header{};
	header.frame_index = frame_key;
	header.total_bodies = static_cast<uint32_t>(bodies.size());
	header.first_index = first_index;
	header.chunk_bodies = count;
	std::memcpy(output.data(), &header, sizeof(header));
	if (count > 0U) {
		std::memcpy(output.data() + sizeof(header), bodies.data() + first_index, body_bytes);
	}
	return total_bytes;
}

BodyCatalogStore::IngestResult BodyCatalogStore::ingest(std::span<const uint8_t> payload, uint32_t& completed_frame, BodyList& completed) {
	if (payload.size() < sizeof(BodyCatalogChunkHeader)) {
		return IngestResult::Invalid;
	}
	BodyCatalogChunkHeader header{};
	std::memcpy(&header, payload.data(), sizeof(header));
	const size_t body_bytes = payload.size() - sizeof(header);
	if (header.total_bodies > kMaximumCatalogBodies || header.chunk_bodies > kBodiesPerCatalogChunk) {
		return IngestResult::Invalid;
	}
	if (body_bytes != static_cast<size_t>(header.chunk_bodies) * sizeof(Render::GpuBodyData)) {
		return IngestResult::Invalid;
	}
	if (static_cast<size_t>(header.first_index) + header.chunk_bodies > header.total_bodies) {
		return IngestResult::Invalid;
	}

	const auto frame_key = static_cast<uint32_t>(header.frame_index);
	for (const auto& entry : published_) {
		if (entry.first == frame_key) {
			return IngestResult::Duplicate;
		}
	}

	auto it = std::find_if(pending_.begin(), pending_.end(), [frame_key](const Pending& candidate) noexcept {
		return candidate.frame_key == frame_key;
	});
	if (it == pending_.end()) {
		if (pending_.size() >= kCatalogRetainedFrames) {
			pending_.pop_front();
		}
		Pending fresh{};
		fresh.frame_key = frame_key;
		fresh.total = header.total_bodies;
		fresh.bodies.resize(header.total_bodies);
		fresh.received.assign(header.total_bodies, 0U);
		pending_.push_back(std::move(fresh));
		it = std::prev(pending_.end());
	} else if (it->total != header.total_bodies) {
		return IngestResult::Invalid;
	}

	const uint8_t* source = payload.data() + sizeof(header);
	for (uint32_t i = 0; i < header.chunk_bodies; ++i) {
		const size_t index = static_cast<size_t>(header.first_index) + i;
		if (it->received[index] != 0U) {
			continue;
		}
		std::memcpy(&it->bodies[index], source + static_cast<size_t>(i) * sizeof(Render::GpuBodyData), sizeof(Render::GpuBodyData));
		it->received[index] = 1U;
		++it->received_count;
	}

	if (it->received_count == it->total) {
		completed_frame = it->frame_key;
		completed = std::move(it->bodies);
		pending_.erase(it);
		return IngestResult::Completed;
	}
	return IngestResult::Accepted;
}

void BodyCatalogStore::publish(uint32_t frame_key, SharedBodies bodies) {
	published_.emplace_back(frame_key, bodies);
	while (published_.size() > kCatalogRetainedFrames) {
		published_.pop_front();
	}
	latest_ = std::move(bodies);
}

BodyCatalogStore::SharedBodies BodyCatalogStore::find(uint32_t frame_key) const {
	for (auto it = published_.rbegin(); it != published_.rend(); ++it) {
		if (it->first == frame_key) {
			return it->second;
		}
	}
	return nullptr;
}

void BodyCatalogStore::clear() noexcept {
	pending_.clear();
	published_.clear();
	latest_.reset();
}

}
