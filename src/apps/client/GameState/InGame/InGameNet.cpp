#include <GameState/InGame/InGame.h>

#include <Client.h>

#include <util/Random.h>
#include <util/BBTime.h>
#include <util/File.h>
#include <util/Container.h>

#include <math/Interpolation.h>

#include <debug/Debug.h>
#include <nlohmann/json.hpp>
#include <httplib/httplib.h>

#include <networking/Networking.h>
#include <game/Input.h>

#include <entity/Game.h>

#include <iostream>
#include <algorithm>

using namespace BlockBuster;
using namespace ::App::Client;

void InGame::OnPlayerJoin(Entity::ID playerId, Entity::ID teamId, Networking::PlayerSnapshot playerState)
{
    Entity::Player player;
    player.id = playerId;
    player.teamId = teamId;
    player.ApplyState(playerState.ToPlayerState(player.ExtractState()));

    playerTable[playerId] = player;
    prevPlayerTable[playerId] = player;

    // Setup player animator
    playerModelStateTable[playerId] = PlayerModelState();
    PlayerModelState& ps = playerModelStateTable[playerId];

    ps.deathPlayer.SetClip(playerAvatar.GetDeathAnim());
    ps.deathPlayer.SetTargetFloat("scale", &ps.gScale);

    ps.shootPlayer.SetClip(playerAvatar.GetShootAnim());
    ps.shootPlayer.SetTargetFloat("zPos", &ps.armsPivot.position.z);
    ps.shootPlayer.SetTargetBool("left-flash", &ps.leftFlashActive);
    ps.shootPlayer.SetTargetBool("right-flash", &ps.rightFlashActive);
    ps.shootPlayer.SetOnDoneCallback([this, playerId](){
        this->playerModelStateTable[playerId].leftFlashActive = false;
        this->playerModelStateTable[playerId].rightFlashActive = false;
    });

    ps.shootPlayer.SetTargetFloat("yPos", &ps.armsPivot.position.y);
    ps.shootPlayer.SetTargetFloat("pitch", &ps.armsPivot.rotation.x);

    // Set up player buffer and name billboard
    ExtraData ed;

    ed.frameBuffer.Init(glm::ivec2{320, 180});
    auto& texMgr = renderMgr.GetTextureMgr();
    auto texId = texMgr.Handle(ed.frameBuffer.GetTexHandle(), ed.frameBuffer.GetTextureSize(), GL_RGBA);

    ed.nameBillboard = renderMgr.CreateBillboard();
    ed.nameBillboard->shader = &billboardShader;
    auto painting = Rendering::Painting{.type = Rendering::PaintingType::TEXTURE, .hasAlpha = true, .texture = texId};
    ed.nameBillboard->painting = painting;

    // Set up player audio sources
    ed.dmgSourceId = audioMgr->CreateSource();
    ed.shootSourceId = audioMgr->CreateSource();

    playersExtraData[playerId] = std::move(ed);
}

void InGame::OnPlayerLeave(Entity::ID playerId)
{
    playerTable.erase(playerId);
    prevPlayerTable.erase(playerId);
    playerModelStateTable.erase(playerId);
    playersExtraData.erase(playerId);
}

void InGame::OnConnectToServer(ENet::PeerId id)
{
    this->serverId = id;
    this->client_->logger->LogInfo("Succes on connection to server");

    // Send login packet
    auto packet = std::make_unique<Networking::Packets::Client::Login>();
    packet->playerUuid = this->playerUuid;
    packet->playerName = this->playerName;
    packet->Write();
    auto buffer = packet->GetBuffer();
    ENet::SentPacket sentPacket{buffer->GetData(), buffer->GetSize(), ENET_PACKET_FLAG_RELIABLE};
    host.SendPacket(serverId, 0, sentPacket);
}

void InGame::OnRecvPacket(ENet::PeerId id, uint8_t channelId, ENet::RecvPacket recvPacket)
{
    Util::Buffer::Reader reader{recvPacket.GetData(), recvPacket.GetSize()};
    Util::Buffer buffer = reader.ReadAll();
    
    auto packet = Networking::MakePacket<Networking::PacketType::Server>(std::move(buffer));
    if(packet)
    {
        packet->Read();
        OnRecvPacket(*packet);
    }
    else
        GetLogger()->LogError("Invalid packet recv from server");
}

