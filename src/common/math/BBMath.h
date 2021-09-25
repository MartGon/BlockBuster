#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

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

    glm::mat4 GetRotMatInverse(const glm::mat4& rotMat);
    glm::vec3 RotateVec3(const glm::mat4& rotMat, glm::vec3 vec);
}