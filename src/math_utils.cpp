#include "math_utils.hpp"

auto divide(int a, int b) -> std::expected<int, std::string>
{
    if (b == 0)
        return std::unexpected("division by zero");
    return a / b;
}

auto sum(std::span<const int> values) -> int
{
    int total = 0;
    for (auto v : values)
        total += v;
    return total;
}
