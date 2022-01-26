#include <FPSAvatar.h>

#include <rendering/Primitive.h>

using namespace Game;

void FPSAvatar::Start(Rendering::RenderMgr& renderMgr, GL::Shader& shader, GL::Shader& quadShader, GL::Texture& texture)
{
    InitModel(renderMgr, shader, quadShader, texture);
    InitAnimations();
}

void FPSAvatar::SetMeshes(Rendering::Mesh& quad, Rendering::Mesh& cube, Rendering::Mesh& cylinder)
{
    quadPtr = &quad;
    cubePtr = &cube;
    cylinderPtr = &cylinder;
}

void FPSAvatar::Draw(const glm::mat4& projMat)
{
    auto t = transform.GetTransformMat();
    auto pt = idlePivot.GetTransformMat();
    auto tMat = projMat * t * pt;
    leftArm->Draw(tMat, Rendering::RenderMgr::IGNORE_DEPTH);
    rightArm->Draw(tMat, Rendering::RenderMgr::IGNORE_DEPTH);
}

void FPSAvatar::InitModel(Rendering::RenderMgr& renderMgr, GL::Shader& shader, GL::Shader& quadShader, GL::Texture& texture)
{
    leftArm = renderMgr.CreateModel();
    rightArm = renderMgr.CreateModel();

    const auto blue = glm::vec4{0.065f, 0.072f, 0.8f, 1.0f};
    const auto lightBlue = glm::vec4{0.130f, 0.142f, 0.8f, 1.0f};

    for(int i = -1; i < 2; i += 2)
    {
        auto arm = i == -1 ? leftArm : rightArm;

        Rendering::Painting painting;
        painting.type = Rendering::PaintingType::COLOR;
        auto armT = Math::Transform{glm::vec3{(float)i * 1.375f, 0.2f, 0.625f}, glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.75}};
        painting.color = lightBlue;
        auto armModel = Rendering::SubModel{armT, painting, cubePtr, &shader};
        arm->AddSubModel(std::move(armModel));

        auto cannonT = Math::Transform{glm::vec3{(float)i * 1.375f, 0.2f, -0.775f}, glm::vec3{90.0f, 0.0f, 0.0f}, glm::vec3{0.25f, 2.5f, 0.25f}};
        painting.color = glm::vec4{glm::vec3{0.15f}, 1.0f};
        auto cannonModel = Rendering::SubModel{cannonT, painting, cylinderPtr, &shader};
        arm->AddSubModel(std::move(cannonModel));

        auto flashT = Math::Transform{glm::vec3{(float)i * 1.375f, 0.2f, -2.125f}, glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{3.f}};
        painting.type = Rendering::PaintingType::TEXTURE;
        painting.texture = &texture;
        painting.hasAlpha = true;
        auto flashModel = Rendering::SubModel{flashT, painting, quadPtr, &quadShader};
        arm->AddSubModel(std::move(flashModel));
    }
}

void FPSAvatar::InitAnimations()
{
    // Idle animation
    Math::Transform t1{glm::vec3{0.0f, -0.05f, 0.0f}, glm::vec3{0.0f}, glm::vec3{1.0f}};
    Animation::KeyFrame f1{Animation::Sample{t1}, 0};
    Math::Transform t2{glm::vec3{0.0f, 0.05f, 0.0f}, glm::vec3{0.0f}, glm::vec3{1.0f}};
    Animation::KeyFrame f2{Animation::Sample{t2}, 60};
    Animation::KeyFrame f3{Animation::Sample{t1}, 120};
    idle.keyFrames = {f1, f2, f3};

    // Shoot animation

    // Set idle clip
    animPlayer.SetClip(&idle);
    animPlayer.SetTarget(&idlePivot);
    animPlayer.isLooping = true;
    animPlayer.Play();
}
