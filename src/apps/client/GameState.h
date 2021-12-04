#pragma once

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
        Client* client_;
    };

    class MainMenu : public GameState
    {
    public:
        MainMenu(Client* client);
        
        void Update() override;

    private:
    };
}