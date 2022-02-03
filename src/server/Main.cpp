#include <Server.h>

int main(int argc, char** args)
{

    // TODO: Read listening port/address from command line

    BlockBuster::Server server;

    server.Start();
    server.Run();

    return 0;
}