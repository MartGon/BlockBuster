#pragma once 

#include <rendering/Model.h>

namespace Game
{
    class FPSAvatar
    {
    public:
        void Start(GL::Shader& modelShader);
        void Draw(const glm::mat4& projMat);

    private:

        void InitModel(GL::Shader& modelShader);

        // Base Meshes
        Rendering::Mesh cube;
        Rendering::Mesh cylinder;

        // Models
        Rendering::Model armsModel;

        // Pos
        const Math::Transform transform{glm::vec3{0.0f, -1.25f, -2.0f}, glm::vec3{0.0f}, glm::vec3{1.0f}};
    };
}