#include <Player.h>

#include <collisions/Collisions.h>

#include <algorithm>

void AppGame::Player::HandleSDLEvent(const SDL_Event& event)
{

}

void AppGame::Player::Update()
{
    // Move
    auto state = SDL_GetKeyboardState(nullptr);

    glm::vec3 moveDir{0.0f};
    if(state[SDL_SCANCODE_A])
        moveDir.x -= 1;
    if(state[SDL_SCANCODE_D])
        moveDir.x += 1;
    if(state[SDL_SCANCODE_W])
        moveDir.z -= 1;
    if(state[SDL_SCANCODE_S])
        moveDir.z += 1;

    // Rotate moveDir
    auto rotMat = transform.GetRotationMat();
    moveDir = glm::vec3{rotMat * glm::vec4{moveDir, 1.0f}};
    moveDir = glm::length(moveDir) > 0.0f ? glm::normalize(moveDir) : moveDir;
    
    this->prevPos = transform.position;
    transform.position += (moveDir * speed);
    bool gravityAffected = gravity;
    if(isOnSlope || gravity)
    {
        transform.position += glm::vec3{0.0f, gravitySpeed, 0.0f};
    }
    gravity = true;
    isOnSlope = false;
}

union IntersectionU
{
    Collisions::AABBSlopeIntersection aabbSlope;
    Collisions::AABBIntersection aabb;
};

struct Intersection
{
    Game::Block block;
    IntersectionU intersection;
};

#include <iostream>

void AppGame::Player::HandleCollisions(const std::vector<Game::Block>& blocks)
{
    bool intersects;
    unsigned int iterations = 0;
    const unsigned int MAX_ITERATIONS = 10;
    std::vector<glm::vec<3, int>> alreadyOffset;
    do
    {        
        intersects = false;
        iterations++;

        std::vector<Intersection> intersections;
        for(const auto& block : blocks)
        {
            Math::Transform prevPlayerTransform{this->prevPos, glm::vec3{0.0f}, transform.scale};

            if(block.type == Game::SLOPE)
            {
                // Collision player and slope i                
                auto slopeIntersect = Collisions::AABBSlopeCollision(transform, prevPlayerTransform, block.transform);
                if(slopeIntersect.collides)
                {
                    glm::vec3 normal = slopeIntersect.normal;

                    if (normal.y > 0.0f && glm::abs(normal.x) < 0.05f && glm::abs(normal.z) < 0.05f)
                    {
                        gravity = false;
                    }
                    else if(normal.y > 0.0f)
                    {
                        isOnSlope = true;
                    }

                    if(slopeIntersect.intersects)
                    {
                        std::cout << "Intersection with " << block.name << "\n";
                        Intersection intersect;
                        intersect.block = block;
                        intersect.intersection.aabbSlope = slopeIntersect;
                        intersections.push_back(intersect);

                        intersects = true;
                    }
                }
            }
            else
            {
                auto boxIntersect = Collisions::AABBCollision(transform.position, glm::vec3{transform.scale}, block.transform.position, glm::vec3{block.transform.scale});
                if(boxIntersect.collides)
                {
                    auto isGround = boxIntersect.normal.y > 0.0f && glm::abs(boxIntersect.normal.x) == 0.0f && glm::abs(boxIntersect.normal.z) == 0.0f;
                    if(isGround)
                        gravity = false;

                    if(boxIntersect.intersects)
                    {
                        Intersection intersect;
                        intersect.block = block;
                        intersect.intersection.aabb = boxIntersect;
                        intersections.push_back(intersect);
                        
                        intersects = true;
                    }
                }
            }
        }

        auto playerPos = transform.position;
        std::sort(intersections.begin(), intersections.end(), [playerPos](Intersection a, Intersection b){
            auto distToA = glm::length(a.block.transform.position - playerPos);
            auto distToB = glm::length(b.block.transform.position - playerPos);
            return distToA < distToB;
        });

        if(!intersections.empty())
        {
            auto first = intersections.front();
            auto block = first.block;
            if(block.type == Game::SLOPE)
            {
                std::cout << "Handling collision with " << block.name << "\n";
                auto slopeIntersect = first.intersection.aabbSlope;
                auto offset = slopeIntersect.offset;
                transform.position += slopeIntersect.offset;
            }
            else
            {
                auto boxIntersect = first.intersection.aabb;
                transform.position += boxIntersect.offset;
            }

            alreadyOffset.push_back(block.transform.position);
        }

    }while(intersects && iterations < MAX_ITERATIONS);
}