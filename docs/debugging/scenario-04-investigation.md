# Scenario 04 — SPSC Queue Buffer Out-of-Bounds

## 1. Problem Description

Four tests failed in the C++23 debugging lab:

- `SPSCQueueTest.FullQueueRejectsPush`
- `SPSCQueueTest.WrapsAroundCorrectly`
- `QueueDataIntegrityTest.SustainedLoadPreservesOrderAndContent`
- `SimulationThroughputSmokeTest.ProcessesALargeFeedWithinBudget`

The initial Debug test run reported **53 passed and 4 failed out of 57 tests**.

The failing test aborted with a C++ standard library assertion:

```text
std::vector::operator[]:
Assertion '__n < this->size()' failed.
```

This suggested an out-of-bounds vector access.

## 2. Reproducing the Failure

Run the Debug build:

```bash
cmake --preset debug -S /home/shajabedi/Projects/Cpp23DebuggingLab
cmake --build /home/shajabedi/Projects/Cpp23DebuggingLab/build/debug
```

Run the failing test in isolation:

```bash
ctest --test-dir /home/shajabedi/Projects/Cpp23DebuggingLab/build/debug \
  -R '^SPSCQueueTest\.FullQueueRejectsPush$' \
  --output-on-failure
```

The test aborted due to the vector bounds assertion.

## 3. GDB Investigation

### Step 1 — Start GDB

```bash
gdb --args /home/shajabedi/Projects/Cpp23DebuggingLab/build/debug/tests/mdlab_tests \
  --gtest_filter=SPSCQueueTest.FullQueueRejectsPush
```

### Step 2 — Run the test

```gdb
run
```

GDB stopped after the process received `SIGABRT`.

The abort was triggered by the failed vector bounds assertion.

### Step 3 — Inspect the call stack

```gdb
bt
```

Relevant frames:

```text
#6 std::vector<int>::operator[] (__n=2)
#7 mdlab::util::SPSCQueue<int>::tryPush (value=3)
   at include/mdlab/util/SPSCQueue.hpp:31
#8 SPSCQueueTest_FullQueueRejectsPush_Test::TestBody
   at tests/SPSCQueueTest.cpp:32
```

The backtrace identified the failing operation:

```cpp
buffer_[head] = std::move(value);
```

### Step 4 — Select the application frame

```gdb
frame 7
```

This selected the `SPSCQueue<int>::tryPush()` stack frame.

### Step 5 — Inspect the index

```gdb
print head
```

Result:

```text
2
```

### Step 6 — Inspect the allocated buffer size

```gdb
print buffer_.size()
```

Result:

```text
2
```

The vector contained two elements, so only indices `0` and `1` were valid.

The code attempted to access index `2`, which was outside the vector's bounds.

### Step 7 — Inspect the internal ring capacity

```gdb
print capacity_
```

Result:

```text
3
```

The queue's internal ring capacity was larger than the allocated vector.


This confirmed the inconsistency responsible for the invalid access.

## 4. Root Cause Analysis

File:

```text
include/mdlab/util/SPSCQueue.hpp
```

Original constructor:

```cpp
explicit SPSCQueue(std::size_t capacity)
    : capacity_(capacity + 1), buffer_(capacity) {}
```

The implementation uses a ring buffer with one additional slot to distinguish a full queue from an empty queue.

The index increment operation is:

```cpp
return (idx + 1) % capacity_;
```

For a requested capacity of `2`:

```text
capacity_     = 3
buffer_.size() = 2
```

The index calculation can produce `0`, `1`, or `2`, but the vector only allocates storage for indices `0` and `1`.

This mismatch causes an out-of-bounds access when the queue reaches the unallocated slot.

The four failing tests exercised different behaviors that depended on the same underlying queue implementation.

## 5. Fix

Updated constructor:

```cpp
explicit SPSCQueue(std::size_t capacity)
    : capacity_(capacity + 1),
      buffer_(capacity + 1) {}
```

The internal capacity and allocated buffer size now match.

For a requested capacity of `2`, the implementation allocates three slots while retaining a usable queue capacity of two elements.

No changes to the queue's public API were required.

## 6. Verification

### Debug build

Rebuild:

```bash
cmake --build /home/shajabedi/Projects/Cpp23DebuggingLab/build/debug
```

Run all tests:

```bash
ctest --test-dir /home/shajabedi/Projects/Cpp23DebuggingLab/build/debug \
  --output-on-failure
```

Result:

```text
100% tests passed, 0 tests failed out of 57
```

### AddressSanitizer and UndefinedBehaviorSanitizer

Configure:

```bash
cmake --preset asan-ubsan \
  -S /home/shajabedi/Projects/Cpp23DebuggingLab
```

Build:

```bash
cmake --build /home/shajabedi/Projects/Cpp23DebuggingLab/build/asan-ubsan
```

Run:

```bash
ctest --test-dir /home/shajabedi/Projects/Cpp23DebuggingLab/build/asan-ubsan \
  --output-on-failure
```

Result: all tests passed, with no sanitizer failures reported during the test run.

## 7. Lessons Learned

1. Multiple failing tests can share a single underlying root cause.
2. A ring buffer's logical capacity and physical storage requirements must be consistent.
3. GDB backtraces help identify the application frame responsible for a crash.
4. Inspecting runtime values provides concrete evidence instead of relying on assumptions.
5. Fixing the root cause is preferable to modifying individual failing tests.
6. Passing unit tests should be supplemented with sanitizer-based verification.
7. Sanitizer success increases confidence but does not prove the absence of all memory or concurrency bugs.

## 8. Final Outcome

**Root cause:** Incorrect vector allocation size in the `SPSCQueue` constructor.

**Fix:** Allocate `capacity + 1` elements to match the ring buffer's internal indexing.

**Verification:** All 57 tests passed in both Debug and ASan/UBSan builds.

**Status:** Fixed and verified.
