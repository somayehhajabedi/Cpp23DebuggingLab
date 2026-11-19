# Scenario 05 — Data Race in a Lock-Free SPSC Queue

## 1. Overview

This scenario investigates a concurrency bug in a multithreaded C++23 market data trading simulation.

The underlying issue was incorrect atomic memory ordering in a lock-free Single-Producer/Single-Consumer (SPSC) ring buffer.

Although the application passed all functional tests under a standard Debug build, ThreadSanitizer detected unsafe concurrent access to shared memory.

The investigation demonstrates how incorrect synchronization can introduce undefined behavior even when atomic variables are used.

## 2. Investigation Objectives

- Detect data races using ThreadSanitizer.
- Interpret sanitizer reports and thread stack traces.
- Identify conflicting memory accesses.
- Understand the relationship between atomic operations and memory ordering.
- Establish correct happens-before relationships.
- Preserve the existing lock-free architecture.
- Distinguish concurrency failures from performance-related failures.
- Validate the fix through regression testing.

## 3. Tools and Technologies

| Tool | Purpose |
|---|---|
| C++23 | Application implementation |
| ThreadSanitizer (TSan) | Dynamic data race detection |
| CMake | Build configuration |
| Ninja | Incremental compilation |
| Google Test | Unit and integration testing |
| CTest | Test execution |
| Git | Source code investigation and change verification |
| Linux | Development and execution environment |

## 4. Switching to the Scenario Branch

```bash
git switch scenario/05
```

This command switches the repository to the branch containing the intentional concurrency bug.

## 5. Initial Debug Build

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
```

The Debug build completed successfully, and all 57 tests passed.

However, passing functional tests does not guarantee thread safety.

Concurrency bugs may remain undetected because their manifestation depends on thread scheduling, CPU architecture, compiler optimizations, and memory visibility.

## 6. Identifying Modified Components

```bash
git diff baseline...HEAD --stat
```

Output:

```text
include/mdlab/util/SPSCQueue.hpp | 10 +++++-----
1 file changed, 5 insertions(+), 5 deletions(-)
```

This narrowed the investigation to the SPSC queue implementation.

## 7. Configuring ThreadSanitizer

```bash
cmake --preset tsan
cmake --build --preset tsan
```

The ThreadSanitizer preset uses the following compiler flags:

```text
-fsanitize=thread
-fno-omit-frame-pointer
```

The ThreadSanitizer runtime instruments memory accesses and synchronization operations to detect data races during program execution.

Preserving frame pointers improves the reliability of diagnostic stack traces.

## 8. Running the ThreadSanitizer Test Suite

```bash
ctest --preset tsan --output-on-failure
```

Initial results:

- Total tests: 57
- Passed: 54
- Failed: 3

Failed tests:

```text
QueueDataIntegrityTest.SustainedLoadPreservesOrderAndContent
PipelineLivenessTest.PipelineAlwaysRunsToCompletion
SimulationThroughputSmokeTest.ProcessesALargeFeedWithinBudget
```

ThreadSanitizer reported data races during execution.

## 9. Isolating the Failure

Instead of repeatedly executing the entire test suite, the investigation focused on the queue data integrity test.

```bash
ctest --preset tsan \
  -R '^QueueDataIntegrityTest\.SustainedLoadPreservesOrderAndContent$' \
  --output-on-failure
```

Command options:

- `--preset tsan`: Selects the ThreadSanitizer test configuration.
- `-R`: Filters tests using a regular expression.
- `^` and `$`: Match the complete test name.
- `\.`: Matches a literal dot.
- `--output-on-failure`: Displays diagnostic information when a test fails.

## 10. Analyzing the ThreadSanitizer Report

The relevant diagnostic output was:

```text
WARNING: ThreadSanitizer: data race

Read of size 8 by main thread:
    tryPop
    include/mdlab/util/SPSCQueue.hpp:41

Previous write of size 8 by thread T1:
    tryPush
    include/mdlab/util/SPSCQueue.hpp:31
