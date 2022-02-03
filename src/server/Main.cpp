#include <Server.h>

int main(int argc, char** args)
{
    BlockBuster::Server server;

    server.Start();
    server.Run();

    return 0;
}