# Cluster Crypto Protocol Test Suite

**Executable:** `test_cluster_crypto_protocol`  
**Source:** `tests/test_cluster_crypto_protocol.cpp`  
**Results:** [docs/testing/results/cluster_crypto_protocol.md](../results/cluster_crypto_protocol.md)

## Purpose
Validates the low-level binary protocols and cryptographic channels bridging the distributed render cluster. This suite guarantees that packet headers satisfy strict alignment (zero allocation direct cast mapping), and that the ChaCha20-Poly1305 with X25519 key exchange guarantees authenticity and confidentiality across the cluster communication.

## Covered Modes and Features
- Binary layout and size of `PacketEnvelope`.
- Standard IEEE 802.3 CRC32 header integrity checks.
- X25519 (Curve25519) Diffie-Hellman Key Agreement symmetry.
- ChaCha20-Poly1305 Authenticated Encryption with Associated Data (AEAD).
- Replay attack mitigation via sliding sequence windows.
- Cryptographic throughput validation.

## Invariants and Checks

| Check | Tolerance / Condition | Mathematical Justification |
| :--- | :--- | :--- |
| PacketEnvelope size is exactly 48 bytes (aligned 16 bytes) | `sizeof(PacketEnvelope) == 48` | Ensures SIMD-aligned, padding-free structural copying across memory boundaries. |
| Magic identifier is 0x52454C44 (RELD) | `magic == 0x52454C44` | Magic bytes must remain invariant to identify valid cluster datagrams. |
| Protocol version initialized to 1 | `version == 1` | Prevents routing incompatible binary packet payloads. |
| Envelope structure validation succeeds on exact size | `size >= 48` | Avoids buffer overrun reading malicious payloads. |
| Envelope structure validation fails on truncated buffer | `size < 48` | Immediate dropping of malformed packets. |
| Alice (Server) computes handshake | Boolean evaluation | ECDH public-key ingestion succeeds. |
| Bob (Client) computes handshake | Boolean evaluation | ECDH public-key ingestion succeeds. |
| Alice channel marked as established | Boolean evaluation | Flag set via state machine flow. |
| Bob channel marked as established | Boolean evaluation | Flag set via state machine flow. |
| Encryption succeeds on 64KB payload | Boolean evaluation | Poly1305 AEAD operates cleanly on massive network slices. |
| Decryption and tag verification succeeds | Boolean evaluation | Verification accepts the generated MAC under the shared key. |
| Decrypted payload exactly matches original plaintext | Bit-identical comparison | Cipher algorithm is perfectly symmetric and lossless. |
| Poly1305 rejects corrupted ciphertext | Boolean evaluation | Any bit manipulation inside ciphertext alters the MAC and invalidates the frame. |
| Poly1305 rejects AAD modification (header manipulation) | Boolean evaluation | Any bit manipulation of unencrypted headers (AAD) invalidates the MAC. |
| Anti-replay window rejects duplicate sequence number | Boolean evaluation | Sequence numbers are monotonic and previously observed sequences inside the mask are refused. |
| ChaCha20-Poly1305 throughput is sufficient for real-time operations | `mb_per_sec > 100.0` | Cryptography must never bottleneck the real-time ray-tracer pipeline dispatch rate. |

## Scenario Parameters
- Plaintext payload: `65536 bytes`
- Synthetic random seed: `0xDEADBEEF42ULL`
- Iteration cycles for throughput: `20000` samples of 32 KB blocks.

## Boundaries and Known Limitations
- The custom implementation of ChaCha20-Poly1305 prioritizes zero heap allocation over hardware AES-NI instructions, sacrificing peak theoretical speed (limit $\approx 800$ MB/s) to maintain the engine's strict memory and compilation independence requirements.

## Acceptance Criteria
- 100% of mathematical invariants pass.
- Handshake X25519 outputs identical shared secrets symmetrically.
- Poly1305 drops 100% of packets subjected to simulated man-in-the-middle bit-flip corruptions.