```

The report also identified the affected memory as a heap allocation created during SPSC queue construction.

The two conflicting operations accessed the same memory address.

### Conflicting Write

The producer thread writes an element into the ring buffer:

```cpp
buffer_[head] = std::move(value);
```

### Conflicting Read

The consumer thread retrieves an element from the ring buffer:

```cpp
T value = std::move(buffer_[tail]);
```

The accesses were not correctly ordered through a happens-before relationship.

Interestingly, Google Test reported the functional test as passed, while CTest reported a failure because ThreadSanitizer detected a data race and returned a nonzero exit status.

This demonstrates why functional correctness and concurrency correctness must be verified independently.

## 11. Inspecting the Source Code

```bash
nl -ba include/mdlab/util/SPSCQueue.hpp
```

The `nl -ba` command displays the source file with numbered lines, including blank lines.

This allows sanitizer stack traces to be mapped directly to the corresponding source code.

## 12. Root Cause Analysis

The queue uses two atomic indices:

```cpp
alignas(64) std::atomic<std::size_t> head_{0};
alignas(64) std::atomic<std::size_t> tail_{0};
```

The producer owns the head index, while the consumer owns the tail index.

The underlying buffer contains ordinary non-atomic elements:

```cpp
std::vector<T> buffer_;
```

The intentional bug replaced the required acquire/release operations with relaxed memory ordering.

### Incorrect Producer Implementation

```cpp
bool tryPush(T value) {
    const std::size_t head =
        head_.load(std::memory_order_relaxed);

    const std::size_t nextHead = increment(head);

    if (nextHead == tail_.load(std::memory_order_relaxed)) {
        return false;
    }

    buffer_[head] = std::move(value);

    head_.store(nextHead, std::memory_order_relaxed);

    return true;
}
```

### Incorrect Consumer Implementation

```cpp
std::optional<T> tryPop() {
    const std::size_t tail =
        tail_.load(std::memory_order_relaxed);

    if (tail == head_.load(std::memory_order_relaxed)) {
        return std::nullopt;
    }

    T value = std::move(buffer_[tail]);

    tail_.store(increment(tail), std::memory_order_relaxed);

    return value;
}
```

Although relaxed atomic operations guarantee atomicity for the indices themselves, they do not establish the synchronization required to protect accesses to the non-atomic buffer elements.

Consequently, producer and consumer accesses to the same buffer slot were not properly ordered under the C++ memory model.

This constitutes a data race and results in undefined behavior.

## 13. Correct Memory Ordering

Three memory ordering modes are relevant to this investigation.

| Memory Ordering | Purpose |
|---|---|
| `memory_order_relaxed` | Atomicity without synchronization of surrounding memory accesses |
| `memory_order_release` | Publishes preceding memory operations |
| `memory_order_acquire` | Acquires memory operations published by a corresponding release operation |

An acquire operation synchronizes with a release operation when it reads the value published by that release or an applicable release sequence.

### Producer-to-Consumer Synchronization

The producer first writes the buffer element:

```cpp
buffer_[head] = std::move(value);
```

It then publishes the updated head index:

```cpp
head_.store(nextHead, std::memory_order_release);
```

The consumer observes the published head using:

```cpp
head_.load(std::memory_order_acquire);
```

This establishes the required happens-before relationship between the producer's buffer write and the consumer's subsequent buffer access.

### Consumer-to-Producer Synchronization

After retrieving an element, the consumer publishes the updated tail:

```cpp
tail_.store(increment(tail), std::memory_order_release);
```

The producer observes the updated tail using:

```cpp
tail_.load(std::memory_order_acquire);
```

This ensures that a consumed buffer slot can be safely reused.

Both synchronization directions are necessary for a reusable ring buffer.

## 14. Implementing the Fix

### Corrected Producer

```cpp
bool tryPush(T value) {
    const std::size_t head =
        head_.load(std::memory_order_relaxed);

    const std::size_t nextHead = increment(head);

    if (nextHead == tail_.load(std::memory_order_acquire)) {
        return false;
    }

    buffer_[head] = std::move(value);

    head_.store(nextHead, std::memory_order_release);

    return true;
}
```

### Corrected Consumer

```cpp
std::optional<T> tryPop() {
    const std::size_t tail =
        tail_.load(std::memory_order_relaxed);

    if (tail == head_.load(std::memory_order_acquire)) {
        return std::nullopt;
    }

    T value = std::move(buffer_[tail]);

    tail_.store(increment(tail), std::memory_order_release);

    return value;
}
```

### Corrected Empty Check

```cpp
[[nodiscard]] bool empty() const noexcept {
    return head_.load(std::memory_order_acquire) ==
           tail_.load(std::memory_order_acquire);
}
```

The acquire loads in `empty()` restore the baseline implementation.

However, they do not provide an atomic snapshot of both indices, nor do they guarantee that the queue remains empty after the function returns.

### Why Some Operations Remain Relaxed

The producer is the only thread that updates `head_`.

The consumer is the only thread that updates `tail_`.

Therefore, each thread can use relaxed ordering when loading its own index for its queue operation.

Stronger memory ordering is required when observing progress published by the other thread.

## 15. Verifying the Isolated Fix

```bash
cmake --build --preset tsan
```

```bash
ctest --preset tsan \
  -R '^QueueDataIntegrityTest\.SustainedLoadPreservesOrderAndContent$' \
  --output-on-failure
