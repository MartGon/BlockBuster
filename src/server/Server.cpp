
#include <Server.h>

#include <entity/Game.h>

#include <util/Container.h>

using namespace BlockBuster;
using namespace Networking::Packets::Client;

Server::Server(Params params, MMServer mmServer) : params{params}, mmServer{mmServer}, 
    asyncClient{mmServer.address, mmServer.port}
{

}

void Server::Start()
{
    InitLogger();
    InitNetworking();
    InitMatch();
    
    auto next = Util::Time::GetTime() + TICK_RATE;
    nextTickDate = next;
}

void Server::Run()
{
    // Server loop
    while(!match.IsOver())
    {
        host->PollAllEvents();

        auto preSimulationTime = Util::Time::GetTime();

        if(match.IsOnGoing())
        {
            HandleClientsInput();
            UpdateWorld();
            SendWorldUpdate();

            tickCount++;
        }

        SendScoreboardReport();
        match.Update(GetWorld(), TICK_RATE);
        logger.Flush();
        SleepUntilNextTick(preSimulationTime);
    }

    logger.LogInfo("Server shutdown");
    ServerEvent::Notification n;
    n.eventType = ServerEvent::Type::GAME_ENDED;
    n.event = ServerEvent::GameEnded();
    SendServerNotification(n);
}

// Initialization

void Server::InitLogger()
{
    // Loggers
    auto clogger = std::make_unique<Log::ConsoleLogger>();
    auto flogger = std::make_unique<Log::FileLogger>();
    std::filesystem::path logFile = "server.log";
    flogger->OpenLogFile("./logs/server.log");
    if(!flogger->IsOk())
        clogger->LogError("Could not create log file " + logFile.string());

    logger.AddLogger(std::move(clogger));
    logger.AddLogger(std::move(flogger));
    logger.SetVerbosity(params.verbosity);
}

void Server::InitNetworking()
{
    // Networking setup
    auto hostFactory = ENet::HostFactory::Get();
    auto localhost = ENet::Address::CreateByDomain(params.address, params.port).value();
    host = hostFactory->CreateHost(localhost, params.maxPlayers, 2);
    logger.LogInfo("Server initialized. Listening on address " + localhost.GetHostName() + ":" + std::to_string(localhost.GetPort()));
    host->SetOnConnectCallback([this](auto peerId)
    {
        this->OnClientJoin(peerId);
    });
    host->SetOnRecvCallback([this](ENet::PeerId peerId, uint8_t channelId, ENet::RecvPacket recvPacket){
        this->OnRecvPacket(peerId, channelId, std::move(recvPacket));
    });
    host->SetOnDisconnectCallback([this](auto peerId)
    {
        this->OnClientLeave(peerId);
    });

    // Allocate clients
    clients.reserve(params.maxPlayers);
}

void Server::InitMatch()
{
    InitAI();
    InitMap();
    InitGameObjects();
    auto mode = GameMode::stringTypes.at(params.mode);
    match.Start(GetWorld(), mode);
}

void Server::InitAI()
{
    // Create AI players
    Client ai;
    ai.isAI = true;
    Entity::Player player;
    ai.player.id = 200;
    //ai.player.transform = Math::Transform{GetRandomPos(), glm::vec3{0.0f}, glm::vec3{2.f, 4.0f, 2.f}};
    //ai.targetPos = GetRandomPos();
    //clients[200] = ai;
}

void Server::InitMap()
{
    auto res = Game::Map::Map::LoadFromFile(params.mapPath);
    if(res.isOk())
    {
        auto mapPtr = std::move(res.unwrap());
        map = std::move(*mapPtr.get());
    }
    else
    {
        logger.LogError("Could not load map " + params.mapPath.string());
        std::exit(-1);
    }
}

void Server::InitGameObjects()
{
    auto criteria = [this](glm::ivec3 pos, Entity::GameObject& go)
    {
        return go.IsInteractable();
    };
    auto goIndices = map.FindGameObjectByCriteria(criteria);
    for(auto goIndex : goIndices)
    {
        auto go = map.GetGameObject(goIndex);
        auto respawnTime = std::get<int>(go->properties["Respawn Time (s)"].value);

        GOState goState;
        goState.state = Entity::GameObject::State{true};
        goState.respawnTimer.SetDuration(Util::Time::Seconds{respawnTime});
        gameObjectStates[goIndex] = goState;
    }
}

