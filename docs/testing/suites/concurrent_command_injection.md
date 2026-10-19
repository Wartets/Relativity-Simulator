# Concurrent command injection suite

Executable: `test_concurrent_command_injection` - Source: `tests/test_concurrent_command_injection.cpp` - Results: [concurrent_command_injection.md](../results/concurrent_command_injection.md)

## Purpose

Verifies the lock-free command path of [ARCHITECTURE.md](../../ARCHITECTURE.md#32-asynchronous-lock-free-command-queue) under load: a producer thread injects commands into the single-producer queue while the simulation thread drains it and advances the scheduler.

## Scenario

100000 commands cycling through eight kinds (warp, mass, spin, charge, single step, tick rate, resume, status) with a queue of 32768 slots and a scheduler at 1000 Hz.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Concurrent producer and simulation worker | Drain | Every command is processed within 10 s of the end of production |
| Concurrent producer and simulation worker | Count | Exactly 100000 commands processed |
| Concurrent producer and simulation worker | Progress | The scheduler tick counter is positive |
| Concurrent producer and simulation worker | Parameters | The final mass is finite and lies in [1, 2) |

## Justification

The queue and the producer mutex must neither lose nor duplicate commands, and the resume commands in the stream must let the scheduler advance concurrently with command processing. The mass values injected lie in [1, 1.99].

## Measurements

Ticks executed and commands processed.

## Limits

Only the final parameter range is checked, not the interleaving order, which depends on thread scheduling.

## Acceptance

All checks pass.

## Recorded results

See [../results/concurrent_command_injection.md](../results/concurrent_command_injection.md).
