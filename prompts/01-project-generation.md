Create a realistic **C++23 Debugging Laboratory** for an experienced Linux C++ engineer.

**Project:** Cpp23DebuggingLab

**Domain:** Multithreaded trading and market-data processing system.

### Requirements

1. Build a modular codebase of approximately 20–30 C++ source/header files, including MarketDataParser, SPSCQueue, OrderBook, RiskManager, ExecutionEngine, Logger, and a runnable simulation.
2. Use C++23, CMake, GoogleTest, Linux, and appropriate C++23 features where supported.
3. Create a fully working clean baseline with deterministic tests and sample input.
4. Introduce 8–12 realistic debugging scenarios covering logic errors, segmentation faults, lifetime bugs, memory corruption, race conditions, deadlocks, and performance regressions. Do not make every bug active simultaneously.
5. Keep a clean baseline on a separate Git branch and create one isolated bug branch per scenario, starting from the baseline.
6. For each scenario, provide only a user-facing bug report, reproduction steps, and expected versus actual behavior. Do not reveal root causes or solutions in the README or exercise files.
7. Keep solutions and root-cause explanations out of the learner-facing repository. Do not expose spoilers in branch names, comments, commit messages, or tests.
8. Add Debug, ASan/UBSan, TSan, and Release CMake presets where feasible; document sanitizer incompatibilities.
9. Include regression tests and a GitHub Actions CI workflow for the clean baseline.
10. Do not initialize a GitHub remote or push anything. Ask before running destructive commands.

**Workflow:** First present the architecture and implementation plan, then implement incrementally, build and run tests, and report which checks actually passed.

Do not solve the debugging exercises for the learner.
