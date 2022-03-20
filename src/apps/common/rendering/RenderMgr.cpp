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

RenderMgr::~RenderMgr()
{
    for(auto model : models)
        delete model;
}

void RenderMgr::Render(const Rendering::Camera& camera)
{
    std::sort(transparentReq.begin(), transparentReq.end(), [&camera](auto a, auto b){
        return a.t[3].z > b.t[3].z;
    });
    
    DrawList(&opaqueReq);
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

        auto submodel = drawReq.toDraw;
        submodel.shader->SetUniformMat4("transform", drawReq.t);
        if(submodel.painting.type == PaintingType::TEXTURE)
            submodel.mesh->Draw(*submodel.shader, submodel.painting.texture);
        else if(submodel.painting.type == PaintingType::COLOR)
            submodel.mesh->Draw(*submodel.shader, submodel.painting.color);

        glEnable(GL_CULL_FACE);
    }
}

void RenderMgr::AddDrawReq(AlphaType alphaType, DrawReq dr)
{
    if(dr.renderFlags & RenderMgr::RenderFlags::IGNORE_DEPTH)
        ignoreDepthReqs.push_back(dr);
    else
    {
        if(alphaType == AlphaType::OPAQUE)
            opaqueReq.push_back(dr);
        else if(alphaType == AlphaType::TRANSPARENT)
            transparentReq.push_back(dr);
    }
}