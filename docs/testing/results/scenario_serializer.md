# scenario_serializer

Verifies the declarative scenario reader and writer: field fidelity across a serialization round trip and the validation rules for names, metrics, mass and spin.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 20 |
| Failed checks | 0 |
| Duration | 0.001868 s |
| Generated | 2026-10-19 22:21:09 UTC |

## YAML round trip

| Result | Check | Detail |
| --- | --- | --- |
| PASS | serialization produces a document |  |
| PASS | the scenario name is written as a quoted string |  |
| PASS | the spin is written with its value |  |
| PASS | the serialized document parses back |  |
| PASS | the scenario name survives the round trip |  |
| PASS | the metric type survives the round trip |  |
| PASS | the central mass survives the round trip | actual=10 expected=10 abs_error=0 tolerance=1e-12 |
| PASS | the central spin survives the round trip | actual=0.95 expected=0.95 abs_error=0 tolerance=1e-12 |
| PASS | the integrator scheme survives the round trip |  |
| PASS | the initial step survives the round trip | actual=0.005 expected=0.005 abs_error=0 tolerance=1e-12 |
| PASS | the relative tolerance survives the round trip | actual=1e-12 expected=1e-12 rel_error=0 tolerance=1e-09 |
| PASS | the export switches survive the round trip |  |

## Validation rules

| Result | Check | Detail |
| --- | --- | --- |
| PASS | a sub-extremal Kerr scenario is valid |  |
| PASS | a Schwarzschild scenario with positive mass is valid |  |
| PASS | an empty scenario name is rejected |  |
| PASS | an empty metric type is rejected |  |
| PASS | an unknown metric is rejected with a message |  |
| PASS | a Schwarzschild scenario with zero mass is rejected |  |
| PASS | a Kerr scenario exceeding the extremal spin is rejected |  |
| PASS | the extremal spin is accepted within the documented tolerance |  |

