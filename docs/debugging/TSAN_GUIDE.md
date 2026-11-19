# ThreadSanitizer (TSan) — Practical C++ Debugging Guide

## 1. Introduction

ThreadSanitizer (TSan) is a dynamic analysis tool designed to detect data races and certain synchronization-related problems in multithreaded applications.

It is particularly useful when developing:

- Low-latency trading systems
- Real-time applications
- Lock-free data structures
- Concurrent containers
- Multithreaded pipelines
- Network servers
- Embedded Linux applications

ThreadSanitizer instruments the application at compile time and monitors memory accesses and synchronization operations at runtime.

## 2. What Is a Data Race?

A data race occurs when:

- Two threads perform conflicting accesses to the same memory location.
- At least one access modifies the memory.
- At least one access is non-atomic.
- No happens-before relationship orders the accesses.

In C++, a data race results in undefined behavior.

### Example

```cpp
#include <thread>

int counter = 0;

int main() {
    std::thread t1([] {
        ++counter;
    });

    std::thread t2([] {
        ++counter;
    });

    t1.join();
    t2.join();
}
```

Both threads modify the same non-atomic variable without synchronization.

Even if the program occasionally produces the expected result, its behavior is undefined.

## 3. What Can ThreadSanitizer Detect?

| Problem | Detection |
|---|---|
| Data races | Yes |
| Incorrect synchronization causing data races | Yes |
| Unsynchronized shared memory access | Yes |
| Some mutex misuse | Yes |
| Some lock-order inversions | Supported in applicable configurations |
| Heap-use-after-free | Use AddressSanitizer primarily |
| Buffer overflow | Use AddressSanitizer |
| Memory leaks | Use LeakSanitizer |
| General logical errors | No |
| All possible deadlocks | No |

ThreadSanitizer is a dynamic analysis tool, not a formal verification tool.

It can only analyze execution paths exercised at runtime.

## 4. Compiling with ThreadSanitizer

### GCC

```bash
g++ -std=c++23 \
    -g \
    -O1 \
    -fsanitize=thread \
    -fno-omit-frame-pointer \
    -pthread \
    main.cpp \
    -o app
```

### Clang

```bash
clang++ -std=c++23 \
    -g \
    -O1 \
    -fsanitize=thread \
    -fno-omit-frame-pointer \
    -pthread \
    main.cpp \
    -o app
```

### Compiler Flags

| Flag | Purpose |
|---|---|
| `-std=c++23` | Selects the C++23 language standard |
| `-g` | Generates debugging information |
| `-O1` | Enables moderate optimization |
| `-fsanitize=thread` | Enables ThreadSanitizer instrumentation |
| `-fno-omit-frame-pointer` | Helps produce useful stack traces |
| `-pthread` | Enables POSIX thread support |

The sanitizer flag must be used during compilation and linking.

## 5. Running an Instrumented Application

```bash
./app
```

If a data race is detected, ThreadSanitizer normally prints a diagnostic report to standard error.

No external debugger is required.

## 6. Integrating ThreadSanitizer with CMake

Example configuration:

```cmake
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)

if(ENABLE_TSAN)
    add_compile_options(
        -fsanitize=thread
        -fno-omit-frame-pointer
        -g
    )

    add_link_options(
        -fsanitize=thread
    )
endif()
```

Configure the project:

```bash
cmake -S . -B build/tsan \
    -DCMAKE_BUILD_TYPE=Debug \
    -DENABLE_TSAN=ON
```

Build the project:

```bash
cmake --build build/tsan
```

Run the tests:

```bash
ctest --test-dir build/tsan --output-on-failure
```

Keep sanitizer builds separate from production builds.

## 7. Using ThreadSanitizer in Cpp23DebuggingLab

The project provides a dedicated TSan CMake preset.

### Configure

```bash
cmake --preset tsan
```

### Build

```bash
cmake --build --preset tsan
```

### Run All Tests

