
#include <Server.h>

#include <entity/Game.h>

#include <util/Container.h>

using namespace BlockBuster;
using namespace Networking::Packets::Client;

Server::Server(std::string address, uint16_t port, std::filesystem::path mapPath, uint8_t maxPlayers, uint8_t startingPlayers, std::string mode) :
    address{address}, port{port}, mapPath{mapPath}, maxPlayers{maxPlayers}, startingPlayers{startingPlayers}, mode{mode}
{

}

void Server::Start()
{
    InitLogger();
    InitNetworking();
    InitAI();
    InitMap();
    

    auto next = Util::Time::GetTime() + TICK_RATE;
    nextTickDate = next;
}

void Server::Run()
{
    // Server loop
    while(true)
    {
        host->PollAllEvents();

        auto preSimulationTime = Util::Time::GetTime();

        HandleClientsInput();
        SendWorldUpdate();

        tickCount++;
        logger.Flush();

        SleepUntilNextTick(preSimulationTime);
    }

    logger.LogInfo("Server shutdown");
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
    logger.SetVerbosity(Log::Verbosity::DEBUG);
}

void Server::InitNetworking()
{
    // Networking setup
    auto hostFactory = ENet::HostFactory::Get();
    auto localhost = ENet::Address::CreateByDomain(address, port).value();
    host = hostFactory->CreateHost(localhost, maxPlayers, 2);
    logger.LogInfo("Server initialized. Listening on address " + localhost.GetHostName() + ":" + std::to_string(localhost.GetPort()));
    host->SetOnConnectCallback([this](auto peerId)
    {
        logger.LogInfo("Connected with peer " + std::to_string(peerId));

        // Add player to table
        Entity::Player player;
        player.id = this->lastId++;
        player.teamId = player.id;
        auto sIndex = FindSpawnPoint(player);
        auto spawn = map.GetRespawn(sIndex);
        auto playerPos = ToSpawnPos(sIndex);
        player.weapon = Entity::WeaponMgr::weaponTypes.at(Entity::WeaponTypeID::SNIPER).CreateInstance();
        logger.LogInfo("SpawnPos " + glm::to_string(playerPos));
        auto playerTransform = Math::Transform{playerPos, glm::vec3{0.0f, spawn->orientation, 0.0f}, glm::vec3{1.0f}};
        player.SetTransform(playerTransform);
        clients[peerId].player = player;

        // Inform client
        // TODO: Change for new packet system
        Networking::Command::Server::ClientConfig clientConfig;
        clientConfig.playerId = player.id;
        clientConfig.sampleRate = TICK_RATE.count();

        Networking::Command::Header header;
        header.tick = tickCount;
        header.type = Networking::Command::Type::CLIENT_CONFIG;

        Networking::Command::Payload data;
        data.config = clientConfig;

        Networking::Command packet{header, data};

        ENet::SentPacket sentPacket{&packet, sizeof(packet), ENET_PACKET_FLAG_RELIABLE};
        host->SendPacket(peerId, 0, sentPacket);
    });
    host->SetOnRecvCallback([this](auto peerId, auto channelId, ENet::RecvPacket recvPacket)
    {   
        auto& client = clients[peerId];

        Util::Buffer::Reader reader{recvPacket.GetData(), recvPacket.GetSize()};
        Util::Buffer buffer = reader.ReadAll();

        auto packet = Networking::MakePacket<Networking::PacketType::Client>(std::move(buffer));
        packet->Read();
        OnRecvPacket(peerId, *packet);
    });
    host->SetOnDisconnectCallback([this](auto peerId)
    {
        logger.LogInfo("Peer with id " + std::to_string(peerId) + " disconnected.");
        auto player = clients[peerId].player;
        clients.erase(peerId);

        // Informing players
        Networking::Command::Server::PlayerDisconnected playerDisconnect;
        playerDisconnect.playerId = player.id;

        Networking::Command::Header header;
        header.tick = tickCount;
        header.type = Networking::Command::Type::PLAYER_DISCONNECTED;

        Networking::Command::Payload payload;
        payload.playerDisconnect = playerDisconnect;

        Networking::Command command{header, payload};

        ENet::SentPacket sentPacket{&command, sizeof(command), ENetPacketFlag::ENET_PACKET_FLAG_RELIABLE};
        host->Broadcast(0, sentPacket);
    });
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
    auto res = Game::Map::Map::LoadFromFile(mapPath);
    if(res.isOk())
    {
        auto mapPtr = std::move(res.unwrap());
        map = std::move(*mapPtr.get());
    }
    else
    {
        logger.LogError("Could not load map " + mapPath.string());
        std::exit(-1);
    }
}

// Networking

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
                if(auto command = client.inputBuffer.PopFront())
                {
                    auto cmdId = command->reqId;
                    client.lastAck = cmdId;

                    logger.LogInfo("Cmd id: " + std::to_string(client.lastAck) + " from " + std::to_string(peerId));
                    HandleClientInput(peerId, command.value());

                    logger.LogInfo("Ring size " + std::to_string(client.inputBuffer.GetSize()));
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

void Server::OnRecvPacket(ENet::PeerId peerId, uint8_t channelId, ENet::RecvPacket recvPacket)
{
    auto& client = clients[peerId];

    Util::Buffer::Reader reader{recvPacket.GetData(), recvPacket.GetSize()};
    Util::Buffer buffer = reader.ReadAll();
    
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
        case Networking::OpcodeClient::OPCODE_CLIENT_INPUT:
        {
            auto inputPacket = packet.To<Networking::Packets::Client::Input>();
            auto inputReq = inputPacket->req;

            auto cmdId = inputReq.reqId;
            logger.LogInfo("Command arrived with cmdid " + std::to_string(cmdId));

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
                    logger.LogInfo("Max Buffer size reached." + std::to_string(bufferSize));

                if(client.state == BufferingState::REFILLING && client.inputBuffer.GetSize() > MIN_INPUT_BUFFER_SIZE)
                    client.state = BufferingState::CONSUMING;
            }
        }
        break;
    default:
        break;
    }
}

