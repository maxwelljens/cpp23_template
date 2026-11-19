#include <catch2/catch_test_macros.hpp>

#include "math_utils.hpp"

#include <array>
#include <vector>

TEST_CASE("divide", "[math]")
{
    auto ok = divide(10, 2);
    REQUIRE(ok.has_value());
    CHECK(*ok == 5);

    auto fail = divide(10, 0);
    REQUIRE(not fail.has_value());
    CHECK(fail.error() == "division by zero");

    CHECK(divide(7, 3) == 2);  // integer division
}

TEST_CASE("sum", "[math]")
{
    std::array a{1, 2, 3, 4, 5};
    CHECK(sum(a) == 15);

    std::vector v{10, 20, 30};
    CHECK(sum(v) == 60);

    CHECK(sum(std::span<const int>{}) == 0);
}
