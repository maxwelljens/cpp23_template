# AGENTS.md — C++23 Template Project

## Quick Start

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/src/cpp23-template      # run binary
ctest --test-dir build           # run tests
```

Or via the convenience `Makefile` (derives target names from `project()`):

```bash
make build    # configure + build (Debug)
make run      # build and run the executable
make test     # build and run the test suite
make rebuild  # clean + build
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
  If not, only UBSan runs — no failure, no config change. The probe is skipped
  entirely when `<PREFIX>_ENABLE_SANITIZERS=OFF`, which removes all sanitizer flags.

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
| `CPP23_TEMPLATE_ENABLE_SANITIZERS=ON` | ON | UBSan always; ASan+LSan if linkable (probe skipped when OFF) |
| `CPP23_TEMPLATE_ENABLE_STL_HARDENING=ON` | ON | `_GLIBCXX_ASSERTIONS` bounds checking (Debug only) |
| `CPP23_TEMPLATE_BUILD_TESTS=ON` | ON | Fetches Catch2 v3.16.1 via FetchContent |

**Renaming the project changes the prefix.** If `project(foo-bar ...)` then prefix
becomes `FOO_BAR`. All four option flags rename themselves automatically.

---

## Project Structure

```
CMakeLists.txt          ← compiler firewall, sanitizers, options (change project() line)
.clang-tidy             ← C++ Core Guidelines (optional, v19+)
.gitignore
AGENTS.md
Makefile                ← convenience wrapper (derives target names from project())
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

- Catch2 is fetched via `FetchContent` from `https://github.com/catchorg/Catch2.git` (tag `v3.16.1`).
- **`test_main.cpp` is intentionally empty.** The `Catch2::Catch2WithMain` target provides `main()`.
- Tests use `TEST_CASE("name", "[tag]")` with `REQUIRE` (fatal on failure) and `CHECK` (non-fatal).
- Test files `#include` the public header `math_utils.hpp` (exported via
  `target_include_directories(... PUBLIC ...)`) and link `${PROJECT_NAME}_lib`.
  The header is the single source of truth for signatures — a signature change
  becomes a compile error, not a link error.
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
`PROJ_PREFIX` variable in CMakeLists.txt auto-derives this, and both `.clang-tidy`
(a `<PREFIX>` placeholder comment) and `Makefile` (parses `project()` from the root
CMakeLists) follow automatically. Only CI scripts or other external references to
the old prefix will need updating.

### 4. Library functions are defined once, in the header

`add()`, `divide()`, and `sum()` are defined in `math_utils.hpp` (with
implementations in `math_utils.cpp`); `main.cpp` includes the header and uses
them. Do NOT re-declare or re-implement library symbols in `main.cpp` — include
the header instead. This keeps the executable wired to the library it links.

### 5. Stale build directories break reconfiguration

CMake stores the absolute source path in `build/CMakeCache.txt`. After moving or
renaming the checkout — or after changing how targets are configured — wipe
`build/` (and any `build_*`) before reconfiguring, or configure fails with
`CMakeCache.txt directory ... is different than ...`. (A leftover `build_rel/`
from the old `example` project name has been removed.)

### 6. STL hardening uses `_GLIBCXX_ASSERTIONS`, not `_GLIBCXX_DEBUG`

`_GLIBCXX_ASSERTIONS` is ABI-safe: it adds bounds checks to `operator[]` etc.
without changing container layout or mangling, so it can be applied per target.
`_GLIBCXX_DEBUG` swaps in checked wrappers with a different ABI (`std::vector`
grows from 24 to 56 bytes) and is only safe if EVERY translation unit in the
binary — including third-party static libs like Catch2 — is compiled with it.
Do not switch to `_GLIBCXX_DEBUG` unless you also apply it globally to all
targets and dependencies. Hardening is attached `PUBLIC` on the library, so
`main.cpp` and the tests inherit it through linking; it is Debug-only.

---

## Adding New Code

1. Create `.hpp` and `.cpp` in `src/`.
2. Add to `src/CMakeLists.txt`:
   - Library: add sources to `add_library(${PROJECT_NAME}_lib ...)`.
   - New executable: call `add_executable(...)`, link `${PROJECT_NAME}_lib`, call all three helper functions.
3. Create test file in `tests/`, add to `tests/CMakeLists.txt`.
4. `#include` the public header in the test file (the library exports its
   include directory as `PUBLIC`, so it resolves through linking).

---

## Style & Formatting

- 4-space indentation (observed throughout).
- Opening braces on their own line (Allman style — see `main.cpp`).
- `auto` preferred for variable declarations and function return types.
- Comment separators with `// ── ... ──` for section dividers.
