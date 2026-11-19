cmake_minimum_required(VERSION 3.28)
project(cpp23-template VERSION 0.1.0 LANGUAGES CXX)

# ── Option prefix derived from project name ────────────────────────────────
#   Change only the project() line above.  Everything else follows.
string(TOUPPER "${PROJECT_NAME}" _raw)
string(REGEX REPLACE "[^A-Z0-9]" "_" PROJ_PREFIX "${_raw}")

# ── Options (prefix auto-derived from project name) ────────────────────────
option(${PROJ_PREFIX}_ENABLE_CLANG_TIDY "Run clang-tidy (requires v19+)" OFF)
option(${PROJ_PREFIX}_ENABLE_SANITIZERS  "Enable runtime sanitizers"     ON)
option(${PROJ_PREFIX}_ENABLE_STL_HARDENING "Enable STL hardening"        ON)
option(${PROJ_PREFIX}_BUILD_TESTS       "Build test suite"               ON)

# ── C++23 Standard ──────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # for clangd / clang-tidy tooling

# ── Compiler Firewall (applied PER-TARGET, not globally) ───────────────────
#   Global add_compile_options would break third-party code (Catch2, etc.).
#   Use apply_firewall() on each target you own.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(FIREWALL_FLAGS
        -Wall -Wextra -Wpedantic -Werror
        -Wshadow -Wconversion -Wsign-conversion -Wcast-align
        -Wnull-dereference -Wformat=2 -Wimplicit-fallthrough
        -Wold-style-cast -Wnon-virtual-dtor -Woverloaded-virtual
    )
elseif(MSVC)
    set(FIREWALL_FLAGS /W4 /WX /permissive-)
else()
    set(FIREWALL_FLAGS)
    message(WARNING "firewall: unknown compiler '${CMAKE_CXX_COMPILER_ID}' — warning flags disabled")
endif()

# ── clang-tidy (optional, v19+ required for C++23) ──────────────────────────
#   Attached per-target in apply_firewall() so it never runs against
#   third-party code such as Catch2.
set(CTIDY_COMMAND "")
if(${PROJ_PREFIX}_ENABLE_CLANG_TIDY)
    find_program(_CTIDY NAMES clang-tidy-19 clang-tidy-20 clang-tidy)
    if(_CTIDY)
        execute_process(COMMAND ${_CTIDY} --version
            OUTPUT_VARIABLE _CTIDY_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        set(_CTIDY_VER "")
        if(_CTIDY_VERSION MATCHES "version ([0-9]+)")
            set(_CTIDY_VER ${CMAKE_MATCH_1})
        endif()
        if(_CTIDY_VER AND _CTIDY_VER GREATER_EQUAL 19)
            set(CTIDY_COMMAND "${_CTIDY}")
            message(STATUS "clang-tidy: ${_CTIDY} (v${_CTIDY_VER})")
        else()
            message(WARNING "clang-tidy found v${_CTIDY_VER} (< 19) — skipping")
        endif()
    else()
        message(WARNING "clang-tidy requested but not found — skipping")
    endif()
endif()

function(apply_firewall target)
    target_compile_options(${target} PRIVATE ${FIREWALL_FLAGS})
    if(CTIDY_COMMAND)
        # Lint project code only: without a header filter clang-tidy reports
        # diagnostics from EVERY included header — including Catch2's
        # generated config header under _deps — failing the build on
        # third-party code.  The filter matches this project's src/ and
        # tests/ trees (rename-proof: built from CMAKE_SOURCE_DIR).
        set_target_properties(${target} PROPERTIES
            CXX_CLANG_TIDY "${CTIDY_COMMAND};-header-filter=${CMAKE_SOURCE_DIR}/(src|tests)/")
    endif()
endfunction()

# ── Sanitizers (Debug only, per-target, option-gated) ──────────────────────
set(SANITIZERS_BASE -fno-omit-frame-pointer)
set(SANITIZERS "")

if(NOT ${PROJ_PREFIX}_ENABLE_SANITIZERS)
    message(STATUS "sanitizers: disabled (${PROJ_PREFIX}_ENABLE_SANITIZERS=OFF)")
elseif(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(WARNING "sanitizers: unsupported compiler '${CMAKE_CXX_COMPILER_ID}' — skipping")
else()
    # Probe whether ASan + LSan can actually be linked on this system.
    # The flags must travel via CMAKE_FLAGS / LINK_OPTIONS: try_compile's
    # COMPILE_DEFINITIONS is for -D macros only, and passing -fsanitize there
    # makes the probe fail even when ASan works.
    unset(_ASAN_LINKABLE CACHE)   # re-probe on every configure, no stale cache
    file(WRITE "${CMAKE_BINARY_DIR}/asan_test.cpp" "int main(){return 0;}\n")
    try_compile(_ASAN_LINKABLE
        "${CMAKE_BINARY_DIR}/asan_probe"
        SOURCES "${CMAKE_BINARY_DIR}/asan_test.cpp"
        CMAKE_FLAGS "-DCMAKE_CXX_FLAGS=-fsanitize=address,leak -fno-omit-frame-pointer"
        LINK_OPTIONS "-fsanitize=address,leak"
        OUTPUT_VARIABLE _ASAN_LOG
    )
    file(REMOVE "${CMAKE_BINARY_DIR}/asan_test.cpp")
    file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/asan_probe")
    if(_ASAN_LINKABLE)
        set(SANITIZERS -fsanitize=address,undefined,leak ${SANITIZERS_BASE})
        message(STATUS "sanitizers: UBSan + ASan + LSan")
    else()
        set(SANITIZERS -fsanitize=undefined ${SANITIZERS_BASE})
        message(STATUS "sanitizers: UBSan only  (ASan link-test failed)")
    endif()
endif()

function(apply_sanitizers target)
    if(SANITIZERS)
        target_compile_options(${target} PRIVATE $<$<CONFIG:Debug>:${SANITIZERS}>)
        target_link_options(${target}    PRIVATE $<$<CONFIG:Debug>:${SANITIZERS}>)
    endif()
endfunction()

# ── STL Hardening (libstdc++, Debug only) ──────────────────────────────────
#   _GLIBCXX_ASSERTIONS turns out-of-bounds access on standard containers,
#   strings, and spans into an immediate abort instead of silent corruption.
#
#   Deliberately NOT _GLIBCXX_DEBUG: that define swaps containers for checked
#   wrappers with a different layout and mangling, which is only safe if EVERY
#   translation unit in the binary — including third-party static libraries
#   such as Catch2 — is compiled with it.  That would force a global define,
#   contradicting the per-target firewall.  _GLIBCXX_ASSERTIONS has no ABI
#   effect, so it can be attached per target.
#
#   Attached PUBLIC on the library so every consumer (executable, tests)
#   inherits the same define; gated to Debug because hardening is a
#   debugging aid, not a Release tax.
set(STL_HARDENING _GLIBCXX_ASSERTIONS _GLIBCXX_ASSERTIONS_PEDANTIC)

function(apply_stl_hardening target)
    if(${PROJ_PREFIX}_ENABLE_STL_HARDENING)
        target_compile_definitions(${target} PUBLIC $<$<CONFIG:Debug>:${STL_HARDENING}>)
    endif()
endfunction()

# ── Subdirectories ─────────────────────────────────────────────────────────
add_subdirectory(src)

if(${PROJ_PREFIX}_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
