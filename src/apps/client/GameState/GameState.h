#pragma once

#include <mglogger/MGLogger.h>

namespace BlockBuster
{
    class Client;

    class GameState
    {
    public:
        GameState(Client* client) : client_{client} {};
        virtual ~GameState() {};

        virtual void Start(){};
        virtual void Update() = 0;
        virtual void Shutdown(){};

    protected:

        Log::Logger* GetLogger();

        Client* client_;
    };
}