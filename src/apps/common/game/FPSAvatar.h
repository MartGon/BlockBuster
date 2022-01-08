#pragma once 

#include <rendering/Model.h>

namespace Game
{
    class FPSAvatar
    {
    public:
        void Start(GL::Shader& modelShader, GL::Shader& quadShader, GL::Texture& texture);
        void Draw(const glm::mat4& projMat);

        // Models
        Rendering::Model armsModel;
    private:

        void InitModel(GL::Shader& modelShader, GL::Shader& quadShader, GL::Texture& texture);

        // Base Meshes
        Rendering::Mesh quad;
        Rendering::Mesh cube;
        Rendering::Mesh cylinder;

        // Pos
        const Math::Transform transform{glm::vec3{0.0f, -1.25f, -2.0f}, glm::vec3{0.0f}, glm::vec3{1.0f}};
    };
}