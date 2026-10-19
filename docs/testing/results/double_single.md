# double_single

Verifies the compensated double-single arithmetic that emulates extended precision with two single-precision floats: conversion round trip, the four arithmetic operations, square root and sine, each against native double precision.

| Property | Value |
| --- | --- |
| Status | PASSED |
| Checks | 12 |
| Failed checks | 0 |
| Duration | 0.001128 s |
| Generated | 2026-10-19 22:19:18 UTC |

## Conversion

| Result | Check | Detail |
| --- | --- | --- |
| PASS | a double survives the split and recombination | actual=1.23456789012 expected=1.23456789012 abs_error=2.22e-16 tolerance=1e-13 |
| PASS | a second double survives the split and recombination | actual=9.87654321099 expected=9.87654321099 abs_error=1.776e-15 tolerance=1e-13 |

## Arithmetic

| Result | Check | Detail |
| --- | --- | --- |
| PASS | addition matches native double precision | actual=11.1111111011 expected=11.1111111011 rel_error=2.078e-15 tolerance=1e-13 |
| PASS | subtraction matches native double precision | actual=8.64197532086 expected=8.64197532086 rel_error=2.055e-16 tolerance=1e-13 |
| PASS | multiplication matches native double precision | actual=12.1932631137 expected=12.1932631137 rel_error=4.371e-16 tolerance=1e-13 |
| PASS | division matches native double precision | actual=8.0000000729 expected=8.0000000729 rel_error=2.22e-16 tolerance=1e-13 |

## Functions

| Result | Check | Detail |
| --- | --- | --- |
| PASS | square root matches native double precision | actual=11.7062376144 expected=11.7062376144 rel_error=7.587e-16 tolerance=1e-13 |
| PASS | sine matches native double precision | actual=0.944005725045 expected=0.944005725045 rel_error=1.176e-16 tolerance=1e-13 |
| PASS | cosine matches native double precision | actual=0.329929069774 expected=0.329929069774 rel_error=1.683e-15 tolerance=1e-13 |

## Ordering

| Result | Check | Detail |
| --- | --- | --- |
| PASS | comparison operators order values consistently |  |
| PASS | the absolute value removes the sign |  |
| PASS | minimum and maximum select the expected operands |  |

