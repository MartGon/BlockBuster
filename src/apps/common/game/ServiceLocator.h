#pragma once

#include <mglogger/MGLogger.h>

namespace App
{
    class ServiceLocator
    {
    public:
        static void SetLogger(std::unique_ptr<Log::Logger> logger);
        static Log::Logger* GetLogger();

    private:
        static std::unique_ptr<Log::Logger> logger_;
    };
}