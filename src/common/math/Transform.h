#pragma once

#include <glm/glm.hpp>

namespace Math
{
    class Transform
    {
    public:
        glm::vec3 position = glm::vec3{0.0f};
        // TODO: Change this to a quaternion
        glm::vec3 rotation = glm::vec3{0.0f};
        glm::vec3 scale = glm::vec3{1.0f};

        void SetUniformScale(float scale);
        float GetUniformScale() const;

        glm::mat4 GetTranslationMat() const;
        // TODO: This has gimbal lock issues
        glm::mat4 GetRotationMat() const;
        glm::mat4 GetScaleMat() const;

        glm::mat4 GetTransformMat() const;
    };

    Transform Interpolate(Transform a, Transform b, float alpha);
}