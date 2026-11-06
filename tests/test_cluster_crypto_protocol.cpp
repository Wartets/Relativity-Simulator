#include "relativistic/core/pcg64.hpp"
#include "relativistic/distributed/packet_protocol.hpp"
#include "relativistic/distributed/crypto_channel.hpp"
#include "support/test_harness.hpp"
#include "support/engine_scenario.hpp"
#include <array>
#include <vector>
#include <cstring>

using namespace Relativistic;
using namespace Relativistic::Distributed;
using namespace Relativistic::Testing;

int main() {
	TestSuite suite("cluster_crypto_protocol", "Validates binary packet layout, X25519 key exchange, ChaCha20-Poly1305 AEAD sealing, and anti-replay constraints.");

	suite.section("Packet Binary Envelopes");
	{
		const PacketEnvelope env = make_envelope(PacketType::HeartbeatPing, 0x12345678, 42, 10, std::span<const uint8_t>{});
		suite.check(sizeof(PacketEnvelope) == 48, "PacketEnvelope size is exactly 48 bytes (aligned 16 bytes)");
		suite.check(env.is_valid_magic(), "Magic identifier is 0x52454C44 (RELD)");
		suite.check(env.protocol_version == 1, "Protocol version initialized to 1");
		suite.check(validate_envelope_structure(env, 48), "Envelope structure validation succeeds on exact size");
		suite.check(!validate_envelope_structure(env, 47), "Envelope structure validation fails on truncated buffer");
	}

	suite.section("X25519 Diffie-Hellman Key Exchange");
	{
		const CryptoKeyPair alice = CryptoChannel::generate_node_identity_keys(0x1122334455667788ULL);
		const CryptoKeyPair bob = CryptoChannel::generate_node_identity_keys(0x99AABBCCDDEEFF00ULL);
		const std::array<uint8_t, 32> shared_token = {0x01, 0x02, 0x03, 0x04};

		CryptoChannel channel_a;
		suite.check(channel_a.perform_handshake_server(bob.public_key, alice.private_key, shared_token), "Alice (Server) computes handshake");

		CryptoChannel channel_b;
		suite.check(channel_b.perform_handshake_client(alice.public_key, bob.private_key, shared_token), "Bob (Client) computes handshake");

		suite.check(channel_a.is_established(), "Alice channel marked as established");
		suite.check(channel_b.is_established(), "Bob channel marked as established");
	}

	suite.section("ChaCha20-Poly1305 AEAD Encrypt & Decrypt");
	{
		CryptoKeyPair alice = CryptoChannel::generate_node_identity_keys(0x1020304050607080ULL);
		CryptoKeyPair bob = CryptoChannel::generate_node_identity_keys(0x0807060504030201ULL);
		std::array<uint8_t, 32> shared_token = {0xAA, 0xBB, 0xCC, 0xDD};

		CryptoChannel channel_tx;
		suite.check(channel_tx.perform_handshake_server(bob.public_key, alice.private_key, shared_token), "Transmitter channel handshake succeeds");

		CryptoChannel channel_rx;
		suite.check(channel_rx.perform_handshake_client(alice.public_key, bob.private_key, shared_token), "Receiver channel handshake succeeds");

		PacketEnvelope header = make_envelope(PacketType::TaskAssignment, 1, 0, 0, std::span<const uint8_t>{});
		std::vector<uint8_t> plaintext(65536);
		Relativistic::Core::PCG64Engine rng(0xDEADBEEF42ULL, 0);
		for (auto& b : plaintext) b = static_cast<uint8_t>(rng() & 0xFF);

		std::vector<uint8_t> ciphertext(plaintext.size());
		std::array<uint8_t, 16> tag{};
		suite.check(channel_tx.encrypt_and_authenticate_packet(header.authenticated_header(), plaintext, ciphertext, tag), "Encryption succeeds on 64KB payload");

		std::vector<uint8_t> decrypted(plaintext.size());
		suite.check(channel_rx.verify_and_decrypt_packet(header.authenticated_header(), 0, ciphertext, tag, decrypted), "Decryption and tag verification succeeds");

		bool match = true;
		for (size_t i = 0; i < plaintext.size(); ++i) {
			if (plaintext[i] != decrypted[i]) {
				match = false;
				break;
			}
		}
		suite.check(match, "Decrypted payload exactly matches original plaintext");

		std::vector<uint8_t> corrupted_cipher = ciphertext;
		corrupted_cipher[1024] ^= 0x01;
		suite.check(!channel_rx.verify_and_decrypt_packet(header.authenticated_header(), 0, corrupted_cipher, tag, decrypted), "Poly1305 rejects corrupted ciphertext");

		PacketEnvelope corrupted_header = header;
		corrupted_header.type = PacketType::NodeDisconnect;
		suite.check(!channel_rx.verify_and_decrypt_packet(corrupted_header.authenticated_header(), 0, ciphertext, tag, decrypted), "Poly1305 rejects AAD modification (header manipulation)");

		suite.check(!channel_rx.verify_and_decrypt_packet(header.authenticated_header(), 0, ciphertext, tag, decrypted), "Anti-replay window rejects duplicate sequence number");
	}

	suite.section("Throughput Performance Measurement");
	{
		CryptoKeyPair alice = CryptoChannel::generate_node_identity_keys(0xAABBCCDDEEFF0011ULL);
		CryptoKeyPair bob = CryptoChannel::generate_node_identity_keys(0x1100FFEEDDCCBBAAULL);
		std::array<uint8_t, 32> shared_token = {0x11, 0x22, 0x33, 0x44};

		CryptoChannel channel;
		suite.check(channel.perform_handshake_server(bob.public_key, alice.private_key, shared_token), "Benchmark channel server handshake succeeds");

		PacketEnvelope header = make_envelope(PacketType::TileResultData, 42, 0, 0, std::span<const uint8_t>{});
		std::vector<uint8_t> data(32768, 0x42);
		std::vector<uint8_t> out(data.size());
		std::array<uint8_t, 16> tag{};

		const auto start = std::chrono::high_resolution_clock::now();
		const size_t iterations = 20000;
		for (size_t i = 0; i < iterations; ++i) {
			static_cast<void>(channel.encrypt_and_authenticate_packet(header.authenticated_header(), data, out, tag));
		}
		const auto end = std::chrono::high_resolution_clock::now();
		const double seconds = std::chrono::duration<double>(end - start).count();
		const double mb_processed = static_cast<double>(iterations * data.size()) / (1024.0 * 1024.0);
		const double mb_per_sec = mb_processed / seconds;

		suite.measure("chacha20_throughput", mb_per_sec, "MB/s");
		suite.check(mb_per_sec > 100.0, "ChaCha20-Poly1305 throughput is sufficient for real-time operations");
	}

	return suite.finish();
}
