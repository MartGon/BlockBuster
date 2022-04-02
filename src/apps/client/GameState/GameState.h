#pragma once

#include <mglogger/MGLogger.h>

#include <app/App.h>

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

        inline bool IsStarted()
        {
            return isStarted;
        }

        virtual void ApplyVideoOptions(App::Configuration::WindowConfig& winConfig) {};

    protected:

        Log::Logger* GetLogger();

        bool isStarted = false;
        Client* client_;
    };
}