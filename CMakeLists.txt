cmake_minimum_required(VERSION 3.28)
project(cpp23-template VERSION 0.1.0 LANGUAGES CXX)

# ── Option prefix derived from project name ────────────────────────────────
#   Change only the project() line above.  Everything else follows.
string(TOUPPER "${PROJECT_NAME}" _raw)
string(REGEX REPLACE "[^A-Z0-9]" "_" PROJ_PREFIX "${_raw}")

# ── C++23 Standard ──────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Compiler Firewall (applied PER-TARGET, not globally) ───────────────────
#   Global add_compile_options would break third-party code (Catch2, etc.).
#   Use apply_firewall() on each target you own.
set(FIREWALL_FLAGS
    -Wall -Wextra -Wpedantic -Werror
    -Wshadow -Wconversion -Wsign-conversion -Wcast-align
    -Wnull-dereference -Wformat=2 -Wimplicit-fallthrough
    -Wold-style-cast -Wnon-virtual-dtor -Woverloaded-virtual
)

function(apply_firewall target)
    target_compile_options(${target} PRIVATE ${FIREWALL_FLAGS})
endfunction()

# ── STL Hardening (libstdc++) ──────────────────────────────────────────────
set(STL_HARDENING
    -D_GLIBCXX_DEBUG
    -D_GLIBCXX_DEBUG_PEDANTIC
)

# ── Sanitizers (Debug only, per-target) ────────────────────────────────────
set(SANITIZERS_BASE -fno-omit-frame-pointer)

set(_ASAN_LINKABLE OFF)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    file(WRITE "${CMAKE_BINARY_DIR}/asan_test.cpp" "int main(){return 0;}\n")
    try_compile(_ASAN_LINKABLE
        "${CMAKE_BINARY_DIR}/asan_probe"
        SOURCES "${CMAKE_BINARY_DIR}/asan_test.cpp"
        COMPILE_DEFINITIONS "-fsanitize=address,leak" "-fno-omit-frame-pointer"
        LINK_OPTIONS "-fsanitize=address,leak"
        OUTPUT_VARIABLE _ASAN_LOG
    )
    file(REMOVE "${CMAKE_BINARY_DIR}/asan_test.cpp")
    file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/asan_probe")
endif()

if(_ASAN_LINKABLE)
    set(SANITIZERS -fsanitize=address,undefined,leak ${SANITIZERS_BASE})
    message(STATUS "sanitizers: UBSan + ASan + LSan")
else()
    set(SANITIZERS -fsanitize=undefined ${SANITIZERS_BASE})
    message(STATUS "sanitizers: UBSan only  (ASan link-test failed)")
endif()

function(apply_sanitizers target)
    target_compile_options(${target} PRIVATE $<$<CONFIG:Debug>:${SANITIZERS}>)
    target_link_options(${target}    PRIVATE $<$<CONFIG:Debug>:${SANITIZERS}>)
endfunction()

function(apply_stl_hardening target)
    target_compile_definitions(${target} PRIVATE ${STL_HARDENING})
endfunction()

# ── Options (prefix auto-derived from project name) ────────────────────────
option(${PROJ_PREFIX}_ENABLE_CLANG_TIDY "Run clang-tidy (requires v19+)" OFF)
option(${PROJ_PREFIX}_ENABLE_SANITIZERS  "Enable runtime sanitizers"     ON)
option(${PROJ_PREFIX}_ENABLE_STL_HARDENING "Enable STL hardening"        ON)
option(${PROJ_PREFIX}_BUILD_TESTS       "Build test suite"               ON)

# ── clang-tidy (optional, v19+ required for C++23) ──────────────────────────
if(${PROJ_PREFIX}_ENABLE_CLANG_TIDY)
    find_program(_CTIDY NAMES clang-tidy-19 clang-tidy)
    if(_CTIDY)
        execute_process(COMMAND ${_CTIDY} --version
            OUTPUT_VARIABLE _CTIDY_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(_CTIDY_VERSION MATCHES "version ([0-9]+)")
            set(_CTIDY_VER ${CMAKE_MATCH_1})
        endif()
        if(_CTIDY_VER GREATER_EQUAL 19)
            set(CMAKE_CXX_CLANG_TIDY "${_CTIDY}")
            message(STATUS "clang-tidy: ${_CTIDY} (v${_CTIDY_VER})")
        else()
            message(WARNING "clang-tidy found v${_CTIDY_VER} (< 19) — skipping")
        endif()
    else()
        message(WARNING "clang-tidy requested but not found — skipping")
    endif()
endif()

# ── Subdirectories ─────────────────────────────────────────────────────────
add_subdirectory(src)

if(${PROJ_PREFIX}_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
