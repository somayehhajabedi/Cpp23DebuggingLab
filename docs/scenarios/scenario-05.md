# Scenario 05 — Consumer Observes a Price That Was Never Sent

**Affected branch:** `scenario/05`
**Reported by:** Desk user, comparing logged prices against the input feed

## Summary

Under high-throughput conditions, a value observed on the consuming side of the pipeline
occasionally doesn't match any value that was actually sent by the producer — not a dropped
value, not a duplicate, but a value that appears to be a mix of two different updates. It is
intermittent and does not reproduce on every run.

## Steps to Reproduce

1. `git checkout scenario/05`
2. `cmake --preset tsan && cmake --build --preset tsan`
3. Run the regression suite several times: `ctest --preset tsan --repeat until-fail:5`
   (or just re-run `ctest --preset tsan` a handful of times)
4. It is also worth comparing behavior against the plain `debug` preset and the `asan-ubsan`
   preset.

## Expected Behavior

Every value that comes out of the producer/consumer handoff should be exactly one of the values
that was put in, in the order it was put in — no exceptions, regardless of load.

## Actual Behavior

Under sustained throughput, the consumer-side integrity check in the regression suite occasionally
fails, having observed a value that doesn't match the expected next value in sequence.

## Notes

Intermittent and timing-sensitive. If a single run looks clean, that does not mean the issue is
gone — try several repeated runs, and try each of the non-release presets.
