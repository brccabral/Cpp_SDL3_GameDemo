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

    constexpr int width = 800;
    constexpr int height = 600;
    as->window = {SDL_CreateWindow("SDL3 Game Demo", width, height, 0), SDL_DestroyWindow};
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

    as->idleTex = {IMG_LoadTexture(as->renderer, "data/idle.png"), SDL_DestroyTexture};
    SDL_SetTextureScaleMode(as->idleTex, SDL_SCALEMODE_NEAREST);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    switch (event->type)
    {
    case SDL_EVENT_QUIT:
        return SDL_APP_SUCCESS;
    default:
        break;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
    auto* as = (AppState*)appstate;

    SDL_SetRenderDrawColor(as->renderer, 0, 0, 0, 255);
    SDL_RenderClear(as->renderer);

    SDL_FRect src{.x = 0, .y = 0, .w = 32, .h = 32};
    SDL_FRect dst{.x = 0, .y = 0, .w = 32, .h = 32};
    SDL_RenderTexture(as->renderer, as->idleTex, &src, &dst);

    SDL_RenderPresent(as->renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    auto* as = (AppState*)appstate;
    as->~AppState();
    SDL_free(as);
}
