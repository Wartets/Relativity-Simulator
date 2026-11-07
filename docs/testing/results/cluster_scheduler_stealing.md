# cluster_scheduler_stealing

Validates capacity-weighted tile partitioning conservation, dynamic load balancing convergence, speculative work stealing on deadlines, and thread-safe scheduler node transitions.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 49 |
| Failed checks | 0 |
| Duration | 0.1164 s |
| Generated | 2026-11-07 18:14:28 UTC |

## Tile Partition Conservation Across Resolutions

| Result | Check | Detail |
| --- | --- | --- |
| PASS | scheduler begins frame successfully |  |
| PASS | slot tile assignment matches allocated quota |  |
| PASS | slot tile assignment matches allocated quota |  |
| PASS | slot tile assignment matches allocated quota |  |
| PASS | slot tile assignment matches allocated quota |  |
| PASS | sum of node tile quotas exactly equals total active tiles |  |
| PASS | initial remaining tiles equals total |  |
| PASS | scheduler begins frame successfully |  |
| PASS | slot tile assignment matches allocated quota |  |
| PASS | slot tile assignment matches allocated quota |  |
| PASS | slot tile assignment matches allocated quota |  |
| PASS | slot tile assignment matches allocated quota |  |
| PASS | sum of node tile quotas exactly equals total active tiles |  |
| PASS | initial remaining tiles equals total |  |
| PASS | scheduler begins frame successfully |  |
| PASS | slot tile assignment matches allocated quota |  |
| PASS | slot tile assignment matches allocated quota |  |
| PASS | slot tile assignment matches allocated quota |  |
| PASS | slot tile assignment matches allocated quota |  |
| PASS | sum of node tile quotas exactly equals total active tiles |  |
| PASS | initial remaining tiles equals total |  |
| PASS | scheduler begins frame successfully |  |
| PASS | slot tile assignment matches allocated quota |  |
| PASS | slot tile assignment matches allocated quota |  |
| PASS | slot tile assignment matches allocated quota |  |
| PASS | slot tile assignment matches allocated quota |  |
| PASS | sum of node tile quotas exactly equals total active tiles |  |
| PASS | initial remaining tiles equals total |  |

## Capacity Share Convergence & Latency Dampening

| Result | Check | Detail |
| --- | --- | --- |
| PASS | capacity shares compute successfully |  |
| PASS | capacity shares are strictly normalized to 1.0 | actual=1 expected=1 abs_error=0 tolerance=1e-12 |
| PASS | node 0 capacity share matches analytical formulation | actual=0.750469043152 expected=0.750469043152 rel_error=0 tolerance=1e-10 |
| PASS | node 1 capacity share matches analytical formulation | actual=0.240150093809 expected=0.240150093809 rel_error=0 tolerance=1e-10 |
| PASS | node 2 capacity share matches analytical formulation | actual=0.0093808630394 expected=0.0093808630394 rel_error=0 tolerance=1e-10 |

## Deadline Timeout Reclamation & Work Stealing

| Result | Check | Detail |
| --- | --- | --- |
| PASS | Scheduler initializes frame with remote and local nodes |  |
| PASS | remote slot receives non-empty tile quota |  |
| PASS | mark dispatched succeeds for remote tile |  |
| PASS | tile status is InFlight after dispatch |  |
| PASS | no tiles reclaimed prior to deadline expiration |  |
| PASS | stalled tile successfully reclaimed after deadline expiration |  |
| PASS | reclaimed tile index matches target |  |
| PASS | reclaim reason is Timeout |  |
| PASS | tile status transitioned to Reclaimed |  |
| PASS | reclaimed tile qualified for immediate local execution |  |
| PASS | local completion of stolen tile succeeds |  |
| PASS | tile status is CompletedLocal |  |
| PASS | late remote response ignored without double count |  |
| PASS | late discard telemetry incremented |  |

## Concurrent Peer Registry Access

| Result | Check | Detail |
| --- | --- | --- |
| PASS | no registration failures under concurrent multi-threaded stress |  |
| PASS | all nodes cleaned up symmetrically |  |

