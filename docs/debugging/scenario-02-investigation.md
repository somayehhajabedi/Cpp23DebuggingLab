# Scenario 02 — Malformed Feed Input Crash Investigation

**Branch:** `scenario/02`  
**Status:** Crash reproduced; root cause investigation in progress  
**Tools:** C++23, CMake, GDB 17.1

## 1. Bug Description

The simulation crashes when processing malformed or truncated market-data records instead of reporting a parsing error and continuing.

## 2. Reproduction

```bash
cmake --preset debug
cmake --build --preset debug
./build/debug/simulation data/malformed_ticks.csv
```

**Observed failure:**

```text
std::vector::operator[]
Assertion '__n < this->size()' failed.
Aborted
```

The assertion indicates an out-of-range vector index.

## 3. GDB Investigation

```gdb
gdb build/debug/simulation
set args data/malformed_ticks.csv
run
bt
```

GDB reported that Thread 3 received `SIGABRT`.

### Understanding the Backtrace

A backtrace displays the active call stack, with frame `#0` representing the current stopping location and higher-numbered frames representing earlier callers.

Important frames identified:

```text
#6 std::vector<std::string_view>::operator[]
   at stl_vector.h:1282

#7 mdlab::marketdata::MarketDataParser::parseLine(...)
   at src/marketdata/MarketDataParser.cpp:65

#8 mdlab::pipeline::MarketDataFeedHandler::feedLoop(...)
   at src/pipeline/MarketDataFeedHandler.cpp:35
```

### How to Identify Important Stack Frames

1. Identify the failure type (`SIGABRT`, `SIGSEGV`, assertion failure, etc.).
2. Find the stack frame corresponding to the failing operation.
3. Locate the nearest caller belonging to the application's source code.
4. Inspect the arguments, local variables, and source code in that frame.
5. Trace earlier callers when necessary to identify where invalid data originated.

In this case, frame `#6` corresponds to the invalid vector access, while frame `#7` is the nearest application frame that invoked it.

The first useful investigation target is therefore `MarketDataParser.cpp:65`.

**Important:** The failing line is not necessarily the ultimate root cause. Further inspection is required.

## 4. Next Investigation Step

```gdb
frame 7
list
```

Inspect the source code around line 65 and determine which vector index is being accessed and how its value is derived.

## 5. Investigation Status

- Crash reproduced successfully.
- Failure identified as an assertion in `std::vector::operator[]`.
- Relevant application stack frame identified.
- Root cause not yet confirmed.
- No fix applied.

## 6. AI-Assisted Development Disclosure

The initial project and debugging scenarios were generated with Claude Code. The developer is reproducing and investigating the failures using GDB, with ChatGPT providing educational guidance and documentation assistance.

## Final Fix and Verification

### Root Cause
The parser accessed fields[5] when the input
contained fewer than six fields.

### Fix
Added a field-count validation immediately
after splitFields(line).

### Verification
- All 3 parser robustness tests passed.
- Simulation completed without crashing.
- 2 ticks parsed successfully.
- 4 malformed lines reported as parse errors.
- Full CTest regression suite passed.

### Status
Fixed and verified.
