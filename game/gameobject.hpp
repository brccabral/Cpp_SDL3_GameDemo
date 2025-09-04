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

enum class EnemyState
{
    shambling, damaged, dead
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
    EnemyState state = EnemyState::shambling;
    Timer damagedTimer{0.5f};
    int healthPoints{100};
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
    // if currentAnimation == -1, will draw the spriteFrame index from object texture
    int currentAnimation = -1;
    SDL_Texture* texture = nullptr;
    bool dynamic{};
    SDL_FRect collider{};
    bool grounded{};
    Timer flashTimer{0.05f}; // object blink on hit
    bool shouldFlash{};
    // index in texture to draw if currentAnimation == -1
    int spriteFrame = 1;

    GameObject() = default;
    SDL_FRect GetCollider() const;
};
