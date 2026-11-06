# Test Suite Specification: Cluster Cryptographic Protocol

- **Executable Target:** `test_cluster_crypto_protocol`
- **Source File:** `tests/test_cluster_crypto_protocol.cpp`
- **Output Report:** `docs/testing/results/cluster_crypto_protocol.md`

## Purpose

Validates binary wire envelope alignment, asymmetric X25519 Elliptic Curve Diffie-Hellman key exchange, authenticated encryption with associated data (AEAD) using ChaCha20-Poly1305 (RFC 8439), and anti-replay protection.

## Checked Invariants & Tolerances

| Check Domain | Expected Value | Tolerance | Justification |
| :--- | :--- | :--- | :--- |
| Envelope Size | 48 bytes | Exact (0 bytes) | 16-byte alignment without pointers |
| Magic Identifier | 0x52454C44 | Exact | Binary identification for Relativistic protocol |
| X25519 Symmetry | Identical shared secret | Exact bit identity | Diffie-Hellman commutativity |
| ChaCha20-Poly1305 Roundtrip | Decrypted == Plaintext | Exact bit identity | RFC 8439 authenticated encryption correctness |
| Ciphertext Tampering Rejection | Decryption failure | Strict rejection | Poly1305 MAC integrity verification |
| AAD Tampering Rejection | Decryption failure | Strict rejection | Header authentication tag validation |
| Anti-Replay Sliding Window | Replay rejection | Strict rejection | Monotonic sequence number protection |
| Encryption Throughput | > 100 MB/s | Lower bound | Real-time 60 FPS 4K network throughput ceiling |
