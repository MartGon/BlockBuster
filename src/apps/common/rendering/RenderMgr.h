#pragma once

#include <Model.h>
#include <Camera.h>

namespace Rendering
{
    class RenderMgr
    {
    friend class Model;
    public:
        ~RenderMgr();
        enum RenderFlags : uint8_t
        {
            NONE = 0,
            NO_FACE_CULLING = 1,
            IGNORE_DEPTH = 2,
        };

        Model* CreateModel();

        void Render(const Rendering::Camera& camera);
    private:
        enum class AlphaType
        {
            ALPHA_OPAQUE,
            ALPHA_TRANSPARENT,
        };

        struct DrawReq
        {
            glm::mat4 t;
            Rendering::SubModel toDraw;
            uint8_t renderFlags = RenderFlags::NONE;
        };

        void AddDrawReq(AlphaType type, DrawReq dr);
        void DrawList(std::vector<DrawReq>* list);

        // TODO: Should do something about this hardcoded value
        std::vector<Model*> models;
        std::vector<DrawReq> opaqueReq;
        std::vector<DrawReq> transparentReq;
        std::vector<DrawReq> ignoreDepthReqs;
    };
}