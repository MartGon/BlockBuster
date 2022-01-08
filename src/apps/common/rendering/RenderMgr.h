#pragma once

#include <Model.h>
#include <Camera.h>

namespace Rendering
{
    class RenderMgr
    {
    friend class Model;
    public:
        enum RenderFlags : uint8_t
        {
            NONE = 0,
            NO_FACE_CULLING = 1,
            CLEAR_DEPTH_BUFFER = 2,
        };

        Model* CreateModel();

        void Render(const Rendering::Camera& camera);
    private:
        enum class AlphaType
        {
            OPAQUE,
            TRANSPARENT,
        };

        struct DrawReq
        {
            glm::mat4 t;
            Rendering::SubModel toDraw;
            uint8_t renderFlags = RenderFlags::NONE;
        };

        void AddDrawReq(AlphaType type, DrawReq dr);
        void DrawList(std::vector<DrawReq>* list);

        Model models[16];
        uint8_t inUse = 0;
        std::vector<DrawReq> opaqueReq;
        std::vector<DrawReq> transparentReq;
        std::vector<DrawReq> ignoreDepthReqs;
    };
}