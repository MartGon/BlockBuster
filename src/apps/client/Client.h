#pragma once

#include <app/App.h>

#include <gl/Shader.h>
#include <gl/VertexArray.h>
#include <gl/Texture.h>

#include <rendering/Camera.h>
#include <rendering/Mesh.h>
#include <rendering/Primitive.h>
#include <rendering/Rendering.h>

#include <game/PlayerController.h>
#include <game/Map.h>
#include <game/CameraController.h>
#include <game/ChunkMeshMgr.h>

#include <util/BBTime.h>
#include <util/CircularVector.h>

#include <entity/Player.h>

#include <networking/enetw/ENetW.h>
#include <networking/Command.h>
#include <networking/Snapshot.h>

#include <GameState/GameState.h>

namespace BlockBuster
{
    class Client : public App::App
    {
    friend class GameState;
    friend class MainMenu;
    friend class InGame;
    public:
        Client(::App::Configuration config);

        void Start() override;
        void Update() override;
        bool Quit() override;
        
    private:
        std::unique_ptr<GameState> state;

        // App
        bool quit = false;
    };
}