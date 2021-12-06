#pragma once

#include <GameState/GameState.h>

namespace BlockBuster
{
    class MainMenu : public GameState
    {
    public:
        MainMenu(Client* client);
        
        void Update() override;

    private:
    };
}