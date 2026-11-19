# C++23 Template

A project skeleton that configures the C++23 toolchain to eliminate entire
classes of bugs at compile time.  The goal is to make the compiler a strict
gatekeeper — catching mistakes that would otherwise become silent corruption,
security holes, or debugging nightmares.

This is *not* a style guide or a framework.  It's a build configuration that
you drop into any C++23 project.  The trade-off is negligible: a handful of
compiler flags and one test framework dependency in exchange for
deterministic, early-failure builds.

## Using it for your own project

Change **one line** and you're done:

```cmake
project(your-project-name VERSION 0.1.0 LANGUAGES CXX)   # line 2
```

From that, everything derives automatically:

| What | How it's derived | Example |
|------|-----------------|---------|
| Target: library | `${PROJECT_NAME}_lib` | `your-project-name_lib` |
| Target: executable | `${PROJECT_NAME}` | `your-project-name` |
| Target: tests | `${PROJECT_NAME}_test` | `your-project-name_test` |
| CMake option prefix | `PROJECT_NAME` uppercased, non-alnum → `_` | `YOUR_PROJECT_NAME_ENABLE_SANITIZERS` |
| Output binary | `${PROJECT_NAME}` | `build/src/your-project-name` |

Then drop your source into `src/` and your tests into `tests/`, replacing
the demonstration files.  The three helper functions —
`apply_firewall()`, `apply_sanitizers()`, `apply_stl_hardening()` —
are available to every target you add.

## What the build enforces

**The compiler treats warnings as errors.**  `-Werror` is non-negotiable.
The specific flags that ship with the template catch shadowing
(`-Wshadow`), implicit truncation (`-Wconversion`, `-Wsign-conversion`),
alignment issues (`-Wcast-align`), null-pointer dereferences
(`-Wnull-dereference`), printf format mismatches (`-Wformat=2`), implicit
fallthrough (`-Wimplicit-fallthrough`), C-style casts (`-Wold-style-cast`),
non-virtual destructors in polymorphic classes (`-Wnon-virtual-dtor`), and
overloaded-virtual mismatches (`-Woverloaded-virtual`).  Every one of these
has been responsible for production incidents.

**STL hardening turns undefined behavior into a clean abort.**  By default
`std::vector::operator[]` does not check bounds.  With `_GLIBCXX_ASSERTIONS`
enabled, an out-of-bounds access crashes immediately at the point of
failure instead of silently corrupting memory and manifesting hours later
in an unrelated function.  This covers `operator[]` on the standard
containers and strings, plus `front()`, `back()`, and `std::span`.

The define is deliberately `_GLIBCXX_ASSERTIONS` rather than the stronger
`_GLIBCXX_DEBUG`: that define replaces standard containers with checked
wrappers that have a different layout and mangling, which is only safe if
*every* translation unit in the binary — including third-party static
libraries such as Catch2 — is compiled with it.  That would force a global
define, contradicting the per-target firewall.  `_GLIBCXX_ASSERTIONS` has no
ABI effect, so it is attached to the library as a `PUBLIC` usage
requirement: the executable and the tests inherit the same define through
linking.  It is gated to Debug builds, so Release builds pay no runtime
cost.

**Sanitisers catch what the compiler cannot see at compile time.**
UndefinedBehaviorSanitizer instruments the binary to detect integer
overflow, misaligned access, null-pointer dereferences through
references, and violated type-punning rules at runtime.  The template
also probes for AddressSanitizer + LeakSanitizer at configure time: if the
runtime library is present on the system, use-after-free,
heap-buffer-overflow, and memory leaks are detected automatically.  If
ASan is absent, UBSan still runs — no failure, no configuration change
needed.  The whole block is gated by `<PREFIX>_ENABLE_SANITIZERS`, and
the probe is skipped entirely when the option is OFF.

