# Cluster crypto protocol suite

Executable: `test_cluster_crypto_protocol` - Source: `tests/test_cluster_crypto_protocol.cpp` - Results: [cluster_crypto_protocol.md](../results/cluster_crypto_protocol.md)

## Purpose

Verifies `PacketEnvelope` binary layout and `CryptoChannel` primitives: X25519 Elliptic Curve Diffie-Hellman key agreement, ChaCha20-Poly1305 AEAD authenticated encryption, anti-replay sliding window tracking, and symmetric throughput ceilings ([ARCHITECTURE.md](../../ARCHITECTURE.md#214-distributed-compute-subsystem--heterogeneous-cluster-architecture)).

## Scenario

Envelopes initialized with magic `0x52454C44` and protocol version 1; Alice and Bob keypairs generated from seeds `0x1122334455667788ULL` and `0x99AABBCCDDEEFF00ULL`; 64 KiB pseudo-random payload generated with PCG64 seed `0xDEADBEEF42ULL`; 20000 ChaCha20-Poly1305 encryption iterations on 32 KiB payloads.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Packet Binary Envelopes | Envelope size | Exactly 48 bytes |
| Packet Binary Envelopes | Magic identifier | Equal to `0x52454C44` |
| Packet Binary Envelopes | Protocol version | Equal to 1 |
| Packet Binary Envelopes | Validation | Success on 48 bytes, failure on 47 bytes |
| X25519 Diffie-Hellman Key Exchange | Handshake | Server and client handshakes succeed and channels established |
| ChaCha20-Poly1305 AEAD Encrypt & Decrypt | Handshake | Transmitter and receiver channels established |
| ChaCha20-Poly1305 AEAD Encrypt & Decrypt | Sealing | Encryption and decryption of 64 KiB payload succeed |
| ChaCha20-Poly1305 AEAD Encrypt & Decrypt | Fidelity | Decrypted payload matches plaintext byte-for-byte |
| ChaCha20-Poly1305 AEAD Encrypt & Decrypt | Tampering rejection | Ciphertext byte flip and AAD header tampering rejected |
| ChaCha20-Poly1305 AEAD Encrypt & Decrypt | Anti-replay | Duplicate sequence number rejected |
| Throughput Performance Measurement | Benchmark setup | Benchmark channel established |
| Throughput Performance Measurement | Rate | ChaCha20-Poly1305 throughput exceeds 5.0 MB/s |

## Justification

Binary envelopes require 16-byte alignment to prevent padding divergence across platforms. Diffie-Hellman commutativity guarantees symmetric secret derivation. Poly1305 MAC verification detects single-bit bitflips in payload and authenticated associated data.

## Measurements

Encryption throughput in MB/s (`chacha20_throughput`).

## Acceptance

All checks pass.

## Recorded results

See [../results/cluster_crypto_protocol.md](../results/cluster_crypto_protocol.md).
