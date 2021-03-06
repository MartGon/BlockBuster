#pragma once

#include <string>
#include <functional>
#include <thread>
#include <mutex>

#include <httplib/httplib.h>

#include <util/Ring.h>
#include <util/BBTime.h>

namespace HTTP
{
    class AsyncClient
    {
    public:
        using RespHandler = std::function<void(httplib::Response&)>;
        using ErrHandler = std::function<void(httplib::Error)>;

        AsyncClient(std::string address, uint16_t port) : address{address}, port{port}
        {

        }
        AsyncClient() = default;
        ~AsyncClient()
        {
            if(waitThreads)
            {
                for(auto& [id, thread] : reqThreads)
                    if(thread.joinable())
                        thread.join();
            }
        }

        inline void SetReadTimeout(Util::Time::Seconds seconds)
        {
            timeout = seconds;
        }

        inline void Disable()
        {
            enabled = false;
        }

        inline void Enable()
        {
            enabled = true;
        }

        inline void Close()
        {
            waitThreads = false;
        }

        inline void SetAddress(std::string address, uint16_t port)
        {
            this->address = address;
            this->port = port;
        }

        inline std::pair<std::string, uint16_t> GetAddress()
        {
            return {address, port};
        }

        inline bool IsConnecting()
        {
            return connecting.load();
        }

        void Request(const std::string& path, const std::string& body, RespHandler respHandler, ErrHandler errHandler);
        void HandleResponses();

    private:

        struct Response
        {
            httplib::Result httpRes;
            std::function<void(httplib::Response&)> onSuccess;
            std::function<void(httplib::Error)> onError;
        };

        std::optional<Response> PollResponse();

        std::string address;
        uint16_t port;
        Util::Time::Seconds timeout{7};

        std::mutex reqMutex;
        using ThreadID = uint8_t;
        std::unordered_map<ThreadID, std::thread> reqThreads;
        std::vector<ThreadID> joinThreads;
        ThreadID lastId = 0;
        std::atomic_bool connecting = false;
        Util::Ring<Response, 16> responses;
        bool enabled = true;
        bool waitThreads = true;
    };
}