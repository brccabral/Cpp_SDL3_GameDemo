#pragma once
#include <vector>
#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include "animation.hpp"

enum class PlayerState
{
    idle, running, jumping
};

enum class BulletState
{
    moving, colliding, inactive
};

struct PlayerData
{
    PlayerState state = PlayerState::idle;
    Timer weaponTimer{0.1f};
};

struct LevelData
{
};

struct EnemyData
{
};

struct BulletData
{
    BulletState state = BulletState::moving;
};

union ObjectData
{
    PlayerData player;
    LevelData level;
    EnemyData enemy;
    BulletData bullet;
};

enum class ObjectType
{
    player, level, enemy, bullet
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
    bool dynamic{};
    SDL_FRect collider{};
    bool grounded{};

    GameObject() = default;
    SDL_FRect GetCollider() const;
};
