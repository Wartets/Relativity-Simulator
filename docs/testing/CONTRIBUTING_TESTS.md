# Rules for writing tests

## Mandatory

- Every test uses `support/test_harness.hpp`: sections for structure, `check*` for assertions, `measure` for numbers worth recording, `finish()` as the return value of `main`.
- Every check has a label that states the property in plain words; numeric checks use the tolerance helpers so values appear in the report.
- Tests are deterministic. Use `Testing::DeterministicRandom` or fixed data, never time or global state.
- Tolerances are scale aware (relative, or relative with a stated floor) and justified in the suite page.
- Every test is registered in `CMakeLists.txt`, receives the `RELATIVISTIC_TEST_REPORT_DIR` environment property, and appears in `docs/testing/test_catalog.md`.
- Every maintained test has a page in `docs/testing/suites/` covering purpose, scenario, every check with its criterion, limits, and a link to its generated result.
- A change that alters a test or its behaviour refreshes the documented results in the same commit.

## Benchmarks

- Report medians, minimum, maximum and standard deviation, and always state the hardware and path.
- Correctness assertions are separate from timing; a benchmark fails on wrong results, never on speed alone, unless a documented threshold exists.

## Review checklist

1. The test fails when the property is broken, not only when it compiles.
2. Tolerances are tight enough to detect regressions and loose enough for the supported compilers and hardware.
3. The suite page and catalogue are updated.
4. The generated result file is committed.