void InGame::OnRecvPacket(Networking::Packet& packet)
{
    using namespace Networking::Packets::Server;

    switch (packet.GetOpcode())
    {
        case Networking::OpcodeServer::OPCODE_SERVER_BATCH:
        {
            auto batch = packet.To<Networking::Batch<Networking::PacketType::Server>>();
            for(auto i = 0; i < batch->GetPacketCount(); i++)
                OnRecvPacket(*batch->GetPacket(i));
        }
        break;

        case Networking::OpcodeServer::OPCODE_SERVER_WELCOME:
        {
            auto welcome = packet.To<Welcome>();
            client_->logger->LogInfo("Server tick rate is " + std::to_string(welcome->tickRate));

            // Set server params
            this->serverTickRate = Util::Time::Seconds(welcome->tickRate);
            this->offsetTime = serverTickRate * 0.5;
            this->playerId = welcome->playerId;
            this->connected = true;

            // Set player state
            Entity::Player player;
            player.id = this->playerId;
            player.teamId = welcome->teamId;
            GetLogger()->LogInfo("We play as player " + std::to_string(this->playerId));

            // Add to table
            playerTable[playerId] = player;
            prevPlayerTable[playerId] = player;

            this->match.Start(GetWorld(), welcome->mode, welcome->startingPlayers);
        }
        break;

        case Networking::OpcodeServer::OPCODE_SERVER_MATCH_STATE:
        {
            auto ms = packet.To<MatchState>();
            auto matchState = ms->state.state;
            // Set server params
            this->match.ApplyState(ms->state);
            OnEnterMatchState(this->match.GetState());
        }
        break;

        case Networking::OpcodeServer::OPCODE_SERVER_SNAPSHOT:
        {
            auto snapShot = packet.To<WorldUpdate>();
            client_->logger->LogInfo("Recv server snapshot for tick: " + std::to_string(snapShot->snapShot.serverTick));

            // Entity Interpolation - Reset offset
            auto mostRecent = snapshotHistory.Back();
            if(mostRecent.has_value() && snapShot->snapShot.serverTick > mostRecent->serverTick)
            {
                auto om = this->offsetTime - this->serverTickRate;
                this->offsetTime = std::min(serverTickRate, std::max(om, -serverTickRate));
                client_->logger->LogDebug("Offset millis " + std::to_string(this->offsetTime.count()));
            }
            
            // Store snapshot
            auto s = snapShot->snapShot;           
            snapshotHistory.PushBack(s);

            // Create projectiles
            for(auto [id, projectile] : s.projectiles)
            {
                if(!Util::Map::Contains(projectiles, id))
                {
                    if(projectile.type == Entity::Projectile::ROCKET)
                        projectiles[id] = std::make_unique<Entity::Rocket>();
                    else if(projectile.type == Entity::Projectile::GRENADE)
                        projectiles[id] = std::make_unique<Entity::Grenade>();
                    projectiles[id]->ApplyState(projectile);
                }
            }

            // Remove projectiles
            std::vector<Entity::ID> toRemove;
            for(auto& [id, projectile] : projectiles)
            {
                if(!Util::Map::Contains(s.projectiles, id))
                {
                    toRemove.push_back(id);
                    OnGrenadeExplode(*projectile.get());
                }
            }
            for(auto id : toRemove)   
                projectiles.erase(id);

            // Sort by tick. This is only needed if a packet arrives late.
            snapshotHistory.Sort([](Networking::Snapshot a, Networking::Snapshot b)
            {
                return a.serverTick < b.serverTick;
            });
        }
        break;

        case Networking::OpcodeServer::OPCODE_SERVER_PLAYER_JOINED:
        {
            auto pd = packet.To<PlayerJoined>();
            OnPlayerJoin(pd->playerId, pd->teamId, pd->playerSnapshot);
        }
        break;

        case Networking::OpcodeServer::OPCODE_SERVER_PLAYER_DISCONNECTED:
        {
            auto pd = packet.To<PlayerDisconnected>();
            OnPlayerLeave(pd->playerId);
        }
        break;

        case Networking::OpcodeServer::OPCODE_SERVER_PLAYER_INPUT_ACK:
        {
            auto pi = packet.To<PlayerInputACK>();

            // Update last ack
            this->lastAck = pi->lastCmd;
            localPlayerStateHistory.PushBack(pi->playerState);
            client_->logger->LogInfo("Server ack command: " + std::to_string(this->lastAck));
        }
        break;

        case Networking::OpcodeServer::OPCODE_SERVER_PLAYER_TAKE_DMG:
        {
            auto ptd = packet.To<PlayerTakeDmg>();
            client_->logger->LogDebug("Player took dmg!");

            auto& player = GetLocalPlayer();
            player.health = ptd->healthState;
            lastDmgOrigin = ptd->origin;

            OnLocalPlayerTakeDmg();

            player.TakeDmg(0.0f);

            inGameGui.PlayScreenEffect();
        }
        break;

        case Networking::OpcodeServer::OPCODE_SERVER_PLAYER_HIT_CONFIRM:
        {
            auto ptd = packet.To<PlayerHitConfirm>();
            client_->logger->LogDebug("Enemy player was hit!");

            // Play sound effect
            if(Util::Map::Contains(playersExtraData, ptd->victimId))
            {
                auto& ed = playersExtraData[ptd->victimId];
                auto soundId = Util::Random::Uniform(0u, 1u) + Game::Sound::BULLET_HIT_METAL_1;
                auto audioId = gallery.GetSoundId((Game::Sound::SoundID)soundId);
                audioMgr->SetSourceAudio(ed.dmgSourceId, audioId);
                audioMgr->PlaySource(ed.dmgSourceId);
            }

            inGameGui.PlayHitMarkerAnim(InGameGUI::HitMarkerType::DMG);
        }
        break;

        case Networking::OpcodeServer::OPCODE_SERVER_PLAYER_DIED:
        {
            auto ptd = packet.To<PlayerDied>();
            auto& player = GetLocalPlayer();
            auto& killer = playerTable[ptd->killerId];
            auto& victim = playerTable[ptd->victimId];

            // Set Dead
            victim.health.hp = 0;

            if(ptd->victimId == playerId)
            {   
                fpsAvatar.isEnabled = false;
                inGameGui.crosshairImg.SetIsVisible(false);

                inGameGui.killText.SetIsVisible(true);
                inGameGui.respawnTimeText.SetIsVisible(true);

                // Clear prediction history. No prediction is useful
                predictionHistory_.Clear();

                respawnTimer.SetDuration(ptd->respawnTime);
                respawnTimer.Start();

                // Set killer
                killerId = ptd->killerId;
            }
            else
            {
                // Play death animation
                playerModelStateTable[ptd->victimId].deathPlayer.Restart();
            }

            if(ptd->killerId == playerId)
                inGameGui.PlayHitMarkerAnim(InGameGUI::HitMarkerType::KILL);

            match.GetGameMode()->PlayerDeath(ptd->killerId, ptd->killerId, killer.teamId);
            GetLogger()->LogDebug("Player died");

            if(killer.teamId == player.teamId && ptd->killerId != playerId)
            {
                auto audioId = (uint32_t)Game::Sound::ANNOUNCER_SOUND_ENEMY_KILLED_0 + Util::Random::Uniform(0u, 1u);
                if(audioId >= Game::Sound::ANNOUNCER_SOUND_ENEMY_KILLED_0 && audioId <= Game::Sound::ANNOUNCER_SOUND_ENEMY_KILLED_1)
                    PlayAnnouncerAudio((Game::Sound::AnnouncerSoundID)audioId);
            }
        }
        break;

        case Networking::OpcodeServer::OPCODE_SERVER_PLAYER_RESPAWN:
        {
            auto respawn = packet.To<PlayerRespawn>();
            auto& player = playerTable[respawn->playerId];
            player.ResetHealth();

            if(respawn->playerId == playerId)
            {
                // Accept last snapshot
                auto& player = GetLocalPlayer();
                localPlayerStateHistory.PushBack(respawn->playerState);
                GetLocalPlayer().ApplyState(respawn->playerState);

                // Set server weapon
                for(auto i = 0; i < Entity::Player::MAX_WEAPONS; i++)
                    if(respawn->weapons[i] != Entity::WeaponTypeID::NONE)
                        player.weapons[i] = Entity::WeaponMgr::weaponTypes.at(respawn->weapons[i]).CreateInstance();

                // Set spawn camera rotation
                auto camRot = camera_.GetRotationDeg();
                camera_.SetRotationDeg(90.0f, respawn->playerState.transform.rot.y + 90.0f);

                // On Player respawn
                fpsAvatar.isEnabled = true;
                inGameGui.crosshairImg.SetIsVisible(true);

                inGameGui.killText.SetIsVisible(false);
                inGameGui.respawnTimeText.SetIsVisible(false);
            }
            else
            {
                // Reset scale after death animation
                playerModelStateTable[respawn->playerId].gScale = 1.0f;
                if(Util::Map::Contains(playerTable, respawn->playerId))
                    playerTable[respawn->playerId].ApplyState(respawn->playerState);
            }

            GetLogger()->LogDebug("Player respawned");
        }
        break;

        case Networking::OpcodeServer::OPCODE_SERVER_PLAYER_WARPED:
        {
            auto warped = packet.To<PlayerWarped>();
            auto& player = playerTable[warped->playerId];

            if(warped->playerId == playerId)
            {
                // Accept last snapshot
                auto& player = GetLocalPlayer();
                localPlayerStateHistory.PushBack(warped->playerState);
                GetLocalPlayer().ApplyState(warped->playerState);

                // Set spawn camera rotation
                auto camRot = camera_.GetRotationDeg();
                camera_.SetRotationDeg(90.0f, warped->playerState.transform.rot.y + 90.0f);

                // Next prediction error should be corrected immediately
                errorCorrectionDuration = Util::Time::Seconds{0.01f};
            }
            else
            {
                if(Util::Map::Contains(playerTable, warped->playerId))
                    playerTable[warped->playerId].ApplyState(warped->playerState);
            }

            GetLogger()->LogError("Player warped to " + glm::to_string(warped->playerState.transform.pos));
        }
        break;

        case Networking::OpcodeServer::OPCODE_SERVER_SCOREBOARD_REPORT:
        {
            auto sr = packet.To<ScoreboardReport>();
            match.GetGameMode()->SetScoreboard(sr->scoreboard);

            if(match.GetState() == Match::StateType::ENDING)
            {
                inGameGui.EnableWinnerText(true);
                client_->SetMouseGrab(false);
            }
        }
        break;

        case Networking::OpcodeServer::OPCODE_SERVER_GAME_EVENT:
        {
            auto ge = packet.To<GameEvent>();
            auto event = ge->event;
            HandleGameEvent(event);
        }
        break;

        case Networking::OpcodeServer::OPCODE_SERVER_GAMEOBJECT_STATE:
        {
            auto gos = packet.To<GameObjectState>();
            auto goPos = gos->goPos;
            if(Util::Map::Contains(gameObjectStates, goPos))
            {
                gameObjectStates[goPos] = gos->state;
            }
        }
        break;

        case Networking::OpcodeServer::OPCODE_SERVER_PLAYER_GAMEOBJECT_INTERACT:
        {
            auto gos = packet.To<PlayerGameObjectInteract>();
            auto goPos = gos->goPos;
            
            if(Util::Map::Contains(gameObjectStates, goPos))
            {
                auto& player = GetLocalPlayer();
                auto go = map_.GetMap()->GetGameObject(goPos);
                player.InteractWith(*go);

                if(go->type == Entity::GameObject::Type::HEALTHPACK)
                    inGameGui.PlayScreenEffect(InGameGUI::SCREEN_EFFECT_HEALING);
                else if(go->type == Entity::GameObject::Type::WEAPON_CRATE)
                {
                    fpsAvatar.PlayReloadAnimation(player.GetCurrentWeapon().cooldown);
                }
                else if(go->type == Entity::GameObject::Type::GRENADES)
                {
                    if(auto pred = predictionHistory_.Back())
                    {
                        pred->origin.grenades = player.grenades;
                        pred->dest.grenades = player.grenades;
                    }
                }
            }
        }
        break;

        default:
            break;
    }
}

