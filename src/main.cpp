#include "math_utils.hpp"

#include <array>
#include <print>
#include <ranges>
#include <string>
#include <vector>

// ── Deducing this (C++23) ──────────────────────────────────────────────────
struct Accumulator
{
    std::vector<int> data;

    void add(int value) { data.push_back(value); }

    auto& back(this auto&& self) { return self.data.back(); }
};

// noexcept + NOLINT: bugprone-exception-escape demands main handle every
// exception, but this demo has no recovery path — an escaping exception
// terminates deliberately (std::println/format can throw on stream errors).
// The check stays enabled project-wide for destructors and move operations.
auto main() noexcept -> int // NOLINT(bugprone-exception-escape)
{
    std::println("=== Guarded C++23 ===");

    // 1. Concepts — readable constrained templates (defined in math_utils.hpp)
    std::println("add(3, 4)           = {}", add(3, 4));

    // 2. std::expected — explicit error paths (divide() in math_utils.hpp)
    auto result = divide(10, 2);
    if (result) {
        std::println("divide(10, 2)       = {}", *result);
    }

    auto bad = divide(10, 0);
    if (not bad) {
        std::println("divide(10, 0)       = error: {}", bad.error());
    }

    // 3. std::span — safe bounds-checked views (sum() in math_utils.hpp)
    std::array arr{1, 2, 3, 4, 5};
    std::println("sum(arr)            = {}", sum(arr));

    // 4. std::vector + ranges (no raw pointers, no C-arrays)
    std::vector<int> vec = {10, 20, 30};
    for (auto [i, v] : vec | std::views::enumerate) {
        std::println("  vec[{}] = {}", i, v);
    }

    // 5. Deducing this — one overload for const + non-const
    Accumulator acc;
    acc.add(42);
    acc.add(99);
    std::println("acc.back()          = {}", acc.back());

    // 6. std::println — type-safe, Python-like formatting
    std::println("Hello, Guarded C++23!");

    return 0;
}
