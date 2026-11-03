#include "relativistic/distributed/packet_protocol.hpp"

namespace Relativistic::Distributed {

namespace {

[[nodiscard]] consteval std::array<uint32_t, 256> generate_crc32_table() noexcept {
	std::array<uint32_t, 256> table{};
	for (uint32_t i = 0; i < 256; ++i) {
		uint32_t entry = i;
		for (int bit = 0; bit < 8; ++bit) {
			entry = (entry & 1U) ? (0xEDB88320U ^ (entry >> 1U)) : (entry >> 1U);
		}
		table[i] = entry;
	}
	return table;
}

static constexpr std::array<uint32_t, 256> kCrc32Table = generate_crc32_table();

}

uint32_t compute_crc32(std::span<const uint8_t> data) noexcept {
	uint32_t crc = 0xFFFFFFFFU;
	for (const uint8_t byte : data) {
		const uint32_t table_index = (crc ^ static_cast<uint32_t>(byte)) & 0xFFU;
		crc = kCrc32Table[table_index] ^ (crc >> 8U);
	}
	return crc ^ 0xFFFFFFFFU;
}

bool verify_crc32(std::span<const uint8_t> data, uint32_t expected_crc32) noexcept {
	return compute_crc32(data) == expected_crc32;
}

PacketEnvelope make_envelope(
	PacketType type,
	uint32_t session_identifier,
	uint32_t packet_sequence,
	uint64_t frame_index,
	std::span<const uint8_t> payload
) noexcept {
	PacketEnvelope env{};
	env.magic_identifier = PacketEnvelope::kMagicIdentifier;
	env.protocol_version = PacketEnvelope::kProtocolVersion;
	env.type = type;
	env.session_identifier = session_identifier;
	env.packet_sequence = packet_sequence;
	env.frame_index = static_cast<uint32_t>(frame_index);
	env.payload_bytes = static_cast<uint32_t>(payload.size());
	env.payload_crc32 = payload.empty() ? 0U : compute_crc32(payload);
	env.authentication_tag.fill(0U);
	return env;
}

bool validate_envelope_structure(const PacketEnvelope& envelope, size_t received_buffer_size) noexcept {
	if (received_buffer_size < sizeof(PacketEnvelope)) {
		return false;
	}
	if (!envelope.is_valid_magic()) {
		return false;
	}
	if (envelope.protocol_version != PacketEnvelope::kProtocolVersion) {
		return false;
	}
	const size_t total_required = sizeof(PacketEnvelope) + static_cast<size_t>(envelope.payload_bytes);
	if (received_buffer_size < total_required) {
		return false;
	}
	return true;
}

}
