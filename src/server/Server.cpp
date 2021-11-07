
#include <mglogger/MGLogger.h>

#include <networking/enetw/ENetW.h>
#include <networking/Command.h>
#include <networking/CommandBuffer.h>
#include <networking/Snapshot.h>

#include <util/BBTime.h>
#include <util/Random.h>

#include <collisions/Collisions.h>

#include <math/Interpolation.h>

#include <entity/Player.h>

#include <vector>
#include <map>

#include <glm/gtx/string_cast.hpp>

const uint32_t MIN_INPUT_BUFFER_SIZE = 2;
const uint32_t MAX_INPUT_BUFFER_SIZE = 5;

enum class BufferingState
{
    REFILLING,
    CONSUMING
};

struct ShotCommand
{
    Networking::Command::User::PlayerShot playerShot;
    Util::Time::Seconds commandTime;
};

struct Client
{
    Entity::Player player;
    Util::Ring<Networking::Command::User::PlayerMovement, MAX_INPUT_BUFFER_SIZE> inputBuffer;
    Util::Ring<ShotCommand, MAX_INPUT_BUFFER_SIZE> shotBuffer;
    uint32_t lastAck = 0;
    BufferingState state = BufferingState::REFILLING;

    bool isAI = false;
    glm::vec3 targetPos;
};

std::unordered_map<ENet::PeerId, Client> clients;
Entity::ID lastId = 0;
const Util::Time::Seconds TICK_RATE{0.050};
const float PLAYER_SPEED = 5.f;
unsigned int tickCount = 0;
Util::Ring<Networking::Snapshot, 30> history; 

glm::vec3 GetRandomPos()
{
    glm::vec3 pos;
    pos.x = Util::Random::Uniform(-7.0f, 7.0f);
    pos.y = 4.15f;
    //pos.z = Util::Random::Uniform(-7.0f, 7.0f);
    pos.z = -7.0f;

    return pos;
}

void HandleMoveCommand(ENet::PeerId peerId, Networking::Command::User::PlayerMovement pm)
{
    auto& player = clients[peerId].player;
    auto len = glm::length(pm.moveDir);
    if(len > 0)
    {
        auto moveDir = glm::normalize(pm.moveDir);
        auto velocity = pm.moveDir * PLAYER_SPEED * (float)TICK_RATE.count();
        player.transform.position += velocity;
    }
}

