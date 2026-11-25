/**
* Author: Yuvraj Chandyok
* Assignment: Escape Room
* Date due: December 7, 2026, 2:00pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#include "LevelScene.h"

// where the green zone sits for each hit in a row (degrees from straight up)
static const float ZONE_ANGLES[]  = { 35.0f, -30.0f, 10.0f, -45.0f, 50.0f, -15.0f };
static const int   ZONE_COUNT     = 6;

LevelScene::LevelScene()
    : Scene{ { 0.0f, 0.0f }, "#000000" }, mNextLevel{ NO_SCENE }
{}

LevelScene::LevelScene(Vector2 origin, const char *bgHexCode, SceneID nextLevel)
    : Scene{ origin, bgHexCode }, mNextLevel{ nextLevel }
{}

LevelScene::~LevelScene() { shutdown(); }

Vector2 LevelScene::tileCentre(int column, int row) const
{
    return {
        mGameState.map->getLeftBoundary() + column * TILE_DIMENSION + TILE_DIMENSION / 2.0f,
        mGameState.map->getTopBoundary()  + row    * TILE_DIMENSION + TILE_DIMENSION / 2.0f
    };
}

int LevelScene::cellIndex(int column, int row) const
{
    return row * mGameState.map->getMapColumns() + column;
}

/*
    ----------- BUILDING A ROOM -----------
*/

// everything that changes while playing goes back to how it started
void LevelScene::resetRoom()
{
    mGameState.nextSceneID = NO_SCENE;
    mGameState.playerDied  = false;

    for (int i = 0; i < MAX_CELLS; i++) mLavaAge[i] = -1.0f;

    mSpreadTimer  = 0.0f;
    mHeat         = 0;
    mDoorUnlocked = false;
    mNearStation  = -1;
    mOpenStation  = -1;
    mShownBanner  = nullptr;
    mBannerTimer  = 0.0f;
    mZoneIndex    = 0;
}

void LevelScene::createMap(int columns, int rows, unsigned int *levelData)
{
    // the walls: the only map anything collides with
    mGameState.map = new Map(
        columns, rows,                  // map grid cols & rows
        levelData,                      // grid data
        "assets/game/tiles.png",        // texture filepath
        TILE_DIMENSION,                 // tile size
        TILESET_COLUMNS, TILESET_ROWS,  // texture cols & rows
        mOrigin                         // in-game origin
    );

    // the floor: plain stone with a few cracked and rubbly tiles mixed in
    mFloorData.assign(columns * rows, 1);
    for (int row = 0; row < rows; row++)
        for (int column = 0; column < columns; column++)
        {
            int pattern = column * 7 + row * 13;
            if      (pattern % 11 == 0) mFloorData[row * columns + column] = 2;
            else if (pattern % 17 == 0) mFloorData[row * columns + column] = 3;
        }

    mFloor = new Map(
        columns, rows, mFloorData.data(), "assets/game/tiles.png",
        TILE_DIMENSION, TILESET_COLUMNS, TILESET_ROWS, mOrigin
    );
}

void LevelScene::createPlayer(int column, int row)
{
    // player.png: 3 frames across; rows face down, left, right and up.
    // The middle frame is standing, so each walk starts and ends on it.
    std::map<Direction, std::vector<int>> playerAnimationAtlas = {
        { DOWN,  {  1,  0,  1,  2 } },
        { LEFT,  {  4,  3,  4,  5 } },
        { RIGHT, {  7,  6,  7,  8 } },
        { UP,    { 10,  9, 10, 11 } },
    };

    // @see https://opengameart.org/content/tiny-16-basic (Lanea Zimmerman, CC-BY 3.0)
    mGameState.player = new Entity(
        tileCentre(column, row),        // position
        { PLAYER_SIZE, PLAYER_SIZE },   // scale
        "assets/game/player.png",       // texture file address
        ATLAS,                          // single image or atlas?
        { 4, 3 },                       // atlas dimensions (rows, columns)
        playerAnimationAtlas,           // actual atlas
        PLAYER                          // entity type
    );

    mGameState.player->setSpeed(PLAYER_SPEED);
    mGameState.player->setFrameSpeed(8);
    mGameState.player->setDirection(DOWN);
    mGameState.player->setColliderDimensions({
        PLAYER_SIZE * 0.6f, PLAYER_SIZE * 0.7f
    });
}

