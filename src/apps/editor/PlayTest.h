#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace BlockBuster::Editor
{
    // Runs scripts/test_game.sh in the background: a local server with bots plus a client, on the current map
    class PlayTest
    {
    public:
        struct Params
        {
            std::string map; // Folder name inside the repo's maps folder
            std::string mode;
            int bots = 3;
            std::string difficulty = "normal";
        };

        enum class State
        {
            IDLE,
            RUNNING,
            FAILED
        };

        ~PlayTest();

        // Returns an error message if it couldn't be started
        std::optional<std::string> Launch(Params params, std::filesystem::path script, std::filesystem::path mapsFolder);
        void Stop();

        // Call every frame. Detects when the script exits
        void Poll();

        inline State GetState() const
        {
            return state;
        }

        inline const std::string& GetError() const
        {
            return error;
        }

        static bool IsSupported();

    private:
        std::string ReadLastLogLine();

        State state = State::IDLE;
        std::string error;
        std::filesystem::path logFile;
        int pid = -1;
    };
}
