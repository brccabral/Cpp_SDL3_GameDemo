#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <autorelease/AutoRelease.hpp>


typedef struct AppState
{
    AutoRelease<int> sdl_init;
    AutoRelease<SDL_Window*> window;
    AutoRelease<SDL_Renderer*> renderer;
    AutoRelease<SDL_Texture*> idleTex;
    int width, height;
    int logW, logH; // logical width/height
    const bool* keys;
    float playerX = 150;
    bool flipHorizontal = false;
    uint64_t prevTime = SDL_GetTicks();

    ~AppState() = default;
} AppState;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    if (!SDL_SetAppMetadata("SDL3 Game Demo", "1.0", "com.brunorcabral.sdl3gamedemo"))
    {
        return SDL_APP_FAILURE;
    }

    auto* as = (AppState*)SDL_calloc(1, sizeof(AppState));
    if (!as)
    {
        return SDL_APP_FAILURE;
    }

    *appstate = as;

    as->sdl_init = {SDL_Init(SDL_INIT_VIDEO), [](const int&) { SDL_Quit(); }};
    if (!as->sdl_init)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", SDL_GetError(), nullptr);
        return SDL_APP_FAILURE;
    }

    as->width = 1600;
    as->height = 900;
    as->window = {SDL_CreateWindow("SDL3 Game Demo", as->width, as->height, SDL_WINDOW_RESIZABLE), SDL_DestroyWindow};
    if (!as->window)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", SDL_GetError(), nullptr);
        return SDL_APP_FAILURE;
    }
    as->renderer = {SDL_CreateRenderer(as->window, NULL), SDL_DestroyRenderer};
    if (!as->renderer)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", SDL_GetError(), as->window);
        return SDL_APP_FAILURE;
    }

    // configure presentation
    // SDL will scale the final render buffer for us
    // SDL_LOGICAL_PRESENTATION_LETTERBOX keeps aspect ratio logW/logH, adding black banners as needed in SDL window
    as->logW = 640;
    as->logH = 320;
    SDL_SetRenderLogicalPresentation(as->renderer, as->logW, as->logH, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    as->idleTex = {IMG_LoadTexture(as->renderer, "data/idle.png"), SDL_DestroyTexture};
    SDL_SetTextureScaleMode(as->idleTex, SDL_SCALEMODE_NEAREST);

    as->keys = SDL_GetKeyboardState(nullptr);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    auto* as = (AppState*)appstate;

    switch (event->type)
    {
    case SDL_EVENT_QUIT:
        {
            return SDL_APP_SUCCESS;
        }
    case SDL_EVENT_WINDOW_RESIZED:
        {
            as->width = event->window.data1;
            as->height = event->window.data2;
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
    auto* as = (AppState*)appstate;

    uint64_t nowTime = SDL_GetTicks();
    float deltaTime = (float)(nowTime - as->prevTime) / 1000.0f;
    as->prevTime = nowTime;

    // handle movement
    float moveAmount = 0;
    if (as->keys[SDL_SCANCODE_A])
    {
        moveAmount += -75.0f;
        as->flipHorizontal = true;
    }
    if (as->keys[SDL_SCANCODE_D])
    {
        moveAmount += 75.0f;
        as->flipHorizontal = false;
    }
    as->playerX += moveAmount * deltaTime;

    const float spriteSize = 32;
    const float floor = as->logH;

    SDL_SetRenderDrawColor(as->renderer, 20, 10, 30, 255);
    SDL_RenderClear(as->renderer);

    SDL_FRect src{.x = 0, .y = 0, .w = spriteSize, .h = spriteSize};
    SDL_FRect dst{.x = as->playerX, .y = floor - spriteSize, .w = spriteSize, .h = spriteSize};
    SDL_RenderTextureRotated(as->renderer, as->idleTex, &src, &dst, 0, nullptr,
                             as->flipHorizontal ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);

    SDL_RenderPresent(as->renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    auto* as = (AppState*)appstate;
    as->~AppState();
    SDL_free(as);
}
