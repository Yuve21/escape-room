#ifndef SCENE_H
#define SCENE_H

#include "Entity.h"

struct GameState
{
    Entity *player;      // nullptr in the menu / win / lose scenes
    Map    *map;         // the walls; the floor is drawn by its own map
    Entity *enemies;
    int     enemyCount;
    Entity *door;        // locked until every task is done, then the way out
    Entity *banner;      // the text image in the menu / win / lose scenes

    bool    playerDied;  // touched the lava or an enemy; main takes away a life
    SceneID nextSceneID;
};

class Scene
{
protected:
    GameState   mGameState;
    Vector2     mOrigin;
    const char *mBGColourHexCode = "#000000";

public:
    Scene();
    Scene(Vector2 origin, const char *bgHexCode);
    virtual ~Scene() {}

    virtual void initialise() = 0;
    virtual void processInput() {}   // once a frame; only levels read keys here
    virtual void update(float deltaTime) = 0;
    virtual void render() = 0;
    virtual void shutdown() = 0;

    GameState   getState()           const { return mGameState;        }
    Vector2     getOrigin()          const { return mOrigin;           }
    const char *getBGColourHexCode() const { return mBGColourHexCode; }
};

#endif // SCENE_H