```bash
ctest --preset tsan --output-on-failure
```

### List Available Tests

```bash
ctest --preset tsan -N
```

### Run a Specific Test

```bash
ctest --preset tsan \
  -R '^QueueDataIntegrityTest\.SustainedLoadPreservesOrderAndContent$' \
  --output-on-failure
```

Isolating a failing test reduces diagnostic noise and speeds up investigation.

## 8. Understanding a ThreadSanitizer Report

A typical report contains:

```text
WARNING: ThreadSanitizer: data race

Read of size 8 by main thread:
    #0 tryPop SPSCQueue.hpp:41

Previous write of size 8 by thread T1:
    #0 tryPush SPSCQueue.hpp:31

Location is heap block allocated by main thread

Thread T1 created by main thread

SUMMARY: ThreadSanitizer: data race
```

### Important Report Sections

| Section | Meaning |
|---|---|
| WARNING | Type of detected problem |
| Read / Write | Conflicting memory accesses |
| Size | Number of bytes accessed |
| Memory address | Location of the conflicting accesses |
| Thread identifier | Thread responsible for the access |
| Stack trace | Function calls leading to the access |
| Allocation stack | Origin of the affected memory |
| Thread creation stack | Where a participating thread was created |
| SUMMARY | Concise description of the detected problem |

The summary is a useful starting point, but it does not necessarily identify the root cause.

Always investigate both conflicting accesses.

## 9. Locating the Relevant Source Code

Display source code with line numbers:

```bash
nl -ba include/mdlab/util/SPSCQueue.hpp
```

Display a specific range:

```bash
nl -ba include/mdlab/util/SPSCQueue.hpp | sed -n '25,45p'
```

Search for a function throughout the project:

```bash
grep -Rn "tryPush" include src tests
```

These commands help map sanitizer stack traces to the implementation.

## 10. Understanding Happens-Before

The happens-before relationship defines ordering and visibility guarantees between operations under the C++ memory model.

Without appropriate synchronization, conflicting non-atomic accesses across threads can cause a data race.

Common synchronization mechanisms include:

- Mutex lock and unlock operations
- Acquire/release atomic operations
- Thread creation and joining
- Semaphores
- Properly synchronized condition variable protocols

The appropriate mechanism depends on the ownership model and concurrency requirements of the application.

## 11. Atomic Memory Ordering

| Memory Ordering | Description |
|---|---|
| `memory_order_relaxed` | Atomicity without additional synchronization guarantees |
| `memory_order_acquire` | Acquires operations published by a corresponding release |
| `memory_order_release` | Publishes preceding operations |
| `memory_order_acq_rel` | Combines acquire and release for applicable read-modify-write operations |
| `memory_order_seq_cst` | Provides sequentially consistent ordering for participating operations |

Atomicity and synchronization are different concepts.

Using atomic variables does not automatically make surrounding non-atomic data accesses thread-safe.

## 12. Example: Publishing Data Between Threads

### Incorrect Implementation

```cpp
#include <atomic>

int data = 0;
std::atomic<bool> ready{false};

void producer() {
    data = 42;
    ready.store(true, std::memory_order_relaxed);
}

void consumer() {
    while (!ready.load(std::memory_order_relaxed)) {
    }

    int value = data;
}
```

The atomic flag is accessed safely, but it does not establish a happens-before relationship for the non-atomic variable `data`.

### Correct Implementation

```cpp
#include <atomic>

int data = 0;
std::atomic<bool> ready{false};

void producer() {
    data = 42;
    ready.store(true, std::memory_order_release);
}

void consumer() {
    while (!ready.load(std::memory_order_acquire)) {
    }

    int value = data;
}
```

When the acquire load observes the value written by the release store, the producer's preceding write to `data` happens-before the consumer's subsequent read.

This example uses a one-time publication protocol.

Repeated publication requires an additional ownership or synchronization protocol.

