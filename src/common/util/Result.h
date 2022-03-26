#pragma once

#include <string>

namespace Util
{
    enum ResultType
    {
        ERR,
        SUCCESS
    };

    struct Error
    {
        const char* info;
    };

    template<typename T>
    struct Result
    {
        ResultType type;
        T data;
        Error err;

        inline bool IsError() const
        {
            return type == ResultType::ERR;
        }

        inline bool IsSuccess() const
        {
            return !IsError();
        }
    };

    template<typename T>
    Result<T> CreateSuccess(T data)
    {
        return Result<T>{ResultType::SUCCESS, data};
    }

    template<typename T>
    Result<T> CreateError(std::string msg)
    {
        return Result<T>{ResultType::ERR, T{}, Error{msg.c_str()}};
    }

    template<typename T>
    Result<T> CreateError(const char * msg)
    {
        return Result<T>{ResultType::ERR, T{}, Error{ msg }};
    }
}