void InGame::HandleGameEvent(Event event)
{
    switch (event.type)
    {
    case EventType::POINT_CAPTURED:
        {
            // Force captured by
            auto gameMode = match.GetGameMode();
            if(gameMode->GetType() == GameMode::DOMINATION)
            {
                auto domination = static_cast<Domination*>(gameMode);
                auto pointCaptured = event.pointCaptured;
                if(Util::Map::Contains(domination->pointsState, pointCaptured.pos))
                {
                    auto& pointState = domination->pointsState[pointCaptured.pos];
                    pointState.capturedBy = pointCaptured.capturedBy;
                    pointState.timeLeft.Restart();

                    if(pointState.capturedBy == GetLocalPlayer().teamId)
                        PlayAnnouncerAudio(Game::Sound::ANNOUNCER_SOUND_CAPTURE_POINT_TAKEN);
                    else
                        PlayAnnouncerAudio(Game::Sound::ANNOUNCER_SOUND_CAPTURE_POINT_LOST);
                }
            }
        }
        break;
    case EventType::FLAG_EVENT:
        {
            auto flagEvent = event.flagEvent;
            auto gameMode = match.GetGameMode();
            GetLogger()->LogDebug("Recv flag event");
            if(gameMode->GetType() == GameMode::CAPTURE_THE_FLAG)
            {
                auto captureFlag = static_cast<CaptureFlag*>(gameMode);
                auto& flag = captureFlag->flags[flagEvent.flagId];

                switch (flagEvent.type)
                {
                case FLAG_TAKEN:
                    {
                        inGameGui.ShowLogMsg("The flag was taken");
                        PlayAnnouncerAudio(Game::Sound::ANNOUNCER_SOUND_FLAG_TAKEN);
                        GetLogger()->LogDebug("The flag wass taken");

                        flag.carriedBy = flagEvent.playerSubject;

                        // Enable flag model
                        if(auto flagCarrier = flag.carriedBy)
                        {
                            if(Util::Map::Contains(playerModelStateTable, flagCarrier.value()))
                                playerModelStateTable[flagCarrier.value()].flagCarrying = true;
                        }
                    }
                    break;

                case FLAG_DROPPED:
                    {
                        inGameGui.ShowLogMsg("The flag was dropped");
                        PlayAnnouncerAudio(Game::Sound::ANNOUNCER_SOUND_FLAG_DROPPED);
                        GetLogger()->LogDebug("The flag wass dropped");

                        flag.carriedBy.reset();
                        flag.pos = flagEvent.pos;
                        flag.recoverTimer.Restart();

                        // Disable flag model
                        if(Util::Map::Contains(playerModelStateTable, flagEvent.playerSubject))
                            playerModelStateTable[flagEvent.playerSubject].flagCarrying = false;
                    }
                    break;

                case FLAG_CAPTURED:
                    {
                        inGameGui.ShowLogMsg("The flag was captured");
                        PlayAnnouncerAudio(Game::Sound::ANNOUNCER_SOUND_FLAG_CAPTURED);
                        GetLogger()->LogDebug("The flag wass captured");

                        flag.carriedBy.reset();
                        flag.pos = flag.origin;
                        flag.recoverTimer.Restart();

                        // Disable flag model
                        if(Util::Map::Contains(playerModelStateTable, flagEvent.playerSubject))
                            playerModelStateTable[flagEvent.playerSubject].flagCarrying = false;
                    }
                    break;

                case FLAG_RECOVERED:
                    {
                        inGameGui.ShowLogMsg("The flag was recovered");
                        PlayAnnouncerAudio(Game::Sound::ANNOUNCER_SOUND_FLAG_RECOVERED);
                        GetLogger()->LogDebug("The flag wass recovered");

                        flag.carriedBy.reset();
                        flag.pos = flag.origin;
                        flag.recoverTimer.Restart();
                    }
                    break;

                case FLAG_RESET:
                    {
                        inGameGui.ShowLogMsg("The flag was reset");
                        PlayAnnouncerAudio(Game::Sound::ANNOUNCER_SOUND_FLAG_RESET);
                        GetLogger()->LogDebug("The flag wass reset");

                        flag.carriedBy.reset();
                        flag.pos = flag.origin;
                        flag.recoverTimer.Restart();
                    }
                    break;

                case FLAG_STATE:
                    {
                        GetLogger()->LogDebug("Flag state recv");
                        if(flagEvent.playerSubject != 255)
                            flag.carriedBy = flagEvent.playerSubject;
                        flag.pos = flagEvent.pos;
                        flag.recoverTimer.Restart();
                        flag.recoverTimer.Update(flagEvent.elapsed);

                        // Enable flag model
                        if(auto flagCarrier = flag.carriedBy)
                        {
                            if(Util::Map::Contains(playerModelStateTable, flagCarrier.value()))
                                playerModelStateTable[flagCarrier.value()].flagCarrying = true;
                        }
                    }
                    break;
                
                default:
                    break;
                }
            }
        }
    
    default:
        break;
    }
}

