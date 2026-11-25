/**
* Author: Yuvraj Chandyok
* Assignment: Rising Heat
* Date due: December 7, 2026, 2:00pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#include "Level1.h"

Level1::Level1() : LevelScene{} {}

Level1::Level1(Vector2 origin, const char *bgHexCode, SceneID nextLevel)
    : LevelScene{ origin, bgHexCode, nextLevel }
{}

void Level1::initialise()
{
    mGameState.nextSceneID = NO_SCENE;
    mGameState.playerDied  = false;

    createMap(LEVEL_COLUMNS, LEVEL_ROWS, mLevelData);
    createPlayer(PLAYER_START_COL, PLAYER_START_ROW);
    createFlag(FLAG_COL, FLAG_ROW);
    createLava(LAVA_START_SPEED, LAVA_ACCELERATION);

    // enemy, AI type, tile column, tile row
    // one slime on the wide floor at the bottom, to learn to jump over
    createEnemies(NUMBER_OF_ENEMIES);
    setUpEnemy(0, WANDERER, 8, 33);
}
