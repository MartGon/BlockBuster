#pragma once

#include <GameState/GameState.h>

#include <gl/VertexArray.h>

#include <util/Ring.h>

#include <httplib/httplib.h>
#include <nlohmann/json.hpp>

#include <thread>
#include <mutex>
#include <atomic>

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

        //#### Function Members ####\\
        // REST service
        enum class MMEndpoint
        {
            LOGIN
        };
        struct MMResponse
        {
            MMEndpoint endpoint;
            httplib::Result httpRes;
        };

        std::optional<MMResponse> PollRestResponse();
        void HandleRestResponses();
        void Login();
        void Request(std::string endpoint, nlohmann::json body);

        // Inputs
        void HandleSDLEvents();

        // Rendering
        void Render();
        void DrawGUI();

        //#### Data Members ####\\
        // GUI
        GL::VertexArray guiVao;

        // Rest Service
        std::mutex reqMutex;
        Util::Ring<MMResponse, 16> responses;

        // Login
        char inputUsername[16];
        std::thread reqThread;
        std::atomic_bool connecting = false;
        std::string userId;
        std::string user;
    };
}