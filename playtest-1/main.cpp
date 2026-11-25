/**
* Author: Yuvraj Chandyok
* Assignment: Escape Room
* Date due: December 7, 2026, 2:00pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

/*
    ESCAPE ROOM

    Controls
        ENTER     start (menu) / back to the menu (win or lose screen)
        W A S D   walk
        E         use a task station / step away from it
        A, D      (valve task) turn the valve, one key after the other
        SPACE     (gauge task) stop the needle in the green
        1         jump straight to level 1 (debugging)
        Q         quit

    Lava is flooding the room one tile at a time. Finish every task to unlock
    the door and get out. Messing up a task makes the lava hotter: it changes
    colour and spreads faster. Three lives for the whole game. Touching the
    lava or a slime costs one and restarts the room.

    Assets:
        tiles, door, stations - "Tiny Dungeon" by Kenney (www.kenney.nl), CC0
        player and enemies    - "Tiny 16: Basic" by Lanea Zimmerman (Sharm),
                                CC-BY 3.0, opengameart.org/content/tiny-16-basic
        hearts                - "Platformer Art Deluxe" by Kenney, CC0
        lava tile             - drawn for this game
        music                 - "Retro Game Music Pack" by Juhani Junkala, CC0
        sounds                - "The Essential Retro Video Game Sound Effects
                                Collection" by Juhani Junkala, CC0
        font                  - Kenney Pixel from "Kenney Fonts" by Kenney, CC0,
                                used for the text images
*/

#include "CS3113/TextScene.h"
#include "CS3113/Level1.h"

// Global Constants
constexpr int SCREEN_WIDTH  = 1000,
              SCREEN_HEIGHT = 600,
              FPS           = 120,
              MAX_LIVES     = 3;

constexpr Vector2 ORIGIN = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 };

constexpr float FIXED_TIMESTEP = 1.0f / 60.0f,
                MUSIC_VOLUME   = 0.4f;

constexpr Vector2 HEART_SIZE      = { 40.0f, 34.0f },
                  FIRST_HEART_POS = { 40.0f, 35.0f };
constexpr float   HEART_SPACING   = 48.0f;

// Global Variables
AppStatus gAppStatus       = RUNNING;
float     gPreviousTicks   = 0.0f,
          gTimeAccumulator = 0.0f;

Camera2D gCamera = {};

Scene                     *gCurrentScene   = nullptr;
SceneID                    gCurrentSceneID = NO_SCENE;
SceneID                    gRequestedScene = NO_SCENE; // set by a key press
std::map<SceneID, Scene *> gScenes         = {};

int gLives = MAX_LIVES;   // for the whole game, not per level

Entity *gFullHearts  = nullptr;
Entity *gEmptyHearts = nullptr;

Music gMusic;
Sound gHurtSound,
      gLevelClearSound,
      gWinSound,
      gLoseSound;

// Function Declarations
void switchToScene(SceneID id);
Vector2 cameraTargetFor(GameState state);
void handleSceneChanges();
void initialise();
void processInput();
void update();
void render();
void shutdown();

void switchToScene(SceneID id)
{
    if (gCurrentScene) gCurrentScene->shutdown();

    gCurrentSceneID = id;
    gCurrentScene   = gScenes[id];
    gCurrentScene->initialise();

    // jump straight to where the camera belongs; no panning in from elsewhere
    gCamera.target = cameraTargetFor(gCurrentScene->getState());
}

// levels: follow the player up and down, but stop at the map's top and
// bottom so the camera never shows past the room. A room that is exactly one
// screen big never moves the camera at all. Text scenes: the centre.
Vector2 cameraTargetFor(GameState state)
{
    if (state.player == nullptr) return ORIGIN;

    float highest = state.map->getTopBoundary()    + ORIGIN.y;
    float lowest  = state.map->getBottomBoundary() - ORIGIN.y;
    float cameraY = state.player->getPosition().y;

    if (cameraY < highest) cameraY = highest;
    if (cameraY > lowest)  cameraY = lowest;

    return { ORIGIN.x, cameraY };
}

// what happened this frame decides which scene comes next
void handleSceneChanges()
{
    SceneID   next  = gRequestedScene;
    GameState state = gCurrentScene->getState();

    if (next == NO_SCENE && state.playerDied)
    {
        gLives--;

        if (gLives <= 0)
        {
            next = LOSE;
            PlaySound(gLoseSound);
        }
        else
        {
            next = gCurrentSceneID;   // same level again, from the start
            PlaySound(gHurtSound);
        }
    }
    else if (next == NO_SCENE && state.nextSceneID != NO_SCENE)
    {
        next = state.nextSceneID;
        PlaySound(next == WIN ? gWinSound : gLevelClearSound);
    }

    gRequestedScene = NO_SCENE;

    if (next != NO_SCENE) switchToScene(next);
}