// the door sits in the wall; it is solid wall until every task is done
void LevelScene::createDoor(int column, int row)
{
    mDoorColumn = column;
    mDoorRow    = row;

    // the wall tile comes back every time the room restarts
    mGameState.map->getLevelData()[cellIndex(column, row)] = WALL_TILE;

    mGameState.door = new Entity(
        tileCentre(column, row), { TILE_DIMENSION, TILE_DIMENSION },
        "assets/game/door_locked.png", NONE
    );
    mOpenDoor = new Entity(
        tileCentre(column, row), { TILE_DIMENSION, TILE_DIMENSION },
        "assets/game/door_open.png", NONE
    );

    // a little taller than the tile, so standing right below it counts
    mOpenDoor->setColliderDimensions({ TILE_DIMENSION * 0.6f, TILE_DIMENSION * 1.4f });
}

void LevelScene::addLavaSource(int column, int row)
{
    mLavaAge[cellIndex(column, row)] = LAVA_GROW_TIME;
}

void LevelScene::createStations(int count)
{
    mStationCount = count;
    mStations     = new Entity[count];
}

void LevelScene::setUpStation(int index, TaskType type, int column, int row)
{
    Entity *station = &mStations[index];

    station->setTexture("assets/game/station_off.png");
    station->setScale({ STATION_SIZE, STATION_SIZE });
    station->setColliderDimensions({ STATION_SIZE * 0.8f, STATION_SIZE * 0.8f });
    station->setEntityType(BLOCK);
    station->setPosition(tileCentre(column, row));

    mStationTypes[index] = type;
    mStationDone[index]  = false;
}

void LevelScene::createEnemies(int count)
{
    mGameState.enemyCount = count;
    mGameState.enemies    = new Entity[count];
}

void LevelScene::setUpEnemy(int index, AIType type, int column, int row,
                            float patrolTiles)
{
    Entity *enemy = &mGameState.enemies[index];

    // every enemy sheet is laid out like the player's
    std::map<Direction, std::vector<int>> enemyAnimationAtlas = {
        { DOWN,  {  0,  1,  2,  1 } },
        { LEFT,  {  3,  4,  5,  4 } },
        { RIGHT, {  6,  7,  8,  7 } },
        { UP,    {  9, 10, 11, 10 } },
    };

    switch (type)
    {
    case WANDERER:   // a magma slime that paces back and forth
        enemy->setTexture("assets/game/slime.png");
        enemy->setSpeed(WANDERER_SPEED);
        break;
    default:
        break;
    }

    enemy->setScale({ TILE_DIMENSION, TILE_DIMENSION });
    enemy->setTextureType(ATLAS);
    enemy->setSpriteSheetDimensions({ 4, 3 });
    enemy->setAnimationAtlas(enemyAnimationAtlas);
    enemy->setFrameSpeed(6);
    enemy->setEntityType(NPC);
    enemy->setAIType(type);
    enemy->setAIState(IDLE);
    enemy->setDirection(LEFT);
    enemy->setColliderDimensions({ TILE_DIMENSION * 0.6f, TILE_DIMENSION * 0.5f });
    enemy->setPosition(tileCentre(column, row));

    if (type == WANDERER) enemy->setPatrolRange(patrolTiles * TILE_DIMENSION);
}

