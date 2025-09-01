#include "gameobject.hpp"

SDL_FRect GameObject::GetCollider() const
{
    return {position.x + collider.x, position.y + collider.y, collider.w, collider.h};
}