void InGame::RecvServerSnapshots()
{
    host.PollAllEvents();
}

void InGame::UpdateNetworking()
{
    auto& player = GetLocalPlayer();
    bool sendInputs = !player.IsDead() && match.GetState() == Match::ON_GOING;
    if(sendInputs)
    {
        // Sample player input
        auto mask = inGameGui.IsMenuOpen() ? Entity::PlayerInput{false} : Entity::PlayerInput{true};
        auto input = Input::GetPlayerInput(mask);

        // Prediction
        Predict(input);
        SendPlayerInput();

        // Update last cmdId
        this->cmdId++;
    }
}

void InGame::SendPlayerInput()
{
    // Build batched player input batch
    auto cmdId = this->cmdId;
    Networking::Batch<Networking::PacketType::Client> batch;

    auto historySize = predictionHistory_.GetSize();
    auto redundancy = std::min(redundantInputs, historySize);
    auto amount = redundancy + 1u;

    // Write inputs
    for(int i = 0; i < amount; i++)
    {
        auto oldCmd = predictionHistory_.At((historySize - (1 + i)));

        using InputPacket = Networking::Packets::Client::Input;
        auto inputPacket = std::make_unique<InputPacket>();
        
        inputPacket->req = oldCmd->inputReq;

        batch.PushPacket(std::move(inputPacket));
    }

    // Send them
    batch.Write();
    auto batchBuffer = batch.GetBuffer();
    ENet::SentPacket sentPacket{batchBuffer->GetData(), batchBuffer->GetSize(), 0};
    host.SendPacket(serverId, 0, sentPacket);
}