// the lava, the task panels, the messages and the sounds
void LevelScene::createInterface()
{
    mLavaTile = new Entity({ 0.0f, 0.0f }, { TILE_DIMENSION, TILE_DIMENSION },
                           "assets/game/lava.png", NONE);
    mLavaShader.load("shaders/vertex.glsl", "shaders/lava.glsl");

    mStationOnTop = new Entity({ 0.0f, 0.0f }, { STATION_SIZE, STATION_SIZE },
                               "assets/game/station_on.png", NONE);
    mKeyPrompt    = new Entity({ 0.0f, 0.0f }, { 34.0f, 34.0f },
                               "assets/game/key_e.png", NONE);
    mIconOff      = new Entity({ 0.0f, 0.0f }, { 36.0f, 36.0f },
                               "assets/game/station_off.png", NONE);
    mIconOn       = new Entity({ 0.0f, 0.0f }, { 36.0f, 36.0f },
                               "assets/game/station_on.png", NONE);

    // the panels sit in the middle of the screen
    mValvePanel  = new Entity(mOrigin, { 640.0f, 380.0f }, "assets/game/panel_valve.png", NONE);
    mGaugePanel  = new Entity(mOrigin, { 640.0f, 380.0f }, "assets/game/panel_gauge.png", NONE);
    mValveWheel  = new Entity({ mOrigin.x, mOrigin.y - 5.0f }, { 170.0f, 170.0f },
                              "assets/game/valve.png", NONE);

    // the dial, zone and needle are squares centred on the same pivot
    Vector2 gaugePivot = { mOrigin.x, mOrigin.y + 80.0f };
    mGaugeDial   = new Entity(gaugePivot, { 300.0f, 300.0f }, "assets/game/gauge.png", NONE);
    mGaugeZone   = new Entity(gaugePivot, { 300.0f, 300.0f },
                              mNarrowGauge ? "assets/game/zone_narrow.png"
                                           : "assets/game/zone_wide.png", NONE);
    mGaugeNeedle = new Entity(gaugePivot, { 300.0f, 300.0f }, "assets/game/needle.png", NONE);

    mBarBack   = new Entity({ 0.0f, 0.0f }, { 1.0f, 1.0f }, "assets/game/bar_back.png",   NONE);
    mBarGreen  = new Entity({ 0.0f, 0.0f }, { 1.0f, 1.0f }, "assets/game/bar_green.png",  NONE);
    mBarOrange = new Entity({ 0.0f, 0.0f }, { 1.0f, 1.0f }, "assets/game/bar_orange.png", NONE);

    Vector2 bannerPosition = { mOrigin.x, 40.0f };
    mHotterBanner = new Entity(bannerPosition, { 560.0f, 70.0f }, "assets/game/hotter.png",    NONE);
    mDoneBanner   = new Entity(bannerPosition, { 560.0f, 70.0f }, "assets/game/task_done.png", NONE);
    mOpenBanner   = new Entity(bannerPosition, { 560.0f, 70.0f }, "assets/game/unlocked.png",  NONE);

    mOpenSound     = LoadSound("assets/game/open.wav");
    mCrankSound    = LoadSound("assets/game/crank.wav");
    mHitSound      = LoadSound("assets/game/hit.wav");
    mHotterSound   = LoadSound("assets/game/hotter.wav");
    mTaskDoneSound = LoadSound("assets/game/task_done.wav");
    mDoorSound     = LoadSound("assets/game/door.wav");
    mStepsSound    = LoadSound("assets/game/steps.wav");
    SetSoundVolume(mStepsSound, 0.5f);
    mSoundsLoaded  = true;
}

/*
    ----------- THE LAVA -----------
*/

// one step of the flood: every floor tile touching lava turns into lava
void LevelScene::spreadLava()
{
    int           columns = mGameState.map->getMapColumns();
    int           rows    = mGameState.map->getMapRows();
    unsigned int *walls   = mGameState.map->getLevelData();

    bool grows[MAX_CELLS] = { false };

    for (int row = 0; row < rows; row++)
        for (int column = 0; column < columns; column++)
        {
            if (mLavaAge[cellIndex(column, row)] < 0.0f) continue;

            // up, down, left, right
            int nextColumns[4] = { column, column, column - 1, column + 1 };
            int nextRows[4]    = { row - 1, row + 1, row, row };

            for (int i = 0; i < 4; i++)
            {
                int c = nextColumns[i], r = nextRows[i];
                if (c < 0 || c >= columns || r < 0 || r >= rows) continue;

                int next = cellIndex(c, r);
                if (walls[next] == 0 && mLavaAge[next] < 0.0f) grows[next] = true;
            }
        }

    for (int i = 0; i < columns * rows; i++)
        if (grows[i]) mLavaAge[i] = 0.0f;
}

