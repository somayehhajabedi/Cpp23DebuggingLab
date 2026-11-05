# Scenario 04 — Heap Corruption Reported Under Sustained Load

**Affected branch:** `scenario/04`
**Reported by:** Infra, during a soak test

## Summary

The queue used to hand data between pipeline stages is supposed to be a safe, bounded ring buffer:
pushes against a full queue should simply be rejected, never corrupt memory. On this branch,
exercising the queue anywhere near its capacity boundary — not necessarily under heavy sustained
load, just hitting the edge of the buffer — triggers a crash or a corruption report.

## Steps to Reproduce

1. `git checkout scenario/04`
2. `cmake --preset debug && cmake --build --preset debug`
3. Run the regression suite: `ctest --preset debug`
4. Also worth trying under `asan-ubsan` for a more precise diagnostic:
   `cmake --preset asan-ubsan && cmake --build --preset asan-ubsan && ctest --preset asan-ubsan`

## Expected Behavior

Every item pushed into the queue should come out the other side unmodified and in order, and the
queue should never read or write outside its own backing storage, no matter how full it gets or
how long it's been running.

## Actual Behavior

Tests that push the queue to its capacity boundary — including a basic fixed-size test, not just
the sustained-load one — crash or abort instead of completing normally.

## Notes

This does not require a long soak — it reproduces quickly on a small, fixed-size queue once it's
driven to (or near) full capacity. The sustained-load test just makes it unmissable.
