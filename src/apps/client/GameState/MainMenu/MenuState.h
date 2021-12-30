#pragma once

namespace BlockBuster
{
    class MainMenu;

    namespace MenuState
    {
        class Base
        {
        public:
            Base(MainMenu* mainMenu);
            virtual ~Base() {};

            virtual void Update() = 0;
            virtual void OnEnter() {};
            virtual void OnExit() {};

        protected:
            MainMenu* mainMenu_;
        };

        class Login : public Base
        {
        public:
            Login(MainMenu* mainMenu) : Base(mainMenu) {}

            void Update() override;
        private:
            char inputUsername[16] = "\0";
        };

        class ServerBrowser : public Base
        {
        public:
            ServerBrowser(MainMenu* mainMenu) : Base(mainMenu){}

            void OnEnter() override;
            void Update() override;
        };

        class CreateGame : public Base
        {
        public:
            CreateGame(MainMenu* mainMenu) : Base(mainMenu){}

            void Update() override;
        private:
            char gameName[32] = "\0";
            char map[64] = "\0";
            char mode[64] = "\0";
            int maxPlayers = 2;
        };

        class Lobby : public Base
        {
        public:
            Lobby(MainMenu* mainMenu) : Base(mainMenu){}

            void OnEnter() override;
            void OnExit() override;
            void Update() override;
            
            void OnGameInfoUpdate();

            bool updatePending = false;
        private:
            
            char chat[4096] = "\0";
            char chatLine[128] = "\0";
        };
    }
}