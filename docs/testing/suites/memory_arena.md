# Memory arena suite

Executable: `test_memory_arena` - Source: `tests/test_memory_arena.cpp` - Results: [memory_arena.md](../results/memory_arena.md)

## Purpose

Verifies the linear arenas that keep the hot path free of dynamic allocation ([ARCHITECTURE.md](../../ARCHITECTURE.md#fundamental-design-paradigms)).

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Allocation and alignment | Empty arena | Capacity 1 MiB, nothing allocated, sentinels intact |
| Allocation and alignment | Alignment | Natural, 64-byte and 128-byte alignment honoured |
| Allocation and alignment | `create` | Constructed object keeps its initial state; three allocations counted |
| Allocation and alignment | Reset and overwrite | Reset releases everything; filling a whole allocation leaves the sentinels intact |
| Markers and scopes | Marker, scope, rewind | Markers advance, scope exit restores the marker, rewind returns to the start |
| Exhaustion | Overflow | An oversized allocation returns null and sentinels stay intact |

## Acceptance

All checks pass.
