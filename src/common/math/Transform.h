#pragma once

#include <glm/glm.hpp>

namespace Math
{
    struct Transform
    {
        glm::vec3 position = glm::vec3{0.0f};
        glm::vec3 rotation = glm::vec3{0.0f};
        float scale = 1.0f;

        glm::mat4 GetTranslationMat() const;
        glm::mat4 GetRotationMat() const;
        glm::mat4 GetScaleMat() const;

        glm::mat4 GetTransformMat() const;
    };
}