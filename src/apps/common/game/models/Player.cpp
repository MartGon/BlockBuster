#include <models/Player.h>

#include <rendering/Primitive.h>

#include <entity/Player.h>

#include <debug/Debug.h>

using namespace Game::Models;

void Player::Start(Rendering::RenderMgr& renderMgr, GL::Shader& shader, GL::Shader& billboardShader)
{
    InitModel(renderMgr, shader, billboardShader);
    InitAnimations();
}

void Player::SetMeshes(Rendering::Mesh& quad, Rendering::Mesh& cube, Rendering::Mesh& cylinder, Rendering::Mesh& slope)
{
    quadPtr = &quad;
    cubePtr = &cube;
    cylinderPtr = &cylinder;
    slopePtr = &slope;
}

void Player::Draw(const glm::mat4& tMat, uint8_t flags)
{
    auto bodyT = bTransform.GetTransformMat();
    auto btMat = tMat * bodyT;
    bodyModel->Draw(btMat);

    auto wheelsT = wTransform.GetTransformMat();
    auto wtMat = tMat * wheelsT;
    wheelsModel->Draw(wtMat);

    auto armsT = aTransform.GetTransformMat();
    auto apT = armsPivot.GetTransformMat();
    auto atMat = tMat * bodyT * armsT * apT;
    armsModel->Draw(atMat);
}

void Player::SetColor(glm::vec4 color)
{
    for(auto id : bodyIds)
        bodyModel->GetSubModel(id)->painting.color = color;
    
    for(auto id : wheelsIds)
        wheelsModel->GetSubModel(id)->painting.color = color;

    for(auto id : armsIds)
        armsModel->GetSubModel(id)->painting.color = color;
}

void Player::SteerWheels(glm::vec3 moveDir, float facingAngle)
{
    // We need to rotate the moveDir by the facing Angle
    auto rotate = glm::rotate(glm::mat4{1.0f}, glm::radians(-facingAngle), glm::vec3{0.0f, 1.0f, 0.0f});
    glm::vec3 offset = rotate * glm::vec4{moveDir, 1.0f};

    wTransform.rotation.y = Entity::Player::GetWheelsRotation(offset);
}

void Player::SetArmsPivot(Math::Transform armsPivot)
{
    this->armsPivot = armsPivot;
}

void Player::SetFlashesActive(bool active)
{
    leftFlash->enabled = active;
    rightFlash->enabled = active;
    leftAltFlash->enabled = active;
    rightAltFlash->enabled = active;
}

void Player::SetFacing(float facingAngle)
{
    bTransform.rotation.y = facingAngle;
}

void Player::RotateArms(float pitch)
{
    aTransform.rotation.x = pitch;
}

void Player::SetFlagActive(bool active, glm::vec4 color)
{
    flagModel->enabled = active;
    flagModel->painting.color = color;
}

Animation::Clip* Player::GetIdleAnim()
{
    return &idle;
}

Animation::Clip* Player::GetShootAnim()
{
    return &shoot;
}

Animation::Clip* Player::GetReloadAnim()
{
    return &reload;
}

Animation::Clip* Player::GetDeathAnim()
{
    return &death;
}

