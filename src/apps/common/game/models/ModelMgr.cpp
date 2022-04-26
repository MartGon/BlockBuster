#include <models/ModelMgr.h>

#include <models/Player.h>

#include <rendering/Primitive.h>
#include <rendering/Rendering.h>

using namespace Game::Models;
using namespace Entity;

void ModelMgr::Start(Rendering::RenderMgr& renderMgr, GL::Shader& shader, GL::Shader& bbShader)
{
    quad = Rendering::Primitive::GenerateQuad();
    sphere = Rendering::Primitive::GenerateSphere(1.0f);
    cylinder = Rendering::Primitive::GenerateCylinder(1.0f, 1.0f);
    cube = Rendering::Primitive::GenerateCube();
    slope = Rendering::Primitive::GenerateSlope();

    this->renderMgr = &renderMgr;

    InitGoModels(renderMgr, shader);
    InitModels(renderMgr, shader);
    InitBillboards(renderMgr, bbShader);
}

void ModelMgr::DrawGo(GameObject::Type goType, const glm::mat4& tMat)
{
    if(auto model = goModels[goType])
        model->Draw(tMat);
}

void ModelMgr::SetGoModel(Entity::GameObject::Type type, Rendering::ModelI* model)
{
    goModels[type] = model;
}

void ModelMgr::Draw(ModelID modelId, const glm::mat4& tMat)
{
    if(auto model = models[modelId])
        model->Draw(tMat);
}

Rendering::ModelI* ModelMgr::GetModel(ModelID modelId)
{
    Rendering::ModelI* ret = nullptr;
    if(modelId >= 0 && modelId < ModelID::COUNT)
        ret = models[modelId];

    return ret;
}

void ModelMgr::DrawBillboard(BillboardID bbId, glm::vec3 pos,
    float rot, glm::vec2 scale, glm::vec4 colorMod, uint8_t flags)
{
    if(auto bb = billboards[bbId])
        bb->Draw(pos, rot, scale, colorMod, flags);
}

GL::Texture* ModelMgr::GetIconTex(BillboardID id)
{
    GL::Texture* tex = nullptr;
    if(auto bbid = iconTextures.Get(id))
        tex = renderMgr->GetTextureMgr().GetTexture(bbid.value());

    return tex;
}

void ModelMgr::DrawWepBillboard(Entity::WeaponTypeID bbId, glm::vec3 pos,
    float rot, glm::vec2 scale, glm::vec4 colorMod, uint8_t flags)
{
    if(auto wepBb = wepIcons.Get(bbId))
        wepBb.value()->Draw(pos, rot, scale, colorMod, flags);
}

GL::Texture* ModelMgr::GetWepIconTex(Entity::WeaponTypeID wepId)
{
    GL::Texture* tex = nullptr;
    if(auto tid = wepIconsTex.Get(wepId))
        tex = renderMgr->GetTextureMgr().GetTexture(tid.value());

    return tex;
}

