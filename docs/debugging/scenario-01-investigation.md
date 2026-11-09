# Scenario 01 — Order Book Debugging Investigation

**Project:** Cpp23DebuggingLab  
**Branch:** `scenario/01`  
**Status:** Root cause confirmed; fix verified.  
**Tools:** C++23, CMake, GoogleTest, CTest, GDB 17.1

## 1. Bug Report

After order cancellation or replacement, the order book may report a best bid price that no longer has any resting quantity.

**Expected:** Empty price levels must not contribute to Best Bid or Best Ask.

**Actual:** A price level with zero quantity remains in the order book.

## 2. Reproduction

Commands:

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
```

**Observed test results:**

- Total tests: 57
- Passed: 55
- Failed: 2

### Failed Test 1

`OrderBookTest.CancelRemovesOrderAndEmptyLevel`

```text
Expected: book.bestBid().has_value() == false
Actual:   book.bestBid().has_value() == true
```

### Failed Test 2

`OrderBookTest.ReplaceClearsOldPriceLevelWhenEmptied`

```text
Expected Best Bid: 99.50
Actual Best Bid:   100
```

## 3. Investigation

Located relevant functions using:

```bash
rg -n 'cancelOrder|replaceOrder|bestBid' include src
```

Relevant source file:

`src/orderbook/OrderBook.cpp`

The investigation focused on `OrderBook::removeFromLevel()`.

## 4. GDB Debugging Session

Started GDB:

```bash
gdb build/debug/tests/mdlab_tests
```

Selected the failing test:

```gdb
set args --gtest_filter=OrderBookTest.CancelRemovesOrderAndEmptyLevel
break mdlab::orderbook::OrderBook::removeFromLevel
run
```

**Breakpoint reached:**

```text
OrderBook::removeFromLevel(
    side=Buy,
    price=100,
    quantity=10
)
```

Inspected quantity before subtraction:

```text
(gdb) print it->second
$2 = 10
```

After stepping past the subtraction, inspected the price-level container:

```text
(gdb) print bids_

keys   = {100}
values = {0}
```

This confirmed that the quantity reached zero while the price level remained in the container.

## 5. Root Cause

`removeFromLevel()` subtracts the cancelled quantity but does not remove the price level when its aggregate quantity becomes zero.

The order book uses `std::flat_map` (C++23).

`bestBid()` checks whether the container is empty and then returns its first key. A zero-quantity level therefore remains eligible as the reported best price.

## 6. Proposed Fix

Remove the price level when its remaining quantity becomes zero, for both bid and ask sides.

The relevant operation is:

```cpp
container.erase(iterator);
```

The fix was implemented in both the Buy and Sell branches of `removeFromLevel()`. The project was rebuilt successfully, and both previously failing tests passed.


## 9. Fix and Verification Results

**Fix:** Updated `OrderBook::removeFromLevel()` to erase a price level when its remaining quantity becomes zero.

**Verification:**
- Debug build completed successfully.
- Both previously failing tests passed.
- Full regression suite: 57/57 tests passed.
- No regression failures observed.

**Current outcome:** Root cause confirmed using GDB. Fix implemented and verified successfully.
