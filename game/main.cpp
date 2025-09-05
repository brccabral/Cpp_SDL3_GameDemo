#include <algorithm>
#include <array>
#include <filesystem>
#include <print>
#include <string>
#include <vector>
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <autorelease/AutoRelease.hpp>

#include "gameobject.hpp"
#include "tmx.hpp"

template <>
struct std::formatter<SDL_FRect>
{
    static constexpr auto parse(const std::format_parse_context& ctx) { return ctx.begin(); }

    static auto format(const SDL_FRect& r, std::format_context& ctx)
    {
        return std::format_to(ctx.out(), "[x: {} y: {} w: {} h: {}]", r.x, r.y, r.w, r.h);
    }
};

template <>
struct std::formatter<glm::vec2>
{
    static constexpr auto parse(const std::format_parse_context& ctx) { return ctx.begin(); }

    static auto format(const glm::vec2& v, std::format_context& ctx)
    {
        return std::format_to(ctx.out(), "[x: {} y: {}]", v.x, v.y);
    }
};

typedef struct SDLState
{
    AutoRelease<bool> sdl_init;
    AutoRelease<SDL_Window*> window;
    AutoRelease<SDL_Renderer*> renderer;
    AutoRelease<bool> mix_init;
    AutoRelease<MIX_Mixer*> mixer;
    int width{}, height{};
    int logW{}, logH{}; // logical width/height
    const bool* keys{};
    uint64_t prevTime{};
    bool fullscreen{};

    ~SDLState() = default;
} SDLState;

struct GameState
{
    std::vector<std::vector<GameObject>> layers{};
    std::vector<GameObject> bullets{};
    int playerLayer{};

    int playerIndex = -1;
    SDL_FRect mapViewport{};
    float bg2Scroll{}, bg3Scroll{}, bg4Scroll{};
    bool debugMode{};

    GameState() : GameState(640, 480, 480)
    {
    }

    explicit GameState(const float viewPortWidth, const float viewPortHeight, const float mapHeight)
    {
        mapViewport = {0, mapHeight - viewPortHeight, viewPortWidth, viewPortHeight};
    }

    GameObject& player() { return layers[playerLayer][playerIndex]; }
};

struct Sound
{
    AutoRelease<MIX_Audio*> audio{};
    AutoRelease<MIX_Track*> track{};
    AutoRelease<SDL_PropertiesID> options{};

    Sound(MIX_Mixer* mixer, const std::string& filepath, const int loops)
    {
        audio = {MIX_LoadAudio(mixer, filepath.c_str(), false), MIX_DestroyAudio};
        if (!audio)
        {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", SDL_GetError(), nullptr);
            throw std::runtime_error("Error");
        }
        track = {MIX_CreateTrack(mixer), MIX_DestroyTrack};
        if (!track)
        {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", SDL_GetError(), nullptr);
            throw std::runtime_error("Error");
        }
        if (!MIX_SetTrackAudio(track, audio))
        {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", SDL_GetError(), nullptr);
            throw std::runtime_error("Error");
        }

        options = {SDL_CreateProperties(), SDL_DestroyProperties};
        if (!options)
        {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", SDL_GetError(), nullptr);
            throw std::runtime_error("Error");
        }
        SDL_SetNumberProperty(options, MIX_PROP_PLAY_LOOPS_NUMBER, loops);
    }

    bool play() const
    {
        return MIX_PlayTrack(track, options);
    }

    bool setGain(const float gain) const
    {
        return MIX_SetTrackGain(track, gain);
    }
};

struct TileSetTextures
{
    int firstgid{};
    std::vector<SDL_Texture*> textures{};
};

struct Resources
{
    // player
    const int ANIM_PLAYER_IDLE = 0;
    const int ANIM_PLAYER_RUNNING = 1;
    const int ANIM_PLAYER_SLIDE = 2;
    const int ANIM_PLAYER_SHOOT = 3;
    const int ANIM_PLAYER_SLIDE_SHOOT = 4;
    std::vector<Animation> playerAnims;

    // bullet
    const int ANIM_BULLET_MOVING = 0;
    const int ANIM_BULLET_HIT = 1;
    std::vector<Animation> bulletAnims;

    // enemy
    const int ANIM_ENEMY = 0;
    const int ANIM_ENEMY_HIT = 1;
    const int ANIM_ENEMY_DIE = 2;
    std::vector<Animation> enemyAnims;