// Networking Prediction

void InGame::Predict(Entity::PlayerInput playerInput)
{
    if(playerTable.find(playerId) == playerTable.end())
        return;

    auto& player = GetLocalPlayer();

    // Check prediction errors
    if(!predictionHistory_.Empty() && predictionHistory_.Front()->inputReq.reqId <= this->lastAck)
    {
        // Discard old commands
        std::optional<Prediction> prediction;
        GetLogger()->LogDebug("Prediction: Last ACK " + std::to_string(this->lastAck));
        do
        {
            prediction = predictionHistory_.PopFront();
            GetLogger()->LogDebug("Prediction: Discarding prediction for " + std::to_string(prediction->inputReq.reqId));
        } while(prediction && prediction->inputReq.reqId < lastAck);

        // Checking prediction errors
        auto lastState = localPlayerStateHistory.Back();
        if(prediction && lastState.has_value())
        {
            auto newState = lastState.value();
            auto pState = prediction->dest;

            // On error
            if(newState != pState)
            {
                #ifdef _DEBUG
                    GetLogger()->LogError("New state pos " + glm::to_string(newState.transform.pos));
                    GetLogger()->LogError("Predicted state pos " + glm::to_string(pState.transform.pos));
                    GetLogger()->LogError("Render pos " + glm::to_string(playerTable[playerId].GetRenderTransform().position));
                    GetLogger()->LogError("Error correction offset " + glm::to_string(errorCorrectionDiff.pos));
                #endif

                GetLogger()->LogError("Prediction: Error on prediction");
                GetLogger()->LogError("Prediction: Prediction Id " + std::to_string(prediction->inputReq.reqId) + " ACK " + std::to_string(this->lastAck));
                auto diff = glm::length(pState.transform.pos - newState.transform.pos);
                if(diff > 0.005f)
                    GetLogger()->LogError("Prediction: D " + std::to_string(diff) + 
                        " P " + glm::to_string(pState.transform.pos) + " S " + glm::to_string(newState.transform.pos));

                // Accept player pos
                auto realState = newState;

                // Run prev commands again
                for(auto i = 0; i < predictionHistory_.GetSize(); i++)
                {
                    auto predict = predictionHistory_.At(i);
                    GetLogger()->LogDebug("Prediction: Repeting prediction for " + std::to_string(predict->inputReq.reqId));

                    // Re-calculate prediction
                    auto origin = realState;
                    realState = PredPlayerState(origin, predict->inputReq.playerInput, predict->inputReq.camYaw, serverTickRate);
                    GetLogger()->LogError("Prediction: Predicted pos is " + glm::to_string(realState.transform.pos));

                    // Update history
                    predict->origin = origin;
                    predict->dest = realState;
                    predictionHistory_.Set(i, predict.value());
                }

                // Update error correction values
                auto renderState = GetLocalPlayer().ExtractState();
                errorCorrectionDiff = renderState.transform - realState.transform;
                errorCorrectionStart = Util::Time::GetTime();
                correctError = true;
            }
        }
    }

    // Get prev pos
    auto preState = localPlayerStateHistory.Back().value();
    if(auto prevPred = predictionHistory_.Back())
        preState = prevPred->dest;

    // Run predicted command for this simulation
    auto camRot = camera_.GetRotationDeg();
    auto predState = PredPlayerState(preState, playerInput, camRot.y, serverTickRate);
    predState.transform.rot.x = camRot.x;
    auto fov = camera_.GetParam(Rendering::Camera::FOV);
    auto aspectRatio = camera_.GetParam(Rendering::Camera::ASPECT_RATIO);
    InputReq inputReq{cmdId, playerInput, camRot.y, camRot.x, fov, aspectRatio, GetRenderTime()};

    // Store prediction
    auto now = Util::Time::GetTime();
    Prediction p{inputReq, preState, predState, now};
    predictionHistory_.PushBack(p);
}

