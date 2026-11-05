# Scenario 06 — Risk Limit Is Sometimes Not Enforced Under Load

**Affected branch:** `scenario/06`
**Reported by:** Risk desk, during a concurrent order-flow test

## Summary

With several order-entry paths submitting concurrently against the same symbol, the risk manager
is supposed to never let the net position for that symbol exceed its configured limit. Under
concurrent load, it occasionally does anyway. Sequential (non-concurrent) order flow never
triggers this.

## Steps to Reproduce

1. `git checkout scenario/06`
2. `cmake --preset tsan && cmake --build --preset tsan`
3. Run the regression suite several times: `ctest --preset tsan --repeat until-fail:5`
4. Also worth trying under the plain `debug` preset with several repeats — this does not strictly
   require sanitizer instrumentation to show up, just concurrency and enough iterations.

## Expected Behavior

No matter how many threads are concurrently submitting fills and risk checks against the same
symbol, every snapshot of that symbol's position that any caller can observe should be internally
consistent, and the net position should never be allowed to exceed the configured limit.

## Actual Behavior

Under concurrent load, the regression test occasionally observes an inconsistent position
snapshot, which is exactly the kind of thing that would let a risk decision be made on bad data.

## Notes

This is inherently timing-sensitive — thread scheduling affects whether and how often it shows up.
More iterations and more concurrent threads make it easier to reproduce.