// a tile only burns once it has finished swelling in
bool LevelScene::isLavaAt(Vector2 position) const
{
    int column = floor((position.x - mGameState.map->getLeftBoundary()) / TILE_DIMENSION);
    int row    = floor((position.y - mGameState.map->getTopBoundary())  / TILE_DIMENSION);

    if (column < 0 || column >= mGameState.map->getMapColumns() ||
        row    < 0 || row    >= mGameState.map->getMapRows()) return false;

    return mLavaAge[cellIndex(column, row)] >= LAVA_GROW_TIME;
}

// a mistake: the lava gets hotter, changes colour and spreads faster
void LevelScene::heatUp()
{
    if (mHeat < MAX_HEAT) mHeat++;

    PlaySound(mHotterSound);
    showBanner(mHotterBanner);
}

void LevelScene::showBanner(Entity *banner)
{
    mShownBanner = banner;
    mBannerTimer = BANNER_TIME;
}

/*
    ----------- TASKS -----------
*/

void LevelScene::openTask(int station)
{
    mOpenStation  = station;
    mTaskTimer    = 0.0f;
    mValveTaps    = 0;
    mLastValveKey = 0;
    mGaugeTime    = 0.0f;
    mGaugeHits    = 0;

    mValveWheel->setAngle(0.0f);
    PlaySound(mOpenSound);
}

void LevelScene::closeTask() { mOpenStation = -1; }

void LevelScene::completeTask()
{
    mStationDone[mOpenStation] = true;
    closeTask();

    PlaySound(mTaskDoneSound);
    showBanner(mDoneBanner);

    for (int i = 0; i < mStationCount; i++)
        if (!mStationDone[i]) return;

    // every task is done: the wall tile under the door goes away
    mDoorUnlocked = true;
    mGameState.map->getLevelData()[cellIndex(mDoorColumn, mDoorRow)] = 0;

    PlaySound(mDoorSound);
    showBanner(mOpenBanner);
}

// where the needle points: it swings back and forth like a pendulum
float LevelScene::gaugeAngle() const
{
    return GAUGE_SWING * sin(mGaugeTime * mGaugeSwingSpeed);
}

float LevelScene::zoneAngle() const
{
    return ZONE_ANGLES[mZoneIndex % ZONE_COUNT];
}

// keys while a panel is open
void LevelScene::taskInput()
{
    if (IsKeyPressed(KEY_E))
    {
        closeTask();   // step away; the task starts over next time
        return;
    }

    if (mStationTypes[mOpenStation] == VALVE_TASK)
    {
        // A and D have to take turns; the same key twice does nothing
        int key = 0;
        if (IsKeyPressed(KEY_A)) key = KEY_A;
        if (IsKeyPressed(KEY_D)) key = KEY_D;

        if (key != 0 && key != mLastValveKey)
        {
            mLastValveKey = key;
            mValveTaps++;
            mValveWheel->setAngle(mValveTaps * 720.0f / mValveTapsNeeded);
            PlaySound(mCrankSound);

            if (mValveTaps >= mValveTapsNeeded) completeTask();
        }
    }
    else if (mStationTypes[mOpenStation] == GAUGE_TASK)
    {
        if (IsKeyPressed(KEY_SPACE))
        {
            float zoneHalf = mNarrowGauge ? NARROW_ZONE_HALF : WIDE_ZONE_HALF;

            if (fabs(gaugeAngle() - zoneAngle()) <= zoneHalf)
            {
                mGaugeHits++;
                mZoneIndex++;   // the zone moves somewhere else
                PlaySound(mHitSound);

                if (mGaugeHits >= mGaugeHitsNeeded) completeTask();
            }
            else heatUp();      // missed: the panel stays open to try again
        }
    }
}

void LevelScene::updateTask(float deltaTime)
{
    mTaskTimer += deltaTime;

    if (mStationTypes[mOpenStation] == VALVE_TASK)
    {
        // too slow: the valve sticks, and that counts as a mistake
        if (mTaskTimer >= mValveTimeLimit)
        {
            closeTask();
            heatUp();
        }
    }
    else if (mStationTypes[mOpenStation] == GAUGE_TASK)
    {
        mGaugeTime += deltaTime;
        mGaugeNeedle->setAngle(gaugeAngle());
        mGaugeZone->setAngle(zoneAngle());
    }
}

/*
    ----------- EVERY FRAME -----------
*/

