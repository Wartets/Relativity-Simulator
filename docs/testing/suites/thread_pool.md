# Thread pool suite

Executable: `test_thread_pool` - Source: `tests/test_thread_pool.cpp` - Results: [thread_pool.md](../results/thread_pool.md)

## Purpose

Verifies the persistent worker pool that distributes ray tracing work ([ARCHITECTURE.md](../../ARCHITECTURE.md#33-thread-pool--raytracing-work-distribution)).

## Checks

| Check | Criterion |
| --- | --- |
| Worker count | A pool built with four workers reports four |
| Task execution | One hundred enqueued tasks have all run when `wait_idle` returns |
| Parallel loop coverage | Over 10000 indices with a grain of 128, every element is processed exactly once |
| Degenerate range | A range of one element is processed exactly once |

## Acceptance

All checks pass. Results do not depend on scheduling order.
