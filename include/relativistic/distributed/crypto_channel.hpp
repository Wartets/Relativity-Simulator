#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <span>
#include <string_view>

namespace Relativistic::Distributed {

struct CryptoKeyPair {
	std::array<uint8_t, 32> public_key{};
	std::array<uint8_t, 32> private_key{};
};

struct SessionSecurityContext {
	std::array<uint8_t, 32> shared_secret{};
	std::array<uint8_t, 32> transmission_key{};
	std::array<uint8_t, 32> reception_key{};
	uint64_t transmit_sequence{0};
	uint64_t receive_sequence{0};
	uint64_t replay_window_mask{0};
	uint32_t session_epoch{0};
	bool authenticated{false};
};

class CryptoChannel {
private:
	SessionSecurityContext context_{};

	static void chacha20_block(
		std::span<const uint32_t, 8> key,
		uint32_t counter,
		std::span<const uint32_t, 3> nonce,
		std::span<uint8_t, 64> block_output
	) noexcept;

	static void poly1305_generate_tag(
		std::span<const uint8_t, 32> one_time_key,
		std::span<const uint8_t> associated_data,
		std::span<const uint8_t> ciphertext,
		std::span<uint8_t, 16> output_tag
	) noexcept;

	static void derive_keys_hkdf_sha256(
		std::span<const uint8_t, 32> shared_secret,
		std::span<const uint8_t, 32> pre_shared_token,
		bool is_server,
		std::span<uint8_t, 32> out_tx_key,
		std::span<uint8_t, 32> out_rx_key
	) noexcept;

	[[nodiscard]] bool check_and_update_replay_window(uint64_t sequence) noexcept;

public:
	CryptoChannel() noexcept = default;
	~CryptoChannel() noexcept = default;

	[[nodiscard]] static CryptoKeyPair generate_node_identity_keys(uint64_t seed_value = 0) noexcept;

	[[nodiscard]] bool perform_handshake_server(
		std::span<const uint8_t, 32> client_public_key,
		std::span<const uint8_t, 32> server_private_key,
		std::span<const uint8_t, 32> pre_shared_token
	) noexcept;

	[[nodiscard]] bool perform_handshake_client(
		std::span<const uint8_t, 32> server_public_key,
		std::span<const uint8_t, 32> client_private_key,
		std::span<const uint8_t, 32> pre_shared_token
	) noexcept;

	[[nodiscard]] bool encrypt_and_authenticate_packet(
		std::span<const uint8_t> associated_header,
		std::span<const uint8_t> plaintext_payload,
		std::span<uint8_t> output_ciphertext,
		std::span<uint8_t, 16> output_tag
	) noexcept;

	[[nodiscard]] bool verify_and_decrypt_packet(
		std::span<const uint8_t> associated_header,
		uint64_t packet_sequence,
		std::span<const uint8_t> ciphertext_payload,
		std::span<const uint8_t, 16> authentication_tag,
		std::span<uint8_t> output_plaintext
	) noexcept;

	[[nodiscard]] bool is_established() const noexcept {
		return context_.authenticated;
	}

	void reset() noexcept {
		context_ = SessionSecurityContext{};
	}
};

}