void ModelMgr::InitGoModels(Rendering::RenderMgr& renderMgr, GL::Shader& shader)
{
    for(int i = GameObject::Type::RESPAWN; i < GameObject::Type::COUNT; i++)
        goModels[i] = nullptr;

    // Colors
    Rendering::Painting painting;
    const auto red = glm::vec4{0.8f, 0.1f, 0.1f, 1.f};
    const auto green = glm::vec4{0.1f, 0.8f, 0.1f, 1.f};
    const auto darkGreen = glm::vec4{0.2f, 0.65f, 0.2f, 1.f};
    const auto blue = glm::vec4{0.1f, 0.1f, 0.8f, 1.f};
    const auto gray = glm::vec4{0.2f, 0.2f, 0.2f, 1.f};
    const auto black = glm::vec4{glm::vec3{0.0f}, 1.0f};

    // Domination point
    auto domPoint = renderMgr.CreateModel();
    auto cubeT = Math::Transform{glm::vec3{0.0f}, glm::vec3{0.0f}, glm::vec3{0.8f, 0.15f, 0.8f}};
    painting.type = Rendering::PaintingType::COLOR;
    painting.color = gray;
    auto sm1 = Rendering::SubModel{cubeT, painting, &cube, &shader};
    domPoint->AddSubModel(std::move(sm1));

    goModels[GameObject::Type::DOMINATION_POINT] = domPoint;

    // Weapon Crate
    auto weaponCrate = renderMgr.CreateModel();
    auto weaponCrateT = Math::Transform{glm::vec3{0.0f, 0.3725f, 0.0f}, glm::vec3{0.0f}, glm::vec3{1.3f, 0.75f, 1.3f}};
    painting.type = Rendering::PaintingType::COLOR;
    painting.color = Rendering::ColorU8ToFloat(59, 71, 49, 255);
    sm1 = Rendering::SubModel{weaponCrateT, painting, &cube, &shader};
    weaponCrate->AddSubModel(std::move(sm1));

    goModels[GameObject::Type::WEAPON_CRATE] = weaponCrate;

    // Health pack
    auto healthPack = renderMgr.CreateModel();
    auto healthPackT = Math::Transform{glm::vec3{0.0f, 0.25f, 0.0f}, glm::vec3{0.0f, 0.f, 0.0f}, glm::vec3{0.9f, 0.5f, 0.9f}};
    painting.type = Rendering::PaintingType::COLOR;
    painting.color = red;
    sm1 = Rendering::SubModel{healthPackT, painting, &cube, &shader};
    healthPack->AddSubModel(std::move(sm1));

    goModels[GameObject::Type::HEALTHPACK] = healthPack;

    // Flag A
    auto flagA = renderMgr.CreateModel();
    auto cylinderT = Math::Transform{glm::vec3{0.0f}, glm::vec3{0.0f}, glm::vec3{0.8f, 0.15f, 0.8f}};
    painting.type = Rendering::PaintingType::COLOR;
    painting.color = blue;

    sm1 = Rendering::SubModel{cylinderT, painting, &cylinder, &shader};
    flagA->AddSubModel(std::move(sm1));
    goModels[GameObject::Type::FLAG_SPAWN_A] = flagA;

    // Flag B
    auto flagB = renderMgr.CreateModel();
    painting.color = red;

    sm1 = Rendering::SubModel{cylinderT, painting, &cylinder, &shader};
    flagB->AddSubModel(std::move(sm1));

    goModels[GameObject::Type::FLAG_SPAWN_B] = flagB;

    // Grenades
    auto grenades = renderMgr.CreateModel();
    float scale = 0.3f;
    painting.color = Rendering::ColorU8ToFloat(78, 79, 71);

    auto leftT = Math::Transform{glm::vec3{-0.35f, scale / 2.0f, 0.0f}, glm::vec3{0.0f}, glm::vec3{1.0f, 0.95f, 1.0f} * scale};
    auto rightT = Math::Transform{glm::vec3{0.35f, scale / 2.0f, 0.0f}, glm::vec3{0.0f}, glm::vec3{1.0f, 0.95f, 1.0f} * scale};

    auto leftFrag = Rendering::SubModel{leftT, painting, &sphere, &shader};
    auto rigthFrag = Rendering::SubModel{rightT, painting, &sphere, &shader};

    grenades->AddSubModel(std::move(leftFrag));
    grenades->AddSubModel(std::move(rigthFrag));

    painting.color = Rendering::ColorU8ToFloat(46, 40, 40, 255);
    auto topScale = 0.15f;
    auto leftTop = Math::Transform{leftT.position + glm::vec3{0.0f, scale, 0.0f}, glm::vec3{0.0f}, glm::vec3{0.85f, 2.0f, 0.85f} * topScale};
    auto rightTop = Math::Transform{rightT.position + glm::vec3{0.0f, scale, 0.0f}, glm::vec3{0.0f}, glm::vec3{0.85f, 2.0f, 0.85f} * topScale};
    auto leftFragTop = Rendering::SubModel{leftTop, painting, &cylinder, &shader};
    auto rigthFragTop = Rendering::SubModel{rightTop, painting, &cylinder, &shader};
    grenades->AddSubModel(std::move(leftFragTop));
    grenades->AddSubModel(std::move(rigthFragTop));

    goModels[GameObject::Type::GRENADES] = grenades;

    // Killbox
    auto killBox = renderMgr.CreateModel();
    painting.type = Rendering::PaintingType::COLOR;
    painting.color = red;
    sm1 = Rendering::SubModel{cubeT, painting, &cube, &shader};
    killBox->AddSubModel(std::move(sm1));

    goModels[GameObject::Type::KILLBOX] = killBox;

        // Teleport Origin
    auto telOriginModel = renderMgr.CreateModel();
    scale = 0.8f;

    // Cylinder
    cylinderT = Math::Transform{glm::vec3{0.0f, 0.125f, 0.0f} * scale, glm::vec3{0.0f}, glm::vec3{1.0f, 0.250f, 1.0f} * scale};
    painting.type = Rendering::PaintingType::COLOR;
    painting.color = gray;
    sm1 = Rendering::SubModel{cylinderT, painting, &cylinder, &shader};
    telOriginModel->AddSubModel(std::move(sm1));
    
    
    // Slope
    for(auto i = 0; i < 4; i++)
    {
        constexpr auto offset = -0.75f;
        glm::vec2 trunc = Math::RotateVec2(glm::vec2{0.0f, offset}, glm::half_pi<float>()*i);
        auto slopeT = Math::Transform{glm::vec3{trunc.x, 0.130f, trunc.y} * scale, glm::vec3{0.0f, 180.0f-90.0f*(float)i, 0.0f}, glm::vec3{1.0f, 0.240f, 1.0f} * scale};

        auto slopeSm = Rendering::SubModel{slopeT, painting, &slope, &shader};
        telOriginModel->AddSubModel(std::move(slopeSm));
    }

    goModels[GameObject::Type::TELEPORT_ORIGIN] = telOriginModel;

    // Teleport Dest
    auto telDestModel = renderMgr.CreateModel();

    cylinderT = Math::Transform{glm::vec3{0.0f, 0.125f, 0.0f} * scale, glm::vec3{0.0f}, glm::vec3{1.0f, 0.250f, 1.0f} * scale};
    sm1 = Rendering::SubModel{cylinderT, painting, &cylinder, &shader};
    telDestModel->AddSubModel(std::move(sm1));

    auto slopeT = Math::Transform{glm::vec3{0.0f, 0.130f, -0.750f} * scale, glm::vec3{0.0f, 180.0f, 0.0f}, glm::vec3{1.0f, 0.240f, 1.0f} * scale};
    auto slopeSm = Rendering::SubModel{slopeT, painting, &slope, &shader};
    telDestModel->AddSubModel(std::move(slopeSm));

    goModels[GameObject::Type::TELEPORT_DEST] = telDestModel;
}

