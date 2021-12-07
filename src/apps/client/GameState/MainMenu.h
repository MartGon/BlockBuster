#pragma once

#include <GameState/GameState.h>

#include <gl/VertexArray.h>

namespace BlockBuster
{
    class Client;

    class MainMenu : public GameState
    {
    public:
        MainMenu(Client* client);
        
        void Start() override;
        void Update() override;

    private:

        // Inputs
        void HandleSDLEvents();

        // Rendering
        void Render();
        void DrawGUI();

        //#### Members ####\\
        // GUI
        GL::VertexArray guiVao;

        // Login
        char username[16];
    };
}