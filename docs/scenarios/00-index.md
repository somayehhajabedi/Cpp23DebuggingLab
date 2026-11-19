# Debugging Scenarios

This catalog contains ten isolated, reproducible software defects introduced into an otherwise healthy multithreaded C++23 trading system.

Each original scenario was created from the `baseline` branch with one intentional behavioral defect.

The objective is to develop practical debugging skills through structured investigation, root cause analysis, minimal corrections, and regression testing.

## Current Progress

**5 of 10 scenarios completed (50%)**

| Scenario | Reported Symptom | Bug Report | Status |
|---|---|---|---|
| 01 | Top-of-book occasionally reports bid above ask | [Report](scenario-01.md) | Completed |
| 02 | Feed handler crashes on a malformed input line | [Report](scenario-02.md) | Completed |
| 03 | Garbage output / crash after an order fills | [Report](scenario-03.md) | Completed |
| 04 | Heap corruption reported under sustained load | [Report](scenario-04.md) | Completed |
| 05 | Consumer observes a price that was never sent | [Report](scenario-05.md) | Completed |
| 06 | Risk limit is sometimes not enforced under load | [Report](scenario-06.md) | Pending |
| 07 | Simulation hangs indefinitely with no error output | [Report](scenario-07.md) | Pending |
| 08 | Feed processing has become much slower | [Report](scenario-08.md) | Pending |
| 09 | Order exactly at the risk limit is handled inconsistently | [Report](scenario-09.md) | Pending |
| 10 | Very large order produces a nonsensical notional value | [Report](scenario-10.md) | Pending |

## Original Challenges and Solutions

Completed scenarios preserve both the original defective implementation and the corrected implementation.

| Scenario | Original Challenge Tag | Solution Branch |
|---|---|---|
| 01 | `buggy/scenario-01` | `scenario/01` |
| 02 | `buggy/scenario-02` | `scenario/02` |
| 03 | `buggy/scenario-03` | `scenario/03` |
| 04 | `buggy/scenario-04` | `scenario/04` |
| 05 | `buggy/scenario-05` | `scenario/05` |
| 06–10 | Corresponding scenario branch | Pending |

## How to Practice

### 1. Verify the Healthy Baseline

```bash
git switch baseline

cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
```

### 2. Select an Original Debugging Challenge

For a completed scenario, create a local practice branch from its preserved tag:

```bash
git switch -c practice/01 buggy/scenario-01
```

For a pending scenario:

```bash
git switch -c practice/06 origin/scenario/06
```

### 3. Reproduce the Defect

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
```

Some defects require a sanitizer-enabled configuration to reproduce reliably.

Available presets:

- `debug`
- `release`
- `asan-ubsan`
- `tsan`

Choose the appropriate configuration based on the reported symptoms.

### 4. Investigate and Resolve

- Read the original bug report.
- Reproduce the reported behavior.
- Isolate the failing component or test.
- Collect diagnostic evidence.
- Identify the root cause.
- Implement a minimal fix.
- Execute regression tests.
- Document the investigation.

### 5. Review the Reference Solution

After completing your own investigation, examine the corresponding solution branch.

For example:

```bash
git log --oneline baseline..scenario/01
```

Completed scenario branches contain fixes and technical investigation reports.

## Learning Principles

The original bug reports intentionally describe observed symptoms, reproduction steps, and expected behavior without revealing the underlying root cause.

To preserve the educational value of the laboratory, avoid reviewing solution branches and investigation reports before attempting the original challenges.

The regression test suite provides a consistent mechanism for reproducing defects and validating corrections.

Passing functional tests alone does not guarantee memory safety or concurrency correctness.

## Project Status

This laboratory is under active development. Additional investigations and technical documentation will be published as the remaining scenarios are completed.

---

**Reproduce. Investigate. Understand. Fix. Validate.**
