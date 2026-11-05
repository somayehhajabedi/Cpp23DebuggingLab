# Scenario 03 — Garbage Output / Crash After an Order Fills

**Affected branch:** `scenario/03`
**Reported by:** Desk user, running an extended simulation session

## Summary

During a longer-running simulation with a steady stream of orders and fills across several
symbols, the process occasionally logs obviously-wrong values (garbage numbers, corrupted symbol
names) shortly after a fill, or crashes outright. It does not happen on every run and does not
happen with small/short input.

## Steps to Reproduce

1. `git checkout scenario/03`
2. `cmake --preset debug && cmake --build --preset debug`
3. Run the regression suite: `ctest --preset debug`
4. If it doesn't reproduce on the first try, re-run the suite a few times, or build with the
   `asan-ubsan` preset and re-run — `cmake --preset asan-ubsan && cmake --build --preset asan-ubsan
   && ctest --preset asan-ubsan`

## Expected Behavior

Order fills should never affect how later market data for the same (or any other) symbol is
processed or logged. Output should stay well-formed for the entire run, regardless of how many
orders have already been filled.

## Actual Behavior

Under sustained order/fill activity, the regression test covering this area occasionally fails,
reports clearly-corrupted values, or the test binary crashes.

## Notes

This one can be sensitive to run-to-run timing — it may not reproduce on every single invocation
of the regression suite. Repeated runs, and sanitizer-instrumented builds in particular, make it
much easier to catch.
