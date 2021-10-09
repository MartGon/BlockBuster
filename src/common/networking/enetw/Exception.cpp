#include <Exception.h>

using namespace ENet;

const char* Exception::what() const noexcept
{
    return msg_.c_str();
}