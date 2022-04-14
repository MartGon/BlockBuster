#pragma once

#include <memory>

#include <Model.h>

namespace Rendering
{

    class Billboard
    {
    friend class RenderMgr;
    public:
        ~Billboard() = default;

        Billboard(const Billboard&) = delete;
        Billboard& operator=(const Billboard&) = delete;

        Billboard(Billboard&&) = default;
        Billboard& operator=(Billboard&&) = default;

        Painting painting;
        GL::Shader* shader;

        void Draw(glm::vec3 pos, float rot = 0.0f, glm::vec2 scale = glm::vec2{1.0f},
            glm::vec4 colorMod = glm::vec4{1.0f}, uint8_t flags = 1 /*NO_FACE_CULLING*/, int frameId = 0);

    private:
        Billboard();
        static std::unique_ptr<Rendering::Mesh> quad;
        RenderMgr* mgr = nullptr;
    };
}