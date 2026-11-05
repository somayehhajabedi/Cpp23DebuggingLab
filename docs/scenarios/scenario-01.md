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
ask price (or one/both sides are simply absent), and the reported best price on either side should
always reflect currently-resting quantity — never a price level that's already been fully
cancelled or replaced away.

## Actual Behavior

After add/cancel/replace activity on a symbol, the book can report a best price that no longer has
any real resting quantity behind it, and in some sequences this surfaces as a best bid reported
above the best ask. Some tests in the regression suite that exercise cancel/replace behavior fail.

## Notes

This reproduces reliably and deterministically — it does not require any particular timing, thread
count, or sanitizer. A plain debug build is sufficient.
