# thread_pool

Verifies the persistent worker pool: worker count, execution of every enqueued task before wait_idle returns, and exact coverage of the index range by chunked parallel loops.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 4 |
| Failed checks | 0 |
| Duration | 0.004436 s |
| Generated | 2026-11-05 10:01:26 UTC |

## Task execution

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the pool creates the requested number of workers |  |
| PASS | wait_idle returns only after all hundred tasks completed |  |

## Chunked parallel loop

| Result | Check | Detail |
| --- | --- | --- |
| PASS | every index is processed exactly once | wrong_elements=0 |
| PASS | a single-element range is processed exactly once |  |