// Networking

void Server::OnClientJoin(ENet::PeerId peerId)
{
    logger.LogError("Connected with peer " + std::to_string(peerId));

    // Create Client
    Entity::Player player;
    player.id = peerId;

    // Add to table
    clients[peerId].id = peerId;
    clients[peerId].player = player;
}

void Server::OnClientLogin(ENet::PeerId peerId, std::string playerUuid, std::string playerName)
{
    auto& client = clients[peerId];
    auto& player = client.player;

    // Set name
    client.playerUuuid = playerUuid;
    client.playerName = playerName;

    // Get Team id
    auto teamId = match.GetGameMode()->PlayerJoin(client.player.id, playerName);
    client.player.teamId = teamId;

    // Spawn player
    SpawnPlayer(peerId);

    // Send welcome packet
    Networking::Batch<Networking::PacketType::Server> batch;

    auto welcome = std::make_unique<Networking::Packets::Server::Welcome>();
    welcome->playerId = peerId;
    welcome->teamId = teamId;
    welcome->tickRate = TICK_RATE.count();
    welcome->mode = match.GetGameMode()->GetType();
    logger.LogError("Game mode sent is " + std::to_string(welcome->mode));
    batch.PushPacket(std::move(welcome));

    // Send match state packet
    auto ms = std::make_unique<Networking::Packets::Server::MatchState>();
    ms->state = match.ExtractState();
    batch.PushPacket(std::move(ms));

    // Send info about other connected players
    std::vector<ENet::PeerId> connectedClients;
    for(auto& [id, client] : clients)
    {
        if(id != peerId)
            connectedClients.push_back(id);
    }

    // Send gameObjects state
    for(auto [goPos, state] : gameObjectStates)
    {
        auto gos = std::make_unique<Networking::Packets::Server::GameObjectState>();
        gos->goPos = goPos;
        gos->state = gameObjectStates[goPos].state;
        batch.PushPacket(std::move(gos));
    }

    for(auto& id : connectedClients)
    {
        auto& client = clients[id];
        auto pj = std::make_unique<Networking::Packets::Server::PlayerJoined>();
        auto& player = client.player;
        pj->playerId = player.id;
        pj->teamId = player.teamId;
        pj->playerSnapshot = Networking::PlayerSnapshot::FromPlayerState(player.ExtractState());
        batch.PushPacket(std::move(pj));
    }

    // Send batch to new client
    SendPacket(peerId, batch);
    BroadcastRespawn(peerId);

    // Broadcast to already connected players
    for(auto& id : connectedClients)
    {
        auto& client = clients[id];
        auto pj = std::make_unique<Networking::Packets::Server::PlayerJoined>();
        pj->playerId = player.id;
        pj->teamId = player.teamId;
        pj->playerSnapshot = Networking::PlayerSnapshot::FromPlayerState(player.ExtractState());
        SendPacket(client.id, *pj);
    }
}

void Server::OnClientLeave(ENet::PeerId peerId)
{
    logger.LogInfo("Peer with id " + std::to_string(peerId) + " disconnected.");
    auto& client = clients[peerId];
    auto& player = client.player;

    // Update scoreboard
    match.GetGameMode()->PlayerLeave(player.id);

    // Informing players
    Networking::Packets::Server::PlayerDisconnected pd;
    pd.playerId = player.id;
    Broadcast(pd);

    // Inform MM Server
    ServerEvent::Notification notification;
    notification.eventType = ServerEvent::PLAYER_LEFT;
    notification.event = ServerEvent::PlayerLeft{client.playerUuuid};
    SendServerNotification(notification);

    // Finally remove peerId
    clients.erase(peerId);
}

void Server::OnRecvPacket(ENet::PeerId peerId, uint8_t channelId, ENet::RecvPacket recvPacket)
{
    auto& client = clients[peerId];

    Util::Buffer::Reader reader{recvPacket.GetData(), recvPacket.GetSize()};
    Util::Buffer buffer = reader.ReadAll();
    
    auto packet = Networking::MakePacket<Networking::PacketType::Client>(std::move(buffer));
    if(packet)
    {
        packet->Read();
        OnRecvPacket(peerId, *packet);
    }
    else
        logger.LogError("Invalid packet recv from " + std::to_string(peerId));
}

