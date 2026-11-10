# Scenario 03 — Heap Use-After-Free Investigation

## 1. Objective

Investigate a memory safety bug in a multithreaded C++23 trading simulation using:

- CMake and CTest
- AddressSanitizer (ASan)
- UndefinedBehaviorSanitizer (UBSan)
- `grep`, `sed`, and `nl`
- C++ object lifetime analysis
- Regression testing

The debugging workflow is:

**Reproduce → Capture → Inspect → Identify → Fix → Verify**

## 2. Step 1 — Reproduce the Problem

### Run the normal Debug tests

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
```

**Observed result:** All 57 tests passed.

This establishes that the normal test suite did not expose the memory safety bug.

### Run the sanitizer-enabled tests

```bash
cmake --preset asan-ubsan
cmake --build --preset asan-ubsan
ctest --preset asan-ubsan --output-on-failure
```

**Observed result:** Three tests failed:

1. `ExecutionEngineTest.TightSpreadSignalsEventuallyProduceAFill`
2. `ExecutionEngineLifecycleTest.SustainedOrderFillCyclesLeaveNoStaleState`
3. `PipelineLivenessTest.PipelineAlwaysRunsToCompletion`

AddressSanitizer reported a `heap-use-after-free`.

**Lesson:** Passing functional tests does not prove memory safety.

## 3. Step 2 — Isolate the Failing Test

Instead of running the entire test suite repeatedly, execute one failing test:

```bash
ctest --test-dir build/asan-ubsan \
  -R '^ExecutionEngineTest\.TightSpreadSignalsEventuallyProduceAFill$' \
  --output-on-failure
```

### Command explanation

- `ctest`: Runs tests registered with CMake.
- `--test-dir`: Selects the CMake build directory.
- `-R`: Selects tests whose names match a regular expression.
- `^`: Matches the beginning of a test name.
- `\.`: Matches a literal period.
- `$`: Matches the end of the test name.
- `--output-on-failure`: Displays output for failing tests.

**Why isolate a test?**

It reduces noise, makes repeated debugging faster, and helps confirm whether the specific failure is resolved.

## 4. Step 3 — Save the ASan Report

Run the failing test with ASan logging enabled:

```bash
ASAN_OPTIONS=log_path=/tmp/asan-scenario03 \
ctest --test-dir build/asan-ubsan \
  -R '^ExecutionEngineTest\.TightSpreadSignalsEventuallyProduceAFill$' \
  --output-on-failure
```

ASan generated a report at:

```text
/tmp/asan-scenario03.1769508
```

The number appended to the filename is the process ID.

### Important distinction

`ASAN_OPTIONS` controls AddressSanitizer's runtime behavior.

It does not enable ASan instrumentation by itself. The executable must already have been compiled with sanitizer instrumentation.

## 5. Step 4 — Find Important Information Using grep

Run:

```bash
grep -nE \
  'ERROR: AddressSanitizer|READ of|WRITE of|freed by|previously allocated by|SUMMARY:' \
  /tmp/asan-scenario03.1769508
```

### Explanation

`grep` searches text for matching patterns.

- `-n`: Prints the matching line number in the report.
- `-E`: Enables extended regular expressions.
- `|`: Means OR between patterns.

### Actual findings

```text
2:ERROR: AddressSanitizer: heap-use-after-free
3:READ of size 1 ... thread T1
23:freed by thread T1 here:
51:previously allocated by thread T0 here:
95:SUMMARY: AddressSanitizer: heap-use-after-free
```

The report identifies three important events:

1. The memory was allocated.
2. The memory was freed.
3. The program accessed the freed memory.

### Important distinction

The line number printed by `grep -n` refers to the ASan report.

For example, report line 95 is not the same as `ExecutionEngine.cpp:78`, which refers to the C++ source code.

## 6. Step 5 — Examine the Deallocation Stack Trace

Run:

```bash
sed -n '23,50p' /tmp/asan-scenario03.1769508
```

### Explanation

- `sed`: A stream editor used here to inspect part of a file.
- `-n`: Suppresses automatic printing of every line.
- `23,50p`: Prints lines 23 through 50.

### Findings

The stack trace contained:

- `operator delete`
- C++ standard library allocator functions
- `std::unordered_map::erase`
- `ExecutionEngine::onFill()` at `ExecutionEngine.cpp:75`
- `OrderGateway::workerLoop()` at `OrderGateway.cpp:38`

### Interpretation

The standard library stack frames explain how the memory was released.

The important project-level frame is:

```text
ExecutionEngine.cpp:75
```

This is where the application requested removal of the map element.

## 7. Step 6 — Examine the Allocation Stack Trace

Run:

```bash
sed -n '51,75p' /tmp/asan-scenario03.1769508
```

### Findings

The allocation stack contained:

- `operator new`
- Standard library allocation functions
- `std::unordered_map::emplace`
- `ExecutionEngine::onBookSignal()` at `ExecutionEngine.cpp:58`

### Interpretation

The `OrderRecord` was originally stored inside `openOrders_`.

ASan identified allocation on thread T0.

The allocation and deallocation stacks help reconstruct the lifetime of the object.

## 8. Step 7 — Investigate the Worker Thread

Run:

```bash
grep -n -A 12 \
  'Thread T1 created by T0 here:' \
  /tmp/asan-scenario03.1769508
