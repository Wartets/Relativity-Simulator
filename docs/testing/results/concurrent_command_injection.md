# concurrent_command_injection

Verifies the lock-free command path under load: one hundred thousand commands of eight different kinds are injected by a producer thread while a simulation thread drains the queue and advances the scheduler, and every command is processed with the scheduler making progress and the injected parameters staying in their injected ranges.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 5 |
| Failed checks | 0 |
| Duration | 0.07687 s |
| Generated | 2026-10-19 22:16:37 UTC |

## Concurrent producer and simulation worker

| Result | Check | Detail |
| --- | --- | --- |
| PASS | the simulation worker drains the queue within ten seconds of the end of production |  |
| PASS | every injected command is processed exactly once | processed=100000 |
| PASS | the scheduler advances while commands are being injected |  |
| PASS | the injected mass stays finite | value=1.93 |
| PASS | the final mass equals one of the injected values in [1, 2) |  |

## Measurements

| Section | Name | Value | Unit |
| --- | --- | --- | --- |
| Concurrent producer and simulation worker | ticks_executed | 12500 | ticks |
| Concurrent producer and simulation worker | commands_processed | 100000 | commands |