void ModelMgr::InitModels(Rendering::RenderMgr& renderMgr, GL::Shader& shader)
{
    // Textures
    auto& texMgr = renderMgr.GetTextureMgr();
    auto decalId = texMgr.LoadFromDefaultFolder("decal.png", true);
    modelTextures.Add(DECAL_MODEL_ID, decalId);

    // Colors
    Rendering::Painting painting;
    const auto red = glm::vec4{0.8f, 0.1f, 0.1f, 1.f};
    const auto green = glm::vec4{0.1f, 0.8f, 0.1f, 1.f};
    const auto darkGreen = glm::vec4{0.2f, 0.65f, 0.2f, 1.f};
    const auto blue = glm::vec4{0.1f, 0.1f, 0.8f, 1.f};
    const auto gray = glm::vec4{0.2f, 0.2f, 0.2f, 1.f};
    const auto black = glm::vec4{glm::vec3{0.0f}, 1.0f};

    // Flag
    auto flag = renderMgr.CreateModel();

    auto cylinderT = Math::Transform{glm::vec3{0.0f, 0.5f, 0.0f}, glm::vec3{0.0f}, glm::vec3{0.1f, 2.f, 0.1f}};
    painting.type = Rendering::PaintingType::COLOR;
    painting.color = gray;
    auto sm1 = Rendering::SubModel{cylinderT, painting, &cylinder, &shader, true};
    flag->AddSubModel(std::move(sm1));

    auto quadT = Math::Transform{glm::vec3{-0.475f, 1.25f, 0.0f}, glm::vec3{0.0f}, glm::vec3{0.75f, 0.5f, 1.f}};
    painting.type = Rendering::PaintingType::COLOR;
    painting.color = green;
    auto sm = Rendering::SubModel{quadT, painting, &quad, &shader, true, Rendering::RenderMgr::NO_FACE_CULLING};
    flag->AddSubModel(std::move(sm));

    models[FLAG_MODEL_ID] = flag;

    // Greanade
    auto grenade = renderMgr.CreateModel();    
    float scale = 0.3f;
    painting.color = Rendering::ColorU8ToFloat(78, 79, 71);

    auto t = Math::Transform{glm::vec3{0.0f, scale / 2.0f, 0.0f}, glm::vec3{0.0f}, glm::vec3{1.0f, 0.95f, 1.0f} * scale};
    auto leftFrag = Rendering::SubModel{t, painting, &sphere, &shader};
    grenade->AddSubModel(std::move(leftFrag));

    painting.color = Rendering::ColorU8ToFloat(46, 40, 40, 255);
    auto topScale = 0.15f;
    auto leftTop = Math::Transform{t.position + glm::vec3{0.0f, scale, 0.0f}, glm::vec3{0.0f}, glm::vec3{0.85f, 2.0f, 0.85f} * topScale};
    auto leftFragTop = Rendering::SubModel{leftTop, painting, &cylinder, &shader};    
    grenade->AddSubModel(std::move(leftFragTop));

    models[GRENADE_MODEL_ID] = grenade;

    // Decal
    auto decal = renderMgr.CreateModel();
    painting = Rendering::Painting{.type = Rendering::PaintingType::TEXTURE, .hasAlpha = true, .texture = decalId};
    auto decalSm = Rendering::SubModel{Math::Transform{glm::vec3{0.0f}, glm::vec3{0.0f}, glm::vec3{0.5f}}, painting, &quad, &shader, true, Rendering::RenderMgr::NO_FACE_CULLING};
    decal->AddSubModel(std::move(decalSm));

    models[DECAL_MODEL_ID] = decal;

    // Rocket
    auto rocket = renderMgr.CreateModel();
    auto rScale = 0.1f;
    painting.hasAlpha = false;
    painting.type = Rendering::PaintingType::COLOR;
    painting.color = Rendering::ColorU8ToFloat(255, 255, 255);

        // Cylinder
    auto cylT = Math::Transform{glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.0f}, glm::vec3{1.0f, 2.0f, 1.0f} * rScale};
    auto csm = Rendering::SubModel{cylT, painting, &cylinder, &shader};
    rocket->AddSubModel(std::move(csm));

        // Sphere
    auto sphereT = Math::Transform{glm::vec3{0.0f, cylT.scale.y / 2.0f, 0.0f}, glm::vec3{0.0f}, glm::vec3{0.98f, 0.9f, 0.98f} * rScale};
    painting.color = Rendering::ColorU8ToFloat(255, 0, 0);
    auto sSM = Rendering::SubModel{sphereT, painting, &sphere, &shader};
    rocket->AddSubModel(std::move(sSM));

        // Left Slope
    auto lSlopeT = Math::Transform{glm::vec3{0.0f, -cylT.scale.y / 4.025f, 1.125f * rScale}, glm::vec3{0.0f}, glm::vec3{rScale}};
    painting.color = Rendering::ColorU8ToFloat(0, 0, 255);
    auto lssm = Rendering::SubModel{lSlopeT, painting, &slope, &shader};
    rocket->AddSubModel(std::move(lssm));

        // Right Slope
    auto rSlopeT = Math::Transform{glm::vec3{0.0f, -cylT.scale.y / 4.025f, -1.125f * rScale} , glm::vec3{0.0f, 180.0f, 0.0f}, glm::vec3{rScale}};
    auto rssm = Rendering::SubModel{rSlopeT, painting, &slope, &shader};
    rocket->AddSubModel(std::move(rssm));

        // Front Slope
    auto fSlopeT = Math::Transform{glm::vec3{1.125f * rScale, -cylT.scale.y / 4.025f, 0}, glm::vec3{0.0f, 90.0f, 0.0f}, glm::vec3{rScale}};
    auto fssm = Rendering::SubModel{fSlopeT, painting, &slope, &shader};
    rocket->AddSubModel(std::move(fssm));

        // Back Slope
    auto bSlopeT = Math::Transform{glm::vec3{-1.125f * rScale, -cylT.scale.y / 4.025f, 0}, glm::vec3{0.0f, 270.0f, 0.0f}, glm::vec3{rScale}};
    auto bssm = Rendering::SubModel{bSlopeT, painting, &slope, &shader};
    rocket->AddSubModel(std::move(bssm));

    models[ROCKET_MODEL_ID] = rocket;
}

