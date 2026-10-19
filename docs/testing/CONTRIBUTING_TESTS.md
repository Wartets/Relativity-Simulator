# Rules for writing tests

This guide is mandatory for every test and benchmark. No external test framework, assertion macro or ad-hoc report format is allowed.

## File placement

| Item | Location |
| --- | --- |
| Functional test | `tests/test_<topic>.cpp` |
| Benchmark | `tests/benchmark_<topic>.cpp` |
| Shared harness | `tests/support/test_harness.hpp` |
| Scenario generation | `tests/support/engine_scenario.hpp` |
| Metric invariant checks | `tests/support/metric_checks.hpp` |
| Suite specification | `docs/testing/suites/<suite_name>.md` |
| Generated reports | `docs/testing/results/<suite_name>.md` and `.json` |
| Status ledger | `docs/testing/test_catalog.md` |

`<suite_name>` is the snake_case identifier passed to `TestSuite`.

## Harness lifecycle

- `main` builds a `TestSuite` with the suite identifier and a plain-text description of the verified invariants, groups work with `section`, and returns `finish()`.
- Assertions use only `check`, `check_near`, `check_relative`, `check_at_most` and `check_finite`. `assert`, exceptions and `abort` are forbidden.
- Every label states the expected property in plain words. Numeric helpers expose actual, expected and error in the report.
- `measure` records numbers worth keeping (timings, ratios, statistics); `note` records context.
- `finish` writes the markdown and json reports into the directory named by `RELATIVISTIC_TEST_REPORT_DIR` (`./test_reports` when unset) and returns 0 or 1.

## Determinism and tolerances

- Tests never read clocks, process identifiers or unseeded random state for their inputs. Use `Testing::DeterministicRandom` from `engine_scenario.hpp` with an explicit hexadecimal seed, or fixed data.
- Tolerances are scale aware: relative, or relative to the largest value of the same row with an explicit floor. Never use `max(1, |value|)` for quantities that may be orders of magnitude below one.
- Report the offending indices when a tensor comparison fails.
- Metric tests call `verify_metric_geometry` from `tests/support/metric_checks.hpp` for the inversion, symmetry, signature and connection invariants instead of re-implementing them.
- Geometric suites verify the invariants listed in the suite pages: metric symmetry, negative determinant, lower-index symmetry of the connection, analytic against eighth-order finite-difference derivatives, vanishing Ricci quantities in vacuum, the Kretschmann invariant and the null constraint along rays.

## Benchmarks

- Correctness is separate from timing. A benchmark fails on wrong results, never on speed alone unless a documented ceiling exists.
- Discard a warm-up pass, then report minimum, maximum, median, mean and standard deviation, and state the hardware and path.
- Compare integrators against an independent scalar reference lane by lane (position, four-momentum, affine parameter, lifecycle masks).
- Keep one pipeline per resolution, check that the frame is complete, finite and opaque, check horizon pixels where expected, and read the telemetry to detect a GPU fallback.

## CMake

- Register each executable with `add_executable`, link `relativistic_core` (plus `imgui_lib` for interface logic) and add `add_test`.
- Benchmarks with profiles register a `--profile quick` CTest entry and a custom target with `USES_TERMINAL` for the full matrix.
- The environment variable `RELATIVISTIC_TEST_REPORT_DIR` is bound to `docs/testing/results` for every registered test.

## Documentation

Every maintained test has a suite page with: executable, source, link to its generated result, purpose, covered metrics or modes, a table of checks with tolerances and their justification, scenario parameters, limits and acceptance criteria. Move the test from Legacy to Maintained in [test_catalog.md](test_catalog.md) and list it in [README.md](README.md). Every suite page ends with a "Recorded results" section linking to `docs/testing/results/<suite_name>.md` and `.json`, and every check of the test must appear in the check table of its page with its tolerance and justification. A test without a suite page, a catalogue entry and committed results is not considered maintained. Commit the generated reports in the same commit as any change that alters a test or the behaviour it covers.

## Review checklist

1. The test fails when the property is broken, not only when it compiles.
2. Tolerances detect regressions yet allow supported compilers and hardware.
3. The suite page, catalogue and results index are updated.
4. The generated result files are committed.
