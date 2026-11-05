# Scenario 09 — Order Exactly at the Risk Limit Is Handled Inconsistently

**Affected branch:** `scenario/09`
**Reported by:** Risk desk, reviewing edge-case behavior

## Summary

The risk manager has a documented convention for what happens when an order would bring some
tracked quantity (order size, notional, or position) to *exactly* its configured limit, versus one
unit past it. On this branch, that boundary isn't being honored consistently — the documented
convention and the actual behavior disagree for at least one of the limit checks.

## Steps to Reproduce

1. `git checkout scenario/09`
2. `cmake --preset debug && cmake --build --preset debug`
3. Run the regression suite: `ctest --preset debug`
4. Look specifically at the boundary-focused tests — they construct orders that land exactly on a
   configured limit and one unit past it, for each limit the risk manager enforces.

## Expected Behavior

A value exactly at a configured limit is allowed; a value that exceeds the limit by even one unit
is rejected. This should hold for every limit the risk manager checks (order quantity, notional,
and position).

## Actual Behavior

For at least one of those checks, a value that should be allowed at the boundary is rejected, or a
value that should be rejected is allowed instead.

## Notes

This is fully deterministic — no concurrency or sanitizer involved. The boundary tests are
designed to pin down exactly which side of "at vs. over the limit" the behavior falls on.
