#include <doctest/doctest.h>

#include <math/BBMath.h>
#include <math/Interpolation.h>

#include <iostream>

// The used function is not a linear interpolation.
TEST_CASE("InterpolateDeg")
{
    SUBCASE("InterpolateDeg basic")
    {
        float a = 0.0f;
        float b = 90.0f;
        auto res = Math::InterpolateDeg(a, b, 0.5f);
        auto res2 = Math::InterpolateDeg(a, b, 0.75f);

        CHECK(Math::AreSame(res, 45.0f, 0.5f));
    }
    SUBCASE("InterpolateDeg overflow")
    {
        float a = 340.0f;
        float b = 20.0f;
        auto res = Math::InterpolateDeg(a, b, 0.5f);

        CHECK(Math::AreSame(res, 0.f, 0.5f));
    }
    SUBCASE("InterpolateDeg changed weight")
    {
        float a = 0.0f;
        float b = 10.0f;
        auto res = Math::InterpolateDeg(a, b, 0.f);
        auto res2 = Math::InterpolateDeg(a, b, 0.25f);
        auto res3 = Math::InterpolateDeg(a, b, 0.75f);
        auto res4 = Math::InterpolateDeg(a, b, 1.0f);

        CHECK(Math::AreSame(res, 10.0f, 0.5f));
        CHECK(Math::AreSame(res2, 7.5f, 0.5f));
        CHECK(Math::AreSame(res3, 2.5f, 0.5f));
        CHECK(Math::AreSame(res4, 0.0f, 0.5f));
    }
}
