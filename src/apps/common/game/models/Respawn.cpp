#include <models/Respawn.h>

#include <rendering/Primitive.h>

using namespace Game::Models;

void Respawn::Start(Rendering::RenderMgr& renderMgr, GL::Shader& shader)
{
    InitModel(renderMgr, shader);
}

void Respawn::SetMeshes(Rendering::Mesh& cylinder, Rendering::Mesh& slope)
{
    cylinderPtr = &cylinder;
    slopePtr = &slope;
}

void Respawn::Draw(const glm::mat4& tMat)
{
    model->Draw(tMat);
}

void Respawn::InitModel(Rendering::RenderMgr& renderMgr, GL::Shader& shader)
{
    // Get model handlers
    model = renderMgr.CreateModel();

    // Cylinder
    auto cylinderT = Math::Transform{glm::vec3{0.0f, 0.125f, 0.0f}, glm::vec3{0.0f}, glm::vec3{1.0f, 0.250f, 1.0f}};
    Rendering::Painting painting;
    painting.type = Rendering::PaintingType::COLOR;
    const auto blue = glm::vec4{0.065f, 0.072f, 0.8f, 1.0f};
    const auto lightBlue = glm::vec4{0.130f, 0.142f, 0.8f, 1.0f};
    painting.color = lightBlue;
    auto sm1 = Rendering::SubModel{cylinderT, painting, cylinderPtr, &shader};
    model->AddSubModel(std::move(sm1));

    // Slope
    auto slopeT = Math::Transform{glm::vec3{0.0f, 0.130f, 0.750f}, glm::vec3{0.0f}, glm::vec3{1.0f, 0.240f, 1.0f}};
    painting.color = lightBlue;
    auto slope = Rendering::SubModel{slopeT, painting, slopePtr, &shader};
    model->AddSubModel(std::move(slope));
}