void Server::OnRecvPacket(ENet::PeerId peerId, Networking::Packet& packet)
{    
    auto& client = clients[peerId];

    switch (packet.GetOpcode())
    {
        case Networking::OpcodeClient::OPCODE_CLIENT_BATCH:
        {
            auto batch = packet.To<Networking::Batch<Networking::PacketType::Client>>();
            for(auto i = 0; i < batch->GetPacketCount(); i++)
                OnRecvPacket(peerId, *batch->GetPacket(i));
        }
        break;
        case Networking::OpcodeClient::OPCODE_CLIENT_LOGIN:
        {
            auto login = packet.To<Networking::Packets::Client::Login>();
            OnClientLogin(peerId, login->playerUuid, login->playerName);
        }
        break;
        case Networking::OpcodeClient::OPCODE_CLIENT_INPUT:
        {
            auto inputPacket = packet.To<Networking::Packets::Client::Input>();
            auto inputReq = inputPacket->req;

            auto cmdId = inputReq.reqId;
            logger.LogInfo("Command arrived with cmdid " + std::to_string(cmdId) + " from " + std::to_string(peerId));

            // Check if we already have this input
            auto found = client.inputBuffer.FindFirst([cmdId](auto input)
            {
                return input.reqId == cmdId;
            });

            if(!found.has_value())
            {
                auto moveDir = Entity::PlayerInputToMove(inputReq.playerInput);

                if(cmdId > client.lastAck)
                    client.inputBuffer.PushBack(inputReq);
                // This packet is either duplicated or it arrived really late
                else
                    logger.LogWarning("Command with cmdid " + std::to_string(cmdId) + " dropped. Last ack " + std::to_string(client.lastAck));

                // Sort to mitigate unordered packets
                client.inputBuffer.Sort([](auto a, auto b){
                    return a.reqId < b.reqId;
                });

                auto bufferSize = client.inputBuffer.GetSize();
                if(bufferSize >= MAX_INPUT_BUFFER_SIZE)
                    logger.LogDebug("Max Buffer size reached." + std::to_string(bufferSize));

                if(client.state == BufferingState::REFILLING && client.inputBuffer.GetSize() > MIN_INPUT_BUFFER_SIZE)
                    client.state = BufferingState::CONSUMING;
            }
        }
        break;
    default:
        break;
    }
}

void Server::HandleClientsInput()
{
    // Handle client inputs
    for(auto& [peerId, client] : clients)
    {
        if(!client.isAI)
        {
            if(client.state == BufferingState::CONSUMING)
            {
                // Consume movement commands
                if(!client.inputBuffer.Empty())
                {
                    auto command = client.inputBuffer.PopFront();
                    auto cmdId = command->reqId;
                    client.lastAck = cmdId;

                    logger.LogDebug("Cmd id: " + std::to_string(client.lastAck) + " from " + std::to_string(peerId));
                    HandleClientInput(peerId, command.value());

                    logger.LogDebug("Ring size " + std::to_string(client.inputBuffer.GetSize()));
                }
                else
                {
                    logger.LogWarning("No cmd handled for player " + std::to_string(peerId) + " waiting for buffer to refill ");
                    client.state = BufferingState::REFILLING;
                }
            }
        }
        /*
        else if(client.isAi)
        {
            auto origin = client.player.transform.position;
            auto dest = client.targetPos;
            auto move = dest - origin;
            auto dist = glm::length(move);

            auto stepDist = PLAYER_SPEED * (float)TICK_RATE.count();
            if(dist > stepDist)
            {
                auto dir = glm::normalize(move);
                //InputReq pm{0, dir};
                //HandleMoveCommand(peerId, pm);
            }
            else
                client.targetPos = GetRandomPos();
        }
        */
    }
}

