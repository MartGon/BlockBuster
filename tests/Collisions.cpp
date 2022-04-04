#include <doctest/doctest.h>
#include <collisions/Collisions.h>

#include <vector>

TEST_CASE("Points")
{
    Math::Transform aabb{glm::vec3{0, 0, 0}, glm::vec3{0}, glm::vec3{3}};
    std::vector<glm::vec3> inPoints = {
        glm::vec3{-1, 0, 0},
        glm::vec3{-1, -1, -1},
        glm::vec3{-1.5f, -1.5f, -1.5f},
        glm::vec3{1.5f, 1.5f, 1.5f}
    };

    for(auto inPoint : inPoints)
    {
        bool inside = Collisions::IsPointInAABB(inPoint, aabb);
        CHECK(inside == true);
    }
}