void ModelMgr::InitBillboards(Rendering::RenderMgr& renderMgr, GL::Shader& bbShader)
{
    // Textures
    auto& texMgr = renderMgr.GetTextureMgr();
    auto flagIconId = texMgr.LoadFromDefaultFolder("flagIcon.png", true);
    auto redCrossId = texMgr.LoadFromDefaultFolder("redCross.png", true);
    iconTextures.Add(FLAG_ICON_ID, flagIconId);
    iconTextures.Add(RED_CROSS_ICON_ID, redCrossId);
    
        // Weapon Icons
    using namespace Entity;
    for(int i = WeaponTypeID::ASSAULT_RIFLE; i < WeaponTypeID::CHEAT_SMG; i++)
    {
        auto filename = "weapon-icon-" + std::to_string(i) + ".png";
        auto id = texMgr.LoadFromDefaultFolder(filename, true);
        wepIconsTex.Add(i, id);
    }

    // Billboards

        // Flag Icon
    auto flagIcon = renderMgr.CreateBillboard();
    flagIcon->shader = &bbShader;
    flagIcon->painting.type = Rendering::PaintingType::TEXTURE;
    flagIcon->painting.hasAlpha = true;
    flagIcon->painting.texture = flagIconId;
    billboards[FLAG_ICON_ID] = flagIcon;

        // Red Cross
    auto redCrossIcon = renderMgr.CreateBillboard();
    redCrossIcon->shader = &bbShader;
    redCrossIcon->painting = Rendering::Painting{.type = Rendering::PaintingType::TEXTURE, .hasAlpha = true, .texture = redCrossId};
    billboards[RED_CROSS_ICON_ID] = redCrossIcon;

    // Weapons
    for(int i = Entity::WeaponTypeID::ASSAULT_RIFLE; i < Entity::WeaponTypeID::COUNT; i++)
    {
        if(auto texId = wepIconsTex.Get(i))
        {
            auto icon = renderMgr.CreateBillboard();
            icon->shader = &bbShader;
            icon->painting.type = Rendering::PaintingType::TEXTURE;
            icon->painting.hasAlpha = true;
            icon->painting.texture = texId.value();
            wepIcons.Add(i, icon);
        }
    }
}

Log::Logger* ModelMgr::GetLogger()
{
    return App::ServiceLocator::GetLogger();
}