void InGame::SmoothPlayerMovement()
{
    if(playerTable.find(playerId) == playerTable.end())
        return;

    auto& player = GetLocalPlayer();
    if(player.IsDead())
        return;

    auto lastPred = predictionHistory_.Back();
    if(lastPred)
    {
        auto now = Util::Time::GetTime();
        Util::Time::Seconds elapsed = now - lastPred->time;
        auto predState = PredPlayerState(lastPred->origin, lastPred->inputReq.playerInput, lastPred->inputReq.camYaw, elapsed);

        // Prediction Error correction
        if(correctError)
        {
            Util::Time::Seconds errorElapsed = now - errorCorrectionStart;
            float weight = glm::max(1.0 - (errorElapsed / errorCorrectionDuration), 0.0);
            auto errorCorrection = errorCorrectionDiff * weight;

            GetLogger()->LogError("Error correction duration " + std::to_string(errorCorrectionDuration.count()));

            predState.transform = predState.transform + errorCorrection;
            correctError = weight <= 0.005f;
            if(!correctError)
                errorCorrectionDuration = ERROR_CORRECTION_DURATION;

            #ifdef _DEBUG
                auto dist = glm::length(errorCorrection.pos);
                if(dist > 0.005)
                    GetLogger()->LogError("Error correction is " + glm::to_string(errorCorrection.pos) + " W " + std::to_string(weight) + " D " + std::to_string(dist));
            #endif
        }
        player.ApplyState(predState);

        // Animation
        if(lastPred->inputReq.reqId != lastRenderedPred)
        {
            auto oldState = lastPred->origin;
            auto renderState = GetLocalPlayer().ExtractState();
            auto renderWepState = renderState.weaponState[oldState.curWep];
            
            auto oldWepState = oldState.weaponState[oldState.curWep];
            auto nextWepState = lastPred->dest.weaponState[oldState.curWep];
            
            if(Entity::HasShot(oldWepState.state, nextWepState.state))
                OnLocalPlayerShot();
            
            bool hasReloaded = Entity::HasReloaded(oldWepState.state, nextWepState.state);
            if(hasReloaded)
                OnLocalPlayerReload();

            bool playReloadAnim = hasReloaded || Entity::HasStartedSwap(oldWepState.state, nextWepState.state) ||
                Entity::HasPickedUp(oldWepState.state, nextWepState.state) || Entity::HasGrenadeThrow(oldWepState.state, nextWepState.state);
            if(playReloadAnim)
                fpsAvatar.PlayReloadAnimation(nextWepState.cooldown);
            
            lastRenderedPred = lastPred->inputReq.reqId;
        }
    }
}

