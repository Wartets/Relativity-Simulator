# replay_loop

Verifies deterministic record and replay: a geodesic bundle is evolved for ten thousand ticks while randomly generated impulse and mass events are recorded in the journal, then the journal is rewound and the same evolution is replayed from the recorded events alone, and the SHA-256 digests of the two final bundle states must be identical.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 4 |
| Failed checks | 0 |
| Duration | 1.214 s |
| Generated | 2026-10-19 22:21:06 UTC |

## Recording

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the recording session stores events in the journal | events=548 |
| PASS | the recorded evolution changes the bundle state |  |

## Replay

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the replay consumes every recorded event |  |
| PASS | the record and replay digests are identical | record=63fc6f79a6252a280aedb90c6af507875b681db45bf8cccb15e033d8bd6e2fae replay=63fc6f79a6252a280aedb90c6af507875b681db45bf8cccb15e033d8bd6e2fae |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Replay | recorded_events | 548 | events |

