# Scenario 01 — Top-of-Book Occasionally Reports Bid Above Ask

**Affected branch:** `scenario/01`
**Reported by:** Desk user, via the end-of-run order book printout

## Summary

After a sequence of order cancellations and replacements on a symbol, the final order book
printout occasionally shows a best bid price that is *higher* than the best ask price for the
same symbol. A healthy book should never report this.

## Steps to Reproduce

1. `git checkout scenario/01`
2. `cmake --preset debug && cmake --build --preset debug`
3. Run the regression suite: `ctest --preset debug`
4. Alternatively, run the simulation directly against the sample feed and inspect the final book
   printout: `./build/debug/simulation data/sample_ticks.csv`

## Expected Behavior

For every symbol, at every point in time, the best bid price is always strictly less than the best
ask price (or one/both sides are simply absent). A test in the regression suite drives a long,
varied sequence of order activity and asserts this never happens.

## Actual Behavior

After enough add/cancel/replace activity on a symbol, the book reports a best bid price greater
than the best ask price for that symbol. The regression test that checks this invariant fails.

## Notes

This reproduces reliably and deterministically — it does not require any particular timing, thread
count, or sanitizer. A plain debug build is sufficient.