void Server::HandleClientInput(ENet::PeerId peerId, Input::Req cmd)
{
    auto& client = clients[peerId];
    auto& player = client.player;

    auto playerTransform = player.GetTransform();
    auto playerPos = playerTransform.position;
    auto playerYaw = cmd.camYaw;

    auto& pController = client.pController;
    playerTransform.position = pController.UpdatePosition(playerPos, playerYaw, cmd.playerInput, &map, TICK_RATE);
    playerTransform.rotation = glm::vec3{cmd.camPitch, playerYaw, 0.0f};
    player.SetTransform(playerTransform);

    auto oldWepState = player.weapon;
    player.weapon = pController.UpdateWeapon(player.weapon, cmd.playerInput, TICK_RATE);
    logger.LogInfo("Player Ammo " + std::to_string(player.weapon.ammoState.magazine));

    if(Entity::HasShot(oldWepState, player.weapon))
    {
        ShotCommand sc{player.id, player.GetFPSCamPos(), glm::radians(playerTransform.rotation), cmd.fov, cmd.aspectRatio, cmd.renderTime};
        HandleShootCommand(sc);
    }

    logger.LogInfo("MovePos " + glm::to_string(playerTransform.position));
}

void Server::HandleShootCommand(ShotCommand sc)
{
    auto commandTime = sc.commandTime;
    logger.LogInfo("Command time is " + std::to_string(commandTime.count()));

    // Find first snapshot before commandTime
    auto s1p = history.FindRevFirstPair([commandTime, this](auto i, Networking::Snapshot s)
    {
        Util::Time::Seconds sT = s.serverTick * TICK_RATE;
        return sT < commandTime;
    });

    if(s1p.has_value())
    {
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
        logger.LogInfo("Handling player shot from " + glm::to_string(ray.origin) + " to " + glm::to_string(ray.GetDir()));

        // Check collision with block
        auto bCol = Game::CastRayFirst(&map, ray, map.GetBlockScale());
        auto bColDist = std::numeric_limits<float>::max();
        if(bCol.intersection.intersects)
            bColDist = bCol.intersection.GetRayLength(ray);

        // Check collision with players. This allows collaterals
        for(auto& [id, client] : clients)
        {
            if(id == sc.playerId)
                continue;

            auto playerId = client.player.id;
            bool s1HasData = s1.players.find(playerId) != s1.players.end();
            bool s2HasData = s2.players.find(playerId) != s2.players.end();

            if(!s1HasData || s2HasData)
                continue;

            // Calculate player pos that client views
            auto ps1 = s1.players.at(playerId);
            auto ps2 = s2.players.at(playerId);
            auto smoothState = Entity::Interpolate(ps1, ps2, alpha);

            auto lastMoveDir = Entity::GetLastMoveDir(ps1, ps2);
            auto rpc = Game::RayCollidesWithPlayer(ray, smoothState.pos, smoothState.rot.y, lastMoveDir);
            if(rpc.collides)
            {
                auto colDist = rpc.intersection.GetRayLength(ray);
                if(colDist < bColDist)
                {
                    logger.LogInfo("Shot from player " + std::to_string(sc.playerId) + " has hit player " + std::to_string(id));
                    client.player.onDmg = true;
                }
                else
                    logger.LogInfo("Shot from player " + std::to_string(sc.playerId) + " has hit block " + glm::to_string(bCol.pos));
            }
            else
                logger.LogInfo("Shot from player " + std::to_string(sc.playerId) + " has NOT hit player " + std::to_string(id));
            logger.LogInfo("Player was at " + glm::to_string(smoothState.pos));
        }
    }
}

void Server::SendWorldUpdate()
{
    // Create snapshot
    auto worldUpdate = std::make_unique<Networking::Packets::Server::WorldUpdate>();

    Networking::Snapshot s;
    s.serverTick = tickCount;
    for(auto& [id, client] : clients)
    {
        auto& player = client.player;
        s.players[player.id] = player.ExtractState();

        client.player.onDmg = false;
    }
    history.PushBack(s);
    worldUpdate->snapShot = s;

    // Send snapshot with acks
    for(auto& [id, client] : clients)
    {
        if(client.isAI)
            continue;
        
        worldUpdate->lastCmd = client.lastAck;
        worldUpdate->Write();
        
        auto packetBuf = worldUpdate->GetBuffer();
        ENet::SentPacket epacket{packetBuf->GetData(), packetBuf->GetSize(), 0};
        
        host->SendPacket(id, 0, epacket);
    }
}

// Misc

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

// Match

const float Server::MIN_SPAWN_ENEMY_DISTANCE = 5.0f;

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

bool Server::IsSpawnValid(glm::ivec3 spawnPoint, Entity::Player player) const
{
    auto spawnPos = Game::Map::ToRealPos(spawnPoint, map.GetBlockScale());

    auto players = GetPlayers();
    for(auto other : players)
    {
        if(other.teamId != player.teamId)
        {
            auto posA = other.GetTransform().position;
            auto dist = glm::length(posA - spawnPos);

            if(dist < MIN_SPAWN_ENEMY_DISTANCE)
                return false;
        }
    }

    return true;
}

std::vector<Entity::Player> Server::GetPlayers() const
{
    std::vector<Entity::Player> players;
    players.reserve(clients.size());
    for(auto [id, client] : clients)
        players.push_back(client.player);

    return players;
}
