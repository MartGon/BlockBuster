#include <Billboard.h>

#include <Primitive.h>
#include <RenderMgr.h>

using namespace Rendering;

std::unique_ptr<Mesh> Billboard::quad = nullptr; 

Billboard::Billboard()
{
    if(quad.get() == nullptr)
        quad = std::make_unique<Mesh>(Primitive::GenerateQuad());
}

void Billboard::Draw(glm::mat4 projView, glm::vec3 pos, glm::vec3 cameraRight, glm::vec3 cameraUp, float rot,
    glm::vec2 scale, glm::vec4 colorMod, uint8_t flags, int frameId)
{
    auto alphaType = painting.hasAlpha ? RenderMgr::AlphaType::ALPHA_TRANSPARENT : RenderMgr::AlphaType::ALPHA_OPAQUE;
    auto params = RenderMgr::BillboardParams{.projView = projView,  .pos = pos, .cameraRight = cameraRight, 
        .cameraUp = cameraUp, .rot = rot, .scale = scale, .colorMod = colorMod, .billboard = this, .frameId = frameId};
    auto drawReq = RenderMgr::DrawReq{.reqType = RenderMgr::ReqType::BILLBOARD, .billboardParams = params, .renderFlags = flags};
    mgr->AddDrawReq(alphaType, drawReq);
}