# Replay loop suite

Executable: `test_replay_loop` - Source: `tests/test_replay_loop.cpp` - Results: [replay_loop.md](../results/replay_loop.md)

## Purpose

Verifies the determinism primitives of [DESCRIPTION.md](../../DESCRIPTION.md#11-bit-level-determinism--random-number-generation) and [DESCRIPTION.md](../../DESCRIPTION.md#112-synchronous-command-journal--replay): `EventJournal`, `PCG64Engine` and the geodesic bundle integrator reproduce an evolution bit for bit from the recorded events.

## Scenario

A four-lane Schwarzschild bundle is evolved for 10000 ticks. In record mode a PCG64 stream (seed 0x1337) creates radial impulses and mass modifications with a 5 percent probability per tick. The journal is then rewound and the evolution is replayed from the journal alone.

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Recording | Events | At least one event is recorded |
| Recording | Evolution | The final digest differs from the initial digest |
| Replay | Consumption | No event remains after the replay |
| Replay | Determinism | The SHA-256 digests of the record and replay states are identical |

## Justification

Identical digests of the full bundle state prove that every component of every lane is reproduced exactly. The change of the digest during recording excludes a trivial equality.

## Limits

The journal is a standalone primitive; replay of orchestrator sessions is not implemented ([DESCRIPTION.md](../../DESCRIPTION.md#112-synchronous-command-journal--replay)).

## Acceptance

All checks pass.

## Recorded results

See [../results/replay_loop.md](../results/replay_loop.md).
