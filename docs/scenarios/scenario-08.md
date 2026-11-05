# Scenario 08 — Feed Processing Has Become Much Slower

**Affected branch:** `scenario/08`
**Reported by:** Desk user, noticing a sluggish simulation run

## Summary

Processing a given feed through the simulation used to finish quickly. On this branch, the exact
same workload takes dramatically longer — and CPU usage during the run looks disproportionate to
the amount of data being processed.

## Steps to Reproduce

1. `git checkout scenario/08`
2. `cmake --preset release && cmake --build --preset release`
3. Run the regression suite: `ctest --preset release`
4. Pay attention to how long the full suite takes, and in particular any single test that seems to
   dominate the total time. You can also time the simulation binary directly against a larger
   generated feed if you want to see the effect more dramatically.

## Expected Behavior

There's a regression test in the suite that budgets a generous but bounded amount of wall-clock
time for processing a sizeable feed. That budget should be met comfortably, as it is on
`baseline`.

## Actual Behavior

The same test now blows well past its time budget and fails.

## Notes

This is a release-build (optimized) concern first and foremost — always compare timings using the
`release` preset, not `debug`, since debug-build timings are not representative of real
performance characteristics.