void Player::InitModel(Rendering::RenderMgr& renderMgr, GL::Shader& shader, GL::Shader& billboardShader)
{
    // Get model handlers
    bodyModel = renderMgr.CreateModel();
    wheelsModel = renderMgr.CreateModel();
    armsModel = renderMgr.CreateModel();
    nameBillboard = renderMgr.CreateBillboard();

    // Textures
    std::string textureName = "flash.png";
    std::string altTextureName = "flashAlt.png";
    auto flashTextureId = renderMgr.GetTextureMgr().LoadFromDefaultFolder(textureName);
    auto altFlashTextureId = renderMgr.GetTextureMgr().LoadFromDefaultFolder(altTextureName);

    // Upper Body
    auto bodyT = Math::Transform{glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.0f}, glm::vec3{2.0f}};
    Rendering::Painting painting;
    painting.type = Rendering::PaintingType::COLOR;
    const auto blue = glm::vec4{0.065f, 0.072f, 0.8f, 1.0f};
    const auto lightBlue = glm::vec4{0.130f, 0.142f, 0.8f, 1.0f};
    painting.color = blue;
    auto sm1 = Rendering::SubModel{bodyT, painting, cubePtr, &shader};
    auto id = bodyModel->AddSubModel(std::move(sm1));
    bodyIds.push_back(id);

    auto neckT = Math::Transform{glm::vec3{0.0f, 1.0f, 0.0f}, glm::vec3{0.0f}, glm::vec3{0.75f, 0.5f, 0.75f}};
    painting.color = glm::vec4{glm::vec3{0.0f}, 1.0f};
    auto neckSM = Rendering::SubModel{neckT, painting, cylinderPtr, &shader};
    bodyModel->AddSubModel(std::move(neckSM));

    auto headT = Math::Transform{glm::vec3{0.0f, 1.625f, -0.30f}, glm::vec3{0.0f}, glm::vec3{1.5f, 0.75f, 0.9f}};
    painting.color = blue;
    auto headSM = Rendering::SubModel{headT, painting, cubePtr, &shader};
    id = bodyModel->AddSubModel(std::move(headSM));
    bodyIds.push_back(id);

    auto headBackT = Math::Transform{glm::vec3{0.0f, 1.625f, 0.525f}, glm::vec3{0.0f}, glm::vec3{1.5f, 0.75f, 0.75f}};
    painting.color = blue;
    auto headBackSM = Rendering::SubModel{headBackT, painting, slopePtr, &shader};
    id = bodyModel->AddSubModel(std::move(headBackSM));
    bodyIds.push_back(id);

    auto flagT = Math::Transform{glm::vec3{0.0f, 0.615f, 1.5f}, glm::vec3{0.0f, 90.0f, 0.0f}, glm::vec3{1.75f, 1.5f, 1.5f}};
    painting.color = blue;
    painting.hasAlpha = true;
    auto flag = Rendering::SubModel{flagT, painting, quadPtr, &shader, false, Rendering::RenderMgr::NO_FACE_CULLING};
    id = bodyModel->AddSubModel(std::move(flag));
    flagModel = bodyModel->GetSubModel(id);

    // Wheels
        // Back wheel
    auto wheelSlopeT = Math::Transform{glm::vec3{0.0f, -1.0f, 1.0f}, glm::vec3{0.0f}, glm::vec3{1.985f, 1.25f, 2.0f}};
    painting.color = lightBlue;
    auto wSlopeM = Rendering::SubModel{wheelSlopeT, painting, slopePtr, &shader};
    id = wheelsModel->AddSubModel(std::move(wSlopeM));
    wheelsIds.push_back(id);

        // Front Wheel
    auto fWheelSlopeT = Math::Transform{glm::vec3{0.0f, -1.0f, -1.0f}, glm::vec3{0.0f, 180.0f, 0.0f}, glm::vec3{1.985f, 1.25f, 2.0f}};
    painting.color = lightBlue;
    auto fWSlopeM = Rendering::SubModel{fWheelSlopeT, painting, slopePtr, &shader};
    id = wheelsModel->AddSubModel(std::move(fWSlopeM));
    wheelsIds.push_back(id);

        // Down cube
    auto wheelCubeT = Math::Transform{glm::vec3{0.0f, -1.75f, -0.0f}, glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{2.0f, 0.25f, 4.0f}};
    painting.color = lightBlue;
    auto wheelCubeModel = Rendering::SubModel{wheelCubeT, painting, cubePtr, &shader};
    id = wheelsModel->AddSubModel(std::move(wheelCubeModel));
    wheelsIds.push_back(id);

    // Wheels
    for(int i = 0; i < 4; i++)
    {
        auto zOffset = -1.5f + i;
        auto wheelT = Math::Transform{glm::vec3{0.0f, -2.0f, zOffset}, glm::vec3{0.0f, 0.0f, 90.0f}, glm::vec3{0.5f, 1.95f, 0.5f}};
        painting.color = glm::vec4{glm::vec3{0.15f}, 1.0f};
        auto wheelModel = Rendering::SubModel{wheelT, painting, cylinderPtr, &shader};
        wheelsModel->AddSubModel(std::move(wheelModel));
    }

    // Weapon
    auto leftId = 0;
    auto rightId = 0;
    auto altLeftId = 0;
    auto altRightId = 0;
    for(int i = -1; i < 2; i += 2)
    {
        bool isLeft = i == -1;
        auto fid = isLeft ? &leftId : &rightId;
        auto altFid = isLeft ? &altLeftId : &altRightId;

        auto armT = Math::Transform{glm::vec3{(float)i * 1.375f, 0.0f, 0.0f}, glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.75}};
        painting.type = Rendering::PaintingType::COLOR;
        painting.color = lightBlue;
        painting.hasAlpha = false;
        auto armModel = Rendering::SubModel{armT, painting, cubePtr, &shader};
        id = armsModel->AddSubModel(std::move(armModel));
        armsIds.push_back(id);

        auto cannonT = Math::Transform{glm::vec3{(float)i * 1.375f, 0.0f, -1.4f}, glm::vec3{90.0f, 0.0f, 0.0f}, glm::vec3{0.25f, 2.5f, 0.25f}};
        painting.color = glm::vec4{glm::vec3{0.15f}, 1.0f};
        auto cannonModel = Rendering::SubModel{cannonT, painting, cylinderPtr, &shader};
        armsModel->AddSubModel(std::move(cannonModel));

        auto flashT = Math::Transform{glm::vec3{(float)i * 1.375f, 0.0f, -2.75f}, glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{3.f}};
        painting.type = Rendering::PaintingType::TEXTURE;
        painting.hasAlpha = true;
        painting.texture = flashTextureId;
        auto flashModel = Rendering::SubModel{flashT, painting, quadPtr, &shader, false, Rendering::RenderMgr::NO_FACE_CULLING};
        *fid = armsModel->AddSubModel(std::move(flashModel));

        auto altFlashT = Math::Transform{glm::vec3{(float)i * 1.35f, 0.0f, -4.f}, glm::vec3{0.0f, 90.0f, 0.0f}, glm::vec3{3.0f}};
        painting.texture = altFlashTextureId;
        auto altFlashModel = Rendering::SubModel{altFlashT, painting, quadPtr, &shader, false, Rendering::RenderMgr::NO_FACE_CULLING};
        *altFid = armsModel->AddSubModel(std::move(altFlashModel));
    }

    leftFlash = armsModel->GetSubModel(leftId);
    rightFlash = armsModel->GetSubModel(rightId);
    leftAltFlash = armsModel->GetSubModel(altLeftId);
    rightAltFlash = armsModel->GetSubModel(altRightId);
}

