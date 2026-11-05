# AI-Assisted Development

This document records which parts of this repository were produced with AI assistance, and draws
a clear line between that AI-generated material and the human-led debugging work the lab exists to
support.

## What Claude Code generated

[Claude Code](https://claude.com/claude-code) generated the initial state of this repository in a
single assisted session, specifically:

- **The baseline C++23 codebase** — the full multithreaded market-data/trading pipeline
  (`MarketDataParser`, `SPSCQueue`, `OrderBook`, `RiskManager`/`PositionTracker`,
  `ExecutionEngine`/`OrderGateway`, the async `Logger`, and the `MarketDataFeedHandler` wiring),
  the `simulation` app, the CMake build (`CMakeLists.txt`, `CMakePresets.json`), the sample feed
  data, and the GitHub Actions CI workflow.
- **The baseline regression test suite** — the full GoogleTest suite under `tests/`, verified to
  pass cleanly (57/57) under all four build presets (`debug`, `release`, `asan-ubsan`, `tsan`)
  before anything else was built on top of it.
- **The 10 debugging scenarios** — one isolated, realistic defect per `scenario/NN` branch,
  forked from `baseline`, each independently built and empirically confirmed to reproduce its
  documented symptom (crash, hang, wrong output, or slowdown, as applicable) before being
  committed. The corresponding bug reports in `docs/scenarios/` were also AI-authored.

All of the above was produced from a single project-generation prompt (see `prompts/`) describing
the desired domain, scope, and constraints — including the constraint that bug reports must never
reveal root causes or solutions.

## What this document does not contain

Consistent with the lab's purpose, this document does **not** describe, hint at, or summarize the
mechanism behind any of the 10 scenarios. It records *that* the scenarios were AI-generated, not
*what* is wrong with any of them. The bug reports in `docs/scenarios/` remain the only
learner-facing description of each scenario's symptoms.

## Drawing the line: AI-generated vs. human-led work

| | Origin | Scope |
|---|---|---|
| Baseline implementation (`include/`, `src/`, `apps/`) | AI-generated | The working reference system |
| Baseline regression suite (`tests/`) | AI-generated | Tests that pass on `baseline` and double as the reproduction mechanism for each scenario |
| The 10 injected defects (`scenario/NN` branches) | AI-generated | One isolated, realistic bug per branch, diagnosed and fixed by a human |
| Debugging, root-cause analysis, and fixes for each scenario | **Human-led** | Not yet performed — this is the exercise |
| Any new regression tests written while diagnosing a scenario | **Human-led** | To be added by whoever works the scenario, as part of fixing it |
| Any future bug fixes, refactors, or hardening beyond the 10 scenarios | **Human-led** | Ongoing project work, outside the scope of what Claude Code generated |

Going forward, commits that fix a scenario, add a regression test for a newly-discovered issue, or
otherwise change runtime behavior should be understood as human-led work on top of this
AI-generated foundation — not an extension of the original generation. Keeping that distinction
clear is also why `baseline` and every `scenario/NN` branch are left untouched by this document and
by the README update that links to it.
