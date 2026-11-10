# AddressSanitizer (ASan) — Report Anatomy and Debugging Guide

## 1. Purpose

This guide explains how to recognize the structure of sanitizer diagnostics, identify the useful evidence in a long report, and investigate memory bugs systematically. It also records observations from **Cpp23DebuggingLab — Scenario 03**.

> **Investigation status:** The sanitizer detected a use-after-free. Source-level root cause confirmation, the fix, and post-fix verification are still pending.

## 2. What is a sanitizer?

A sanitizer instruments a program to detect classes of errors while the program runs. Different sanitizers detect different problems:

| Tool | Primary purpose | Example diagnostic |
|---|---|---|
| AddressSanitizer (ASan) | Memory safety violations | `heap-use-after-free`, `heap-buffer-overflow` |
| UndefinedBehaviorSanitizer (UBSan) | Certain kinds of undefined behavior | Signed integer overflow, invalid shift |
| ThreadSanitizer (TSan) | Data races and some threading issues | `data race` |

A passing ordinary Debug test run does **not** establish memory safety. Sanitizer builds may reveal problems that ordinary runs do not expose.

## 3. Anatomy of an AddressSanitizer report

**There is no single universal report layout.** The following structure is typical for a **heap-use-after-free** report; other error categories may omit or replace sections.

### 3.1 Error header — what happened?

Illustrative format:

```text
==12345==ERROR: AddressSanitizer: heap-use-after-free on address 0x...
READ of size 1 at 0x... thread T1
```

Read this first:

- **`heap-use-after-free`**: heap memory was accessed after its lifetime ended.
- **`READ` / `WRITE`**: the kind of invalid access.
- **`size 1`**: number of bytes involved in that access, not the size of the whole allocation.
- **`thread T1`**: thread in which ASan observed the access; thread identifiers can vary between runs.

The address identifies the affected memory location; it is not a source-code line number.

### 3.2 Invalid-access stack trace — where was the error observed?

Illustrative format:

```text
    #0 ... ExecutionEngine::onFill(...) ... ExecutionEngine.cpp:78
    #1 ... OrderGateway::workerLoop(...) ...
    ...
```

- `#0` is the top stack frame in this trace, typically the point where the invalid access was observed.
- Higher frame numbers describe callers farther up the call chain.
- `ExecutionEngine.cpp:78` means **source file and line 78**.
- Library and sanitizer frames may appear; focus first on relevant frames from our own project.
- Stack frame numbers such as `#0` are **not** source line numbers.

### 3.3 Deallocation stack trace — where was the memory freed?

For a use-after-free error, ASan typically includes a section like:

```text
freed by thread T1 here:
    #0 ... operator delete(...)
    ...
    #N ... ExecutionEngine::onFill(...) ... ExecutionEngine.cpp:75
```

This trace identifies the operation that ended the allocation's lifetime. It may be a direct `delete`, or an indirect deallocation caused by a container operation such as erasing an element.

**Why look for `freed by`?** Because the error category is *use-after-free*. We need to compare the location of the invalid use with the location where the object was destroyed. The phrase is part of ASan's usual diagnostic format, **not** a keyword we should search for in every sanitizer report.

### 3.4 Allocation stack trace — where did the memory originate?

A typical section is:

```text
previously allocated by thread T4 here:
    #0 ... operator new(...)
    ...
    #N ... ExecutionEngine::onBookSignal(...) ... ExecutionEngine.cpp:58
```

This trace shows where the affected allocation was created. The allocating thread can differ from the thread that later frees or accesses the allocation. Do **not** assume the problem is a data race solely because multiple thread IDs appear.

### 3.5 Shadow memory and legend — what does ASan know about the bytes?

ASan tracks memory accessibility using *shadow memory*. A report may show surrounding shadow bytes and a legend. For example:

```text
Shadow byte legend (one shadow byte represents 8 application bytes):
  Addressable:           00
  Freed heap region:     fd
  Heap left redzone:     fa
```

The `fd` marker in a shadow-byte dump indicates a freed heap region. The legend is useful confirmation, but the source locations and stack traces are usually the best starting point for investigation.

### 3.6 Summary and termination

A report may end with:

```text
SUMMARY: AddressSanitizer: heap-use-after-free ... ExecutionEngine.cpp:78
==12345==ABORTING
```

The summary is a compact restatement of the detected issue. The process often exits with a nonzero status after an ASan error; whether it terminates immediately depends on sanitizer settings and error type.

## 4. Different errors produce different reports

| Error category | First things to investigate |
|---|---|
| `heap-use-after-free` | Invalid-access trace, `freed by`, `previously allocated by` |
| `heap-buffer-overflow` | Invalid-access trace, allocation size and out-of-bounds offset |
| `stack-buffer-overflow` | Invalid-access trace, affected stack frame and local variable |
| `stack-use-after-return` | Access after a function returned; lifetime of stack locals |
| `double-free` | Repeated deallocation traces and allocation origin |
| UBSan runtime error | Undefined operation, source location, and runtime values |
| TSan data race | Conflicting accesses, thread creation, and synchronization traces |

**Rule:** Identify the sanitizer and error category **before** deciding which sections or phrases to search for.

## 5. How to investigate a long report

