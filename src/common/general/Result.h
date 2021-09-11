
#include <string>

namespace General
{

    enum ResultType
    {
        ERROR,
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
    };

    template<typename T>
    Result<T> CreateSuccess(T data)
    {
        return Result<T>{.type = ResultType::SUCCESS, .data = data};
    }

    template<typename T>
    Result<T> CreateError(std::string msg)
    {
        return Result<T>{.type = ResultType::ERROR, .err = Error{msg.c_str()}};
    }

    template<typename T>
    Result<T> CreateError(const char * msg)
    {
        return Result<T>{.type = ResultType::ERROR, .err = Error{msg}};
    }
}
