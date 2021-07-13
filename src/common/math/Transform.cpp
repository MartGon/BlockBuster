#include <Transform.h>

#include <glm/gtc/matrix_transform.hpp>

glm::mat4 Math::Transform::GetTranslationMat() const
{
    return glm::translate(glm::mat4{1.0f}, position);
}

glm::mat4 Math::Transform::GetRotationMat() const
{
    glm::mat4 mat{1.0f};
    mat = glm::rotate(mat, glm::radians(rotation.y), glm::vec3{0.0f, 1.0f, 0.0f});
    mat = glm::rotate(mat, glm::radians(rotation.x), glm::vec3{1.0f, 0.0f, 0.0f});
    mat = glm::rotate(mat, glm::radians(rotation.z), glm::vec3{0.0f, 0.0f, 1.0f});

    return mat;
}

glm::mat4 Math::Transform::GetScaleMat() const
{
    return glm::scale(glm::mat4{1.0f}, glm::vec3{scale});
}

glm::mat4 Math::Transform::GetTransformMat() const
{
    return  GetScaleMat() * GetTranslationMat() * GetRotationMat();
}