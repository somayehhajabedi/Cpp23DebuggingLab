# Debugging Scenarios

Each row below is one isolated, reproducible defect reported against an otherwise-working build.
Every scenario lives on its own branch, created from `baseline`, with exactly one behavioral change
relative to `baseline`. Nothing else in the codebase is touched.

To work a scenario:

```sh
git checkout scenario/01   # pick a scenario branch
cmake --preset debug       # or asan-ubsan / tsan / release — see the root README
cmake --build --preset debug
ctest --preset debug
```

The regression test suite (`tests/`) is identical across every branch — it's the same suite that
passes cleanly on `baseline`. On a scenario branch, one or more of those tests (or the `simulation`
binary itself) will misbehave; that's your reproduction. The bug reports below intentionally describe
only the observed symptom, how to trigger it, and what should have happened instead — not the
mechanism behind it.

| # | Branch | Report | Symptom |
|---|--------|--------|---------|
| 01 | `scenario/01` | [scenario-01.md](scenario-01.md) | Top-of-book occasionally reports bid above ask |
| 02 | `scenario/02` | [scenario-02.md](scenario-02.md) | Feed handler crashes on a malformed input line |
| 03 | `scenario/03` | [scenario-03.md](scenario-03.md) | Garbage output / crash after an order fills |
| 04 | `scenario/04` | [scenario-04.md](scenario-04.md) | Heap corruption reported under sustained load |
| 05 | `scenario/05` | [scenario-05.md](scenario-05.md) | Consumer observes a price that was never sent |
| 06 | `scenario/06` | [scenario-06.md](scenario-06.md) | Risk limit is sometimes not enforced under load |
| 07 | `scenario/07` | [scenario-07.md](scenario-07.md) | Simulation hangs indefinitely with no error output |
| 08 | `scenario/08` | [scenario-08.md](scenario-08.md) | Feed processing has become much slower |
| 09 | `scenario/09` | [scenario-09.md](scenario-09.md) | Order exactly at the risk limit is handled inconsistently |
| 10 | `scenario/10` | [scenario-10.md](scenario-10.md) | Very large order produces a nonsensical notional value |

No root cause, fix, or hint about the mechanism is recorded anywhere in this repository — on any
branch, in any commit message, or in any test name. That's intentional.
