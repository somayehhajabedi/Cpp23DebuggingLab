# Scenario 07 — Simulation Hangs Indefinitely With No Error Output

**Affected branch:** `scenario/07`
**Reported by:** Desk user, running the regression suite locally

## Summary

Occasionally, either the simulation binary or the regression test suite simply stops making
progress. No crash, no error message, no sanitizer report — the process is still running, CPU
usage may be at or near zero, and it never finishes on its own. It has to be killed manually.

## Steps to Reproduce

1. `git checkout scenario/07`
2. `cmake --preset debug && cmake --build --preset debug`
3. Run the regression suite several times: for i in 1 2 3 4 5; do ctest --preset debug; done
4. If it doesn't reproduce quickly, try the `tsan` preset — it's worth noting that even when the
   process doesn't actually freeze on a given run, ThreadSanitizer may still print a diagnostic
   about the situation that leads to the freeze.

## Expected Behavior

Every run of the simulation and every test in the regression suite should run to completion in a
bounded amount of time, every time, regardless of how threads happen to get scheduled.

## Actual Behavior

On some runs, a test (or the simulation binary directly) never returns. The CI configuration for
this project enforces a timeout on every test specifically so that this failure mode shows up as a
clear timeout rather than a silently-stuck job.

## Notes

This will not reproduce on every single run — that unpredictability is itself part of the report.
Running the same test repeatedly, and/or under the `tsan` preset, is the most effective way to
catch it.
