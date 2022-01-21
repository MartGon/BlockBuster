#pragma once 

#include <rendering/Model.h>
#include <rendering/RenderMgr.h>

namespace Game
{
    class FPSAvatar
    {
    public:
        void Start(Rendering::RenderMgr& renderMgr, GL::Shader& shader, GL::Shader& quadShader, GL::Texture& texture);
        void Draw(const glm::mat4& projMat);

        // Models
        Rendering::Model* armsModel;
    private:

        void InitModel(Rendering::RenderMgr& renderMgr, GL::Shader& shader, GL::Shader& quadShader, GL::Texture& texture);

        // Base Meshes
        Rendering::Mesh quad;
        Rendering::Mesh cube;
        Rendering::Mesh cylinder;

        // Base Meshes
        Rendering::Mesh* quadPtr = nullptr;
        Rendering::Mesh* cubePtr = nullptr;
        Rendering::Mesh* cylinderPtr = nullptr;

        // Pos
        const Math::Transform transform{glm::vec3{0.0f, -1.25f, -2.0f}, glm::vec3{0.0f}, glm::vec3{1.0f}};
    };
}