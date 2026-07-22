#include <expected>
#include <span>
#include <string>
#include <print>
#include <vector>
#include <array>
#include <ranges>
#include <concepts>

// ── Constrained template using Concepts (C++20) ───────────────────────────
auto add(std::integral auto a, std::integral auto b)
{
    return a + b;
}

// ── Error handling with std::expected (C++23) ────────────────────────────
std::expected<int, std::string> divide(int a, int b)
{
    if (b == 0)
        return std::unexpected("division by zero");
    return a / b;
}

// ── Span instead of pointer + size ─────────────────────────────────────────
int sum(std::span<const int> values)
{
    int total = 0;
    for (auto v : values)
        total += v;
    return total;
}

// ── Deducing this (C++23) ──────────────────────────────────────────────────
struct Accumulator
{
    std::vector<int> data;

    void add(int value) { data.push_back(value); }

    auto& back(this auto&& self) { return self.data.back(); }
};

auto main() -> int
{
    std::println("=== Guarded C++23 ===");

    // 1. Concepts — readable constrained templates
    std::println("add(3, 4)           = {}", add(3, 4));

    // 2. std::expected — explicit error paths
    auto result = divide(10, 2);
    if (result) {
        std::println("divide(10, 2)       = {}", *result);
    }

    auto bad = divide(10, 0);
    if (not bad) {
        std::println("divide(10, 0)       = error: {}", bad.error());
    }

    // 3. std::span — safe bounds-checked views
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
