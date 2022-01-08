#pragma once

#include <rendering/Model.h>

namespace Game
{
    class PlayerAvatar
    {
    public:
        void Start(GL::Shader& modelShader);
        void Draw(const glm::mat4& tMat);
        
    private:
        void InitModel(GL::Shader& modelShader);

        // Models
        Rendering::Model bodyModel;
        Rendering::Model wheelsModel;
        Rendering::Model armsModel;

        // Base Meshes
        Rendering::Mesh cube;
        Rendering::Mesh slope;
        Rendering::Mesh cylinder;
    };
}