void Server::HandleClientInput(ENet::PeerId peerId, Input::Req cmd)
{
    auto& client = clients[peerId];
    auto& player = client.player;

    if(player.IsDead())
        return;

    auto input = cmd.playerInput;

    auto playerTransform = player.GetTransform();
    auto playerPos = playerTransform.position;
    auto playerYaw = cmd.camYaw;

    auto& pController = client.pController;
    playerTransform.position = pController.UpdatePosition(playerPos, playerYaw, input, &map, TICK_RATE);
    playerTransform.rotation = glm::vec3{cmd.camPitch, playerYaw, 0.0f};
    player.SetTransform(playerTransform);
    logger.LogDebug("MovePos " + glm::to_string(playerTransform.position));

    auto oldWepState = player.GetCurrentWeapon();
    auto nextWeapon = player.weapons[player.GetNextWeaponId()];
    player.GetCurrentWeapon() = pController.UpdateWeapon(player.GetCurrentWeapon(), nextWeapon, input, TICK_RATE);
    logger.LogDebug("Player Ammo " + std::to_string(player.GetCurrentWeapon().ammoState.magazine));

    if(Entity::HasShot(oldWepState.state, player.GetCurrentWeapon().state))
    {
        ShotCommand sc{peerId, player.GetFPSCamPos(), glm::radians(playerTransform.rotation), cmd.fov, cmd.aspectRatio, cmd.renderTime};
        HandleShootCommand(sc);
    }

    if(Entity::HasSwapped(oldWepState.state, player.GetCurrentWeapon().state))
        player.WeaponSwap();

    if(input[Entity::Inputs::ACTION])
        HandleActionCommand(player);

    if(input[Entity::Inputs::GRENADE])
        HandleGrenadeCommand(peerId);

    if(IsPlayerOutOfBounds(peerId))
        OnPlayerDeath(peerId, peerId);
}

void Server::HandleShootCommand(ShotCommand sc)
{
    auto commandTime = sc.commandTime;
    logger.LogDebug("Command time is " + std::to_string(commandTime.count()));

    // Find first snapshot before commandTime
    auto s1p = history.FindRevFirstPair([commandTime, this](auto i, Networking::Snapshot s)
    {
        Util::Time::Seconds sT = s.serverTick * TICK_RATE;
        return sT < commandTime;
    });

    if(!s1p.has_value())
        return;

    auto s2o = history.At(s1p->first + 1);
    if(!s2o.has_value())
        return;

    // Samples to use for interpolation
    // S1 last sample before commandTime
    // S2 first sample after commandTime
    auto s1 = s1p->second;
    auto s2 = s2o.value();

    // Find weights
    Util::Time::Seconds t1 = s1.serverTick * TICK_RATE;
    Util::Time::Seconds t2 = s2.serverTick * TICK_RATE;
    auto ws = Math::GetWeights(t1.count(), t2.count(), commandTime.count());
    auto alpha = ws.x;

    // Calculate client projViewMat
    auto projMat = Math::GetPerspectiveMat(sc.fov, sc.aspectRatio);
    auto viewMat = Math::GetViewMat(sc.origin, sc.playerOrientation);
    auto projViewMat = projMat * viewMat;

    // Get Ray
    Collisions::Ray ray = Collisions::ScreenToWorldRay(projViewMat, glm::vec2{0.5f, 0.5f}, glm::vec2{1.0f});
    logger.LogDebug("Handling player shot from " + glm::to_string(ray.origin) + " to " + glm::to_string(ray.GetDir()));

    // Check collision with block
    auto bCol = Game::CastRayFirst(&map, ray, map.GetBlockScale());
    auto bColDist = std::numeric_limits<float>::max();
    if(bCol.intersection.intersects)
        bColDist = bCol.intersection.GetRayLength(ray);

    // Check collision with players. This allows collaterals
    auto& author = clients[sc.clientId].player;
    for(auto& [peerId, client] : clients)
    {
        if(peerId == sc.clientId)
            continue;

        // No Friendly fire or dmg to dead units
        auto& victim = client.player;
        if(victim.IsDead() || victim.teamId == author.teamId)
            continue;

        auto playerId = victim.id;
        bool s1HasData = s1.players.find(playerId) != s1.players.end();
        bool s2HasData = s2.players.find(playerId) != s2.players.end();

        if(!s1HasData || !s2HasData)
            continue;

        // Calculate player pos that client views
        auto ps1 = s1.players.at(playerId);
        auto ps2 = s2.players.at(playerId);
        auto smoothState = Networking::PlayerSnapshot::Interpolate(ps1, ps2, alpha);

        auto lastMoveDir = Entity::GetLastMoveDir(ps1.transform.pos, ps2.transform.pos);
        auto rpc = Game::RayCollidesWithPlayer(ray, smoothState.transform.pos, smoothState.transform.rot.y, lastMoveDir);
        if(rpc.collides)
        {
            auto colDist = rpc.intersection.GetRayLength(ray);
            if(colDist < bColDist)
            {
                victim.TakeWeaponDmg(author.GetCurrentWeapon(), rpc.hitboxType, colDist);
                OnPlayerTakeDmg(sc.clientId, peerId);

                logger.LogDebug("Shot from player " + std::to_string(author.id) + " has hit player " + std::to_string(victim.id));
                logger.LogDebug("Victim's shield " + std::to_string(victim.health.shield) + " Victim health " + std::to_string(victim.health.hp));
            }
            else
                logger.LogDebug("Shot from player " + std::to_string(author.id) + " has hit block " + glm::to_string(bCol.pos));
        }
        else
            logger.LogDebug("Shot from player " + std::to_string(author.id) + " has NOT hit player " + std::to_string(peerId));
        //logger.LogInfo("Player was at " + glm::to_string(smoothState.transform.pos));
    }
}

