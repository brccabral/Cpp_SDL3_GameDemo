#pragma once
#include <vector>
#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include "animation.hpp"

enum class ObjectType
{
    player, level, enemy
};

struct GameObject
{
    ObjectType type = ObjectType::level;
    glm::vec2 position{}, velocity{}, acceleration{};
    float direction = 1;
    std::vector<Animation> animations{};
    int currentAnimation = -1;
    SDL_Texture* texture = nullptr;

    GameObject() = default;
};
