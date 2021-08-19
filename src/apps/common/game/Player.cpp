#include <Player.h>

#include <collisions/Collisions.h>

#include <algorithm>
#include <iostream>

void AppGame::Player::HandleSDLEvent(const SDL_Event& event)
{

}

void AppGame::Player::Update()
{
    this->prevPos = transform.position;

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
    
    glm::vec3 velocity = moveDir * speed;
    // Velocity on slope's normal axis is doubled
    if(isOnSlope || wasOnSlope)
    {
        // Projected dir onto slope plane
        moveDir = moveDir - glm::dot(moveDir, slopeNormal) * slopeNormal;
        auto mod = glm::max(glm::step(glm::vec3{0.005}, glm::abs(slopeNormal)) * 2.0f, glm::vec3{1.0f});
        velocity = moveDir * mod * speed;
    }
    
    transform.position += velocity;
    // Fix height on slope
    if(isOnSlope || wasOnSlope)
    {
        auto slopeHeight = slopeTransform.position.y;
        auto slopeScale = slopeTransform.scale;
        auto maxHeight = slopeHeight + (slopeScale - transform.scale / 2.0f).y;
        auto minHeight = slopeHeight - (slopeScale - transform.scale / 2.0f).y;
        transform.position.y = glm::min(maxHeight, transform.position.y);
    }

    auto pos = transform.position;
    std::cout << "Desired pos " << pos.x << " " << pos.y << " " << pos.z << "\n";
    if(gravity && (!wasOnSlope || !isOnSlope))
    {
        transform.position += glm::vec3{0.0f, gravitySpeed, 0.0f};
    }
    gravity = true;
    wasOnSlope = isOnSlope;
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
    Math::Transform blockTransform;
};

void AppGame::Player::HandleCollisions(Game::Map::Map* map, float blockScale)
{

    std::vector<std::pair<Math::Transform, Game::Block>> blocks;
    auto bIt = map->CreateIterator();
    for(auto b = bIt.GetNextBlock(); !bIt.IsOver(); b = bIt.GetNextBlock())
        blocks.push_back({Game::GetBlockTransform(*b.second, b.first, blockScale), *b.second});

    HandleCollisions(blocks);
}

void AppGame::Player::HandleCollisions(const std::vector<std::pair<Math::Transform, Game::Block>> &blocks)
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
        for(const auto& b : blocks)
        {
            auto blockTransform = b.first;
            auto block = b.second;
            Math::Transform prevPlayerTransform{this->prevPos, glm::vec3{0.0f}, transform.scale};

            if(block.type == Game::SLOPE)
            {
                // Collision player and slope i                
                auto slopeIntersect = Collisions::AABBSlopeCollision(transform, prevPlayerTransform, blockTransform);
                if(slopeIntersect.collides)
                {
                    //std::cout << "Collides w Slope\n";
                    glm::vec3 normalSign = glm::sign(slopeIntersect.normal);
                    glm::vec3 normal = glm::step(glm::vec3{0.005}, glm::abs(slopeIntersect.normal)) * normalSign; 

                    if (normal.y > 0.0f)
                    {
                        gravity = false;
                        if(glm::abs(normal.x) > 0.0f || glm::abs(normal.z) > 0.0f)
                        {
                            isOnSlope = true;
                            slopeNormal = glm::normalize(normal);
                            slopeTransform = blockTransform;
                        }
                    }

                    if(slopeIntersect.intersects)
                    {
                        //std::cout << "Intersection with " << block.name << "\n";
                        Intersection intersect;
                        intersect.block = block;
                        intersect.intersection.aabbSlope = slopeIntersect;
                        intersections.push_back(intersect);
                        intersect.blockTransform = blockTransform;

                        intersects = true;
                    }
                }
            }
            else
            {
                auto boxIntersect = Collisions::AABBCollision(transform.position, glm::vec3{transform.scale}, blockTransform.position, blockTransform.scale);
                if(boxIntersect.collides)
                {
                    //std::cout << "Collides w Box\n";
                    auto isGround = boxIntersect.normal.y > 0.0f && glm::abs(boxIntersect.normal.x) == 0.0f && glm::abs(boxIntersect.normal.z) == 0.0f;
                    if(isGround)
                        gravity = false;

                    if(boxIntersect.intersects)
                    {
                        Intersection intersect;
                        intersect.block = block;
                        intersect.intersection.aabb = boxIntersect;
                        intersect.blockTransform = blockTransform;
                        intersections.push_back(intersect);
                        
                        intersects = true;
                    }
                }
            }
        }

        auto playerPos = transform.position;
        std::sort(intersections.begin(), intersections.end(), [playerPos](const auto& a, const auto& b){
            auto distToA = glm::length(a.blockTransform.position - playerPos);
            auto distToB = glm::length(b.blockTransform.position - playerPos);
            return distToA < distToB;
        });

        if(!intersections.empty())
        {
            auto first = intersections.front();
            auto block = first.block;
            if(block.type == Game::SLOPE)
            {
                //std::cout << "Handling collision with " << block.name << "\n";
                auto slopeIntersect = first.intersection.aabbSlope;
                auto offset = slopeIntersect.offset;
                transform.position += slopeIntersect.offset;
            }
            else
            {
                auto boxIntersect = first.intersection.aabb;
                transform.position += boxIntersect.offset;
            }

            auto o = first.intersection.aabb.offset;
            alreadyOffset.push_back(first.blockTransform.position);
        }

    }while(intersects && iterations < MAX_ITERATIONS);

    auto pos = transform.position;
}