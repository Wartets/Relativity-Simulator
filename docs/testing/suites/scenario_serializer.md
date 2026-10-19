# Scenario serializer suite

Executable: `test_scenario_serializer` - Source: `tests/test_scenario_serializer.cpp` - Results: [scenario_serializer.md](../results/scenario_serializer.md)

## Purpose

Verifies `ScenarioSerializer`, which reads and writes the declarative files of [FILE_FORMATS.md](../../FILE_FORMATS.md#1-declarative-scenario-definition-yaml), including the validation rules of [Section 1.2](../../FILE_FORMATS.md#12-parsing-validation-and-application-rules).

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Round trip | Document content | The quoted scenario name and the spin appear in the YAML text |
| Round trip | Fidelity | Name, metric, mass (1e-12), spin (1e-12), integrator scheme, initial step (1e-12), relative tolerance (relative 1e-9) and export switches survive |
| Validation | Accepted | Kerr with a = 0.95 M, Schwarzschild with positive mass, extremal Kerr within the documented 1.0001 tolerance |
| Validation | Rejected | Empty name, empty metric, unknown metric (with a message), zero mass, Kerr spin of 1.5 M |

## Limits

The suite does not cover bodies, observers or interactions; those are exercised through the orchestrator in [default startup scenario](../test_catalog.md) tests.

## Acceptance

All checks pass.