int main()
{
    // Loggers
    auto clogger = std::make_unique<Log::ConsoleLogger>();
    auto flogger = std::make_unique<Log::FileLogger>();
    std::filesystem::path logFile = "server.log";
    flogger->OpenLogFile("server.log");
    if(!flogger->IsOk())
        clogger->LogError("Could not create log file " + logFile.string());

    Log::ComposedLogger logger;
    logger.AddLogger(std::move(clogger));
    logger.AddLogger(std::move(flogger));
    logger.SetVerbosity(Log::Verbosity::DEBUG);

    // Networking setup
    auto hostFactory = ENet::HostFactory::Get();
    auto localhost = ENet::Address::CreateByIPAddress("127.0.0.1", 8081).value();
    auto host = hostFactory->CreateHost(localhost, 4, 2);
    logger.LogInfo("Server initialized. Listening on address " + localhost.GetHostName() + ":" + std::to_string(localhost.GetPort()));
    host.SetOnConnectCallback([&logger, &host](auto peerId)
    {
        logger.LogInfo("Connected with peer " + std::to_string(peerId));

        // Add player to table
        Entity::Player player;
        player.id = lastId++;
        player.transform = Math::Transform{GetRandomPos(), glm::vec3{0.0f}, glm::vec3{2.f, 4.0f, 2.f}};
        clients[peerId].player = player;

        // Inform client
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
        host.SendPacket(peerId, 0, sentPacket);
    });
    host.SetOnRecvCallback([&logger, &host](auto peerId, auto channelId, ENet::RecvPacket recvPacket)
    {   
        auto& client = clients[peerId];

        Util::Buffer::Reader reader{recvPacket.GetData(), recvPacket.GetSize()};
        auto command = reader.Read<Networking::Command>();

        if(command.header.type == Networking::Command::PLAYER_MOVEMENT_BATCH)
        {
            auto inputs = command.data.playerMovementBatch.amount;
            for(int i = 0; i < inputs; i++)
            {
                auto move = reader.Read<Networking::Command::User::PlayerMovement>();
                auto cmdId = move.reqId;
                logger.LogInfo("Command arrived with cmdid " + std::to_string(cmdId));

                // Check if we already have this input
                auto found = client.inputBuffer.FindFirst([cmdId](auto input)
                {
                    return input.reqId == cmdId;
                });

                if(!found.has_value())
                {
                    if(cmdId > client.lastAck)
                        client.inputBuffer.PushBack(move);
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
        }
        else if(command.header.type == Networking::Command::PLAYER_SHOT)
        {
            auto playerShot = command.data.playerShot;

            // Calculate command time
            auto peerInfo = host.GetPeerInfo(peerId);
            auto rtt = Util::Time::Millis(peerInfo.roundTripTimeMs);
            
            Util::Time::Seconds now = tickCount * TICK_RATE;
            Util::Time::Seconds lerp = TICK_RATE * 2.0;
            Util::Time::Seconds commandTime = now - (rtt / 2.0) - lerp;
            commandTime = Util::Time::Seconds(playerShot.clientTime);

            ShotCommand sc{playerShot, commandTime};
            client.shotBuffer.PushBack(sc);
        }
    });
    host.SetOnDisconnectCallback([&logger, &host](auto peerId)
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
        host.Broadcast(0, sentPacket);
    });

    // Create AI players
    Client ai;
    ai.isAI = true;
    Entity::Player player;
    ai.player.id = 200;
    ai.player.transform = Math::Transform{GetRandomPos(), glm::vec3{0.0f}, glm::vec3{2.f, 4.0f, 2.f}};
    ai.targetPos = GetRandomPos();
    clients[200] = ai;

    // Server loop
    Util::Time::Seconds deltaTime{0};
    Util::Time::Seconds lag{0};
    auto nextTickDate = Util::Time::GetTime() + TICK_RATE;
    while(true)
    {
        host.PollAllEvents();

        auto preSimulationTime = Util::Time::GetTime();

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
                        HandleMoveCommand(peerId, command.value());

                        logger.LogInfo("Ring size " + std::to_string(client.inputBuffer.GetSize()));
                    }
                    else
                    {
                        logger.LogWarning("No cmd handled for player " + std::to_string(peerId) + " waiting for buffer to refill ");
                        client.state = BufferingState::REFILLING;
                    }

                    // Consume shoot commands
                    if(auto sc = client.shotBuffer.PopFront())
                    {
                        auto commandTime = sc->commandTime;
                        //commandTime = Util::Time::Seconds(sc->playerShot.clientTime);
                        logger.LogInfo("Command time is " + std::to_string(commandTime.count()));

                        // Find first snapshot before commandTime
                        auto s1p = history.FindRevFirstPair([commandTime](auto i, Networking::Snapshot s)
                        {
                            Util::Time::Seconds sT = s.serverTick * TICK_RATE;
                            return sT < commandTime;
                        });

                        if(s1p.has_value())
                        {
                            auto s2o = history.At(s1p->first + 1);
                            if(s2o.has_value())
                            {
                                // Samples to use for interpolation
                                // S1 last sample before commandTime
                                // S2 first sample after commandTime
                                auto s1 = s1p->second;
                                auto s2 = s2o.value();

                                // Find weights
                                Util::Time::Seconds t1 = s1.serverTick * TICK_RATE;
                                Util::Time::Seconds t2 = s2.serverTick * TICK_RATE;
                                auto ws = Math::Interpolation::GetWeights(t1.count(), t2.count(), commandTime.count());
                                auto alpha = ws.x;

                                // Perform interpolation and shot
                                auto shot = &sc->playerShot;
                                logger.LogInfo("Handling player shot from " + glm::to_string(shot->origin) + " to " + glm::to_string(shot->dir));
                                Collisions::Ray ray{shot->origin, shot->dir};
                                for(auto& [id, client] : clients)
                                {
                                    auto playerId = client.player.id;
                                    bool s1HasData = s1.players.find(playerId) != s1.players.end();
                                    bool s2HasData = s2.players.find(playerId) != s2.players.end();

                                    auto pos1 = s1.players.at(playerId).pos;
                                    auto pos2 = s2.players.at(playerId).pos;
                                    auto smoothPos = pos1 * alpha + pos2 * (1 - alpha);

                                    auto rewoundTrans = client.player.transform;
                                    rewoundTrans.position = smoothPos;
                                    auto collision = Collisions::RayAABBIntersection(ray, rewoundTrans.GetTransformMat());
                                    if(collision.intersects)
                                    {
                                        client.player.onDmg = true;
                                        logger.LogInfo("Shot from player " + std::to_string(id) + " has hit player " + std::to_string(client.player.id));
                                        logger.LogInfo("Player was at " + glm::to_string(smoothPos));
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                auto origin = client.player.transform.position;
                auto dest = client.targetPos;
                auto move = dest - origin;
                auto dist = glm::length(move);

                auto stepDist = PLAYER_SPEED * (float)TICK_RATE.count();
                if(dist > stepDist)
                {
                    auto dir = glm::normalize(move);
                    Networking::Command::User::PlayerMovement pm{0, dir};
                    HandleMoveCommand(peerId, pm);
                }
                else
                    client.targetPos = GetRandomPos();
            }
        }

        // Create snapshot
        Networking::Snapshot s;
        s.serverTick = tickCount;
        for(auto& [id, client] : clients)
        {
            auto& player = client.player;
            s.players[player.id].pos = player.transform.position;
            s.players[player.id].onDmg = player.onDmg;
            client.player.onDmg = false;
        }
        history.PushBack(s);

        auto snapshotBuffer = s.ToBuffer();

        // Send snapshot with acks
        for(auto pair : clients)
        {
            auto client = pair.second;
            Util::Buffer buffer;

            Networking::Command::Header header;
            header.type = Networking::Command::Type::SERVER_SNAPSHOT;
            header.tick = tickCount;

            Networking::Command::Server::Update update;
            update.lastCmd = client.lastAck;
            update.snapshotDataSize = snapshotBuffer.GetSize();

            Networking::Command::Payload snapshotData;
            snapshotData.snapshot = update;

            Networking::Command cmd{header, snapshotData};

            Util::Buffer packetBuf;
            packetBuf.Write(cmd);
            
            packetBuf = Util::Buffer::Concat(std::move(packetBuf), snapshotBuffer.Clone());

            ENet::SentPacket epacket{packetBuf.GetData(), packetBuf.GetSize(), 0};
            if(!client.isAI)
                host.SendPacket(pair.first, 0, epacket);
        }

        // Increase tick
        tickCount++;

        // Flush logs
        logger.Flush();

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
        logger.LogInfo("Server tick took " + std::to_string(simulationTime.count()) + " s");
        logger.LogInfo("Server delay " + std::to_string(lag.count()));

        // Update next tick date
        nextTickDate += TICK_RATE;
    }

    logger.LogInfo("Server shutdown");

    return 0;
}