```

### Explanation

- `-n`: Displays matching line numbers.
- `-A 12`: Displays 12 lines after the matching line.

### Findings

The thread creation stack included:

- `pthread_create`
- `std::thread::_M_start_thread`
- `std::jthread`
- `OrderGateway::start()` at `OrderGateway.cpp:10`
- Test harness initialization at `ExecutionEngineTest.cpp:36`

### Interpretation

Thread T0 created thread T1.

T1 was executing the order gateway worker.

The report showed that T1 freed the map element and subsequently accessed it.

**Important:** Different threads participating in allocation and deallocation does not automatically prove a data race.

This investigation established a use-after-free bug, not a data race.

## 9. Step 8 — Inspect the Source Code Using nl and sed

Run:

```bash
nl -ba src/execution/ExecutionEngine.cpp | sed -n '65,82p'
```

### Explanation

- `nl`: Numbers lines in a file.
- `-ba`: Numbers all lines, including blank lines.
- `|`: Sends output from the first command into the second command.
- `sed -n '65,82p'`: Displays only the requested source range.

### Original problematic code

```cpp
void ExecutionEngine::onFill(
    std::uint64_t orderId,
    double fillPrice,
    std::int64_t fillQuantity)
{
    OrderRecord* record = nullptr;

    {
        std::lock_guard<std::mutex> lock(ordersMutex_);

        const auto it = openOrders_.find(orderId);

        if (it == openOrders_.end()) {
            return;
        }

        record = &it->second;
        openOrders_.erase(it);
    }

    positions_.applyFill(
        record->symbol,
        record->side,
        fillPrice,
        fillQuantity
    );

    stats_.recordFill();

    MDLAB_LOG_INFO(
        "fill: order={} symbol={} qty={} price={:.2f}",
        record->orderId,
        record->symbol,
        fillQuantity,
        fillPrice
    );
}
```

## 10. Step 9 — Identify the Root Cause

The critical sequence was:

```cpp
record = &it->second;
openOrders_.erase(it);
```

The first statement stored the address of an element inside `std::unordered_map`.

The second statement destroyed that element.

After `erase()`, the pointer still contained an address, but the object at that address was no longer alive.

This is a **dangling pointer**.

The subsequent access:

```cpp
record->symbol
```

caused a `heap-use-after-free`.

### Why didn't the mutex prevent the bug?

`std::lock_guard` prevents other threads from concurrently accessing the protected data without the same mutex.

It does not keep a destroyed object alive.

Even if only one thread executed the function, accessing the erased element would still be invalid.

### Why didn't the normal Debug tests detect it?

Use-after-free is undefined behavior.

The memory may remain mapped and readable temporarily, allowing the program to appear to work.

ASan detects the invalid access using instrumentation and shadow memory.

## 11. Step 10 — Fix the Bug

### Before

```cpp
OrderRecord* record = nullptr;
```

```cpp
record = &it->second;
openOrders_.erase(it);
```

### After

```cpp
std::optional<OrderRecord> record;
```

```cpp
record = it->second;
openOrders_.erase(it);
```

Add the appropriate header if it is not already included:

```cpp
#include <optional>
```

### Explanation

`std::optional<OrderRecord>` initially contains no `OrderRecord`.

After:

```cpp
record = it->second;
```

it contains an independent copy.

Removing the original map element does not destroy this copy.

The function returns early if the order is not found, so the later accesses occur only after the optional has been populated.

### Corrected function

```cpp
void ExecutionEngine::onFill(
    std::uint64_t orderId,
    double fillPrice,
    std::int64_t fillQuantity)
{
    std::optional<OrderRecord> record;

    {
        std::lock_guard<std::mutex> lock(ordersMutex_);

        const auto it = openOrders_.find(orderId);

        if (it == openOrders_.end()) {
            return;
        }

        record = it->second;
        openOrders_.erase(it);
    }

    positions_.applyFill(
        record->symbol,
        record->side,
        fillPrice,
        fillQuantity
    );

    stats_.recordFill();

    MDLAB_LOG_INFO(
        "fill: order={} symbol={} qty={} price={:.2f}",
        record->orderId,
        record->symbol,
        fillQuantity,
        fillPrice
    );
}
```

### Scope issue encountered during the fix

During editing, the `optional` declaration was initially placed inside the nested lock scope:

```cpp
{
    std::optional<OrderRecord> record;
}
```

Using `record` outside that scope caused a compilation error:

```text
error: 'record' was not declared in this scope
```

The declaration was moved outside the nested block, while remaining inside `onFill()`.

This illustrates the difference between **variable scope** and **object lifetime**.

## 12. Step 11 — Rebuild with Sanitizers

Run:

```bash
cmake --build --preset asan-ubsan
```

The project must be rebuilt so that the sanitizer-enabled executable contains the corrected code.

## 13. Step 12 — Verify the Original Failure

Run:

```bash
ctest --test-dir build/asan-ubsan \
  -R '^ExecutionEngineTest\.TightSpreadSignalsEventuallyProduceAFill$' \
  --output-on-failure
