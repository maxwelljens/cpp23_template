#pragma once

#include <expected>
#include <span>
#include <string>
#include <concepts>

// ── Constrained template using Concepts ────────────────────────────────────
auto add(std::integral auto a, std::integral auto b)
{
    return a + b;
}

// ── Error handling with std::expected (C++23) ────────────────────────────
std::expected<int, std::string> divide(int a, int b);

// ── Span instead of pointer + size ─────────────────────────────────────────
int sum(std::span<const int> values);