1. **Find the first error header.** Determine the sanitizer and error type.
2. **Read the access description.** Is it a read or write? How many bytes? Which thread?
3. **Find the first relevant project frame.** Note the source file, function, and line.
4. **Use error-specific evidence.** For use-after-free, locate deallocation and allocation traces.
5. **Reconstruct object lifetime.** Ask when the object was created, destroyed, and subsequently used.
6. **Inspect source code.** A stack trace narrows the search; it does not automatically prove the exact source-level mistake.
7. **Form and test a hypothesis.** Make a minimal fix only after identifying the lifetime violation.
8. **Rerun the failing test and full sanitizer suite.** Record the outcome.

### Reading stack traces correctly

Suppose a report contains:

```text
#0 ... ExecutionEngine.cpp:78
#1 ... OrderGateway.cpp:120
```

The `#0` and `#1` labels are stack-frame indices. The `:78` and `:120` suffixes are source-code line numbers. A long list of C++ standard-library frames is common, especially with templates and containers.

## 6. Scenario 03 — evidence observed so far

### 6.1 Symptom

A long-running trading simulation can intermittently show corrupted output or crash after fills. Ordinary Debug tests passed, but the ASan/UBSan test run revealed a memory safety violation.

### 6.2 Test results

- **Ordinary Debug configuration:** 57/57 tests passed.
- **ASan/UBSan configuration:** 54/57 tests passed; 3 failed.

Failing tests:

```text
ExecutionEngineTest.TightSpreadSignalsEventuallyProduceAFill
ExecutionEngineLifecycleTest.SustainedOrderFillCyclesLeaveNoStaleState
PipelineLivenessTest.PipelineAlwaysRunsToCompletion
```

### 6.3 Relevant ASan findings

```text
ERROR: AddressSanitizer: heap-use-after-free
READ of size 1
```

| Event | Function | Source location |
|---|---|---|
| Original allocation | `ExecutionEngine::onBookSignal` | `src/execution/ExecutionEngine.cpp:58` |
| Deallocation | `ExecutionEngine::onFill` | `src/execution/ExecutionEngine.cpp:75` |
| Invalid read | `ExecutionEngine::onFill` | `src/execution/ExecutionEngine.cpp:78` |

The deallocation and invalid read were reported on thread `T1`. The original allocation occurred while inserting into an `std::unordered_map`; deallocation was associated with erasing an element. **We still need to inspect the source to confirm which reference, pointer, or iterator becomes invalid.**

### 6.4 Questions for source-level investigation

- What is stored in the `std::unordered_map`?
- Which iterator, pointer, or reference is obtained before line 75?
- Does `erase()` destroy the element that it refers to?
- What expression at line 78 accesses that element?
- Can the relevant data be read or copied safely before erasure?
- After the change, do the previously failing tests pass under ASan?

**Do not mark the bug fixed until the sanitizer run verifies it.**

## 7. Useful commands

### 7.1 Configure and build with sanitizers

From the project root:

```bash
cmake --preset asan-ubsan
cmake --build --preset asan-ubsan
```

`cmake --preset` configures the build according to `CMakePresets.json`. `cmake --build --preset` compiles the configured targets.

### 7.2 Run the sanitizer test suite

```bash
ctest --preset asan-ubsan --output-on-failure
```

`--output-on-failure` prints diagnostic output for failed tests.

### 7.3 Save the full CTest output

```bash
ctest --preset asan-ubsan --output-on-failure > /tmp/asan-report.txt 2>&1
```

- `>` redirects standard output to the file.
- `2>&1` redirects standard error to the same destination.
- Keep the full report: a filtered view is useful for navigation but omits context.

### 7.4 Find the important lines

```bash
grep -nE 'ERROR: AddressSanitizer|READ of|WRITE of|freed by|previously allocated by|ExecutionEngine.cpp:|SUMMARY:' /tmp/asan-report.txt
```

- `grep` searches the file.
- `-n` prints matching line numbers **in the report file**.
- `-E` enables extended regular expressions.
- `|` means **OR** inside this regular expression.

**Important:** `grep` finds matching lines but does not necessarily preserve enough surrounding stack-trace context. Open the full report when interpreting evidence.

### 7.5 Inspect relevant source lines

```bash
nl -ba src/execution/ExecutionEngine.cpp | sed -n '65,82p'
```

- `nl -ba` displays every source line with its line number.
- `sed -n '65,82p'` prints only lines 65 through 82.

### 7.6 Run one failing test

First identify the test executable path in the actual build directory. Then use the GoogleTest filter:

```bash
/path/to/test_executable --gtest_filter='ExecutionEngineTest.TightSpreadSignalsEventuallyProduceAFill'
```

The path depends on the project's build configuration; do not assume a particular executable location without checking it.

## 8. Investigation checklist

- [x] Reproduce a failure under ASan.
- [x] Identify `heap-use-after-free`.
- [x] Locate the invalid read (line 78).
- [x] Locate the deallocation (line 75).
- [x] Locate the allocation (line 58).
- [ ] Inspect the code and confirm the invalidated object/reference.
- [ ] Implement and explain a minimal fix.
- [ ] Rerun the failing test under ASan.
- [ ] Rerun the complete ASan/UBSan suite.
- [ ] Record results and commit the investigation.

## 9. Key lessons

1. **Read the error type first.** It tells you what evidence to seek.
2. **A sanitizer report is a timeline of evidence, not just a crash location.**
3. **Stack-frame indices and source line numbers are different.**
4. **Container operations can end object lifetimes.** References and iterators may become invalid.
5. **Thread IDs alone do not establish a data race.**
6. **Passing Debug tests does not prove memory safety.**
7. **Confirm fixes with the same sanitizer configuration that exposed the problem.**
