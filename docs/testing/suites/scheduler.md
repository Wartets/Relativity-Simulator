# Scheduler suite

Executable: `test_scheduler` - Source: `tests/test_scheduler.cpp` - Results: [scheduler.md](../results/scheduler.md)

## Purpose

Verifies the deterministic simulation clock described in [ARCHITECTURE.md](../../ARCHITECTURE.md#31-deterministic-simulation-scheduler): the fixed logical tick, the nanosecond accumulator, warp scaling and pause and step semantics. The scheduler is the single source of simulation time, so a regression here breaks determinism everywhere.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Initial state | Tick, time, rate, interval, warp | 0, 0, 60 Hz, 1/60 s, 1 within 1e-15 |
| Initial state | Paused and no steps | The scheduler starts paused and cannot advance |
| Tick rate limits | In-range, below, above | 100 Hz kept, 5 Hz clamped to 10 Hz, 2000 Hz clamped to 1000 Hz; the interval follows |
| Real-time advance | One interval gives one tick | 100 intervals of 10 ms at 100 Hz yield 100 ticks and one logical second within 1e-12 |
| Real-time advance | Warp | A factor of 2.5 yields 2.5 logical seconds; zero and negative factors are rejected |
| Accumulator | Clamp | A 10 s stall is clamped to 0.25 s, giving 15 ticks at 60 Hz (interval 16666666 ns) |
| Accumulator | Invalid input | Zero and negative nanosecond increments are ignored |
| Accumulator | Interpolation factor | 0 when empty, 0.5 at half an interval, saturating at 1 |
| Pause and stepping | Paused accumulator | A paused scheduler does not consume accumulated time |
| Pause and stepping | Requested steps | Exactly the requested ticks execute, then the scheduler pauses; `pause` discards pending steps |
| Reset | State | Tick, time, warp and rate return to defaults, the scheduler is paused and the accumulator is empty |

## Notes

The previous version of this test asserted that a new scheduler is not paused and that it is running after a reset. The implementation starts and resets paused, so those assertions contradicted the documented behaviour. The suite now calls `resume` before measuring real-time advance.

## Acceptance

All checks pass. The suite is deterministic and runs in milliseconds.