void LevelScene::processInput()
{
    Entity *player = mGameState.player;
    player->resetMovement();

    if (mOpenStation != -1)
    {
        StopSound(mStepsSound);
        taskInput();
        return;
    }

    if      (IsKeyDown(KEY_A)) player->moveLeft();
    else if (IsKeyDown(KEY_D)) player->moveRight();

    if      (IsKeyDown(KEY_W)) player->moveUp();
    else if (IsKeyDown(KEY_S)) player->moveDown();

    // to avoid faster diagonal speed
    if (GetLength(player->getMovement()) > 1.0f) player->normaliseMovement();

    // footsteps while walking
    bool walking = GetLength(player->getMovement()) > 0.0f;
    if (walking && !IsSoundPlaying(mStepsSound)) PlaySound(mStepsSound);
    if (!walking && IsSoundPlaying(mStepsSound)) StopSound(mStepsSound);

    if (IsKeyPressed(KEY_E) && mNearStation != -1) openTask(mNearStation);
}

void LevelScene::update(float deltaTime)
{
    // the stations are solid, so the player walks around them
    mGameState.player->update(
        deltaTime,      // delta time / fixed timestep
        nullptr,        // player
        mGameState.map, // map
        mStations,      // collidable entities
        mStationCount   // col. entity count
    );

    for (int i = 0; i < mGameState.enemyCount; i++)
    {
        // each enemy's AI decides its movement again every step
        mGameState.enemies[i].resetMovement();
        mGameState.enemies[i].update(
            deltaTime, mGameState.player, mGameState.map, nullptr, 0
        );
    }

    if (mOpenStation != -1) updateTask(deltaTime);

    // the lava: new tiles swell in, and every so often it spreads a step.
    // The hotter it is, the shorter the wait.
    for (int i = 0; i < MAX_CELLS; i++)
        if (mLavaAge[i] >= 0.0f) mLavaAge[i] += deltaTime;

    mSpreadTimer += deltaTime;
    if (mSpreadTimer >= mSpreadIntervals[mHeat])
    {
        mSpreadTimer = 0.0f;
        spreadLava();
    }

    // which unfinished station is close enough to use
    mNearStation = -1;
    float closest = INTERACT_RANGE;
    for (int i = 0; i < mStationCount; i++)
    {
        if (mStationDone[i]) continue;

        float distance = Vector2Distance(mGameState.player->getPosition(),
                                         mStations[i].getPosition());
        if (distance < closest)
        {
            closest      = distance;
            mNearStation = i;
        }
    }

    if (mBannerTimer > 0.0f) mBannerTimer -= deltaTime;

    // touching an enemy or standing in the lava costs a life
    for (int i = 0; i < mGameState.enemyCount; i++)
        if (mGameState.player->isColliding(&mGameState.enemies[i]))
            mGameState.playerDied = true;

    if (isLavaAt(mGameState.player->getPosition()))
        mGameState.playerDied = true;

    if (mGameState.playerDied) StopSound(mStepsSound);

    // through the open door: on to the next room
    if (!mGameState.playerDied && mDoorUnlocked &&
        mGameState.player->isColliding(mOpenDoor))
    {
        StopSound(mStepsSound);
        mGameState.nextSceneID = mNextLevel;
    }
}

// a bar that fills from the left
void LevelScene::drawBar(Entity *fill, float fraction, Vector2 centre, float width)
{
    const float height = 16.0f;

    if (fraction < 0.0f) fraction = 0.0f;
    if (fraction > 1.0f) fraction = 1.0f;

    mBarBack->setScale({ width + 6.0f, height + 6.0f });
    mBarBack->setPosition(centre);
    mBarBack->render();

    float left = centre.x - width / 2.0f;
    fill->setScale({ width * fraction, height });
    fill->setPosition({ left + width * fraction / 2.0f, centre.y });
    fill->render();
}