```

Results:

```text
1/1 Test Passed
100% tests passed, 0 tests failed out of 1
Total Test time (real) = 1.65 sec
```

No data race was reported during this execution.

## 16. Investigating the Remaining Performance Failure

The full ThreadSanitizer suite was executed again.

```bash
ctest --preset tsan --output-on-failure
```

Results:

- Passed: 56
- Failed: 1
- Reported data races: None in the observed run

The remaining failure was:

```text
SimulationThroughputSmokeTest.ProcessesALargeFeedWithinBudget
```

The test reported:

```text
Expected: elapsedSeconds < 5.0
Actual: 5.416100377 seconds
```

The pipeline processed 100,000 market data ticks but exceeded its configured time budget.

ThreadSanitizer introduces runtime instrumentation overhead, making timing-sensitive tests susceptible to slower execution.

To investigate this hypothesis, the same test was executed without ThreadSanitizer.

```bash
cmake --build --preset debug
```

```bash
ctest --preset debug \
  -R '^SimulationThroughputSmokeTest\.ProcessesALargeFeedWithinBudget$' \
  --output-on-failure
```

### Execution Time Comparison

| Configuration | CTest Test Duration | Result |
|---|---:|---|
| Debug | 0.59 seconds | Passed |
| ThreadSanitizer | 5.81 seconds | Failed |

The instrumented test took approximately 9.85 times longer in these observed executions.

These measurements support sanitizer overhead as a likely contributor to the performance failure. They are not a controlled benchmark, and execution times may vary with system load and thread scheduling.

No performance threshold was modified.

## 17. Debug Regression Testing

```bash
ctest --preset debug --output-on-failure
```

Results:

```text
57/57 tests passed
```

All functional tests passed following the memory ordering correction.

## 18. Reviewing and Minimizing the Fix

The initial manual correction also introduced formatting and comment changes.

The following commands were used to review these differences:

```bash
git diff -- include/mdlab/util/SPSCQueue.hpp
```

```bash
git --no-pager diff -w baseline -- include/mdlab/util/SPSCQueue.hpp
```

The comparison confirmed that the corrected implementation matched the synchronization logic of the healthy baseline.

To remove unrelated formatting and documentation changes, the verified baseline version of the individual file was restored:

```bash
git restore --source=baseline -- include/mdlab/util/SPSCQueue.hpp
```

The resulting file was verified:

```bash
git diff baseline -- include/mdlab/util/SPSCQueue.hpp
```

The command produced no output, confirming that the file matched the baseline.

The final working-tree changes were inspected:

```bash
git diff --stat
```

Output:

```text
include/mdlab/util/SPSCQueue.hpp | 10 +++++-----
1 file changed, 5 insertions(+), 5 deletions(-)
```

This produced a minimal correction without unrelated source code changes.

## 19. Final ThreadSanitizer Validation

After minimizing the fix, the final source code was rebuilt and tested:

```bash
cmake --build --preset tsan
ctest --preset tsan --output-on-failure
```

Final results:

```text
100% tests passed, 0 tests failed out of 57

Total Test time (real) = 4.66 sec
```

The throughput smoke test also passed, completing in 2.69 seconds as reported by CTest.

No data race was reported during the final execution.

## 20. Final Results

| Metric | Before Fix | After Final Fix |
|---|---|---|
| ThreadSanitizer tests passed | 54/57 | 57/57 |
| ThreadSanitizer tests failed | 3 | 0 |
| Data races reported | Yes | None observed |
| Mutex introduced | No | No |
| Queue architecture | Unchanged | Unchanged |
| Performance threshold modified | No | No |

## 21. Lessons Learned

- Atomic variables alone do not guarantee thread safety.
- Relaxed memory ordering does not establish inter-thread synchronization.
- Acquire/release operations can safely publish non-atomic data between threads.
- Reusable ring buffers require synchronization in both directions.
- Functional tests may pass even when the program contains undefined behavior.
- ThreadSanitizer reports are valuable for identifying conflicting memory accesses.
- Sanitizer instrumentation can significantly affect timing-sensitive tests.
- A clean sanitizer execution increases confidence but does not prove the absence of all possible data races.
- Minimal fixes simplify code review and reduce regression risk.

## 22. Conclusion

The root cause of Scenario 05 was incorrect atomic memory ordering in a lock-free SPSC ring buffer.

The issue was diagnosed using ThreadSanitizer, conflicting memory access analysis, source code inspection, and the C++ memory model.

Correct acquire/release synchronization was restored without introducing mutexes or changing the queue architecture.

The final implementation passed all 57 ThreadSanitizer tests and preserved the existing performance threshold.

**Status: Resolved and validated.**