void Server::HandleActionCommand(Entity::Player& player)
{
    auto blockScale = map.GetBlockScale();
    for(auto& [pos, goState] : gameObjectStates)
    {
        if(!goState.state.isActive)
            continue;

        auto goPos = Game::Map::ToRealPos(pos, blockScale);
        auto pPos = player.GetTransform().position;
        if(Collisions::IsPointInSphere(pPos, goPos, Entity::GameObject::ACTION_AREA))
        {
            auto go = map.GetGameObject(pos);
            player.InteractWith(*go);

            // Inform clients
            goState.state.isActive = false;
            goState.respawnTimer.Restart();
            BroadcastGameObjectState(pos);

            // Inform player of interaction
            SendPlayerGameObjectInteraction(player.id, pos);
        }
    }
}

void Server::HandleGrenadeCommand(ENet::PeerId peerId)
{
    auto& player = clients[peerId].player;
    auto& weapon = player.GetCurrentWeapon();

    if(weapon.state == Entity::Weapon::State::IDLE && player.HasGrenades())
    {
        player.ThrowGrenade();

        auto origin = player.GetFPSCamPos();
        auto rotation = player.GetTransform().rotation;
        auto dir = Math::GetFront(glm::radians(glm::vec2{rotation}));

        auto p = std::make_unique<Entity::Grenade>();
        float SPEED = 20.0f;
        glm::vec3 acceleration{0.0f, -10.0f, 0.0f};
        p->Launch(player.id, origin, dir * SPEED, acceleration);
        projectiles.MoveInto(std::move(p));
    }
}

// Sending

void Server::SendWorldUpdate()
{
    // Create snapshot
    auto worldUpdate = std::make_unique<Networking::Packets::Server::WorldUpdate>();

    Networking::Snapshot s;
    s.serverTick = tickCount;

    // Players
    for(auto& [id, client] : clients)
    {
        auto& player = client.player;
        if(player.IsDead())
            continue;

        s.players[player.id] = Networking::PlayerSnapshot::FromPlayerState(player.ExtractState());
    }
    
    // Projectiles
    for(auto& id : projectiles.GetIDs())
    {
        auto& projectile = projectiles.GetRef(id);
        s.projectiles[id] = projectile->ExtractState();
    }

    // Save snapshot
    history.PushBack(s);

    // Send snapshot with acks
    for(auto& [id, client] : clients)
    {
        if(client.isAI)
            continue;

        Networking::Batch<Networking::PacketType::Server> batch;

        auto worldUpdate = std::make_unique<Networking::Packets::Server::WorldUpdate>();
        worldUpdate->snapShot = s;
        batch.PushPacket(std::move(worldUpdate));

        auto playerInfo = std::make_unique<Networking::Packets::Server::PlayerInputACK>();
        playerInfo->playerState = client.player.ExtractState();
        playerInfo->lastCmd = client.lastAck;
        batch.PushPacket(std::move(playerInfo));

        batch.Write();
        
        SendPacket(id, batch);
    }
}

void Server::SendScoreboardReport()
{
    auto& scoreboard = match.GetGameMode()->GetScoreboard();
    if(!scoreboard.HasChanged())
        return;

    scoreboard.CommitChanges();
    logger.LogError("Sending scoreboard");

    Networking::Packets::Server::ScoreboardReport sr;
    sr.scoreboard = scoreboard;
    Broadcast(sr);
}

void Server::SendPlayerTakeDmg(ENet::PeerId peerId, Entity::Player::HealthState health, glm::vec3 dmgOrigin)
{
    Networking::Packets::Server::PlayerTakeDmg ptd;
    ptd.healthState = health;
    ptd.origin = dmgOrigin;

    SendPacket(peerId, ptd);
}

