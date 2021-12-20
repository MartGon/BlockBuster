#include <AsyncClient.h>

#include <util/BBTime.h>

using namespace HTTP;

void AsyncClient::Request(const std::string& path, const std::string& body, RespHandler respHandler, ErrHandler errHandler)
{
    connecting = true;

    if(reqThread.joinable())
        reqThread.join();

    reqThread = std::thread{
        [this, body, path, respHandler, errHandler](){

            auto res = client.Post(path.c_str(), body.c_str(), body.size(), "application/json");
            auto response = Response{std::move(res), respHandler, errHandler};
            Util::Time::Sleep(Util::Time::Seconds{1});  

            this->reqMutex.lock();
            this->responses.PushBack(std::move(response));
            this->reqMutex.unlock();

            this->connecting = false;
        }
    };
}

void AsyncClient::HandleResponses()
{
    while(auto response = PollResponse())
    {
        auto res = std::move(response.value());
        auto httpRes = std::move(res.httpRes);
        if(httpRes)
            res.onSuccess(httpRes.value());
        else
            res.onError(httpRes.error());
    }
}

std::optional<AsyncClient::Response> AsyncClient::PollResponse()
{
    this->reqMutex.lock();
    auto ret = this->responses.PopFront();
    this->reqMutex.unlock();

    return ret;
}
