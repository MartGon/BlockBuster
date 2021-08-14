
#include <glm/glm.hpp>

#include <string>

#ifndef _DEBUG
    #define NDEBUG 1
#endif
#include <cassert>

#define assertm(exp, msg) assert(((void)msg, exp))

namespace Debug
{
   std::string ToString(glm::vec3 vector);
   void PrintVector(glm::vec3 vector, std::string name);
}