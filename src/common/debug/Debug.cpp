#include <Debug.h>

#include <iostream>

std::string Debug::ToString(glm::vec3 vec)
{
    return std::to_string(vec.x) + " " + std::to_string(vec.y) + " " + std::to_string(vec.z);
}

void Debug::PrintVector(glm::vec3 vec, std::string name)
{
    std::cout << name << " is " <<  ToString(vec) << "\n";
}

void Debug::PrintVector(std::string name, glm::vec3 vec)
{
    std::cout << name << " is " <<  ToString(vec) << "\n";
}