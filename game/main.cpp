#include <array>
#include <print>
#include <string>
#include <vector>
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <autorelease/AutoRelease.hpp>

#include "gameobject.hpp"

template <>
struct std::formatter<SDL_FRect>
{
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

    auto format(const SDL_FRect& r, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "[x: {} y: {} w: {} h: {}]", r.x, r.y, r.w, r.h);
    }
};

template <>
struct std::formatter<glm::vec2>
{
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

    auto format(const glm::vec2& v, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "[x: {} y: {}]", v.x, v.y);
    }
};

typedef struct SDLState
{
    AutoRelease<int> sdl_init;
    AutoRelease<SDL_Window*> window;
    AutoRelease<SDL_Renderer*> renderer;
    int width, height;
    int logW, logH; // logical width/height
    const bool* keys;
    uint64_t prevTime{};

    ~SDLState() = default;
} SDLState;

const size_t LAYER_IDX_LEVEL = 0;
const size_t LAYER_IDX_CHARACTERS = 1;
const int MAP_ROWS = 5;
const int MAP_COLS = 50;
const int TILE_SIZE = 32;

struct GameState
{
    std::array<std::vector<GameObject>, 2> layers;
    int playerIndex = -1;
    SDL_FRect mapViewport{};
    float bg2Scroll{}, bg3Scroll{}, bg4Scroll{};

    GameState(const SDLState* ss)
    {
        mapViewport = {0, 0, static_cast<float>(ss->logW), static_cast<float>(ss->logH)};
    };

    GameObject& player() { return layers[LAYER_IDX_CHARACTERS][playerIndex]; }
};

struct Resources
{
    const int ANIM_PLAYER_IDLE = 0;
    const int ANIM_PLAYER_RUNNING = 1;
    const int ANIM_PLAYER_SLIDE = 2;
    std::vector<Animation> playerAnims;

    std::vector<AutoRelease<SDL_Texture*>> textures;

    // player
    SDL_Texture* texIdle;
    SDL_Texture* texRun;
    SDL_Texture* texSlide;

    // tiles
    SDL_Texture* texBrick;
    SDL_Texture* texGrass;
    SDL_Texture* texGround;
    SDL_Texture* texPanel;

    // backgrounds
    SDL_Texture* texBg1;
    SDL_Texture* texBg2;
    SDL_Texture* texBg3;
    SDL_Texture* texBg4;

    SDL_Texture* loadTexture(SDLState* state, const std::string& filepath)
    {
        AutoRelease<SDL_Texture*> tex = {IMG_LoadTexture(state->renderer, filepath.c_str()), SDL_DestroyTexture};
        SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
        textures.push_back(std::move(tex));
        return textures.back();
    }

    void load(SDLState* state)
    {
        playerAnims.resize(5);
        playerAnims[ANIM_PLAYER_IDLE] = Animation{8, 1.6f};
        playerAnims[ANIM_PLAYER_RUNNING] = Animation{4, 0.5f};
        playerAnims[ANIM_PLAYER_SLIDE] = Animation{1, 1.0f};

        texIdle = loadTexture(state, "data/idle.png");
        texRun = loadTexture(state, "data/run.png");
        texSlide = loadTexture(state, "data/slide.png");
        texBrick = loadTexture(state, "data/tiles/brick.png");
        texGrass = loadTexture(state, "data/tiles/grass.png");
        texGround = loadTexture(state, "data/tiles/ground.png");
        texPanel = loadTexture(state, "data/tiles/panel.png");
        texBg1 = loadTexture(state, "data/bg/bg_layer1.png");
        texBg2 = loadTexture(state, "data/bg/bg_layer2.png");
        texBg3 = loadTexture(state, "data/bg/bg_layer3.png");
        texBg4 = loadTexture(state, "data/bg/bg_layer4.png");
    }

    ~Resources() = default;
};

typedef struct AppState
{
    SDLState* sdlState{};
    GameState* gameState{};
    Resources* resources{};

    ~AppState()
    {
        delete sdlState;
        delete gameState;
        delete resources;
    };
} AppState;

void drawObject(const SDLState* state, GameState* gs, GameObject& obj, float deltaTime);
void update(const SDLState* state, GameState* gs, Resources* res, GameObject& obj, float deltaTime);
void createTiles(const SDLState* state, GameState* gs, Resources* res);
void checkCollision(const SDLState* state, GameState* gs, Resources* res, GameObject& objA,
                    GameObject& objB,
                    float deltaTime, bool isHorizontal);
