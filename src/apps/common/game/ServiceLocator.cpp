#include <ServiceLocator.h>

using namespace App;

std::unique_ptr<Log::Logger> App::ServiceLocator::logger_ = std::unique_ptr<Log::Logger>{};

void ServiceLocator::SetLogger(std::unique_ptr<Log::Logger> logger)
{
    logger_ = std::move(logger);
}

Log::Logger* ServiceLocator::GetLogger()
{
    return logger_.get();
}