#pragma once

#include <math/Transform.h>
#include <rendering/Model.h>

namespace Game
{
    class PlayerAvatar
    {
    public:
        void Start(GL::Shader& modelShader);
        void Draw(const glm::mat4& tMat);

        Math::Transform transform;
    private:

        void InitModel(GL::Shader& modelShader);

        // Base Meshes
        Rendering::Mesh cube;
        Rendering::Mesh slope;
        Rendering::Mesh cylinder;

        // Models
        Rendering::Model bodyModel;
        Rendering::Model wheelsModel;
        Rendering::Model armsModel;
    };
}