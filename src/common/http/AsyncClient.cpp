#include <AsyncClient.h>

#include <util/BBTime.h>

using namespace HTTP;

void AsyncClient::Request(const std::string& path, const std::string& body, RespHandler respHandler, ErrHandler errHandler)
{
    if(!enabled)
        return;

    connecting = true;

    auto threadId = lastId++;
    auto reqThread = std::thread{
        [this, threadId, body, path, respHandler, errHandler](){

            httplib::Client client{address, port};
            client.set_read_timeout(timeout);

            auto res = client.Post(path.c_str(), body.c_str(), body.size(), "application/json");
            auto response = Response{std::move(res), respHandler, errHandler};
            Util::Time::Sleep(Util::Time::Seconds{0.25});  

            this->reqMutex.lock();
                this->responses.PushBack(std::move(response));
                this->joinThreads.push_back(threadId);
            this->reqMutex.unlock();

            this->connecting = false;
        }
    };
    reqThreads[threadId++] = std::move(reqThread);
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

    while(!joinThreads.empty())
    {
        auto threaId = joinThreads.back();
        joinThreads.pop_back();

        auto& thread = reqThreads[threaId];
        if(thread.joinable())
            thread.join();

        reqThreads.erase(threaId);
    }
}

std::optional<AsyncClient::Response> AsyncClient::PollResponse()
{
    this->reqMutex.lock();
    auto ret = this->responses.PopFront();
    this->reqMutex.unlock();

    return ret;
}
