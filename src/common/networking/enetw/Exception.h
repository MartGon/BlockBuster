#pragma once

#include <stdexcept>

namespace ENet
{
    class Exception : public std::runtime_error
    {
    public:
        Exception(std::string msg) : std::runtime_error{msg} {}

        const char* what() const noexcept;

    private:
        std::string msg_;
    };
}