void LevelScene::render()
{
    ClearBackground(ColorFromHex(mBGColourHexCode));

    mFloor->render();
    mGameState.map->render();

    if (mDoorUnlocked) mOpenDoor->render();
    else               mGameState.door->render();

    for (int i = 0; i < mStationCount; i++)
    {
        mStations[i].render();
        if (mStationDone[i])
        {
            mStationOnTop->setPosition(mStations[i].getPosition());
            mStationOnTop->render();
        }
    }

    // the lava: one tile drawn at every lava cell, coloured by the shader
    // according to how hot it is
    mLavaShader.begin();
    mLavaShader.setInt("heat", mHeat);
    mLavaShader.setFloat("time", (float) GetTime());

    int columns = mGameState.map->getMapColumns();
    int rows    = mGameState.map->getMapRows();
    for (int row = 0; row < rows; row++)
        for (int column = 0; column < columns; column++)
        {
            float age = mLavaAge[cellIndex(column, row)];
            if (age < 0.0f) continue;

            float size = TILE_DIMENSION * (age < LAVA_GROW_TIME ? age / LAVA_GROW_TIME : 1.0f);
            mLavaTile->setScale({ size, size });
            mLavaTile->setPosition(tileCentre(column, row));
            mLavaTile->render();
        }

    mLavaShader.end();

    for (int i = 0; i < mGameState.enemyCount; i++)
        mGameState.enemies[i].render();

    mGameState.player->render();

    if (mOpenStation == -1 && mNearStation != -1)
    {
        Vector2 above = mStations[mNearStation].getPosition();
        above.y -= TILE_DIMENSION * 0.9f;
        mKeyPrompt->setPosition(above);
        mKeyPrompt->render();
    }

    // one icon per task in the top right corner, lit up once it is done
    for (int i = 0; i < mStationCount; i++)
    {
        Entity *icon = mStationDone[i] ? mIconOn : mIconOff;
        icon->setPosition({ mOrigin.x * 2.0f - 40.0f - i * 44.0f, 35.0f });
        icon->render();
    }

    // the open task's panel
    if (mOpenStation != -1)
    {
        if (mStationTypes[mOpenStation] == VALVE_TASK)
        {
            mValvePanel->render();
            mValveWheel->render();

            drawBar(mBarGreen, (float) mValveTaps / mValveTapsNeeded,
                    { mOrigin.x, mOrigin.y + 105.0f }, 400.0f);
            drawBar(mBarOrange, 1.0f - mTaskTimer / mValveTimeLimit,
                    { mOrigin.x, mOrigin.y + 135.0f }, 400.0f);
        }
        else
        {
            mGaugePanel->render();
            mGaugeDial->render();
            mGaugeZone->render();
            mGaugeNeedle->render();

            drawBar(mBarGreen, (float) mGaugeHits / mGaugeHitsNeeded,
                    { mOrigin.x, mOrigin.y + 120.0f }, 400.0f);
        }
    }

    if (mBannerTimer > 0.0f && mShownBanner != nullptr) mShownBanner->render();
}

void LevelScene::shutdown()
{
    delete mGameState.player;   mGameState.player = nullptr;
    delete mGameState.map;      mGameState.map    = nullptr;
    delete mFloor;              mFloor            = nullptr;
    delete mGameState.door;     mGameState.door   = nullptr;
    delete mOpenDoor;           mOpenDoor         = nullptr;

    delete[] mGameState.enemies;
    mGameState.enemies    = nullptr;
    mGameState.enemyCount = 0;

    delete[] mStations;
    mStations     = nullptr;
    mStationCount = 0;

    Entity **interface[] = {
        &mLavaTile, &mStationOnTop, &mKeyPrompt, &mIconOff, &mIconOn,
        &mValvePanel, &mGaugePanel, &mValveWheel, &mGaugeDial, &mGaugeZone,
        &mGaugeNeedle, &mBarBack, &mBarGreen, &mBarOrange, &mHotterBanner,
        &mDoneBanner, &mOpenBanner
    };
    for (Entity **entity : interface)
    {
        delete *entity;
        *entity = nullptr;
    }
    mShownBanner = nullptr;

    mLavaShader.unload();

    if (mSoundsLoaded)
    {
        UnloadSound(mOpenSound);
        UnloadSound(mCrankSound);
        UnloadSound(mHitSound);
        UnloadSound(mHotterSound);
        UnloadSound(mTaskDoneSound);
        UnloadSound(mDoorSound);
        UnloadSound(mStepsSound);
        mSoundsLoaded = false;
    }
}
