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

    // Floats
    bool AreSame(float a, float b, float threshold = 0.005f);

    glm::mat4 GetPerspectiveMat(float fov, float aspectRatio, float near = 0.1f, float far = 100.f);
    glm::mat4 GetViewMat(glm::vec3 pos, glm::vec2 orientation);
    glm::vec3 GetFront(glm::vec2 orientation);
}