    std::vector<AutoRelease<SDL_Texture*>> textures;

    // player
    SDL_Texture* texIdle{};
    SDL_Texture* texRun{};
    SDL_Texture* texSlide{};
    // player shooting
    SDL_Texture* texShoot{}; // idle
    SDL_Texture* texRunShoot{};
    SDL_Texture* texSlideShoot{};

    // tiles
    SDL_Texture* texBrick{};
    SDL_Texture* texGrass{};
    SDL_Texture* texGround{};
    SDL_Texture* texPanel{};

    // backgrounds
    SDL_Texture* texBg1{};
    SDL_Texture* texBg2{};
    SDL_Texture* texBg3{};
    SDL_Texture* texBg4{};

    // bullets
    SDL_Texture* texBullet{};
    SDL_Texture* texBulletHit{};

    // enemy
    SDL_Texture* texEnemy{};
    SDL_Texture* texEnemyHit{};
    SDL_Texture* texEnemyDie{};

    // Audio
    std::vector<Sound> sounds{};

    typedef size_t Sound_ID;

    Sound_ID music{};
    Sound_ID enemy_hit{};
    Sound_ID enemy_die{};
    Sound_ID shoot{};

    // Tiled map
    std::unique_ptr<tmx::Map> map{};
    std::vector<TileSetTextures> tileSetTextures{};

    SDL_Texture* loadTexture(SDL_Renderer* renderer, const std::string& filepath)
    {
        AutoRelease<SDL_Texture*> tex = {IMG_LoadTexture(renderer, filepath.c_str()), SDL_DestroyTexture};
        if (tex == nullptr)
        {
            throw std::runtime_error("Failed to load " + std::string(filepath));
        }
        SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
        textures.push_back(std::move(tex));
        return textures.back();
    }

    Sound_ID loadAudio(MIX_Mixer* mixer, const std::string& filepath, int loops)
    {
        sounds.emplace_back(mixer, filepath, loops);
        return sounds.size() - 1;
    }

    void load(const SDLState* state)
    {
        playerAnims.resize(5);
        playerAnims[ANIM_PLAYER_IDLE] = Animation{8, 1.6f};
        playerAnims[ANIM_PLAYER_RUNNING] = Animation{4, 0.5f};
        playerAnims[ANIM_PLAYER_SLIDE] = Animation{1, 1.0f};
        playerAnims[ANIM_PLAYER_SHOOT] = Animation{4, 0.5f};
        playerAnims[ANIM_PLAYER_SLIDE_SHOOT] = Animation{4, 0.5f};

        bulletAnims.resize(2);
        bulletAnims[ANIM_BULLET_MOVING] = Animation{4, 0.05f};
        bulletAnims[ANIM_BULLET_HIT] = Animation{4, 0.15f};

        enemyAnims.resize(3);
        enemyAnims[ANIM_ENEMY] = Animation{8, 1.0f};
        enemyAnims[ANIM_ENEMY_HIT] = Animation{8, 1.0f};
        enemyAnims[ANIM_ENEMY_DIE] = Animation{18, 2.0f};

        texIdle = loadTexture(state->renderer, "data/idle.png");
        texRun = loadTexture(state->renderer, "data/run.png");
        texSlide = loadTexture(state->renderer, "data/slide.png");
        texShoot = loadTexture(state->renderer, "data/shoot.png");
        texRunShoot = loadTexture(state->renderer, "data/shoot_run.png");
        texSlideShoot = loadTexture(state->renderer, "data/slide_shoot.png");
        texBrick = loadTexture(state->renderer, "data/tiles/brick.png");
        texGrass = loadTexture(state->renderer, "data/tiles/grass.png");
        texGround = loadTexture(state->renderer, "data/tiles/ground.png");
        texPanel = loadTexture(state->renderer, "data/tiles/panel.png");
        texBg1 = loadTexture(state->renderer, "data/bg/bg_layer1.png");
        texBg2 = loadTexture(state->renderer, "data/bg/bg_layer2.png");
        texBg3 = loadTexture(state->renderer, "data/bg/bg_layer3.png");
        texBg4 = loadTexture(state->renderer, "data/bg/bg_layer4.png");
        texBullet = loadTexture(state->renderer, "data/bullet.png");
        texBulletHit = loadTexture(state->renderer, "data/bullet_hit.png");
        texEnemy = loadTexture(state->renderer, "data/enemy.png");
        texEnemyHit = loadTexture(state->renderer, "data/enemy_hit.png");
        texEnemyDie = loadTexture(state->renderer, "data/enemy_die.png");

        sounds.reserve(4);
        music = loadAudio(state->mixer, "data/audio/Juhani Junkala [Retro Game Music Pack] Level 1.mp3", -1);
        enemy_hit = loadAudio(state->mixer, "data/audio/enemy_hit.wav", 0);
        enemy_die = loadAudio(state->mixer, "data/audio/monster_die.wav", 0);
        shoot = loadAudio(state->mixer, "data/audio/shoot.wav", 0);

        // map = tmx::loadMap("data/maps/smallmap.tmx");
        // map = tmx::loadMap("data/maps/bigmap.tmx");
        map = tmx::loadMap("data/maps/original.tmx");
        if (!map)
        {
            throw std::runtime_error("Error loading map.");
        }
        for (tmx::TileSet& tileSet : map->tileSets)
        {
            TileSetTextures tst;
            tst.firstgid = tileSet.firstgid;
            tst.textures.reserve(tileSet.tiles.size());

            for (const auto& [id, image] : tileSet.tiles)
            {
                const std::string imagePath = "data/tiles/" + std::filesystem::path(image.source).filename().
                    string();
                tst.textures.push_back(loadTexture(state->renderer, imagePath));
            }

            tileSetTextures.push_back(std::move(tst));
        }
    }

