# High-order integrators suite

Executable: `test_high_order_integrators` - Source: `tests/test_high_order_integrators.cpp` - Results: [high_order_integrators.md](../results/high_order_integrators.md)

## Purpose

Verifies the embedded Cash-Karp 5(4) and Vernier 9(8) integrators of [ARCHITECTURE.md](../../ARCHITECTURE.md#23-differential-solvers--geodesic-integrators).

## Scenario

Circular timelike orbits in Schwarzschild: radius 12 over 50 units of proper time with Cash-Karp (rtol 1e-12, atol 1e-14) and radius 15 over 100 units with Vernier (rtol 1e-13, atol 1e-15).

## Checks

| Section | Check | Criterion |
| --- | --- | --- |
| Cash-Karp 5(4) | Steps | Every adaptive step succeeds and at least one is accepted |
| Cash-Karp 5(4) | Radius | Stays at 12 within 1e-6 |
| Vernier 9(8) | Steps | Every adaptive step succeeds and at least one is accepted |
| Vernier 9(8) | Radius | Stays at 15 within 1e-7 |

## Justification

On a circular geodesic the radius is constant, so its drift measures the accumulated truncation error directly. The Vernier tolerance is tighter because of its higher order.

## Acceptance

All checks pass.

## Recorded results

See [../results/high_order_integrators.md](../results/high_order_integrators.md).
