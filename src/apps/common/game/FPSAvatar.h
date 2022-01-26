#pragma once 

#include <rendering/Model.h>
#include <rendering/RenderMgr.h>
#include <animation/Animation.h>

namespace Game
{
    class FPSAvatar
    {
    public:
        void Start(Rendering::RenderMgr& renderMgr, GL::Shader& shader, GL::Shader& quadShader, GL::Texture& texture);
        void SetMeshes(Rendering::Mesh& quad, Rendering::Mesh& cube, Rendering::Mesh& cylinder); // NOTE: This should be called before Start
        void Draw(const glm::mat4& projMat);

        Animation::Player animPlayer;
        Math::Transform idlePivot;

        // TODO: Add bool enabled to submodel, to enable/disable flashbang
        // TODO: Implement Idle/Shoot animation
        // TODO: Keep var tracking last shooting arm, toggle after each shot. 
    private:

        void InitModel(Rendering::RenderMgr& renderMgr, GL::Shader& shader, GL::Shader& quadShader, GL::Texture& texture);
        void InitAnimations();

        // Base Meshes
        Rendering::Mesh* quadPtr = nullptr;
        Rendering::Mesh* cubePtr = nullptr;
        Rendering::Mesh* cylinderPtr = nullptr;

        // Models
        Rendering::Model* leftArm;
        Rendering::Model* rightArm;

        // Animations
        Animation::Clip idle;
        

        // Pos
        const Math::Transform transform{glm::vec3{0.0f, -1.25f, -2.0f}, glm::vec3{0.0f}, glm::vec3{1.0f}};
    };
}