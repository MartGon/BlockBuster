#include <Server.h>

#include <argparse/argparse.hpp>

#include <Logger.h>

int main(int argc, char** argv)
{   
    argparse::ArgumentParser ap{"BlockBusterServer", "0.1a"};
    ap.add_argument("-a", "--address").default_value<std::string>("0.0.0.0");
    ap.add_argument("-p", "--port").default_value<uint16_t>(8080).scan<'u', uint16_t>();
    ap.add_argument("-m", "--map").required();
    ap.add_argument("-mp", "--max-players").default_value<uint8_t>(8).scan<'u', uint8_t>();
    ap.add_argument("-sp", "--starting-players").required().scan<'u', uint8_t>();
    ap.add_argument("-gm", "--gamemode").required();
    
    ap.add_argument("-mma", "--match-making-address").default_value<std::string>("127.0.0.1");
    ap.add_argument("-mmp", "--match-making-port").default_value<uint16_t>(3030).scan<'u', uint16_t>();
    ap.add_argument("-mmid", "--match-making-id");
    ap.add_argument("-mmk", "--match-making-key");

    ap.add_argument("-v", "--verbosity").default_value<Log::Verbosity>(Log::Verbosity::VERBOSITY_ERROR).scan<'u', uint8_t>();

    try {
        ap.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << ap;
        std::exit(1);
    }

    auto address = ap.get<std::string>("--address");
    auto port = ap.get<uint16_t>("--port");
    auto map = ap.get<std::string>("--map");
    auto maxPlayers = ap.get<uint8_t>("--max-players");
    auto startingPlayers = ap.get<uint8_t>("--starting-players");
    auto gameMode = ap.get<std::string>("--gamemode");
    auto verbosity = static_cast<Log::Verbosity>(ap.get<uint8_t>("--verbosity"));

    auto mmAddress = ap.get<std::string>("--match-making-address");
    auto mmPort = ap.get<uint16_t>("--match-making-port");

    std::string mmGameId;
    if(ap.is_used("--match-making-id"))
        mmGameId = ap.get<std::string>("--match-making-id");
    std::string mmKey;
    if(ap.is_used("--match-making-key"))
        mmKey = ap.get<std::string>("--match-making-key");

    std::cout << "Server starting\n";
    std::cout << "Address: " << address << '\n';
    std::cout << "Port: " << port << '\n';
    std::cout << "Map: " << map << '\n';
    std::cout << "Max Players: " << std::to_string(maxPlayers) << '\n';
    std::cout << "Starting Players: " << std::to_string(startingPlayers) << '\n';
    std::cout << "Game mode: " << gameMode << '\n';

    std::cout << "Match Making address: " << mmAddress << '\n';
    std::cout << "Match Making port: " << mmPort << '\n';
    std::cout << "Match Making game id: " << mmGameId << '\n';
    std::cout << "Match Making game key: " << mmKey << '\n';

    using namespace BlockBuster;

    Server::Params params{address, port, maxPlayers, startingPlayers, map, gameMode, verbosity};
    Server::MMServer mmServer{mmAddress, mmPort, mmGameId, mmKey};

    Server server{params, mmServer};
    server.Start();
    server.Run();

    return 0;
}