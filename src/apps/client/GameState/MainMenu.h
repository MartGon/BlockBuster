#pragma once

#include <GameState/GameState.h>

#include <gl/VertexArray.h>

#include <thread>
#include <mutex>

namespace BlockBuster
{
    class Client;

    class MainMenu : public GameState
    {
    public:
        MainMenu(Client* client);
        ~MainMenu();
        
        void Start() override;
        void Update() override;

    private:

        // REST service
        void Login();

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
        std::thread reqThread;
        std::mutex mutex;
        bool connecting = false;
    };
}