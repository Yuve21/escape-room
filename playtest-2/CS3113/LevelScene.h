#ifndef LEVELSCENE_H
#define LEVELSCENE_H

#include "Scene.h"
#include "ShaderProgram.h"

enum TaskType { VALVE_TASK, GAUGE_TASK };

// What every room shares: the player, the walls, the enemies, the task
// stations, the locked door and the lava that floods in one tile at a time.
// Each level only fills in its own initialise(): its map, where things go and
// how hard its tasks and lava are.
class LevelScene : public Scene
{
public:
    static constexpr float TILE_DIMENSION   = 50.0f,
                           PLAYER_SIZE      = 50.0f,
                           STATION_SIZE     = 50.0f,
                           INTERACT_RANGE   = 75.0f,  // how close to a station E works
                           LAVA_GROW_TIME   = 0.35f,  // a new lava tile swells in over this long
                           BANNER_TIME      = 1.6f,   // how long a message stays up
                           STEP_TIME        = 0.3f,   // seconds between footsteps
                           GAUGE_SWING      = 70.0f,  // the needle swings this far each way (degrees)
                           WIDE_ZONE_HALF   = 16.0f,  // half the green zone, matches zone_wide.png
                           NARROW_ZONE_HALF = 9.0f;   // half the green zone, matches zone_narrow.png

    static constexpr int   TILESET_COLUMNS = 4,
                           TILESET_ROWS    = 1,
                           WALL_TILE       = 4,      // tiles.png: 1-3 floor, 4 wall
                           MAX_CELLS       = 400,    // biggest room: 20 x 20 tiles
                           MAX_STATIONS    = 6,
                           MAX_HEAT        = 2,      // 0 orange, 1 red, 2 violet
                           PLAYER_SPEED    = 180,
                           WANDERER_SPEED  = 70;

protected:
    SceneID mNextLevel;   // where the door takes you

    // ----- how hard the room is (each level sets these) ----- //
    float mSpreadIntervals[MAX_HEAT + 1] = { 2.0f, 1.3f, 0.8f }; // seconds per lava step
    int   mValveTapsNeeded  = 12;
    float mValveTimeLimit   = 6.0f;
    float mGaugeSwingSpeed  = 2.2f;
    int   mGaugeHitsNeeded  = 2;
    bool  mNarrowGauge      = false;

    // ----- the room ----- //
    Map                      *mFloor = nullptr;   // drawn under the walls, never solid
    std::vector<unsigned int> mFloorData;
    int                       mDoorColumn = 0,
                              mDoorRow    = 0;
    bool                      mDoorUnlocked = false;
    Entity                   *mOpenDoor     = nullptr;

    // ----- the lava ----- //
    float   mLavaAge[MAX_CELLS];   // seconds each tile has been lava, -1 if it is not
    float   mSpreadTimer = 0.0f;
    int     mHeat        = 0;
    Entity *mLavaTile    = nullptr; // one tile, drawn at every lava cell
    ShaderProgram mLavaShader;

    // ----- the task stations ----- //
    Entity  *mStations     = nullptr;
    int      mStationCount = 0;
    TaskType mStationTypes[MAX_STATIONS];
    bool     mStationDone[MAX_STATIONS];
    int      mNearStation  = -1;   // the station E would use right now
    int      mOpenStation  = -1;   // the station whose panel is open

    // what the open task is up to
    float mTaskTimer    = 0.0f;
    int   mValveTaps    = 0;
    int   mLastValveKey = 0;
    float mGaugeTime    = 0.0f;
    int   mGaugeHits    = 0;
    int   mZoneIndex    = 0;

    // ----- things drawn on top of the room ----- //
    Entity *mStationOnTop = nullptr,   // the glow on a finished station
           *mKeyPrompt    = nullptr,   // "E" over the station you can use
           *mIconOff      = nullptr,   // task icons in the corner
           *mIconOn       = nullptr,
           *mValvePanel   = nullptr,
           *mGaugePanel   = nullptr,
           *mValveWheel   = nullptr,
           *mGaugeDial    = nullptr,
           *mGaugeZone    = nullptr,
           *mGaugeNeedle  = nullptr,
           *mBarBack      = nullptr,
           *mBarGreen     = nullptr,
           *mBarOrange    = nullptr,
           *mHotterBanner = nullptr,
           *mDoneBanner   = nullptr,
           *mOpenBanner   = nullptr,
           *mShownBanner  = nullptr;   // whichever message is up, if any
    float   mBannerTimer  = 0.0f;

    // ----- sounds ----- //
    bool  mSoundsLoaded = false;
    Sound mOpenSound, mCrankSound, mHitSound, mHotterSound,
          mTaskDoneSound, mDoorSound, mStepSound;
    float mStepTimer = 0.0f;   // a footstep plays every STEP_TIME while walking

    // ----- building a room ----- //
    void resetRoom();
    void createMap(int columns, int rows, unsigned int *levelData);
    void createPlayer(int column, int row);
    void createDoor(int column, int row);
    void addLavaSource(int column, int row);
    void createStations(int count);
    void setUpStation(int index, TaskType type, int column, int row);
    void createEnemies(int count);
    void setUpEnemy(int index, AIType type, int column, int row, float patrolTiles);
    void createInterface();

    // ----- while playing ----- //
    void spreadLava();
    bool isLavaAt(Vector2 position) const;
    void openTask(int station);
    void closeTask();
    void taskInput();
    void updateTask(float deltaTime);
    void completeTask();
    void heatUp();
    void showBanner(Entity *banner);
    void drawBar(Entity *fill, float fraction, Vector2 centre, float width);
    float gaugeAngle() const;
    float zoneAngle() const;

    // the centre of a tile, measured from the map's own edges
    Vector2 tileCentre(int column, int row) const;
    int     cellIndex(int column, int row) const;

public:
    LevelScene();
    LevelScene(Vector2 origin, const char *bgHexCode, SceneID nextLevel);
    ~LevelScene();

    void processInput() override;
    void update(float deltaTime) override;
    void render() override;
    void shutdown() override;
};

#endif // LEVELSCENE_H
