#include <ServiceLocator.h>

using namespace App;

std::unique_ptr<Log::Logger> ServiceLocator::logger_ = std::make_unique<Log::NullLogger>();

void ServiceLocator::SetLogger(std::unique_ptr<Log::Logger> logger)
{
    logger_ = std::move(logger);
}

Log::Logger* ServiceLocator::GetLogger()
{
    return logger_.get();
}