```

### Actual result

```text
1/1 Test #51: ExecutionEngineTest.TightSpreadSignalsEventuallyProduceAFill ... Passed

100% tests passed, 0 tests failed out of 1
```

The original failing test now passes under ASan/UBSan.

## 14. Step 13 — Run Regression Tests

Run:

```bash
ctest --preset asan-ubsan --output-on-failure
```

### Actual result

The complete sanitizer-enabled test suite passed.

This verified that the fix resolved the observed sanitizer failures without causing any test failures in the existing suite.

Passing tests does not guarantee that every possible memory bug has been eliminated.

## 15. Step 14 — Useful ASan Report Keywords

| Keyword | Meaning |
|---|---|
| `heap-use-after-free` | Access to heap memory after it has been freed |
| `READ of size N` | Invalid read involving N bytes |
| `WRITE of size N` | Invalid write involving N bytes |
| `freed by thread` | Stack trace showing where memory was released |
| `previously allocated by thread` | Stack trace showing where memory was allocated |
| `SUMMARY:` | Summary of the detected error |
| `heap-buffer-overflow` | Access outside the bounds of a heap allocation |
| `stack-buffer-overflow` | Access outside the bounds of a stack object |
| `double-free` | Attempt to free the same allocation more than once |
| `Direct leak` | Directly leaked allocation reported by LeakSanitizer |
| `Indirect leak` | Leaked allocation reachable from another leaked allocation |

### Shadow memory notation

Some common ASan shadow bytes:

- `fd`: Freed heap region.
- `00`: Eight addressable application bytes.
- `fa`: Heap redzone.
- `f1`: Stack left redzone.

Shadow memory helps ASan track whether memory is valid for access.

## 16. Step 15 — Review and Commit

Before committing, review the working tree:

```bash
git status
```

Inspect changes:

```bash
git diff
```

Review only the source file:

```bash
git diff -- src/execution/ExecutionEngine.cpp
```

After confirming the source changes and documentation:

```bash
git add src/execution/ExecutionEngine.cpp
git add docs/debugging/scenario-03-investigation.md
git commit -m "Fix scenario 03 heap use-after-free and document investigation"
```

If the branch is intended to be published:

```bash
git push -u origin scenario/03
```

Do not commit generated build directories or sanitizer logs.

## 17. Final Lessons Learned

1. Functional tests alone cannot establish memory safety.
2. ASan reports should be read as a timeline of allocation, deallocation, and invalid access.
3. `grep`, `sed`, and `nl` are effective tools for navigating large reports and source files.
4. A pointer can remain non-null after the object it references has been destroyed.
5. `std::unordered_map::erase()` invalidates pointers and references to the erased element.
6. A mutex protects synchronized access, not the lifetime of erased objects.
7. `std::optional<T>` can store an independent object and represent the absence of a value.
8. Scope and object lifetime are different C++ concepts.
9. Verify the specific failing test before running the full regression suite.
10. Preserve the investigation steps so the debugging process is reproducible.
