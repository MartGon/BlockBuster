#pragma once

#include <memory>

#include <Model.h>

namespace Rendering
{

    class Billboard
    {
    friend class RenderMgr;
    public:
        ~Billboard();

        Billboard(const Billboard&) = delete;
        Billboard& operator=(const Billboard&) = delete;

        Billboard(Billboard&&) = default;
        Billboard& operator=(Billboard&&) = default;

        Painting painting;
        GL::Shader* shader;

        void Draw(glm::mat4 projView, glm::vec3 pos, glm::vec3 cameraRight, glm::vec3 cameraUp, glm::vec2 scale, glm::vec4 colorMod = glm::vec4{1.0f}, uint8_t flags = 0);

    private:
        Billboard();
        static std::unique_ptr<Rendering::Mesh> quad;
        RenderMgr* mgr = nullptr;
    };
}