void Player::InitAnimations()
{
    // Idle animation
    Animation::Sample s1{
        {{"yPos", -0.05f}}
    };
    Animation::KeyFrame f1{s1, 0};    
    Animation::Sample s2{
        {{"yPos", 0.05f}}
    };
    Animation::KeyFrame f2{s2, 60};
    Animation::KeyFrame f3{s1, 120};
    idle.keyFrames = {f1, f2, f3};

    // Shoot animation
    Animation::Sample sS1{
        {{"zPos", 0.00f}},
        {
            {"left-flash", true},
            {"right-flash", true}
        },
    };
    Animation::KeyFrame sF1{sS1, 0};
    Animation::Sample sS2{
        {{"zPos", 0.25f}},
        {
            {"left-flash", false},
            {"right-flash", false}
        }
    };
    Animation::KeyFrame sF2{sS2, 6};

    Animation::Sample sS3{
        {{"zPos", 0.00f}},
        {
            {"left-flash", false},
            {"right-flash", false}
        }
    };
    Animation::KeyFrame sF3{sS3, 20};
    shoot.keyFrames = {sF1, sF2, sF3};
    shoot.fps = 60;

    // Reload animation
    Animation::Sample rs1{
        {
            {"yPos", 0.0f},
            {"pitch", 0.0f},
        }

    };
    Animation::KeyFrame rf1{rs1, 0};    
    Animation::Sample rs2{
        {
            {"yPos", -0.75f},
            {"pitch", -45.0f},
        }
    };

    Animation::KeyFrame rf2{rs2, 30};
    Animation::KeyFrame rf3{rs2, 120};
    Animation::KeyFrame rf4{rs1, 150};
    reload.keyFrames = {rf1, rf2, rf3, rf4};

    // Death animation
    Animation::Sample ds1{
        {
            {"scale", 1.0f},
        }

    };
    Animation::KeyFrame df1{ds1, 0};    
    Animation::Sample ds2{
        {
            {"scale", 0.f},
        }
    };

    Animation::KeyFrame df2{ds2, 30};
    death.keyFrames = {df1, df2};
}