**clang-tidy adds a second opinion.**  The included `.clang-tidy`
applies the C++ Core Guidelines, banning raw memory ownership, pointer
arithmetic, and C-style arrays at the static-analysis level.  It is
optional and pinned to version 19 or newer — earlier versions do not
understand `-std=c++23`.  It is attached per target by `apply_firewall()`,
and a `-header-filter` limits diagnostics to this project's `src/` and
`tests/` trees — so neither analysis nor reporting ever touches
third-party code such as Catch2.  The compiler firewall already catches
the vast majority of issues; clang-tidy is a safety net for the remaining
patterns that are technically valid C++ but indicative of fragile design.

## The C++23 conventions the template demonstrates

These are *not* enforced by the build — they cannot be, because they are
design choices, not syntax errors.  The template demonstrates them in
`src/main.cpp` and `src/math_utils.hpp` so that a contributor sees the
intended style on first read — the executable includes the library header
and uses its functions rather than re-implementing them.

**`std::println` over `printf` or `std::cout`.**  Type-safe,
buffer-overflow-proof, and Python-like formatting syntax.  `printf` relies
on format strings that the compiler cannot fully validate at the call site;
`std::cout` chains `operator<<` in ways that are easy to misorder.

**`std::expected<T, E>` over exceptions.**  Exceptions create invisible
control-flow paths that must be mentally traced through every function
call.  `std::expected` makes failure a value: the function signature
advertises that something can go wrong, and the caller handles both the
value and the error paths explicitly.  This is closer to how Rust's
`Result` works and produces code that is easier to audit.

**Concepts over SFINAE.**  The `std::enable_if` / `std::declval` pattern
is famously unreadable and produces compiler errors that are difficult to
decipher.  Concepts (`std::integral`, `std::floating_point`, or your own
`requires` expressions) constrain templates at the function signature level
and produce clear, self-documenting diagnostics.

**`std::span` over pointer + size pairs.**  A raw pointer gives no
information about how many elements it points to.  `std::span` carries
the size along with the data, supports range-based `for`, and works with
any contiguous container without the caller having to know the difference.

**Deducing `this` over const/non-const overload duplication.**  Writing
separate `const` and non-`const` versions of the same member function is
boilerplate that invites copy-paste errors.  C++23's deducing `this` lets
a single member function handle both cases.

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/src/your-project-name
ctest --test-dir build
```

### Makefile shortcuts

The convenience `Makefile` derives the target names from the `project()`
line (so renaming the project needs no Makefile edits) and wraps the
common workflow:

```bash
make build     # configure + build (Debug)
make run       # build and run the executable
make test      # build and run the test suite
make rebuild   # clean + build
```

### Options

Options are prefixed with your project name (uppercased, non-alphanumeric
characters replaced by underscores).  For the default project name
`cpp23-template` the prefix is `CPP23_TEMPLATE`:

| Flag | Default | What it controls |
|------|---------|------------------|
| `<PREFIX>_ENABLE_CLANG_TIDY=ON` | OFF | clang-tidy with C++ Core Guidelines (needs v19+), project targets only |
| `<PREFIX>_ENABLE_SANITIZERS=ON` | ON | UBSan (+ ASan+LSan if linkable); OFF removes all sanitizer flags |
| `<PREFIX>_ENABLE_STL_HARDENING=ON` | ON | `_GLIBCXX_ASSERTIONS` bounds checking (Debug only) |
| `<PREFIX>_BUILD_TESTS=ON` | ON | Catch2 v3 test suite |

### Requirements

- **CMake** ≥ 3.28
- **GCC** ≥ 14 (for C++23 features)
- **clang-tidy** ≥ 19 (optional, for static analysis)

## Project structure

```
CMakeLists.txt        →  compiler firewall, sanitisers, hardening (change project() on line 2)
.clang-tidy           →  optional C++ Core Guidelines layer
Makefile              →  convenience wrapper (derives target names from project())
src/
  CMakeLists.txt      →  derives target names from ${PROJECT_NAME}
  main.cpp            →  C++23 feature demonstrations
  math_utils.hpp/.cpp →  example library with std::expected + std::span
tests/
  CMakeLists.txt      →  derives test target from ${PROJECT_NAME}
  test_math.cpp       →  Catch2 v3 unit tests
.gitignore
```

The library and tests exist only to show the toolchain in action.  In your
own project, replace them with your code — the CMake helper functions carry
the safety without leaking flags into third-party dependencies.