void Server::SendPlayerHitConfirm(ENet::PeerId peerId, Entity::ID victimId)
{
    Networking::Packets::Server::PlayerHitConfirm phc;
    phc.victimId = victimId;

    SendPacket(peerId, phc);
}

void Server::BroadcastPlayerDied(Entity::ID killerId, Entity::ID victimId, Util::Time::Seconds respawnTime)
{
    Networking::Packets::Server::PlayerDied pd;
    pd.killerId = killerId;
    pd.victimId = victimId;
    pd.respawnTime = respawnTime;

    Broadcast(pd);
}

void Server::BroadcastRespawn(ENet::PeerId peerId)
{
    auto& player = this->clients[peerId].player;

    Networking::Packets::Server::PlayerRespawn pr;
    pr.playerId = player.id;
    pr.playerState = player.ExtractState();
    for(auto i = 0 ; i < Entity::Player::MAX_WEAPONS; i++)
        pr.weapons[i] = player.weapons[i].weaponTypeId;

    Broadcast(pr);
}

void Server::BroadcastGameObjectState(glm::ivec3 pos)
{
    Networking::Packets::Server::GameObjectState gos;
    gos.goPos = pos;
    gos.state = gameObjectStates[pos].state;

    Broadcast(gos);
}

void Server::SendPlayerGameObjectInteraction(ENet::PeerId peerId, glm::ivec3 pos)
{
    Networking::Packets::Server::PlayerGameObjectInteract pgoi;
    pgoi.goPos = pos;
    
    SendPacket(peerId, pgoi);
}

void Server::SendPacket(ENet::PeerId peerId, Networking::Packet& packet)
{
    packet.Write();

    auto packetBuf = packet.GetBuffer();
    ENet::SentPacket epacket{packetBuf->GetData(), packetBuf->GetSize(), packet.GetFlags()};

    host->SendPacket(peerId, 0, epacket);
}

void Server::Broadcast(Networking::Packet& packet)
{
    packet.Write();

    auto packetBuf = packet.GetBuffer();
    ENet::SentPacket epacket{packetBuf->GetData(), packetBuf->GetSize(), packet.GetFlags()};

    host->Broadcast(0, epacket);
}

// Simulation

void Server::UpdateWorld()
{
    // Player respawns
    for(auto& [id, client] : clients)
    {
        auto& player = client.player;
        if(player.IsDead())
        {
            if(client.respawnTime < Util::Time::Seconds{0.0f})
            {
                SpawnPlayer(id);
                BroadcastRespawn(id);
            }
            else
                client.respawnTime -= TICK_RATE;
        }
    }

    // GameObject respawns
    for(auto& [goPos, goState] : gameObjectStates)
    {
        goState.respawnTimer.Update(TICK_RATE);
        if(goState.respawnTimer.IsDone())
        {
            goState.respawnTimer.Restart();
            goState.state.isActive = true;
            BroadcastGameObjectState(goPos);
        }
    }

    UpdateProjectiles();

    // Send broad events
    auto gameMode = match.GetGameMode();
    auto broadEvents = gameMode->PollEventMsgs(MsgType::BROADCAST);
    Networking::Batch<Networking::PacketType::Server> batch;
    for(auto event : broadEvents)
    {
        auto eventPacket = std::make_unique<Networking::Packets::Server::GameEvent>();
        eventPacket->event = event;
        batch.PushPacket(std::move(eventPacket));
    }
    Broadcast(batch);

    // Send targeted events
    for(auto& [id, client] : clients)
    {
        auto targetedEvents = gameMode->PollEventMsgs(MsgType::PLAYER_TARGET, id);
        if(!targetedEvents.empty())
        {
            logger.LogError("Sending targeted evenst");
            Networking::Batch<Networking::PacketType::Server> batch;
            for(auto event : targetedEvents)
            {
                auto eventPacket = std::make_unique<Networking::Packets::Server::GameEvent>();
                eventPacket->event = event;
                batch.PushPacket(std::move(eventPacket));
            }
            SendPacket(id, batch);
        }
    }
}

