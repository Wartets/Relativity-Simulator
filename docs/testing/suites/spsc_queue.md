# SPSC queue suite

Executable: `test_spsc_queue` - Source: `tests/test_spsc_queue.cpp` - Results: [spsc_queue.md](../results/spsc_queue.md)

## Purpose

Verifies the lock-free single-producer single-consumer ring buffer used for the command and result queues ([ARCHITECTURE.md](../../ARCHITECTURE.md#32-asynchronous-lock-free-command-queue)).

## Scenario

A queue of 16 `uint32_t` is filled and drained sequentially. A queue of 1024 `uint64_t` then carries the values 1 to 500000 from a producer thread to a consumer thread.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Sequential | Empty state and capacity | Empty, size zero, capacity 16 |
| Sequential | Fill and drain | Fifteen pushes succeed, size 15, pops return the values in order, popping when empty fails |
| Sequential | Wrap-around | Fifty fill and drain cycles of eight elements keep the order |
| Concurrent | Integrity | No element is lost, duplicated or reordered, the sum equals 500000 x 500001 / 2, the queue ends empty |

## Measurements

The number of transferred elements is recorded.

## Limits

The suite exercises one producer and one consumer, which is the only supported topology. Memory ordering errors can remain undetected on strongly ordered hardware.

## Acceptance

All checks pass.