Entity::PlayerState InGame::PredPlayerState(Entity::PlayerState a, Entity::PlayerInput playerInput, float playerYaw, Util::Time::Seconds deltaTime)
{
    auto& player = GetLocalPlayer();
    
    // Position
    a.transform.rot.y = playerYaw;
    Entity::PlayerState nextState = pController.UpdatePosition(a, playerInput, map_.GetMap(), deltaTime);

    // Weapons
    auto nextWep = (a.curWep + 1) % Entity::Player::MAX_WEAPONS;
    nextState.weaponState[a.curWep] = pController.UpdateWeapon(a.weaponState[a.curWep], a.weaponState[nextWep], playerInput, deltaTime);

    if(Entity::HasSwapped(a.weaponState[a.curWep].state, nextState.weaponState[nextState.curWep].state))
    {   
        // Swap weap
        nextState.curWep = player.WeaponSwap();
    }

    if(playerInput[Entity::GRENADE] && a.weaponState[a.curWep].state == Entity::Weapon::State::IDLE && a.grenades > 0)
    {
        Entity::StartGrenadeThrow(nextState.weaponState[a.curWep]);
        nextState.grenades = std::max(nextState.grenades - 1, 0);
    }

    return nextState;
}

// Networking Entity Interpolation

Util::Time::Seconds InGame::TickToTime(uint32_t tick)
{
    return tick * this->serverTickRate;
}

Util::Time::Seconds InGame::GetCurrentTime()
{
    auto maxTick = snapshotHistory.Back()->serverTick;
    auto base = TickToTime(maxTick);

    return base + this->offsetTime;
}

Util::Time::Seconds InGame::GetRenderTime()
{
    auto clientTime = GetCurrentTime();
    auto windowSize = 2.0 * this->serverTickRate;
    auto renderTime = clientTime - windowSize;
    
    return renderTime;
}

