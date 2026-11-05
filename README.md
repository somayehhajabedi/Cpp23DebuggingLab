# Cpp23DebuggingLab

A small but realistic multithreaded trading / market-data processing system, built as a C++23
debugging laboratory. The `baseline` branch is a fully working, fully tested reference
implementation. Every `scenario/NN` branch forks from `baseline` with exactly one realistic defect
injected — see [`docs/scenarios/00-index.md`](docs/scenarios/00-index.md) for the catalog of bug
reports.

## Architecture

```
MarketDataParser --(Tick)--> SPSCQueue --> OrderBook --(BookSignal)--> SPSCQueue --> ExecutionEngine
                                                                              |             |
                                                                        RiskManager <-------+
                                                                              |
                                                                       OrderGateway (simulated
                                                                       exchange, async fills)

Logger: async, multi-producer structured logger used by every component above.
```

Each arrow between the first three stages is a dedicated OS thread, wired together in
`MarketDataFeedHandler`:

- **feed thread** reads the CSV feed, parses each line (`MarketDataParser`), and pushes `Tick`s
  into a bounded SPSC queue.
- **book thread** pops ticks, applies them to the right symbol's `OrderBook` (add/cancel/replace),
  and pushes a top-of-book `BookSignal` into a second bounded SPSC queue.
- **execution thread** pops signals and runs a minimal liquidity-taking rule in
  `ExecutionEngine`, checking every candidate order against `RiskManager` before routing it to
  `OrderGateway`.
- **gateway thread** (inside `OrderGateway`) simulates an exchange: it accepts orders
  asynchronously and always fully fills them, calling back into `ExecutionEngine::onFill` from its
  own thread.
- **logger thread** (inside `Logger`) drains a bounded, multi-producer queue that every other
  component logs into, so logging never blocks a hot path on I/O.

### Key components

| Component | Location | Responsibility |
|---|---|---|
| `MarketDataParser` | `include/mdlab/marketdata` | Parses one CSV line into a `Tick`, or a typed `ParseError` |
| `SPSCQueue<T>` | `include/mdlab/util/SPSCQueue.hpp` | Header-only bounded single-producer/single-consumer ring buffer |
| `OrderBook` | `include/mdlab/orderbook` | Per-symbol limit order book (add/cancel/replace, best bid/ask) |
| `RiskManager` / `PositionTracker` | `include/mdlab/risk` | Pre-trade limit checks and net position/notional tracking |
| `ExecutionEngine` / `OrderGateway` | `include/mdlab/execution` | Order lifecycle: signal → risk check → route → fill |
| `Logger` | `include/mdlab/logger` | Async structured logging |
| `MarketDataFeedHandler` | `include/mdlab/pipeline` | Wires the three pipeline threads together |
| `simulation` | `apps/simulation_main.cpp` | Runnable end-to-end simulation over a CSV feed |

### C++23 (and C++20) features in use

- `std::expected<Tick, ParseError>` for parser results (no exceptions on the hot path)
- `std::print` / `std::println` for formatted output, `std::format` elsewhere
- `std::jthread` + `std::stop_token` for every long-lived worker thread
- `std::flat_map` for order book price levels, with an automatic fallback to `std::map` if the
  standard library in use doesn't ship `<flat_map>` yet (see
  `include/mdlab/core/OrderedMap.hpp`) — probed at configure time
- `std::stacktrace` on the logger's fatal-log path, behind a configure-time probe (needs
  `-lstdc++exp` on libstdc++; falls back to no-op if unavailable — see `CMakeLists.txt`)
- `std::source_location` for automatic file/line capture in log calls
- Concepts (`QueueElement`) constraining `SPSCQueue<T>`
- Designated initializers throughout for aggregate construction

This was all verified against the toolchain actually present (GCC 15 / libstdc++) rather than
assumed; if you build with an older compiler and one of the probes fails, the fallback path is used
automatically and nothing breaks.

## Building

Requires CMake ≥ 3.25, Ninja, a C++23 compiler (developed against GCC 15; Clang 21+ also has the
required library support), and GoogleTest (either the system `libgtest-dev` package with its CMake
config, or `/usr/src/googletest` sources — `tests/CMakeLists.txt` falls back to building from
source automatically if `find_package(GTest)` doesn't find a config package).

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug

./build/debug/simulation data/sample_ticks.csv
```

### Presets

| Preset | Purpose |
|---|---|
| `debug` | Plain `-g` debug build, no sanitizers. Default for day-to-day work. |
| `release` | `RelWithDebInfo`, optimized — use this for anything timing-sensitive. |
| `asan-ubsan` | AddressSanitizer + UndefinedBehaviorSanitizer, `-fno-sanitize-recover=all`. Best for memory-safety and undefined-behavior issues. |
| `tsan` | ThreadSanitizer. Best for data races and lock-order-inversion (potential deadlock) issues. |

Each preset configures into its own `build/<preset>/` directory, so you can keep several around at
once. Build and test with the matching `--preset` flag, e.g. `cmake --build --preset tsan` /
`ctest --preset tsan`.

### Sanitizer incompatibilities

**ASan/UBSan and TSan cannot be combined in the same binary.** They instrument memory access and
synchronization in conflicting ways; linking both into one executable is either rejected by the
toolchain or produces unreliable results. This is why they're two separate presets rather than one
— pick whichever is relevant to what you're chasing, build clean, and don't mix object files built
under one with object files built under the other.

MemorySanitizer (uninitialized-read detection) is intentionally not provided as a preset: it
requires every piece of code it links against — including the C++ standard library — to be built
with MSan instrumentation too, which isn't available out of the box here. ASan with
`-fsanitize=address` catches a useful subset of the same class of bug (and is included in
`asan-ubsan`), just not everything MSan would.

## Regression tests

`ctest --preset <name>` runs the full GoogleTest suite under that preset. On `baseline`, every test
passes under every preset (57/57, confirmed under debug, release, asan-ubsan, and tsan). The same
suite is unchanged on every `scenario/NN` branch — it's the mechanism by which each scenario's bug
report reproduces: one or more tests (or `ctest`'s per-test timeout, for a hang) will fail there
that pass here.

## CI

`.github/workflows/ci.yml` builds and tests the `debug` and `asan-ubsan` presets on every push/PR
to `main`/`baseline`. It does **not** run on `scenario/*` branches — those are expected to fail,
crash, or hang by design, and that's not something a CI badge should be reporting on.

## Working the debugging scenarios

See [`docs/scenarios/00-index.md`](docs/scenarios/00-index.md). Each scenario is a bug report with
reproduction steps and expected-vs-actual behavior only — no root cause, no fix, no hint about the
underlying mechanism, on any branch, in any commit message, or in any test name.

```sh
git checkout scenario/01
cmake --preset debug && cmake --build --preset debug && ctest --preset debug
```
