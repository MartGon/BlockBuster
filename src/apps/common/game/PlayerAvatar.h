#pragma once

#include <rendering/Model.h>
#include <rendering/RenderMgr.h>

namespace Game
{
    class PlayerAvatar
    {
    public:
        void Start(Rendering::RenderMgr& renderMgr, GL::Shader& shader, GL::Shader& quadShader, GL::Texture& texture);
        void Draw(const glm::mat4& tMat);
        
        void SteerWheels(glm::vec3 moveDir);
        void RotateArms(float pitch);

        Math::Transform wTransform;
        Math::Transform aTransform{glm::vec3{0.0f, 0.2f, 0.625f}, glm::vec3{0.0f}, glm::vec3{1.0f}};
        Rendering::Model* armsModel;

    private:
        void InitModel(Rendering::RenderMgr& renderMgr, GL::Shader& shader, GL::Shader& quadShader, GL::Texture& texture);

        // Models
        Rendering::Model* bodyModel;
        Rendering::Model* wheelsModel;

        // Base Meshes
        Rendering::Mesh quad;
        Rendering::Mesh cube;
        Rendering::Mesh slope;
        Rendering::Mesh cylinder;
    };
}