void InGame::EntityInterpolation()
{
    if(snapshotHistory.GetSize() < 2)
        return;

    // Calculate render time
    auto clientTime = GetCurrentTime();
    auto renderTime = GetRenderTime();

    // Get last snapshot before renderTime
    auto s1p = snapshotHistory.FindRevFirstPair([this, renderTime](auto i, auto s)
    {
        auto time = TickToTime(s.serverTick);
        return time < renderTime;
    });
    
    if(s1p.has_value())
    {
        auto s2o = snapshotHistory.At(s1p->first + 1);
        if(s2o.has_value())
        {
            // Find samples to use for interpolation
            // Sample 1: Last one before current render time
            // Sample 2: First one after current render time
            auto s1 = s1p->second;
            auto s2 = s2o.value();

            // Find weights
            auto t1 = TickToTime(s1.serverTick);
            auto t2 = TickToTime(s2.serverTick);
            auto ws = Math::GetWeights(t1.count(), t2.count(), renderTime.count());
            auto w1 = ws.x; auto w2 = ws.y;

            client_->logger->LogDebug("Tick 1 " + std::to_string(s1.serverTick) + " Tick 2 " + std::to_string(s2.serverTick));
            client_->logger->LogDebug("RT " + std::to_string(renderTime.count()) + " CT " + std::to_string(clientTime.count()) + " OT " + std::to_string(offsetTime.count()));
            client_->logger->LogDebug("T1 " + std::to_string(t1.count()) + " T2 " + std::to_string(t2.count()));
            client_->logger->LogDebug("W1 " + std::to_string(w1) + " W2 " + std::to_string(w2));

            // Player interpolation
            for(auto pair : playerTable)
            { 
                auto playerId = pair.first;
                if(playerId == this->playerId)
                    continue;

                bool s1HasData = s1.players.find(pair.first) != s1.players.end();
                bool s2HasData = s2.players.find(pair.first) != s2.players.end();
                bool canInterpolate = s1HasData && s2HasData;

                if(canInterpolate)
                    EntityInterpolation(playerId, s1, s2, w1);
            }

            // Projectile interpolation
            for(auto& [id, projectile] : projectiles)
            {
                bool s1HasData = s1.projectiles.find(id) != s1.projectiles.end();
                bool s2HasData = s2.projectiles.find(id) != s2.projectiles.end();
                bool canInterpolate = s1HasData && s2HasData;

                if(canInterpolate)
                {
                    Entity::Projectile::State s = Entity::Projectile::State::Interpolate(s1.projectiles[id], s2.projectiles[id], w1);
                    projectile->ApplyState(s);
                }
            }
        }
        // We don't have enough data. We need to extrapolate
        else
        {
            // Get prev snapshot with player data
            auto prev = snapshotHistory.At(s1p->first - 1);

            if(prev.has_value())
            {
                auto s0 = prev.value();
                auto s1 = s1p.value().second;

                Networking::Snapshot exS;
                exS.serverTick = s1.serverTick + 1;

                for(auto pair : playerTable)
                {
                    auto playerId = pair.first;
                    if(playerId == this->playerId)
                        continue;

                    // Extrapolate player pos
                    /*
                    auto s0Pos = s0.players[playerId].pos;
                    auto s1Pos = s1.players[playerId].pos;
                    auto offset = s1Pos - s0Pos;
                    auto s2Pos = PredPlayerState(s1Pos, offset, EXTRAPOLATION_DURATION);
                    exS.players[playerId].pos = s2Pos;

                    // Interpolate
                    auto t1 = TickToTime(s1.serverTick);
                    auto t2 = TickToTime(s1.serverTick) + EXTRAPOLATION_DURATION;
                    auto ws = Math::GetWeights(t1.count(), t2.count(), renderTime.count());
                    auto alpha = ws.x;
                    EntityInterpolation(playerId, s1, exS, alpha);
                    */
                }
            }
        }
    }
}

void InGame::EntityInterpolation(Entity::ID playerId, const Networking::Snapshot& s1, const Networking::Snapshot& s2, float alpha)
{
    auto state1 = s1.players.at(playerId);
    auto state2 = s2.players.at(playerId);

    auto& player = playerTable[playerId];
    auto oldState = player.ExtractState();
    auto interpolation = Networking::PlayerSnapshot::Interpolate(state1, state2, alpha);

    if(Entity::HasShot(oldState.weaponState[oldState.curWep].state, interpolation.wepState))
    {
        auto& modelState = playerModelStateTable.at(playerId);
        modelState.shootPlayer.SetClip(playerAvatar.GetShootAnim());
        modelState.shootPlayer.Restart();

        // HACK Reset pitch, in case it was reloading
        modelState.armsPivot.rotation.x = 0.0f;
        modelState.armsPivot.position.y = 0.0f;

        // Play audio effect
        auto& ed = playersExtraData[playerId];
        auto audioId = gallery.GetWepSoundID(oldState.weaponState->weaponTypeId);
        audioMgr->SetSourceAudio(ed.shootSourceId, audioId);
        audioMgr->PlaySource(ed.shootSourceId);
    }

    if(Entity::HasReloaded(oldState.weaponState[oldState.curWep].state, interpolation.wepState) ||
        Entity::HasStartedSwap(oldState.weaponState[oldState.curWep].state, interpolation.wepState))
    {
        auto& modelState = playerModelStateTable.at(playerId);
        modelState.shootPlayer.SetClip(playerAvatar.GetReloadAnim());
        modelState.shootPlayer.Restart();

        modelState.leftFlashActive = false;
        modelState.rightFlashActive = false;
    }

    auto newState = interpolation.ToPlayerState(oldState);
    player.ApplyState(newState);
}

glm::vec3 InGame::GetLastMoveDir(Entity::ID playerId) const
{
    auto ps1 = prevPlayerTable.at(playerId).ExtractState();
    auto ps2 = playerTable.at(playerId).ExtractState();
    auto moveDir = Entity::GetLastMoveDir(ps1.transform.pos, ps2.transform.pos);

    return moveDir;
}