    bool playSound(const Sound_ID sound_id) const
    {
        return sounds.at(sound_id).play();
    }

    bool setSoundGain(const Sound_ID sound_id, const float gain) const
    {
        return sounds.at(sound_id).setGain(gain);
    }

    ~Resources() = default;
};

typedef struct AppState
{
    SDLState sdlState{};
    GameState gameState{};
    Resources resources{};
} AppState;

void drawObject(const SDLState* state, const GameState* gs, GameObject& obj, float width, float height,
                float deltaTime);
void update(const SDLState* state, GameState* gs, const Resources* res, GameObject& obj, float deltaTime);
void createTiles(const SDLState* state, GameState* gs, const Resources* res);
void checkCollision(const Resources* res, GameObject& objA, GameObject& objB, bool isHorizontal);
void collisionResponse(const Resources* res, const SDL_FRect& rectB, GameObject& a, GameObject& b, bool isHorizontal);
void drawParallaxBackground(SDL_Renderer* renderer, SDL_Texture* texture, float xVelocity, float& scrollPos,
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
    auto* ss = &as->sdlState;
    auto* res = &as->resources;

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
    SDL_SetRenderVSync(ss->renderer, 1);

    // configure presentation
    // SDL will scale the final render buffer for us
    // SDL_LOGICAL_PRESENTATION_LETTERBOX keeps aspect ratio logW/logH, adding black banners as needed in SDL window
    ss->logW = 640;
    ss->logH = 320;
    SDL_SetRenderLogicalPresentation(ss->renderer, ss->logW, ss->logH, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    ss->keys = SDL_GetKeyboardState(nullptr);

    // Mixer
    ss->mix_init = {MIX_Init(), [](const bool&) { MIX_Quit(); }};
    if (!ss->mix_init)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", SDL_GetError(), nullptr);
        return SDL_APP_FAILURE;
    }
    ss->mixer = {MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr), MIX_DestroyMixer};
    if (!ss->mixer)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", SDL_GetError(), nullptr);
        return SDL_APP_FAILURE;
    }

    res->load(ss);
    res->setSoundGain(res->music, 0.333f);
    if (!res->playSound(res->music))
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", SDL_GetError(), ss->window);
        return SDL_APP_FAILURE;
    }

    auto* gs = &as->gameState;
    *gs = GameState(ss->logW, ss->logH, res->map->mapHeight * res->map->tileHeight);
    createTiles(ss, gs, res);

    // force double buffer allocate memory
    SDL_SetRenderDrawColor(ss->renderer, 0, 0, 0, 255);
    SDL_RenderClear(ss->renderer);
    SDL_RenderPresent(ss->renderer);
    SDL_RenderClear(ss->renderer);
    SDL_RenderPresent(ss->renderer);

    // getTicks() start with SDL_Init, but we spent time loading resources, so, getTicks() before first deltaTime
    ss->prevTime = SDL_GetTicks();
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    auto* ss = &((AppState*)appstate)->sdlState;
    auto* gs = &((AppState*)appstate)->gameState;

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
    case SDL_EVENT_KEY_UP:
        {
            if (event->key.scancode == SDL_SCANCODE_F12)
            {
                gs->debugMode = !gs->debugMode;
            }
            if (event->key.scancode == SDL_SCANCODE_F11)
            {
                ss->fullscreen = !ss->fullscreen;
                SDL_SetWindowFullscreen(ss->window, ss->fullscreen);
            }
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
    auto* ss = &((AppState*)appstate)->sdlState;
    auto* gs = &((AppState*)appstate)->gameState;
    const auto* res = &((AppState*)appstate)->resources;

    const uint64_t nowTime = SDL_GetTicks();
    const float deltaTime = (float)(nowTime - ss->prevTime) / 1000.0f;
    ss->prevTime = nowTime;

    // calculate viewport position
    gs->mapViewport.x = gs->player().position.x + res->map->tileWidth / 2.0f - gs->mapViewport.w / 2.0f;

    // Draw
    SDL_SetRenderDrawColor(ss->renderer, 20, 10, 30, 255);
    SDL_RenderClear(ss->renderer);

    SDL_RenderTexture(ss->renderer, res->texBg1, nullptr, nullptr);
    drawParallaxBackground(ss->renderer, res->texBg4, gs->player().velocity.x, gs->bg4Scroll, 0.075f, deltaTime);
    drawParallaxBackground(ss->renderer, res->texBg3, gs->player().velocity.x, gs->bg3Scroll, 0.150f, deltaTime);
    drawParallaxBackground(ss->renderer, res->texBg2, gs->player().velocity.x, gs->bg2Scroll, 0.3f, deltaTime);

    // update
    for (auto& layer : gs->layers)
    {
        for (auto& obj : layer)
        {
            if (obj.dynamic)
            {
                update(ss, gs, res, obj, deltaTime);
            }
        }
    }

    for (auto& bullet : gs->bullets)
    {
        update(ss, gs, res, bullet, deltaTime);
    }


    // draw
    for (auto& layer : gs->layers)
    {
        for (auto& obj : layer)
        {
            drawObject(ss, gs, obj, res->map->tileWidth, res->map->tileHeight, deltaTime);
        }
    }

    for (auto& bullet : gs->bullets)
    {
        if (bullet.data.bullet.state != BulletState::inactive)
        {
            drawObject(ss, gs, bullet, bullet.collider.w, bullet.collider.h, deltaTime);
        }
    }

    if (gs->debugMode)
    {
        SDL_SetRenderDrawColor(ss->renderer, 255, 255, 255, 255);
        SDL_RenderDebugText(ss->renderer, 5, 5,
                            std::format("S: {} B: {} G: {} D: {} dt: {} FPS: {}",
                                        static_cast<int>(gs->player().data.player.state),
                                        gs->bullets.size(),
                                        gs->player().grounded,
                                        gs->player().direction,
                                        deltaTime,
                                        1.0f / deltaTime
                            ).c_str()
        );

        SDL_RenderDebugText(ss->renderer, 5, 15,
                            std::format("Rect: {}", gs->player().GetCollider()).c_str()
        );
        SDL_RenderDebugText(ss->renderer, 5, 25,
                            std::format("Vel: {}", gs->player().velocity).c_str()
        );
        SDL_RenderDebugText(ss->renderer, 5, 35,
                            std::format("View: {}", gs->mapViewport).c_str()
        );
    }

    SDL_RenderPresent(ss->renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    auto* as = (AppState*)appstate;
    as->~AppState();
    SDL_free(as);
}

void drawObject(const SDLState* state, const GameState* gs, GameObject& obj, const float width, const float height,
                const float deltaTime)
{
    SDL_FRect src{.x = 0, .y = 0, .w = width, .h = height};

    // if currentAnimation == -1, draw the specific frame index spriteFrame
    src.x = obj.currentAnimation >= 0
                ? obj.animations[obj.currentAnimation].currentFrame() * width
                : (obj.spriteFrame - 1) * width;

    const SDL_FRect dst{
        .x = obj.position.x - gs->mapViewport.x, .y = obj.position.y - gs->mapViewport.y, .w = width, .h = height
    };

    const SDL_FlipMode flipMode = obj.direction < 0 ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

    if (!obj.shouldFlash)
    {
        SDL_RenderTextureRotated(state->renderer, obj.texture, &src, &dst, 0, nullptr,
                                 flipMode);
    }
    else
    {
        // flash object with a red-ish tint
        SDL_SetTextureColorModFloat(obj.texture, 2.5f, 1.0f, 1.0f);
        SDL_RenderTextureRotated(state->renderer, obj.texture, &src, &dst, 0, nullptr,
                                 flipMode);
        SDL_SetTextureColorModFloat(obj.texture, 1.5f, 1.0f, 1.0f);

        // check if timer has finished
        if (obj.flashTimer.step(deltaTime))
        {
            obj.shouldFlash = false;
        }
    }

    if (gs->debugMode)
    {
        SDL_SetRenderDrawBlendMode(state->renderer, SDL_BLENDMODE_BLEND);

        // collision
        const SDL_FRect rectA = {
            obj.position.x + obj.collider.x - gs->mapViewport.x,
            obj.position.y + obj.collider.y - gs->mapViewport.y,
            obj.collider.w,
            obj.collider.h,
        };
        SDL_SetRenderDrawColor(state->renderer, 255, 0, 0, 150);
        SDL_RenderFillRect(state->renderer, &rectA);

        // ground sensor
        const SDL_FRect ground_sensor{
            .x = obj.position.x + obj.collider.x - gs->mapViewport.x,
            .y = obj.position.y + obj.collider.y + obj.collider.h - gs->mapViewport.y,
            .w = obj.collider.w, .h = 1
        };
        SDL_SetRenderDrawColor(state->renderer, 0, 0, 255, 150);
        SDL_RenderFillRect(state->renderer, &ground_sensor);

        SDL_SetRenderDrawBlendMode(state->renderer, SDL_BLENDMODE_NONE);
    }
}

void update(const SDLState* state, GameState* gs, const Resources* res, GameObject& obj, const float deltaTime)
{
    if (obj.currentAnimation >= 0)
    {
        obj.animations[obj.currentAnimation].step(deltaTime);
    }

    // apply some gravity
    if (obj.dynamic)
    {
        obj.velocity += glm::vec2(0, 500) * deltaTime;
    }

    float currentDirection = 0;
    if (obj.type == ObjectType::player)
    {
        if (state->keys[SDL_SCANCODE_A])
        {
            currentDirection += -1;
        }
        if (state->keys[SDL_SCANCODE_D])
        {
            currentDirection += 1;
        }

        const auto handleJump = [&]()
        {
            if (state->keys[SDL_SCANCODE_K] && obj.grounded)
            {
                constexpr float JUMP_FORCE = -200.0f;
                obj.velocity.y += JUMP_FORCE;
                obj.data.player.state = PlayerState::jumping;
                obj.grounded = false;
            }
        };

        Timer& weaponTimer = obj.data.player.weaponTimer;
        weaponTimer.step(deltaTime);
        const auto handleShooting = [&](SDL_Texture* tex, SDL_Texture* shootTex, const int animIndex,
                                        const int shootAnimIndex)
        {
            if (state->keys[SDL_SCANCODE_J])
            {
                // set shooting tex/anim
                obj.texture = shootTex;
                obj.currentAnimation = shootAnimIndex;

                if (weaponTimer.isTimeout())
                {
                    weaponTimer.reset();
                    GameObject bullet;
                    bullet.type = ObjectType::bullet;
                    bullet.data.bullet = BulletData();
                    bullet.direction = gs->player().direction;
                    bullet.texture = res->texBullet;
                    bullet.currentAnimation = res->ANIM_BULLET_MOVING;
                    bullet.collider = {
                        0, 0, static_cast<float>(res->texBullet->h), static_cast<float>(res->texBullet->h)
                    };
                    // bullets have random Y velocity
                    constexpr Sint32 yVariation = 40.f;
                    const Sint32 yVel = SDL_rand(yVariation) - yVariation / 2;
                    bullet.velocity = glm::vec2(obj.velocity.x + 600.0f * obj.direction, yVel);
                    bullet.animations = res->bulletAnims;
                    bullet.maxSpeedX = 1000.0f;

                    // adjust bullet position (lerp)
                    constexpr float left = 0;
                    const float right = res->map->tileWidth - bullet.collider.w;
                    const float t = (obj.direction + 1) / 2.0f; // 0 to 1
                    const float xOffset = left + right * t;
                    bullet.position = glm::vec2(obj.position.x + xOffset,
                                                obj.position.y + res->map->tileHeight / 2.0f + 1);

                    // reuse inactive slots
                    bool foundInactive = false;
                    for (auto& bullet_obj : gs->bullets)
                    {
                        if (bullet_obj.data.bullet.state == BulletState::inactive)
                        {
                            foundInactive = true;
                            bullet_obj = std::move(bullet);
                            break;
                        }
                    }
                    // if no inactive, push as new one
                    if (!foundInactive)
                    {
                        gs->bullets.push_back(std::move(bullet));
                    }
                    res->playSound(res->shoot);
                }
            }
            else
            {
                obj.texture = tex;
                obj.currentAnimation = animIndex;
            }
        };

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
                        const float amount = factor * obj.acceleration.x * deltaTime;
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
                handleJump();
                handleShooting(res->texIdle, res->texShoot, res->ANIM_PLAYER_IDLE, res->ANIM_PLAYER_SHOOT);
                break;
            }
        case PlayerState::running:
            {
                if (currentDirection == 0)
                {
                    obj.data.player.state = PlayerState::idle;
                }
                handleJump();

                // moving in opposite direction of velocity, sliding! (changing direction during move)
                if (obj.velocity.x * obj.direction < 0 && obj.grounded)
                {
                    handleShooting(res->texSlide, res->texSlideShoot, res->ANIM_PLAYER_SLIDE,
                                   res->ANIM_PLAYER_SLIDE_SHOOT);
                }
                else
                {
                    // when running, use same index. Both texture have the same size, so, the Animation class
                    // can use the same frameIndex when switching images
                    handleShooting(res->texRun, res->texRunShoot, res->ANIM_PLAYER_RUNNING,
                                   res->ANIM_PLAYER_RUNNING);
                }
                break;
            }
        case PlayerState::jumping:
            {
                handleShooting(res->texRun, res->texRunShoot, res->ANIM_PLAYER_RUNNING,
                               res->ANIM_PLAYER_RUNNING);
                if (obj.grounded)
                {
                    obj.data.player.state = PlayerState::running;
                    // if player stopped running, the next frame will change to idle
                }
                break;
            }
        default:
            {
                break;
            }
        }
    }
    else if (obj.type == ObjectType::bullet)
    {
        switch (obj.data.bullet.state)
        {
        case BulletState::moving:
            {
                if (obj.position.x - gs->mapViewport.x < 0 ||
                    obj.position.x - gs->mapViewport.x > state->logW ||
                    obj.position.y - gs->mapViewport.y < 0 ||
                    obj.position.y - gs->mapViewport.y > state->logH
                )
                {
                    obj.data.bullet.state = BulletState::inactive;
                }
                break;
            }
        case BulletState::colliding:
            {
                if (obj.animations[obj.currentAnimation].isDone())
                {
                    obj.data.bullet.state = BulletState::inactive;
                }
                break;
            }
        default: { break; }
        }
    }
    else if (obj.type == ObjectType::enemy)
    {
        EnemyData& d = obj.data.enemy;
        switch (d.state)
        {
        case EnemyState::shambling:
            {
                const glm::vec2 playerDir = gs->player().position - obj.position;
                if (glm::length(playerDir) < 100)
                {
                    currentDirection = playerDir.x > 0 ? 1 : -1;
                    obj.acceleration = glm::vec2(30, 0);
                }
                else
                {
                    obj.acceleration = glm::vec2{0};
                    obj.velocity.x = 0;
                }
                break;
            }
        case EnemyState::damaged:
            {
                // if damaged timer has finished, go back to shambling
                if (d.damagedTimer.step(deltaTime))
                {
                    d.state = EnemyState::shambling;
                    obj.texture = res->texEnemy;
                    obj.currentAnimation = res->ANIM_ENEMY;
                }
                break;
            }
        case EnemyState::dead:
            {
                obj.velocity.x = 0;
                // when enemy is dead, make it draw only the last frame
                if (obj.currentAnimation != -1 &&
                    obj.animations[obj.currentAnimation].isDone())
                {
                    obj.currentAnimation = -1;
                    obj.spriteFrame = 18;
                }
                break;
            }
        }
    }

    // an object always need a direction
    if (currentDirection != 0)
    {
        obj.direction = currentDirection;
    }
    obj.velocity += currentDirection * obj.acceleration * deltaTime;
    obj.velocity.x = glm::clamp(obj.velocity.x, -obj.maxSpeedX, obj.maxSpeedX);

    // horizontal
    obj.position.x += obj.velocity.x * deltaTime;
    for (auto& layer : gs->layers)
    {
        for (auto& objB : layer)
        {
            if (&obj == &objB || objB.collider.w == 0 || objB.collider.h == 0)
            {
                continue;
            }
            checkCollision(res, obj, objB, true);
        }
    }
    // vertical
    obj.grounded = false;
    obj.position.y += obj.velocity.y * deltaTime;
    for (auto& layer : gs->layers)
    {
        for (auto& objB : layer)
        {
            if (&obj == &objB || objB.collider.w == 0 || objB.collider.h == 0)
            {
                continue;
            }
            checkCollision(res, obj, objB, false);
        }
    }
}

