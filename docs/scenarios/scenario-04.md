# Scenario 04 — Heap Corruption Reported Under Sustained Load

**Affected branch:** `scenario/04`
**Reported by:** Infra, during a soak test

## Summary

When the system is driven with a large, sustained volume of market data (well beyond the small
sample feed shipped in `data/`), the process either crashes with a heap-corruption-style error or
starts producing clearly-corrupted data partway through the run. Short runs on the small sample
feed look fine.

## Steps to Reproduce

1. `git checkout scenario/04`
2. `cmake --preset asan-ubsan && cmake --build --preset asan-ubsan`
3. Run the regression suite: `ctest --preset asan-ubsan`
4. The relevant test drives sustained high-volume traffic through the system and checks the data
   that comes out the other end for corruption; it's also worth trying the plain `debug` preset to
   see how it behaves without instrumentation.

## Expected Behavior

Regardless of sustained volume, every item that goes into the system should come out the other
side unmodified and in order. The regression test checks this with a built-in integrity check and
should pass cleanly under every preset.

## Actual Behavior

Under sustained load, the test either fails its integrity check, crashes, or (most informatively)
AddressSanitizer reports a heap-buffer-overflow or similar memory-corruption diagnostic.

## Notes

This is a volume-dependent issue: short bursts of traffic are not enough to trigger it reliably.
Give the sustained-load test enough iterations to run to completion.