void collisionResponse(const SDLState* state, GameState* gs, Resources* res, const SDL_FRect& rectA,
                       const SDL_FRect& rectB, const SDL_FRect& rectC, GameObject& a, GameObject& b, float deltaTime,
                       bool isHorizontal);
void handleKeyInput(const SDLState* state, GameState* gs, GameObject& obj, SDL_Scancode key, bool keyDown);
void drawParalaxBackground(SDL_Renderer* renderer, SDL_Texture* texture, float xVelocity, float& scrollPos,
                           float scrollFactor, float deltaTime);

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    if (!SDL_SetAppMetadata("SDL3 Game Demo", "1.0", "com.brunorcabral.sdl3gamedemo"))
    {
        return SDL_APP_FAILURE;
    }

    void* raw = SDL_calloc(1, sizeof(AppState));
    if (!raw)
    {
        SDL_free(raw);
        return SDL_APP_FAILURE;
    }
    // SDL_calloc sets all values to 0, even those with default values.
    // call `new(raw) AppState()` to call C++ constructor
    auto* as = new(raw) AppState();

    *appstate = as;
    SDLState* ss = new SDLState();
    Resources* res = new Resources();

    ss->sdl_init = {SDL_Init(SDL_INIT_VIDEO), [](const int&) { SDL_Quit(); }};
    if (!ss->sdl_init)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", SDL_GetError(), nullptr);
        return SDL_APP_FAILURE;
    }

    ss->width = 1600;
    ss->height = 900;
    ss->window = {
        SDL_CreateWindow("SDL3 Game Demo", ss->width, ss->height, SDL_WINDOW_RESIZABLE), SDL_DestroyWindow
    };
    if (!ss->window)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", SDL_GetError(), nullptr);
        return SDL_APP_FAILURE;
    }
    ss->renderer = {SDL_CreateRenderer(ss->window, NULL), SDL_DestroyRenderer};
    if (!ss->renderer)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", SDL_GetError(), ss->window);
        return SDL_APP_FAILURE;
    }

    // configure presentation
    // SDL will scale the final render buffer for us
    // SDL_LOGICAL_PRESENTATION_LETTERBOX keeps aspect ratio logW/logH, adding black banners as needed in SDL window
    ss->logW = 640;
    ss->logH = 320;
    SDL_SetRenderLogicalPresentation(ss->renderer, ss->logW, ss->logH, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    ss->keys = SDL_GetKeyboardState(nullptr);

    res->load(ss);

    GameState* gs = new GameState(ss);
    createTiles(ss, gs, res);

    // force double buffer allocate memory
    SDL_SetRenderDrawColor(ss->renderer, 0, 0, 0, 255);
    SDL_RenderClear(ss->renderer);
    SDL_RenderPresent(ss->renderer);
    SDL_RenderClear(ss->renderer);
    SDL_RenderPresent(ss->renderer);

    as->sdlState = ss;
    as->resources = res;
    as->gameState = gs;

    // getTicks() start with SDL_Init, but we spent time loading resources, so, getTicks() before first deltaTime
    ss->prevTime = SDL_GetTicks();
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    auto* ss = ((AppState*)appstate)->sdlState;
    auto* gs = ((AppState*)appstate)->gameState;

    switch (event->type)
    {
    case SDL_EVENT_QUIT:
        {
            return SDL_APP_SUCCESS;
        }
    case SDL_EVENT_WINDOW_RESIZED:
        {
            ss->width = event->window.data1;
            ss->height = event->window.data2;
            break;
        }
    case SDL_EVENT_KEY_DOWN:
        {
            handleKeyInput(ss, gs, gs->player(), event->key.scancode, true);
            break;
        }
    case SDL_EVENT_KEY_UP:
        {
            handleKeyInput(ss, gs, gs->player(), event->key.scancode, false);
            break;
        }
    default:
        {
            break;
        }
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
    auto* ss = ((AppState*)appstate)->sdlState;
    auto* gs = ((AppState*)appstate)->gameState;
    auto* res = ((AppState*)appstate)->resources;

    uint64_t nowTime = SDL_GetTicks();
    float deltaTime = (float)(nowTime - ss->prevTime) / 1000.0f;
    ss->prevTime = nowTime;

    // Draw
    SDL_SetRenderDrawColor(ss->renderer, 20, 10, 30, 255);
    SDL_RenderClear(ss->renderer);

    SDL_RenderTexture(ss->renderer, res->texBg1, nullptr, nullptr);
    drawParalaxBackground(ss->renderer, res->texBg4, gs->player().velocity.x, gs->bg4Scroll, 0.075f, deltaTime);
    drawParalaxBackground(ss->renderer, res->texBg3, gs->player().velocity.x, gs->bg3Scroll, 0.150f, deltaTime);
    drawParalaxBackground(ss->renderer, res->texBg2, gs->player().velocity.x, gs->bg2Scroll, 0.3f, deltaTime);

    // update
    for (auto& layer : gs->layers)
    {
        for (auto& obj : layer)
        {
            update(ss, gs, res, obj, deltaTime);
            if (obj.currentAnimation >= 0)
            {
                obj.animations[obj.currentAnimation].step(deltaTime);
            }
        }
    }

    // calculate viewport position
    gs->mapViewport.x = gs->player().position.x + TILE_SIZE / 2 - gs->mapViewport.w / 2;

    for (auto& layer : gs->layers)
    {
        for (auto& obj : layer)
        {
            drawObject(ss, gs, obj, deltaTime);
        }
    }

    SDL_SetRenderDrawColor(ss->renderer, 255, 255, 255, 255);
    SDL_RenderDebugText(ss->renderer, 5, 5,
                        std::format("State: {}", static_cast<int>(gs->player().data.player.state)).c_str()
    );

    SDL_RenderDebugText(ss->renderer, 5, 15,
                        std::format("Rect: {}", gs->player().GetCollider()).c_str()
    );
    SDL_RenderDebugText(ss->renderer, 5, 25,
                        std::format("Vel: {}", gs->player().velocity).c_str()
    );

    SDL_RenderPresent(ss->renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    auto* as = (AppState*)appstate;
    as->~AppState();
    SDL_free(as);
}

void drawObject(const SDLState* state, GameState* gs, GameObject& obj, float deltaTime)
{
    const float spriteSize = 32;
    float srcX = obj.currentAnimation >= 0 ? obj.animations[obj.currentAnimation].currentFrame() * spriteSize : 0.0f;

    SDL_FRect src{.x = srcX, .y = 0, .w = spriteSize, .h = spriteSize};
    SDL_FRect dst{.x = obj.position.x - gs->mapViewport.x, .y = obj.position.y, .w = spriteSize, .h = spriteSize};

    SDL_FlipMode flipMode = obj.direction < 0 ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    SDL_RenderTextureRotated(state->renderer, obj.texture, &src, &dst, 0, nullptr,
                             flipMode);
}

void update(const SDLState* state, GameState* gs, Resources* res, GameObject& obj, float deltaTime)
{
    // apply some gravity
    if (obj.dynamic)
    {
        obj.velocity += glm::vec2(0, 500) * deltaTime;
    }
    else
    {
        return;
    }

    if (obj.type == ObjectType::player)
    {
        float currentDirection = 0;
        if (state->keys[SDL_SCANCODE_A])
        {
            currentDirection += -1;
        }
        if (state->keys[SDL_SCANCODE_D])
        {
            currentDirection += 1;
        }
        if (currentDirection != 0)
        {
            obj.direction = currentDirection;
        }

        switch (obj.data.player.state)
        {
        case PlayerState::idle:
            {
                if (currentDirection != 0)
                {
                    obj.data.player.state = PlayerState::running;
                }
                else
                {
                    // deacceleration
                    if (obj.velocity.x != 0)
                    {
                        const float factor = obj.velocity.x > 0 ? -1.5f : 1.5f;
                        float amount = factor * obj.acceleration.x * deltaTime;
                        if (std::abs(obj.velocity.x) < std::abs(amount))
                        {
                            obj.velocity.x = 0;
                        }
                        else
                        {
                            obj.velocity.x += amount;
                        }
                    }
                }
                obj.texture = res->texIdle;
                obj.currentAnimation = res->ANIM_PLAYER_IDLE;
                break;
            }
        case PlayerState::running:
            {
                if (currentDirection == 0)
                {
                    obj.data.player.state = PlayerState::idle;
                }
                // moving in opposite direction of velocity, sliding! (changing direction during move)
                if (obj.velocity.x * obj.direction < 0 && obj.grounded)
                {
                    obj.texture = res->texSlide;
                    obj.currentAnimation = res->ANIM_PLAYER_SLIDE;
                }
                else
                {
                    obj.texture = res->texRun;
                    obj.currentAnimation = res->ANIM_PLAYER_RUNNING;
                }
                break;
            }
        case PlayerState::jumping:
            {
                obj.texture = res->texRun;
                obj.currentAnimation = res->ANIM_PLAYER_RUNNING;
                break;
            }
        default:
            {
                break;
            }
        }

        obj.velocity += currentDirection * obj.acceleration * deltaTime;
        obj.velocity.x = glm::clamp(obj.velocity.x, -obj.maxSpeedX, obj.maxSpeedX);
    }

    // horizontal
    obj.position.x += obj.velocity.x * deltaTime;
    for (auto& layer : gs->layers)
    {
        for (auto& objB : layer)
        {
            if (&obj == &objB)
            {
                continue;
            }
            checkCollision(state, gs, res, obj, objB, deltaTime, true);
        }
    }
    // vertical
    bool foundGround = false;
    obj.position.y += obj.velocity.y * deltaTime;
    for (auto& layer : gs->layers)
    {
        for (auto& objB : layer)
        {
            if (&obj == &objB)
            {
                continue;
            }
            checkCollision(state, gs, res, obj, objB, deltaTime, false);

            // grounded sensor
            SDL_FRect sensor{
                obj.position.x + obj.collider.x,
                obj.position.y + obj.collider.y + obj.collider.h,
                obj.collider.w,
                1
            };
            SDL_FRect rectB{
                objB.position.x + objB.collider.x,
                objB.position.y + objB.collider.y,
                objB.collider.w,
                objB.collider.h
            };
            SDL_FRect rectC;
            if (SDL_GetRectIntersectionFloat(&sensor, &rectB, &rectC) && (rectC.w > 0.00001f && rectC.h > 0.00001f))
            {
                foundGround = true;
            }
        }
    }
    if (foundGround != obj.grounded)
    {
        obj.grounded = foundGround;
        if (foundGround && obj.type == ObjectType::player)
        {
            obj.data.player.state = PlayerState::running;
            // if player stopped running, the next frame will change to idle
        }
    }
}

void collisionResponse(const SDLState* ss, GameState* gs, Resources* res, SDL_FRect& rectA,
                       const SDL_FRect& rectB, const SDL_FRect& rectC, GameObject& a, GameObject& b, float deltaTime,
                       bool isHorizontal)
{
    if (a.type == ObjectType::player)
    {
        switch (b.type)
        {
        case ObjectType::level:
            {
                if (isHorizontal) // horizontal collision
                {
                    if (a.velocity.x > 0) // going right
                    {
                        a.position.x = rectB.x - a.collider.w - a.collider.x;
                        a.velocity.x = 0;
                    }
                    else if (a.velocity.x < 0)
                    {
                        a.position.x = rectB.x + rectB.w - a.collider.x;
                        a.velocity.x = 0;
                    }
                }
                else if (!isHorizontal) // vertical
                {
                    if (a.velocity.y > 0) // going down
                    {
                        a.position.y = rectB.y - a.collider.h - a.collider.y;
                        a.velocity.y = 0;
                    }
                    else if (a.velocity.y < 0)
                    {
                        a.position.y = rectB.y + rectB.h - a.collider.y;
                        a.velocity.y = 0;
                    }
                }
                break;
            }
        }
    }
}

void checkCollision(const SDLState* state, GameState* gs, Resources* res, GameObject& objA,
                    GameObject& objB, float deltaTime, bool isHorizontal)
{
    SDL_FRect rectA = objA.GetCollider();
    SDL_FRect rectB = objB.GetCollider();
    SDL_FRect rectC{}; // collision result

    if (SDL_GetRectIntersectionFloat(&rectA, &rectB, &rectC) && (rectC.w > 0.00001f && rectC.h > 0.00001f))
    {
        collisionResponse(state, gs, res, rectA, rectB, rectC, objA, objB, deltaTime, isHorizontal);
    }
}

void createTiles(const SDLState* state, GameState* gs, Resources* res)
{
    /*
     * 1 - Ground
     * 2 - Panel
     * 3 - Enemy
     * 4 - Player
     * 5 - Grass
     * 6 - Brick
     */
    short map[MAP_ROWS][MAP_COLS] = {
        0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 3, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 3, 3, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 2, 0, 0, 2, 2, 2, 2, 0, 2, 2, 2, 0, 0, 3, 2, 2, 2, 2, 0, 0, 2, 0, 0, 0, 0, 0, 2, 2, 2, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 2, 0, 2, 2, 0, 0, 0, 3, 0, 0, 3, 0, 2, 2, 2, 2, 2, 0, 0, 2, 2, 0, 3, 0, 0, 3, 0, 2, 3, 3, 3, 0, 2, 0,
        3, 3, 0, 0, 3, 0, 3, 0, 3, 0, 0, 0, 3,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
    };

    const auto createObject = [state](const int r, const int c, SDL_Texture* tex, const ObjectType type)
    {
        GameObject o;
        o.type = type;
        o.position = glm::vec2(c * TILE_SIZE, state->logH - (MAP_ROWS - r) * TILE_SIZE);
        o.texture = tex;
        o.collider = {0, 0, TILE_SIZE, TILE_SIZE};
        return o;
    };

    for (int r = 0; r < MAP_ROWS; r++)
    {
        for (int c = 0; c < MAP_COLS; c++)
        {
            switch (map[r][c])
            {
            case 1: // ground
                {
                    GameObject ground = createObject(r, c, res->texGround, ObjectType::level);
                    gs->layers[LAYER_IDX_LEVEL].push_back(std::move(ground));
                    break;
                }
            case 2: // panel
                {
                    GameObject panel = createObject(r, c, res->texPanel, ObjectType::level);
                    gs->layers[LAYER_IDX_LEVEL].push_back(std::move(panel));
                    break;
                }
            case 4: // player
                {
                    GameObject player = createObject(r, c, res->texIdle, ObjectType::player);
                    player.data.player = PlayerData();
                    player.acceleration = glm::vec2(300.f, 0.f);
                    player.maxSpeedX = 100.f;
                    player.animations = res->playerAnims;
                    player.currentAnimation = res->ANIM_PLAYER_IDLE;
                    player.dynamic = true;
                    player.collider = {11, 6, 10, 26};
                    gs->layers[LAYER_IDX_CHARACTERS].push_back(std::move(player));
                    gs->playerIndex = gs->layers[LAYER_IDX_CHARACTERS].size() - 1;
                    break;
                }
            case 5: // grass
                {
                    GameObject grass = createObject(r, c, res->texGrass, ObjectType::level);
                    gs->layers[LAYER_IDX_LEVEL].push_back(std::move(grass));
                    break;
                }
            case 6: // brick
                {
                    GameObject brick = createObject(r, c, res->texBrick, ObjectType::level);
                    gs->layers[LAYER_IDX_LEVEL].push_back(std::move(brick));
                    break;
                }
            default:
                {
                    break;
                }
            }
        }
    }
    assert(gs->playerIndex != -1);
}

void handleKeyInput(const SDLState* state, GameState* gs, GameObject& obj, SDL_Scancode key, bool keyDown)
{
    const float JUMP_FORCE = -200.0f;
    if (obj.type != ObjectType::player)
    {
        return;
    }
    switch (obj.data.player.state)
    {
    case PlayerState::idle:
        {
            if (key == SDL_SCANCODE_K && keyDown)
            {
                obj.velocity.y += JUMP_FORCE;
                obj.data.player.state = PlayerState::jumping;
            }
            break;
        }
    case PlayerState::running:
        {
            if (key == SDL_SCANCODE_K && keyDown)
            {
                obj.velocity.y += JUMP_FORCE;
                obj.data.player.state = PlayerState::jumping;
            }
            break;
        }
    default:
        {
            break;
        }
    }
}

void drawParalaxBackground(SDL_Renderer* renderer, SDL_Texture* texture, float xVelocity, float& scrollPos,
                           float scrollFactor, float deltaTime)
{
    scrollPos -= xVelocity * scrollFactor * deltaTime;
    if (scrollPos <= -texture->w)
    {
        scrollPos = 0;
    }

    // double the dest width makes SDL_RenderTextureTiled draw same texture twice
    // avoiding calling renderTexture twice
    SDL_FRect dst
    {
        scrollPos, 30, texture->w * 2.0f,
        static_cast<float>(texture->h)
    };

    SDL_RenderTextureTiled(renderer, texture, nullptr, 1, &dst);
}
