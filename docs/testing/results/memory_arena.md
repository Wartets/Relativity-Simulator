# memory_arena

Verifies the linear memory arenas used by the zero-allocation hot path: capacity accounting, natural and cache-line alignment, object creation, reset, markers, scoped rewinding, sentinel integrity and exhaustion handling.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 16 |
| Failed checks | 0 |
| Duration | 0.004283 s |
| Generated | 2026-10-19 13:43:33 UTC |

## Allocation and alignment

| Result | Check | Detail |
| --- | --- | --- |
| PASS | a new arena is empty with its full capacity |  |
| PASS | a new arena has no allocations and intact sentinels |  |
| PASS | a scalar allocation is naturally aligned |  |
| PASS | a 64-byte aligned allocation honours its alignment |  |
| PASS | create constructs the object with its initial state |  |
| PASS | the allocation count and sentinels remain consistent |  |
| PASS | reset releases every allocation |  |
| PASS | a 128-byte aligned array honours its alignment |  |
| PASS | writing the whole allocation does not corrupt the sentinels |  |

## Markers and scopes

| Result | Check | Detail |
| --- | --- | --- |
| PASS | an allocation advances the marker |  |
| PASS | a scoped allocation advances the marker |  |
| PASS | leaving the scope restores the marker |  |
| PASS | rewinding returns to the initial marker |  |

## Exhaustion

| Result | Check | Detail |
| --- | --- | --- |
| PASS | an allocation within capacity succeeds |  |
| PASS | an allocation beyond capacity returns null |  |
| PASS | a failed allocation leaves the sentinels intact |  |

