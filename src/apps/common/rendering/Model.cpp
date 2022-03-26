#include <rendering/Model.h>

#include <RenderMgr.h>

using namespace Rendering;

void Model::Draw(const glm::mat4& tMat, uint8_t flags)
{
    for(auto& submodel : meshes)
    {
        if(submodel.enabled)
        {
            auto mMat = submodel.transform.GetTransformMat();
            auto t = tMat  * mMat;
            auto alphaType = submodel.painting.hasAlpha ? RenderMgr::AlphaType::ALPHA_TRANSPARENT : RenderMgr::AlphaType::ALPHA_OPAQUE;
            auto drawReq = RenderMgr::DrawReq{t, submodel, flags};
            mgr->AddDrawReq(alphaType, drawReq);
        }
    }
}