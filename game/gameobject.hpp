#pragma once
#include <vector>
#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include "animation.hpp"

enum class PlayerState
{
    idle, running, jumping
};

struct PlayerData
{
    PlayerState state = PlayerState::idle;
    PlayerData() = default;
};

struct LevelData
{
};

struct EnemyData
{
};

union ObjectData
{
    PlayerData player;
    LevelData level;
    EnemyData enemy;
};

enum class ObjectType
{
    player, level, enemy
};

struct GameObject
{
    ObjectType type = ObjectType::level;
    ObjectData data{.level = LevelData{}};
    glm::vec2 position{}, velocity{}, acceleration{};
    float direction = 1;
    float maxSpeedX = 0;
    std::vector<Animation> animations{};
    int currentAnimation = -1;
    SDL_Texture* texture = nullptr;

    GameObject() = default;
};
