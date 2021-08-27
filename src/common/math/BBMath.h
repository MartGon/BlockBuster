#pragma once

#include <glm/glm.hpp>

#include <debug/Debug.h>

namespace Math
{
    template<typename T>
    T OverflowSumInt(T a, T b, T lBound, T hBound)
    {
        assertm((lBound < hBound), "Lbound is not lower than hoOund");

        T res = a + b;
        if(res < lBound)
            res = hBound - (lBound - res) + 1;
        if(res > hBound)
            res = lBound + (res - hBound) - 1;
        return res;
    }

    template<typename T>
    T OverflowSumFloat(T a, T b, T lBound, T hBound)
    {
        assertm((lBound < hBound), "Lbound is not lower than hoOund");

        T res = a + b;
        if(res < lBound)
            res = hBound - (lBound - res);
        if(res > hBound)
            res = lBound + (res - hBound);
        return res;
    }
}