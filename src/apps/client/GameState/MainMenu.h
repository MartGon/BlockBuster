#pragma once

#include <GameState/GameState.h>

#include <gl/VertexArray.h>
#include <gui/PopUp.h>
#include <util/Ring.h>

#include <httplib/httplib.h>
#include <nlohmann/json.hpp>

#include <thread>
#include <mutex>
#include <atomic>
#include <functional>

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
        struct MMResponse
        {
            httplib::Result httpRes;
            std::function<void(httplib::Response&)> onSuccess;
            std::function<void(httplib::Error)> onError;
        };

        std::optional<MMResponse> PollRestResponse();
        void HandleRestResponses();
        void Login();
        void Request(std::string endpoint, nlohmann::json body, std::function<void(httplib::Response&)> onSuccess, std::function<void(httplib::Error)> onError);

        // Inputs
        void HandleSDLEvents();

        // Rendering
        void Render();
        void DrawGUI();

        // GUI
        void LoginWindow();
        void ServerBrowserWindow();

        //#### Data Members ####\\
        // GUI
        GL::VertexArray guiVao;
        GUI::PopUp popUp;

        // Rest Service
        std::mutex reqMutex;
        std::thread reqThread;
        std::atomic_bool connecting = false;
        Util::Ring<MMResponse, 16> responses;

        // Login
        char inputUsername[16];
        std::string userId;
        std::string user;
    };
}