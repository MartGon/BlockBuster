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
        ~AsyncClient()
        {
            if(reqThread.joinable())
                reqThread.join();
        }

        inline void SetReadTimeout(Util::Time::Seconds seconds)
        {
            timeout = seconds;
        }

        void Request(const std::string& path, const std::string& body, RespHandler respHandler, ErrHandler errHandler);
        void HandleResponses();

        inline bool IsConnecting()
        {
            return connecting.load();
        }

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
        Util::Time::Seconds timeout{5};

        std::mutex reqMutex;
        std::thread reqThread;
        std::atomic_bool connecting = false;
        Util::Ring<Response, 16> responses;
    };
}