# C++ Debugging — Tools and Commands Reference

## 1. Compilation and Debug Symbols

### `-g` — Debug Information

Instructs GCC or Clang to generate debugging information, allowing GDB to associate machine instructions with source lines, functions, types, and variables.

```bash
g++ -g main.cpp -o app
```

Without `-g`, GDB can still debug machine instructions, registers, and memory, but source-level debugging is more limited.

### `-O0` — Disable Optimization

Disables compiler optimizations, making source-level debugging more predictable.

```bash
g++ -g -O0 main.cpp -o app
```

`-g` and `-O0` are independent options.

## 2. Source Code vs. Executable

| Item | Example | Purpose |
|---|---|---|
| Source file | `tests/OrderBookTest.cpp` | Contains C++ test code |
| Executable | `build/debug/tests/mdlab_tests` | Compiled program used by GDB |
| Debug symbols | Generated using `-g` | Help GDB inspect source-level state |

## 3. CMake and CTest

Configure and build:

```bash
cmake --preset debug
cmake --build --preset debug
```

Run the full regression suite:

```bash
ctest --preset debug --output-on-failure
```

Run one test:

```bash
ctest --preset debug -R '^OrderBookTest\.CancelRemovesOrderAndEmptyLevel$' --output-on-failure
```

`-R` filters CTest test names using a regular expression.

Run a GoogleTest directly:

```bash
./build/debug/tests/mdlab_tests --gtest_filter=OrderBookTest.CancelRemovesOrderAndEmptyLevel
```

## 4. Searching Code with ripgrep

```bash
rg -n 'cancelOrder|replaceOrder|bestBid' include src
```

- `rg`: Search file contents recursively.
- `-n`: Display line numbers.
- `|`: Regular-expression OR.
- `include src`: Search within both directories.

Unlike traditional `grep`, `rg` recursively searches by default and normally respects `.gitignore`.

## 5. Finding Test Executables

```bash
find build/debug -type f -executable -name '*test*'
```

- `find`: Search filesystem paths.
- `-type f`: Regular files.
- `-executable`: Files executable by the current user.
- `-name '*test*'`: Names containing `test`.

## 6. Essential GDB Commands

Start GDB:

```bash
gdb build/debug/tests/mdlab_tests
```

Select one GoogleTest:

```gdb
set args --gtest_filter=OrderBookTest.CancelRemovesOrderAndEmptyLevel
```

Set a breakpoint:

```gdb
break mdlab::orderbook::OrderBook::removeFromLevel
```

Run the program:

```gdb
run
```

Inspect a variable:

```gdb
print quantity
```

Execute the next source line without stepping into called functions:

```gdb
next
```

Step into a function call:

```gdb
step
```

Continue until the next breakpoint:

```gdb
continue
```

Display the call stack:

```gdb
backtrace
```

Exit GDB:

```gdb
quit
```

## 7. Variable Scope in GDB

A variable defined inside a C++ block has block scope.

```cpp
if (condition) {
    auto it = container.find(key);
}
```

After stepping outside that block, GDB may report:

```text
No symbol "it" in current context.
```

This does not by itself indicate memory corruption.

## 8. C++23 std::flat_map

`std::flat_map` stores sorted keys and corresponding values in underlying sequence containers.

In Scenario 01, GDB displayed:

```text
keys   = {100}
values = {0}
```

Changing the mapped quantity to zero does not automatically remove the price level.

A container element must be explicitly erased when the application's logic requires its removal.

## 9. Debugging Workflow

1. Reproduce the failure.
2. Identify the failing test.
3. Locate relevant source code.
4. Form a hypothesis.
5. Set a breakpoint.
6. Inspect runtime state.
7. Confirm the root cause.
8. Implement a fix.
9. Run regression tests.
10. Document the results.