void Server::UpdateProjectiles()
{
     // Projectiles
    auto pIds = projectiles.GetIDs();
    std::vector<uint16_t> idsToRemove;
    for(auto id : pIds)
    {
        auto& projectile = projectiles.GetRef(id);
        projectile->Update(TICK_RATE);
        
        Math::Transform t{projectile->GetPos(), glm::vec3{0.0f}, projectile->GetScale()};
        auto collision = Game::AABBCollidesBlock(&map, t);
        if(collision.collides)
        {
            logger.LogError("Surface normal " + glm::to_string(collision.normal));
            logger.LogError("Offset " + glm::to_string(collision.offset));
            projectile->OnCollide(collision.normal);
            projectile->SetPos(t.position + collision.offset);
        }

        // Apply dmg to players
        if(projectile->HasDenotaded())
        {
            if(Util::Map::Contains(clients, projectile->GetAuthor()))
            {
                auto& author = clients[projectile->GetAuthor()].player;
                auto radius = projectile->GetRadius();
                for(auto& [id, client] : clients)
                {
                    auto& victim = client.player;

                    if(victim.IsDead())
                        continue;

                    auto victimPos = victim.GetTransform().position;
                    if(auto collision = Collisions::PointInSphere(victimPos, projectile->GetPos(), radius))
                    {
                        bool doDmg = victim.teamId != author.teamId || author.id == victim.id; // Different team or player
                        auto dmg = Entity::GetDistanceDmgMod(projectile->GetRadius() * 0.5f, collision.distance) * projectile->GetDmg();
                        victim.TakeDmg(dmg);
                        OnPlayerTakeDmg(author.id, victim.id);
                    }
                }
            }
            idsToRemove.push_back(id);
        }
    }
    for(auto id : idsToRemove)
        projectiles.Remove(id);
}

bool Server::IsPlayerOutOfBounds(ENet::PeerId peerId)
{
    bool outOfBounds = false;

    auto& player = clients[peerId].player;
    auto pos = player.GetRenderTransform().position;

    // Is in any of the chunks?
    auto blockScale = map.GetBlockScale();
    auto chunkPos = Game::Map::ToChunkIndex(pos, blockScale);
    if(!map.HasChunk(chunkPos))
        return true;

    // Is in a killbox area
    auto playerPos = Game::Map::ToRealPos(pos);
    auto killboxes = map.FindGameObjectByType(Entity::GameObject::KILLBOX);
    for(auto goPos : killboxes)
    {
        auto go = map.GetGameObject(goPos);
        Math::Transform box;

        box.position = Game::Map::ToRealPos(goPos, blockScale);
        float scaleX = std::get<float>(go->properties["Scale X"].value);
        float scaleY = std::get<float>(go->properties["Scale Y"].value);
        float scaleZ = std::get<float>(go->properties["Scale Z"].value);
        box.scale = glm::vec3{scaleX, scaleY, scaleZ} * blockScale;    
        box.position.y += ((box.scale.y) / 2.0f);

        if(Collisions::IsPointInAABB(playerPos, box))
            return true;
    }

    return false;
}

void Server::OnPlayerDeath(ENet::PeerId authorId, ENet::PeerId victimId)
{
    auto& victim = clients[victimId].player;
    victim.health.hp = 0.0f;
    
    auto gameMode = match.GetGameMode();
    clients[victimId].respawnTime = gameMode->GetRespawnTime();
    BroadcastPlayerDied(authorId, victimId, clients[victimId].respawnTime);
    gameMode->PlayerDeath(authorId, victimId, clients[authorId].player.teamId);
}


void Server::SleepUntilNextTick(Util::Time::SteadyPoint preSimulationTime)
{
    // Sleep until next tick
    auto preSleepTime = Util::Time::GetTime();
    auto elapsed = preSleepTime - preSimulationTime;
    auto wait = TICK_RATE - elapsed - lag;
    Util::Time::Sleep(wait);

    // Calculate sleep lag
    auto afterSleepTime = Util::Time::GetTime();
    Util::Time::Seconds simulationTime = afterSleepTime - preSimulationTime;

    // Calculate how far behind the server is
    lag = afterSleepTime - nextTickDate;
    //logger.LogInfo("Server tick took " + std::to_string(simulationTime.count()) + " s");
    //logger.LogInfo("Server delay " + std::to_string(lag.count()));

    // Update next tick date
    nextTickDate = nextTickDate + TICK_RATE;
}

// MMServer