## 13. Case Study: Lock-Free SPSC Queue

Scenario 05 of Cpp23DebuggingLab contains an intentional synchronization bug in a Single-Producer/Single-Consumer ring buffer.

The queue contains:

```cpp
std::vector<T> buffer_;

alignas(64) std::atomic<std::size_t> head_{0};
alignas(64) std::atomic<std::size_t> tail_{0};
```

The atomic indices track producer and consumer progress.

The buffer elements themselves are non-atomic.

### Producer-to-Consumer Synchronization

```cpp
buffer_[head] = std::move(value);

head_.store(nextHead, std::memory_order_release);
```

The consumer observes the published head:

```cpp
head_.load(std::memory_order_acquire);
```

This orders the producer's write before the consumer's access to the published element.

### Consumer-to-Producer Synchronization

```cpp
T value = std::move(buffer_[tail]);

tail_.store(increment(tail), std::memory_order_release);
```

The producer observes consumer progress:

```cpp
tail_.load(std::memory_order_acquire);
```

This allows the producer to safely reuse consumed slots.

### Thread-Owned Indices

The producer can read its own head index using relaxed ordering.

```cpp
head_.load(std::memory_order_relaxed);
```

The consumer can read its own tail index using relaxed ordering.

```cpp
tail_.load(std::memory_order_relaxed);
```

This is valid under the single-producer/single-consumer ownership model.

The implementation does not support multiple producers or multiple consumers.

## 14. ThreadSanitizer Runtime Options

Runtime behavior can be configured through the `TSAN_OPTIONS` environment variable.

### Stop After the First Report

```bash
TSAN_OPTIONS="halt_on_error=1" ./app
```

### Save Diagnostic Reports

```bash
TSAN_OPTIONS="log_path=/tmp/tsan_report" ./app
```

The runtime generally appends the process ID to the log filename.

### Set a Custom Exit Code

```bash
TSAN_OPTIONS="exitcode=66" ./app
```

### Display Supported Options

```bash
TSAN_OPTIONS="help=1" ./app
```

### Use Options with CTest

```bash
TSAN_OPTIONS="halt_on_error=1" \
    ctest --preset tsan --output-on-failure
```

Available runtime options may vary depending on the compiler and sanitizer runtime version.

## 15. Reproducing Intermittent Concurrency Bugs

Concurrency bugs may depend on thread scheduling.

Running a test once may not exercise the problematic execution path.

### Repeat a Specific Test Until Failure

```bash
ctest --preset tsan \
    -R QueueDataIntegrityTest \
    --repeat until-fail:100 \
    --output-on-failure
```

This executes matching tests up to 100 times, stopping when a failure occurs.

Additional investigation techniques include:

- Increasing workload size
- Extending test execution duration
- Exercising queue wraparound
- Testing full and empty queue transitions
- Increasing contention where appropriate
- Varying thread scheduling conditions
- Testing multiple CPU architectures

Repeated successful tests increase confidence but do not prove the absence of data races.

## 16. ThreadSanitizer and Performance

ThreadSanitizer adds runtime instrumentation to memory accesses and synchronization operations.

Consequently, instrumented applications typically consume more CPU time and memory.

Sanitizer builds should not be treated as production performance benchmarks.

### Observed Scenario 05 Results

| Configuration | Throughput Test Duration | Result |
|---|---:|---|
| Debug | 0.59 seconds | Passed |
| Initial corrected TSan build | 5.81 seconds | Failed |
| Final corrected TSan build | 2.69 seconds | Passed |

The test processes 100,000 market data ticks with a five-second internal execution budget.

The observed durations demonstrate that instrumented execution times can vary significantly.

These results are individual test measurements rather than controlled benchmarks.

Performance should normally be evaluated separately using optimized, non-sanitized builds.

## 17. ThreadSanitizer vs. Other Sanitizers