void initialise()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Escape Room");
    InitAudioDevice();

    gMusic = LoadMusicStream("assets/game/music.ogg");
    SetMusicVolume(gMusic, MUSIC_VOLUME);
    PlayMusicStream(gMusic);

    gHurtSound       = LoadSound("assets/game/hurt.wav");
    gLevelClearSound = LoadSound("assets/game/level_clear.wav");
    gWinSound        = LoadSound("assets/game/win.wav");
    gLoseSound       = LoadSound("assets/game/lose.wav");

    /*
        ----------- SCENES -----------
    */
    gScenes[MENU]    = new TextScene(ORIGIN, "#1E1412", "assets/game/menu.png", { 800.0f, 470.0f });
    gScenes[LEVEL_1] = new Level1(ORIGIN, "#1E1412", WIN);
    gScenes[WIN]     = new TextScene(ORIGIN, "#1F3D2A", "assets/game/win.png",  { 800.0f, 220.0f });
    gScenes[LOSE]    = new TextScene(ORIGIN, "#3D1F24", "assets/game/lose.png", { 800.0f, 220.0f });

    /*
        ----------- LIVES (drawn on top of the level, not in it) -----------
    */
    gFullHearts  = new Entity[MAX_LIVES];
    gEmptyHearts = new Entity[MAX_LIVES];

    for (int i = 0; i < MAX_LIVES; i++)
    {
        Vector2 heartPosition = {
            FIRST_HEART_POS.x + i * HEART_SPACING, FIRST_HEART_POS.y
        };

        gFullHearts[i].setTexture("assets/game/heart_full.png");
        gFullHearts[i].setScale(HEART_SIZE);
        gFullHearts[i].setPosition(heartPosition);

        gEmptyHearts[i].setTexture("assets/game/heart_empty.png");
        gEmptyHearts[i].setScale(HEART_SIZE);
        gEmptyHearts[i].setPosition(heartPosition);
    }

    gCamera.offset   = ORIGIN;
    gCamera.rotation = 0.0f;
    gCamera.zoom     = 1.0f;

    switchToScene(MENU);

    SetTargetFPS(FPS);
}

void processInput()
{
    if (IsKeyPressed(KEY_Q) || WindowShouldClose()) gAppStatus = TERMINATED;

    // ENTER: the menu starts a new game; the end screens go back to the menu.
    // Both put the lives back to full.
    if (IsKeyPressed(KEY_ENTER))
    {
        if (gCurrentSceneID == MENU)
        {
            gLives          = MAX_LIVES;
            gRequestedScene = LEVEL_1;
        }
        else if (gCurrentSceneID == WIN || gCurrentSceneID == LOSE)
        {
            gLives          = MAX_LIVES;
            gRequestedScene = MENU;
        }
    }

    // debugging: jump straight to a level
    if (IsKeyPressed(KEY_ONE)) gRequestedScene = LEVEL_1;

    // a debug jump out of the lose screen still needs a life to play with
    if (gRequestedScene != NO_SCENE && gRequestedScene != MENU && gLives <= 0)
        gLives = MAX_LIVES;

    // walking and the tasks are handled by the level itself
    gCurrentScene->processInput();
}

void update()
{
    UpdateMusicStream(gMusic);   // the music keeps looping across every scene

    // Delta time
    float ticks     = static_cast<float>(GetTime());
    float deltaTime = ticks - gPreviousTicks;
    gPreviousTicks  = ticks;

    // Fixed timestep
    deltaTime += gTimeAccumulator;

    if (deltaTime < FIXED_TIMESTEP)
    {
        gTimeAccumulator = deltaTime;
        return;
    }

    while (deltaTime >= FIXED_TIMESTEP)
    {
        gCurrentScene->update(FIXED_TIMESTEP);
        deltaTime -= FIXED_TIMESTEP;

        Vector2 cameraTarget = cameraTargetFor(gCurrentScene->getState());
        panCamera(&gCamera, &cameraTarget);
    }

    gTimeAccumulator = deltaTime;
}

void render()
{
    BeginDrawing();
    BeginMode2D(gCamera);

    gCurrentScene->render();

    EndMode2D();

    // the hearts are drawn after the camera, so they stay put on screen
    if (gCurrentScene->getState().player)
        for (int i = 0; i < MAX_LIVES; i++)
        {
            if (i < gLives) gFullHearts[i].render();
            else            gEmptyHearts[i].render();
        }

    EndDrawing();
}

void shutdown()
{
    for (std::pair<const SceneID, Scene *> &entry : gScenes)
        delete entry.second;

    gScenes.clear();

    delete[] gFullHearts;
    delete[] gEmptyHearts;

    UnloadMusicStream(gMusic);
    UnloadSound(gHurtSound);
    UnloadSound(gLevelClearSound);
    UnloadSound(gWinSound);
    UnloadSound(gLoseSound);

    CloseAudioDevice();
    CloseWindow();
}

int main(void)
{
    initialise();

    while (gAppStatus == RUNNING)
    {
        processInput();
        update();
        handleSceneChanges();
        render();
    }

    shutdown();

    return 0;
}
