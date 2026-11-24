#ifndef LEVELSCENE_H
#define LEVELSCENE_H

#include "Scene.h"

// What every level shares: the player, the map, the enemies, the exit flag
// and the rising lava, and how they update, draw and clean up. Each level only
// fills in its own initialise(): its map, its enemies and how fast its lava is.
class LevelScene : public Scene
{
protected:
    SceneID mNextLevel;           // where reaching the flag takes you

    float mLavaSpeed        = 0.0f,   // how fast the lava is rising right now
          mLavaAcceleration = 0.0f;   // how much faster it gets every second

    void createMap(int columns, int rows, unsigned int *levelData);
    void createPlayer(int column, int row);
    void createFlag(int column, int row);
    void createLava(float startSpeed, float acceleration);
    void createEnemies(int count);
    void setUpEnemy(int index, AIType type, int column, int row);

    // the centre of a tile, measured from the map's own edges
    Vector2 tileCentre(int column, int row) const;

public:
    static constexpr float TILE_DIMENSION          = 50.0f,
                           ACCELERATION_OF_GRAVITY = 981.0f,
                           JUMPING_POWER           = 600.0f,
                           WANDERER_PATROL_RANGE   = 75.0f,   // 1.5 tiles each way
                           LAVA_START_DEPTH        = 50.0f,   // starts this far below the map
                           LAVA_SURFACE_GAP        = 15.0f;   // see-through wave tops

    static constexpr int   TILESET_COLUMNS = 3,
                           TILESET_ROWS    = 1,
                           LAVA_COLUMNS    = 20,  // the lava image is 20 x 14 tiles
                           LAVA_ROWS       = 14,
                           WANDERER_SPEED  = 60;

    LevelScene();
    LevelScene(Vector2 origin, const char *bgHexCode, SceneID nextLevel);
    ~LevelScene();

    void update(float deltaTime) override;
    void render() override;
    void shutdown() override;
};

#endif // LEVELSCENE_H