| Tool | Primary Purpose |
|---|---|
| TSan | Data race detection |
| ASan | Memory safety violations |
| UBSan | Selected undefined behavior detection |
| LSan | Memory leak detection |

ThreadSanitizer and AddressSanitizer cannot be enabled together in the same build using standard GCC and Clang sanitizer configurations.

Maintain separate build configurations:

```text
build/
    debug/
    release/
    asan-ubsan/
    tsan/
```

Each configuration serves a different validation objective.

## 18. Recommended Debugging Workflow

1. Reproduce the issue using a standard Debug build.
2. Configure a dedicated ThreadSanitizer build.
3. Execute the test suite.
4. Identify failing tests.
5. Isolate the relevant test.
6. Read both conflicting memory access stack traces.
7. Identify the shared memory location.
8. Inspect thread ownership and synchronization.
9. Determine the missing happens-before relationship.
10. Implement a minimal correction.
11. Rebuild the instrumented application.
12. Rerun the isolated test.
13. Execute the complete TSan regression suite.
14. Validate functional correctness without instrumentation.
15. Evaluate performance separately if necessary.
16. Review and document the final source code changes.

## 19. Git Commands for Reviewing Fixes

Inspect modified files:

```bash
git diff --stat
```

Inspect a specific file:

```bash
git diff -- include/mdlab/util/SPSCQueue.hpp
```

Compare against a known healthy baseline:

```bash
git diff baseline -- include/mdlab/util/SPSCQueue.hpp
```

Ignore whitespace differences within lines:

```bash
git --no-pager diff -w baseline -- \
    include/mdlab/util/SPSCQueue.hpp
```

Check for whitespace errors:

```bash
git diff --check
```

Review the final changes before committing.

Avoid unrelated formatting changes and unnecessary architectural modifications.

## 20. Common Mistakes

- Assuming atomic variables automatically guarantee thread safety.
- Using relaxed memory ordering without analyzing synchronization requirements.
- Ignoring the second conflicting access in a TSan report.
- Assuming passing unit tests guarantee concurrency correctness.
- Adding mutexes without understanding the root cause.
- Using sanitizer builds to evaluate production latency.
- Suppressing warnings without investigating them.
- Assuming one successful TSan execution proves thread safety.
- Modifying performance thresholds simply to make tests pass.

## 21. Limitations

ThreadSanitizer does not provide a mathematical proof of correctness.

Its effectiveness depends on:

- Executed code paths
- Thread scheduling
- Test coverage
- Compiler instrumentation
- Visibility of memory accesses
- Supported synchronization mechanisms
- Platform and runtime support

Some custom synchronization mechanisms, inline assembly, and uninstrumented libraries may produce incomplete or misleading diagnostic results.

Always combine sanitizer results with code review, ownership analysis, and reasoning about the C++ memory model.

## 22. Final Checklist

- [ ] Dedicated ThreadSanitizer build configured
- [ ] Data race reproduced
- [ ] Conflicting accesses identified
- [ ] Thread ownership verified
- [ ] Root cause understood
- [ ] Happens-before relationship established
- [ ] Minimal fix implemented
- [ ] Isolated test passed
- [ ] Full TSan regression suite passed
- [ ] Functional regression tests passed
- [ ] Performance evaluated independently
- [ ] Source code changes reviewed
- [ ] Investigation documented

## 23. References

- Clang ThreadSanitizer documentation: https://clang.llvm.org/docs/ThreadSanitizer.html
- GCC instrumentation options: https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html
- C++ memory ordering: https://en.cppreference.com/w/cpp/atomic/memory_order
- CTest documentation: https://cmake.org/cmake/help/latest/manual/ctest.1.html

## 24. Conclusion

ThreadSanitizer is an essential debugging tool for concurrent C++ applications.

Combined with a strong understanding of the C++ memory model, it enables developers to investigate subtle synchronization failures, preserve efficient concurrency architectures, and improve software reliability.

**Detect. Investigate. Understand. Fix. Validate.**
