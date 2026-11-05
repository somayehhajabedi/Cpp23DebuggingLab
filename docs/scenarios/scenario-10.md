# Scenario 10 — Very Large Order Produces a Nonsensical Notional Value

**Affected branch:** `scenario/10`
**Reported by:** Risk desk, stress-testing with an unusually large order

## Summary

An order with an unusually large (but not unreasonable for a stress test) quantity was submitted
to the risk manager to see how it handled the edge of its input range. Instead of computing a
large notional value and rejecting the order for exceeding the notional limit, the risk manager
computed a notional value that doesn't make sense — in particular, it was not the large positive
number it should have been.

## Steps to Reproduce

1. `git checkout scenario/10`
2. `cmake --preset asan-ubsan && cmake --build --preset asan-ubsan`
3. Run the regression suite: `ctest --preset asan-ubsan`
4. The relevant test submits an order with a deliberately large quantity and checks both that the
   resulting notional is a large positive number and that the order is correctly rejected for
   exceeding the configured notional limit.

## Expected Behavior

Notional exposure for an order is quantity times price. For a large quantity at a normal price,
that should be a large *positive* number, and an order whose notional genuinely exceeds the
configured limit should always be rejected — regardless of how large the quantity is.

## Actual Behavior

For a sufficiently large quantity, the computed notional comes out wrong (and in particular, is
not the large positive value the quantity and price would imply), and the corresponding test
fails — the order is approved when it should have been rejected.

## Notes

UndefinedBehaviorSanitizer in particular is useful here — if the underlying computation does
something it shouldn't for a large input, UBSan tends to flag the exact operation at the moment it
happens.
