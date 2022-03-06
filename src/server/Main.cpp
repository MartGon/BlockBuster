#include <Server.h>

#include <argparse/argparse.hpp>

int main(int argc, char** argv)
{   
    argparse::ArgumentParser ap{"BlockBusterServer", "0.1a"};
    ap.add_argument("-a", "--address").default_value<std::string>("0.0.0.0");
    ap.add_argument("-p", "--port").default_value<uint16_t>(8080);
    ap.add_argument("-m", "--map").required();
    ap.add_argument("-mp", "--max-players").default_value<uint8_t>(8);
    ap.add_argument("-sp", "--starting-players").required().scan<'u', uint8_t>();
    ap.add_argument("-gm", "--gamemode").required();

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

    std::cout << "Server starting\n";
    std::cout << "Address: " << address << '\n';
    std::cout << "Port: " << port << '\n';
    std::cout << "Map: " << map << '\n';
    std::cout << "Max Players: " << std::to_string(maxPlayers) << '\n';
    std::cout << "Starting Players: " << std::to_string(startingPlayers) << '\n';
    std::cout << "Game mode: " << gameMode << '\n';

    BlockBuster::Server server{address, port, map, maxPlayers, startingPlayers, gameMode};
    server.Start();
    server.Run();

    return 0;
}