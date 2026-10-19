# scheduler

Verifies the deterministic simulation scheduler: initial paused state, tick rate limits, exact real-time to tick conversion, warp scaling, accumulator clamping, interpolation factor, pause and single-step semantics, and reset.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 38 |
| Failed checks | 0 |
| Duration | 0.002652 s |
| Generated | 2026-10-19 22:21:09 UTC |

## Initial state

| Result | Check | Detail |
| --- | --- | --- |
| PASS | a new scheduler starts at tick zero |  |
| PASS | a new scheduler starts at logical time zero | actual=0 expected=0 abs_error=0 tolerance=1e-15 |
| PASS | the default tick rate is 60 Hz | actual=60 expected=60 abs_error=0 tolerance=1e-15 |
| PASS | the default tick interval is 1/60 s | actual=0.0166666666667 expected=0.0166666666667 abs_error=0 tolerance=1e-15 |
| PASS | the default warp factor is one | actual=1 expected=1 abs_error=0 tolerance=1e-15 |
| PASS | a new scheduler starts paused |  |
| PASS | a new scheduler has no requested steps |  |
| PASS | the snapshot reports the initial state |  |
| PASS | a paused scheduler without requested steps cannot advance |  |

## Tick rate limits

| Result | Check | Detail |
| --- | --- | --- |
| PASS | an in-range tick rate is stored unchanged | actual=100 expected=100 abs_error=0 tolerance=1e-15 |
| PASS | the tick interval follows the tick rate | actual=0.01 expected=0.01 abs_error=0 tolerance=1e-15 |
| PASS | a tick rate below the minimum is clamped to 10 Hz | actual=10 expected=10 abs_error=0 tolerance=1e-15 |
| PASS | a tick rate above the maximum is clamped to 1000 Hz | actual=1000 expected=1000 abs_error=0 tolerance=1e-15 |
| PASS | the tick interval follows the clamped tick rate | actual=0.001 expected=0.001 abs_error=0 tolerance=1e-15 |

## Real-time advance and warp

| Result | Check | Detail |
| --- | --- | --- |
| PASS | one tick interval of real time yields exactly one tick |  |
| PASS | one hundred intervals yield one hundred ticks |  |
| PASS | one hundred ticks at 100 Hz span one logical second | actual=1 expected=1 abs_error=6.661e-16 tolerance=1e-12 |
| PASS | the warp factor scales the logical time per tick | actual=2.5 expected=2.5 abs_error=4.885e-15 tolerance=1e-12 |
| PASS | a zero warp factor is rejected | actual=1 expected=1 abs_error=0 tolerance=1e-15 |
| PASS | a negative warp factor is rejected | actual=1 expected=1 abs_error=0 tolerance=1e-15 |

## Accumulator behaviour

| Result | Check | Detail |
| --- | --- | --- |
| PASS | a long stall is clamped to 0.25 s, which yields 15 ticks at 60 Hz |  |
| PASS | non-positive real-time increments are ignored |  |
| PASS | the interpolation factor is zero with an empty accumulator | actual=0 expected=0 abs_error=0 tolerance=1e-15 |
| PASS | half an interval of accumulated time gives an interpolation factor of 0.5 | actual=0.5 expected=0.5 abs_error=0 tolerance=1e-12 |
| PASS | the interpolation factor saturates at one | actual=1 expected=1 abs_error=0 tolerance=1e-12 |

## Pause and single stepping

| Result | Check | Detail |
| --- | --- | --- |
| PASS | a paused scheduler does not consume accumulated time |  |
| PASS | requested steps allow advancing while paused |  |
| PASS | exactly the requested number of ticks executes |  |
| PASS | the scheduler pauses again when the requested steps are exhausted |  |
| PASS | resume clears the paused state |  |
| PASS | pause discards pending requested steps |  |

## Reset

| Result | Check | Detail |
| --- | --- | --- |
| PASS | ten ticks execute before the reset |  |
| PASS | reset restores tick zero |  |
| PASS | reset restores logical time zero | actual=0 expected=0 abs_error=0 tolerance=1e-15 |
| PASS | reset leaves the scheduler paused |  |
| PASS | reset restores the default warp factor | actual=1 expected=1 abs_error=0 tolerance=1e-15 |
| PASS | reset restores the default tick rate | actual=60 expected=60 abs_error=0 tolerance=1e-15 |
| PASS | reset clears the accumulator | actual=0 expected=0 abs_error=0 tolerance=1e-15 |

