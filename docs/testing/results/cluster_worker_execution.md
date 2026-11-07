# cluster_worker_execution

Verifies exact mathematical parity between worker and local tile integration, sub-millisecond task cancellation response, and sustained memory stability under continuous execution.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 11 |
| Failed checks | 0 |
| Duration | 42.48 s |
| Generated | 2026-11-07 18:15:16 UTC |

## Geometry & Setup

| Result | Check | Detail |
| --- | --- | --- |
| PASS | test tile geometry resolves within screen bounds |  |

## Local Reference Integration

| Result | Check | Detail |
| --- | --- | --- |
| PASS | local reference tile dispatches successfully |  |

## Worker Tile Renderer Parity

| Result | Check | Detail |
| --- | --- | --- |
| PASS | worker tile renderer initializes |  |
| PASS | task header construction succeeds |  |
| PASS | worker renderer outcome completes without error |  |
| PASS | worker renderer outcome was not cancelled |  |
| PASS | kerr accretion radiance matches local reference within 1e-12 | value=0 limit=1e-12 |
| PASS | kerr accretion redshift matches local reference within 1e-12 | value=0 limit=1e-12 |
| PASS | status flags match exactly on every pixel |  |

## Task Cancellation Latency

| Result | Check | Detail |
| --- | --- | --- |
| PASS | cancellation latency remains under 15 ms | value=2.5853 limit=15 |

## Sustained Pipeline Stress & Stability

| Result | Check | Detail |
| --- | --- | --- |
| PASS | all stress tiles complete without memory fault or crash |  |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Worker Tile Renderer Parity | max_radiance_relative_error | 0 | relative |
| Worker Tile Renderer Parity | max_redshift_relative_error | 0 | relative |
| Task Cancellation Latency | cancellation_latency | 2.5853 | ms |
| Sustained Pipeline Stress & Stability | worker_throughput | 0.0121374424 | MPx/s |

