# Scenario 02 — Feed Handler Crashes on a Malformed Input Line

**Affected branch:** `scenario/02`
**Reported by:** Ops, during a feed-format migration test

## Summary

While testing how the feed handler copes with occasional bad rows from an upstream data vendor
(truncated lines, missing fields), the simulation process crashes outright with a segmentation
fault instead of logging a parse error and continuing.

## Steps to Reproduce

1. `git checkout scenario/02`
2. `cmake --preset debug && cmake --build --preset debug`
3. Run the regression suite: `ctest --preset debug`
4. Or run the simulation directly against the malformed-input fixture:
   `./build/debug/simulation data/malformed_ticks.csv`

## Expected Behavior

A malformed or truncated line is reported as a parse error (counted and logged) and the feed
handler moves on to the next line. No line of input, however short or garbled, should be able to
crash the process.

## Actual Behavior

The process terminates with a segmentation fault partway through the malformed-input fixture.

## Notes

If the crash doesn't reproduce immediately on a plain debug build, try the `asan-ubsan` preset —
sanitizer-instrumented builds tend to surface memory-safety issues more consistently and with a
more precise crash location.
