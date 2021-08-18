#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <math/Math.h>

TEST_CASE("Tests test")
{
    CHECK(true == true);

    CHECK(Math::OverflowSumInt(1, 3, 0, 3) == 0);
    CHECK(Math::OverflowSumInt(0, -1, 0, 3) == 3);
    CHECK(Math::OverflowSumInt(2, 1, -2, 2) == -2);
}
