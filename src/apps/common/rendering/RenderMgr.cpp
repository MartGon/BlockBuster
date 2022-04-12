#include <RenderMgr.h>

#include <algorithm>

using namespace Rendering;

Model* RenderMgr::CreateModel()
{
    Model* model = new Model();
    model->mgr = this;

    models.push_back(model);

    return model;
}

Billboard* RenderMgr::CreateBillboard()
{
    Billboard* billboard = new Billboard();
    billboard->mgr = this;

    billboards.push_back(billboard);

    return billboard;
}

RenderMgr::~RenderMgr()
{
    for(auto model : models)
        delete model;

    for(auto billboard : billboards)
        delete billboard;
}

void RenderMgr::Start()
{
    textureMgr.Start();
}

void RenderMgr::Render(const Rendering::Camera& camera)
{
    std::sort(transparentReq.begin(), transparentReq.end(), [&camera](auto a, auto b){

        return a.GetDepth(camera.GetPos()) > b.GetDepth(camera.GetPos());
    });
    
    DrawList(&opaqueReq);

    // Remove duplicate depths
    DrawList(&transparentReq);
    
    glClear(GL_DEPTH_BUFFER_BIT);
    DrawList(&ignoreDepthReqs);
    opaqueReq.clear();
    transparentReq.clear();
    ignoreDepthReqs.clear();
}

void RenderMgr::DrawList(std::vector<DrawReq>* list)
{
    for(auto drawReq : *list)
    {
        if(drawReq.renderFlags & RenderMgr::NO_FACE_CULLING)
            glDisable(GL_CULL_FACE);
        
        if(drawReq.reqType == ReqType::MODEL)
        {
            auto submodel = drawReq.modelParams.toDraw;
            submodel.shader->SetUniformMat4("transform", drawReq.modelParams.t);
            submodel.shader->SetUniformInt("textureType", submodel.painting.type);

            if(submodel.painting.type == PaintingType::TEXTURE)
            {
                textureMgr.Bind(submodel.painting.texture);
                submodel.mesh->Draw(*submodel.shader);
            }
            else if(submodel.painting.type == PaintingType::COLOR)
                submodel.mesh->Draw(*submodel.shader, submodel.painting.color);
        }
        else if(drawReq.reqType == ReqType::BILLBOARD)
        {
            auto params = drawReq.billboardParams;
            auto billboard = params.billboard;
            billboard->shader->SetUniformVec3("center", params.pos);
            billboard->shader->SetUniformVec3("camRight", params.cameraRight);
            billboard->shader->SetUniformVec3("camUp", params.cameraUp);
            billboard->shader->SetUniformFloat("rot", params.rot);
            billboard->shader->SetUniformVec2("scale", params.scale);
            billboard->shader->SetUniformVec4("colorMod", params.colorMod);
            billboard->shader->SetUniformMat4("projView", params.projView);

            billboard->shader->SetUniformInt("frameId", params.frameId);

            billboard->shader->SetUniformInt("textureType", billboard->painting.type);
            if(billboard->painting.type == PaintingType::TEXTURE)
            {
                textureMgr.Bind(billboard->painting.texture);
                billboard->quad->Draw(*billboard->shader);
            }
            else if(billboard->painting.type == PaintingType::COLOR)
                billboard->quad->Draw(*billboard->shader, billboard->painting.color);
        }

        glEnable(GL_CULL_FACE);
    }
}

void RenderMgr::AddDrawReq(AlphaType alphaType, DrawReq dr)
{
    if(dr.renderFlags & RenderMgr::RenderFlags::IGNORE_DEPTH)
        ignoreDepthReqs.push_back(dr);
    else
    {
        if(alphaType == AlphaType::ALPHA_OPAQUE)
            opaqueReq.push_back(dr);
        else if(alphaType == AlphaType::ALPHA_TRANSPARENT)
            transparentReq.push_back(dr);
    }
}

float RenderMgr::DrawReq::GetDepth(glm::vec3 camPos)
{
    float depth = 0;
    if(reqType == ReqType::MODEL)
    // TODO: Review this
        depth = glm::length(camPos - glm::vec3{modelParams.t[3]});
    else if(reqType == ReqType::BILLBOARD)
        depth = glm::length(camPos - billboardParams.pos);

    return depth;
}