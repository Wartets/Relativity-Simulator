# cluster_crypto_protocol

Validates binary packet layout, X25519 key exchange, ChaCha20-Poly1305 AEAD sealing, and anti-replay constraints.

| Property | Value |
| --- | --- |
| Status | FAILED |
| Checks | 19 |
| Failed checks | 1 |
| Duration | 113.6 s |
| Generated | 2026-11-07 17:58:04 UTC |

## Packet Binary Envelopes

| Result | Check | Detail |
| --- | --- | --- |
| PASS | PacketEnvelope size is exactly 48 bytes (aligned 16 bytes) |  |
| PASS | Magic identifier is 0x52454C44 (RELD) |  |
| PASS | Protocol version initialized to 1 |  |
| PASS | Envelope structure validation succeeds on exact size |  |
| PASS | Envelope structure validation fails on truncated buffer |  |

## X25519 Diffie-Hellman Key Exchange

| Result | Check | Detail |
| --- | --- | --- |
| PASS | Alice (Server) computes handshake |  |
| PASS | Bob (Client) computes handshake |  |
| PASS | Alice channel marked as established |  |
| PASS | Bob channel marked as established |  |

## ChaCha20-Poly1305 AEAD Encrypt & Decrypt

| Result | Check | Detail |
| --- | --- | --- |
| PASS | Transmitter channel handshake succeeds |  |
| PASS | Receiver channel handshake succeeds |  |
| PASS | Encryption succeeds on 64KB payload |  |
| PASS | Decryption and tag verification succeeds |  |
| PASS | Decrypted payload exactly matches original plaintext |  |
| PASS | Poly1305 rejects corrupted ciphertext |  |
| PASS | Poly1305 rejects AAD modification (header manipulation) |  |
| PASS | Anti-replay window rejects duplicate sequence number |  |

## Throughput Performance Measurement

| Result | Check | Detail |
| --- | --- | --- |
| PASS | Benchmark channel server handshake succeeds |  |
| FAIL | ChaCha20-Poly1305 throughput is sufficient for real-time operations |  |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Throughput Performance Measurement | chacha20_throughput | 5.50919417 | MB/s |

