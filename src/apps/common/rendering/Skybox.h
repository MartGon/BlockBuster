#pragma once
#include <gl/Cubemap.h>
#include <Primitive.h>

namespace Rendering
{
    class Skybox
    {
    public:
        Skybox();

        void Draw(GL::Shader& skyboxShader, const glm::mat4 view, const glm::mat4 proj, bool cullFaceEnabled = true);
        void Load(const GL::Cubemap::TextureMap& textureMap, bool flip_vertically = false);

    private:
        GL::Cubemap cubemap;
        Rendering::Mesh cube;
    };
}