void collisionResponse(const Resources* res, const SDL_FRect& rectB, GameObject& a, GameObject& b,
                       const bool isHorizontal)
{
    const auto genericResponse = [&]()
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
                if (b.type == ObjectType::level)
                {
                    a.grounded = true;
                }
            }
            else if (a.velocity.y < 0)
            {
                a.position.y = rectB.y + rectB.h - a.collider.y;
                a.velocity.y = 0;
            }
        }
    };

    if (a.type == ObjectType::player)
    {
        switch (b.type)
        {
        case ObjectType::level:
            {
                genericResponse();
                break;
            }
        case ObjectType::enemy:
            {
                // bounce player if collides with enemy
                if (b.data.enemy.state != EnemyState::dead)
                {
                    a.velocity = glm::vec2(100, 0) * -a.direction;
                }
                break;
            }
        default: { break; }
        }
    }
    else if (a.type == ObjectType::bullet)
    {
        switch (a.data.bullet.state)
        {
        case BulletState::moving:
            {
                const auto bulletResponse = [&]()
                {
                    genericResponse();
                    a.data.bullet.state = BulletState::colliding;
                    a.texture = res->texBulletHit;
                    a.currentAnimation = res->ANIM_BULLET_HIT;
                    // force velocity 0 bullet changes state on vertical and next frame genericResponse()
                    // is not called for horizontal because of change state
                    a.velocity *= 0;
                };
                switch (b.type)
                {
                case ObjectType::level:
                    {
                        bulletResponse();
                        break;
                    }
                case ObjectType::enemy:
                    {
                        EnemyData& d = b.data.enemy;
                        if (d.state == EnemyState::dead)
                        {
                            break;
                        }
                        b.direction = -a.direction;
                        b.shouldFlash = true;
                        b.flashTimer.reset();
                        b.texture = res->texEnemyHit;
                        b.currentAnimation = res->ANIM_ENEMY_HIT;
                        d.state = EnemyState::damaged;
                        d.healthPoints -= 10;
                        if (d.healthPoints <= 0)
                        {
                            d.state = EnemyState::dead;
                            b.texture = res->texEnemyDie;
                            b.currentAnimation = res->ANIM_ENEMY_DIE;
                            res->playSound(res->enemy_die);
                        }
                        else
                        {
                            res->playSound(res->enemy_hit);
                        }
                        bulletResponse();
                        break;
                    }
                default: { break; }
                }
                break;
            }
        default: { break; }
        }
    }
    else if (a.type == ObjectType::enemy)
    {
        genericResponse();
        if (b.type == ObjectType::player)
        {
            // bounce player if collides with enemy
            if (a.data.enemy.state != EnemyState::dead)
            {
                const int ax = a.position.x + a.collider.x + a.collider.w / 2;
                const int bx = b.position.x + b.collider.x + b.collider.w / 2;
                b.velocity = glm::vec2(100, 0) * (ax > bx ? -1.0f : 1.0f);
            }
        }
    }
}

