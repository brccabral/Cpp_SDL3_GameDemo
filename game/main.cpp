#include <string>
#include <vector>
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <autorelease/AutoRelease.hpp>

#include "animation.hpp"


typedef struct GameState
{
    AutoRelease<int> sdl_init;
    AutoRelease<SDL_Window*> window;
    AutoRelease<SDL_Renderer*> renderer;
    int width, height;
    int logW, logH; // logical width/height
    const bool* keys;
    float playerX = 150;
    bool flipHorizontal = false;
    uint64_t prevTime = SDL_GetTicks();

    ~GameState() = default;
} GameState;

struct Resources
{
    const int ANIM_PLAYER_IDLE = 0;
    std::vector<Animation> playerAnims;

    std::vector<AutoRelease<SDL_Texture*>> textures;
    SDL_Texture* texIdle;

    SDL_Texture* loadTexture(GameState* state, const std::string& filepath)
    {
        AutoRelease<SDL_Texture*> tex = {IMG_LoadTexture(state->renderer, filepath.c_str()), SDL_DestroyTexture};
        SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
        textures.push_back(std::move(tex));
        return textures.back();
    }

    void load(GameState* state)
    {
        playerAnims.resize(5);
        playerAnims[ANIM_PLAYER_IDLE] = Animation{8, 1.6f};

        texIdle = loadTexture(state, "data/idle.png");
    }

    ~Resources() = default;
};

typedef struct SDLState
{
    GameState gameState;
    Resources resources;

    ~SDLState() = default;
} SDLState;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    if (!SDL_SetAppMetadata("SDL3 Game Demo", "1.0", "com.brunorcabral.sdl3gamedemo"))
    {
        return SDL_APP_FAILURE;
    }

    auto* ss = (SDLState*)SDL_calloc(1, sizeof(SDLState));
    if (!ss)
    {
        return SDL_APP_FAILURE;
    }

    *appstate = ss;
    GameState* gs = &ss->gameState;

    gs->sdl_init = {SDL_Init(SDL_INIT_VIDEO), [](const int&) { SDL_Quit(); }};
    if (!gs->sdl_init)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", SDL_GetError(), nullptr);
        return SDL_APP_FAILURE;
    }

    gs->width = 1600;
    gs->height = 900;
    gs->window = {SDL_CreateWindow("SDL3 Game Demo", gs->width, gs->height, SDL_WINDOW_RESIZABLE), SDL_DestroyWindow};
    if (!gs->window)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", SDL_GetError(), nullptr);
        return SDL_APP_FAILURE;
    }
    gs->renderer = {SDL_CreateRenderer(gs->window, NULL), SDL_DestroyRenderer};
    if (!gs->renderer)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", SDL_GetError(), gs->window);
        return SDL_APP_FAILURE;
    }

    // configure presentation
    // SDL will scale the final render buffer for us
    // SDL_LOGICAL_PRESENTATION_LETTERBOX keeps aspect ratio logW/logH, adding black banners as needed in SDL window
    gs->logW = 640;
    gs->logH = 320;
    SDL_SetRenderLogicalPresentation(gs->renderer, gs->logW, gs->logH, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    gs->keys = SDL_GetKeyboardState(nullptr);

    ss->resources.load(gs);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    auto* gs = &((SDLState*)appstate)->gameState;

    switch (event->type)
    {
    case SDL_EVENT_QUIT:
        {
            return SDL_APP_SUCCESS;
        }
    case SDL_EVENT_WINDOW_RESIZED:
        {
            gs->width = event->window.data1;
            gs->height = event->window.data2;
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
    auto* gs = &((SDLState*)appstate)->gameState;
    auto* res = &((SDLState*)appstate)->resources;

    uint64_t nowTime = SDL_GetTicks();
    float deltaTime = (float)(nowTime - gs->prevTime) / 1000.0f;
    gs->prevTime = nowTime;

    // handle movement
    float moveAmount = 0;
    if (gs->keys[SDL_SCANCODE_A])
    {
        moveAmount += -75.0f;
        gs->flipHorizontal = true;
    }
    if (gs->keys[SDL_SCANCODE_D])
    {
        moveAmount += 75.0f;
        gs->flipHorizontal = false;
    }
    gs->playerX += moveAmount * deltaTime;

    const float spriteSize = 32;
    const float floor = gs->logH;

    SDL_SetRenderDrawColor(gs->renderer, 20, 10, 30, 255);
    SDL_RenderClear(gs->renderer);

    SDL_FRect src{.x = 0, .y = 0, .w = spriteSize, .h = spriteSize};
    SDL_FRect dst{.x = gs->playerX, .y = floor - spriteSize, .w = spriteSize, .h = spriteSize};
    SDL_RenderTextureRotated(gs->renderer, res->texIdle, &src, &dst, 0, nullptr,
                             gs->flipHorizontal ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);

    SDL_RenderPresent(gs->renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    auto* ss = (SDLState*)appstate;
    ss->~SDLState();
    SDL_free(ss);
}