void Server::SendServerNotification(ServerEvent::Notification notification)
{
    nlohmann::json body;
    body["game_id"] = mmServer.gameId;
    body["server_key"] = mmServer.serverKey;
    body["event"] = notification.ToJson();
    auto bodyStr = nlohmann::to_string(body);

    this->logger.LogInfo("Sending notification to mm server: " + bodyStr);

    auto onSuccess = [this](httplib::Response& res){
        this->logger.LogInfo("Notification sent succesfully!");
    };

    auto onError = [this, notification](httplib::Error err){
        this->logger.LogError("Erro while sending notification of type " + std::to_string(notification.eventType));
    };

    asyncClient.Request("/notify_server_event", bodyStr, onSuccess, onError);
}

// Match
const float Server::MIN_SPAWN_ENEMY_DISTANCE = 5.0f; // In Blocks

// This should get a random spawn point from a list of valid ones
// A valid spawn point is one which doesn't have an enemy in X distance
// Return the first one, if it 
glm::ivec3 Server::FindSpawnPoint(Entity::Player player)
{
    auto spawnPoints = map.GetRespawnIndices();

    std::vector<glm::ivec3> validSpawns;
    for(auto sPoint : spawnPoints)
    {
        if(IsSpawnValid(sPoint, player))
            validSpawns.push_back(sPoint);
    }

    glm::ivec3 spawn;
    if(!validSpawns.empty())
        spawn = *Util::Vector::PickRandom(validSpawns);
    else
        spawn = *Util::Vector::PickRandom(spawnPoints);

    return spawn;
}

glm::vec3 Server::ToSpawnPos(glm::ivec3 spawnPoint)
{
    auto blockCenter = Game::Map::ToRealPos(spawnPoint, map.GetBlockScale());
    auto mcb = Entity::Player::GetMoveCollisionBox();
    auto offsetY = (mcb.scale.y / 2.0f) - mcb.position.y - map.GetBlockScale() / 2.0f;
    auto pos = blockCenter + glm::vec3{0.0f, offsetY, 0.0f};

    return pos;
}

bool Server::IsSpawnValid(glm::ivec3 spawnPoint, Entity::Player player)
{
    auto spawn = map.GetRespawn(spawnPoint);
    if(spawn->teamId != player.teamId)
        return false;

    auto spawnPos = Game::Map::ToRealPos(spawnPoint, map.GetBlockScale());
    auto players = GetPlayers();
    for(auto other : players)
    {
        if(other.teamId != player.teamId)
        {
            auto posA = other.GetTransform().position;
            auto dist = glm::length(posA - spawnPos);

            if(dist < MIN_SPAWN_ENEMY_DISTANCE * map.GetBlockScale())
                return false;
        }
    }

    return true;
}

// Players

void Server::SpawnPlayer(ENet::PeerId clientId)
{
    auto& player = clients[clientId].player;

    // Set pos
    auto sIndex = FindSpawnPoint(player);
    auto spawn = map.GetRespawn(sIndex);
    auto playerPos = ToSpawnPos(sIndex);
    auto playerTransform = Math::Transform{playerPos, glm::vec3{0.0f, spawn->orientation, 0.0f}, glm::vec3{1.0f}};
    player.SetTransform(playerTransform);

    // Set weapon and health
    player.ResetWeapons();
    player.GetCurrentWeapon() = Entity::WeaponMgr::weaponTypes.at(Entity::WeaponTypeID::ASSAULT_RIFLE).CreateInstance();
    //player.weapons[1] = Entity::WeaponMgr::weaponTypes.at(Entity::WeaponTypeID::SNIPER).CreateInstance();
    player.ResetHealth();
    player.grenades = 2;
}

void Server::OnPlayerTakeDmg(ENet::PeerId authorId, ENet::PeerId victimId)
{
    auto& author = clients[authorId].player;
    auto& victim = clients[victimId].player;

    auto dmgOrigin = author.GetTransform().position;
    SendPlayerTakeDmg(victimId, victim.health, dmgOrigin);

    SendPlayerHitConfirm(authorId, victim.id);
    if(victim.IsDead())
    {   
        OnPlayerDeath(authorId, victimId);
    }
}

std::vector<Entity::Player> Server::GetPlayers() const
{
    std::vector<Entity::Player> players;
    players.reserve(clients.size());
    for(auto& [id, client] : clients)
        players.push_back(client.player);

    return players;
}


// World

World Server::GetWorld()
{
    World world;

    world.map = &map;
    for(auto& [peerId, client] : clients)
        world.players[peerId] = &client.player;

    world.logger = &logger;

    return world;
}