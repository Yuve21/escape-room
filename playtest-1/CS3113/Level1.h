#ifndef LEVEL1_H
#define LEVEL1_H

#include "LevelScene.h"

/*
    Level 1: The Cellar. ( . floor, W wall, D door, P player, G gauge,
    V valve, L where the lava breaks in, ~ the slime's patrol )

    WWWWWWWWWWDWWWWWWWWW
    W..................W
    W..G...........V...W
    W..................W
    W....WWWW..WWWW....W
    W.....~~~~~~~~.....W
    W..................W
    W....WWWW..WWWW....W
    W..................W
    W.P................W
    W.................LW
    WWWWWWWWWWWWWWWWWWWW

    Two easy tasks, then out the door before the lava fills the room.
*/
class Level1 : public LevelScene
{
public:
    static constexpr int LEVEL_COLUMNS     = 20,
                         LEVEL_ROWS        = 12,
                         PLAYER_START_COL  = 2,
                         PLAYER_START_ROW  = 9,
                         DOOR_COL          = 10,
                         DOOR_ROW          = 0,
                         LAVA_COL          = 18,
                         LAVA_ROW          = 10,
                         NUMBER_OF_TASKS   = 2,
                         NUMBER_OF_ENEMIES = 1;

private:
    unsigned int mLevelData[LEVEL_COLUMNS * LEVEL_ROWS] = {
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4,
        4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4,
        4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4,
        4, 0, 0, 0, 0, 4, 4, 4, 4, 0, 0, 4, 4, 4, 4, 0, 0, 0, 0, 4,
        4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4,
        4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4,
        4, 0, 0, 0, 0, 4, 4, 4, 4, 0, 0, 4, 4, 4, 4, 0, 0, 0, 0, 4,
        4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4,
        4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4,
        4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4
    };

public:
    Level1();
    Level1(Vector2 origin, const char *bgHexCode, SceneID nextLevel);

    void initialise() override;
};

#endif // LEVEL1_H
