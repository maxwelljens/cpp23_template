# AGENTS.md — C++23 Template Project

## Quick Start

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/src/cpp23-template      # run binary
ctest --test-dir build           # run tests
```

**Requirements**: CMake ≥ 3.28, GCC ≥ 14. clang-tidy ≥ 19 optional.

---

## Build System (CMake)

### Key mechanics

- **Everything derives from `project(...)` on line 2 of root `CMakeLists.txt`**.
  Change one line and every target name, option prefix, and binary name follows.
- **Compiler firewall is per-target, not global.** `apply_firewall(target)` must be
  called explicitly for each target you own. Global `add_compile_options` would
  leak into third-party code (Catch2, etc.).
- **Sanitizers auto-probe ASan.** The build tries to compile+link a tiny test with
  `-fsanitize=address,leak`. If it works, ASan+LSan+UBSan all run in Debug builds.
  If not, only UBSan runs — no failure, no config change.

### Target naming convention

| What | Name | Example |
|------|------|---------|
| Library | `${PROJECT_NAME}_lib` | `cpp23-template_lib` |
| Executable | `${PROJECT_NAME}` | `cpp23-template` |
| Tests | `${PROJECT_NAME}_test` | `cpp23-template_test` |

### CMake options (prefix = `CPP23_TEMPLATE` for this project)

| Flag | Default | Notes |
|------|---------|-------|
| `CPP23_TEMPLATE_ENABLE_CLANG_TIDY=ON` | OFF | Needs clang-tidy ≥ 19 (C++23 support) |
| `CPP23_TEMPLATE_ENABLE_SANITIZERS=ON` | ON | UBSan always; ASan+LSan if linkable |
| `CPP23_TEMPLATE_ENABLE_STL_HARDENING=ON` | ON | `_GLIBCXX_DEBUG` bounds checking |
| `CPP23_TEMPLATE_BUILD_TESTS=ON` | ON | Fetches Catch2 v3.7.0 via FetchContent |

**Renaming the project changes the prefix.** If `project(foo-bar ...)` then prefix
becomes `FOO_BAR`. All four option flags rename themselves automatically.

---

## Project Structure

```
CMakeLists.txt          ← compiler firewall, sanitizers, options (change project() line)
.clang-tidy             ← C++ Core Guidelines (optional, v19+)
.gitignore
AGENTS.md
README.md
src/
  CMakeLists.txt        ← library + executable targets
  main.cpp              ← C++23 feature demonstrations
  math_utils.hpp        ← example library header (#pragma once)
  math_utils.cpp        ← example library implementation
tests/
  CMakeLists.txt        ← Catch2 v3 fetched from GitHub
  test_main.cpp         ← intentionally BLANK (Catch2WithMain provides main())
  test_math.cpp         ← unit tests for math_utils
build_rel/              ← stale build (from when project was named "example")
```

---

## C++23 Conventions (demonstrated in src/)

### Always use

- **`std::println`** — type-safe, Python-like formatting. Never `printf` or `std::cout`.
- **`std::expected<T, E>`** — explicit failure-as-value. Never exceptions for recoverable errors.
- **Concepts** — `std::integral`, `std::floating_point`, custom `requires` clauses. Never SFINAE.
- **`std::span`** — bounds-safe views. Never pointer + size pairs.
- **Deducing `this`** — single member function handling both `const` and non-`const`. Never duplicated overloads.
- **`#pragma once`** — header guards.
- **Trailing return type** — `auto fn() -> int` style in implementation files (not required but demonstrated in `math_utils.cpp`).

### What to avoid

- C-style casts (caught by `-Wold-style-cast`).
- Raw `new`/`delete` (caught by clang-tidy C++ Core Guidelines).
- Pointer arithmetic (caught by clang-tidy).
- `printf`, `std::cout` chains.

---

## Testing (Catch2 v3)

- Catch2 is fetched via `FetchContent` from `https://github.com/catchorg/Catch2.git` (tag `v3.7.0`).
- **`test_main.cpp` is intentionally empty.** The `Catch2::Catch2WithMain` target provides `main()`.
- Tests use `TEST_CASE("name", "[tag]")` with `REQUIRE` (fatal on failure) and `CHECK` (non-fatal).
- Test files forward-declare library functions (no header includes the `.cpp` — the linker resolves against `*_lib.a`).
- Add new test files to `tests/CMakeLists.txt` in the `add_executable(...)` call.

---

## Key Gotchas

### 1. clangd LSP diagnostics are false positives for C++23 stdlib

clangd (the language server) will report errors like:
```
No template named 'expected' in namespace 'std'
No member named 'span' in namespace 'std'
No type named 'integral' in namespace 'std'
```

These are **expected** when clangd uses a clang < 19 that does not support
C++23 standard library features. **The project compiles fine with GCC 14+.**
Do NOT alter working code just to silence clangd. Trust the compiler, not the LSP.

### 2. Per-target firewall is mandatory

`apply_firewall()`, `apply_sanitizers()`, and `apply_stl_hardening()` must be
called explicitly for every new target. They are NOT inherited from the
directory. Example for a new `src/foo.cpp`:

```cmake
add_library(my_new_lib foo.cpp)
apply_firewall(my_new_lib)
apply_sanitizers(my_new_lib)
apply_stl_hardening(my_new_lib)
```

### 3. Option names change when the project is renamed

If you change `project(cpp23-template ...)` to `project(my-app ...)`, all CMake
flags become `MY_APP_ENABLE_CLANG_TIDY`, `MY_APP_BUILD_TESTS`, etc. The
`PROJ_PREFIX` variable in CMakeLists.txt auto-derives this, but any CI script or
`.clang-tidy` comment referencing `EXAMPLE_ENABLE_*` will need updating.

### 4. `add()` is defined in both the header and `main.cpp`

The template demonstrates the same `add()` constrained template in both
`math_utils.hpp` and `main.cpp`. This is intentional redundancy for a demo
project. In real code, define constrained templates once in a header.

### 5. build_rel directory is stale

The `build_rel/` directory was configured when the project was named `example`
with `BUILD_TESTS=OFF`. Delete it and regenerate.

---

## Adding New Code

1. Create `.hpp` and `.cpp` in `src/`.
2. Add to `src/CMakeLists.txt`:
   - Library: add sources to `add_library(${PROJECT_NAME}_lib ...)`.
   - New executable: call `add_executable(...)`, link `${PROJECT_NAME}_lib`, call all three helper functions.
3. Create test file in `tests/`, add to `tests/CMakeLists.txt`.
4. Forward-declare symbols in the test file (header is fine too if the library exposes it).

---

## Style & Formatting

- 4-space indentation (observed throughout).
- Opening braces on their own line (Allman style — see `main.cpp`).
- `auto` preferred for variable declarations and function return types.
- Comment separators with `// ── ... ──` for section dividers.