void checkCollision(const Resources* res, GameObject& objA, GameObject& objB, const bool isHorizontal)
{
    const SDL_FRect rectA = objA.GetCollider();
    const SDL_FRect rectB = objB.GetCollider();
    SDL_FRect rectC{}; // collision result

    if (SDL_GetRectIntersectionFloat(&rectA, &rectB, &rectC) && (rectC.w > 0.00001f && rectC.h > 0.00001f))
    {
        collisionResponse(res, rectB, objA, objB, isHorizontal);
    }
}

void createTiles(const SDLState* state, GameState* gs, const Resources* res)
{
    struct LayerVisitor
    {
        const SDLState* state;
        GameState* gs;
        const Resources* res;

        LayerVisitor(const SDLState* state, GameState* gs, const Resources* res) : state(state), gs(gs), res(res)
        {
        }

        GameObject createObject(const int r, const int c, SDL_Texture* tex, const ObjectType type) const
        {
            GameObject o;
            o.type = type;
            o.position = glm::vec2(c * res->map->tileWidth, r * res->map->tileHeight);
            o.texture = tex;
            o.collider = {0, 0, static_cast<float>(res->map->tileWidth), static_cast<float>(res->map->tileHeight)};
            return o;
        };

        void operator()(const tmx::Layer& layer) const
        {
            std::vector<GameObject> newLayer;
            for (auto r = 0; r < res->map->mapHeight; ++r)
            {
                for (auto c = 0; c < res->map->mapWidth; ++c)
                {
                    // tile global ID
                    const int tGid = layer.data[r * res->map->mapWidth + c];
                    if (!tGid) // 0 = empty tile
                    {
                        continue;
                    }
                    // find the texture for that id
                    const auto itr = std::ranges::find_if(res->tileSetTextures.begin(), res->tileSetTextures.end(),
                                                          [tGid](const TileSetTextures& res_tst)
                                                          {
                                                              return tGid >= res_tst.firstgid && tGid < res_tst.firstgid
                                                                  +
                                                                  res_tst.textures.
                                                                          size();
                                                          }
                    );
                    assert(itr != res->tileSetTextures.end());
                    const auto& [firstgid, textures] = *itr;
                    SDL_Texture* tex = textures[tGid - firstgid];

                    auto tile = createObject(r, c, tex, ObjectType::level);
                    if (layer.name != "Level") // foreground/background
                    {
                        tile.collider.w = tile.collider.h = 0;
                    }

                    newLayer.push_back(std::move(tile));
                }
            }
            gs->layers.push_back(std::move(newLayer));
        }

        void operator()(tmx::ObjectGroup& objectGroup) const
        {
            std::vector<GameObject> newLayer;
            for (tmx::LayerObject& obj : objectGroup.objects)
            {
                glm::vec2 objPos(obj.x - res->map->tileWidth / 2.0f, obj.y - res->map->tileHeight / 2.0f);

                if (obj.type == "player")
                {
                    GameObject player = createObject(1, 1, res->texIdle, ObjectType::player);
                    player.position = objPos;
                    player.data.player = PlayerData();
                    player.animations = res->playerAnims;
                    player.currentAnimation = res->ANIM_PLAYER_IDLE;
                    player.acceleration = glm::vec2(300.f, 0.f);
                    player.maxSpeedX = 100.f;
                    player.dynamic = true;
                    player.collider = {11, 6, 10, 26};

                    gs->playerIndex = newLayer.size();
                    newLayer.push_back(std::move(player));
                    gs->playerLayer = gs->layers.size();
                }
                else if (obj.type == "enemy")
                {
                    GameObject enemy = createObject(1, 1, res->texEnemy, ObjectType::enemy);
                    enemy.position = objPos;
                    enemy.data.enemy = EnemyData();
                    enemy.currentAnimation = res->ANIM_ENEMY;
                    enemy.animations = res->enemyAnims;
                    enemy.collider = {10, 4, 12, 28};
                    enemy.dynamic = true;
                    enemy.maxSpeedX = 15;

                    newLayer.push_back(std::move(enemy));
                }
            }
            gs->layers.push_back(std::move(newLayer));
        }
    };

    LayerVisitor visitor(state, gs, res);
    for (auto& layer : res->map->layers)
    {
        std::visit(visitor, layer);
    }

    assert(gs->playerIndex != -1);
}

void drawParallaxBackground(SDL_Renderer* renderer, SDL_Texture* texture, const float xVelocity, float& scrollPos,
                            const float scrollFactor, const float deltaTime)
{
    scrollPos -= xVelocity * scrollFactor * deltaTime;
    if (scrollPos <= -texture->w)
    {
        scrollPos = 0;
    }

    // double the dest width makes SDL_RenderTextureTiled draw same texture twice
    // avoiding calling renderTexture twice
    const SDL_FRect dst
    {
        scrollPos, 30, texture->w * 2.0f,
        static_cast<float>(texture->h)
    };

    SDL_RenderTextureTiled(renderer, texture, nullptr, 1, &dst);
}
