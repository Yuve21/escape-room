/**
* Author: Yuvraj Chandyok
* Assignment: Escape Room
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
    resetRoom();

    // the easiest room: slow lava, a slow needle, a wide green zone
    mSpreadIntervals[0] = 2.0f;   // seconds per lava step: orange
    mSpreadIntervals[1] = 1.3f;   //                        red
    mSpreadIntervals[2] = 0.8f;   //                        violet
    mValveTapsNeeded    = 12;
    mValveTimeLimit     = 6.0f;
    mGaugeSwingSpeed    = 2.2f;
    mGaugeHitsNeeded    = 2;
    mNarrowGauge        = false;

    createMap(LEVEL_COLUMNS, LEVEL_ROWS, mLevelData);
    createPlayer(PLAYER_START_COL, PLAYER_START_ROW);
    createDoor(DOOR_COL, DOOR_ROW);
    addLavaSource(LAVA_COL, LAVA_ROW);

    // station, task, tile column, tile row
    createStations(NUMBER_OF_TASKS);
    setUpStation(0, VALVE_TASK, 15, 2);
    setUpStation(1, GAUGE_TASK,  3, 2);

    // enemy, AI type, tile column, tile row, patrol (tiles each way)
    // one slime pacing the middle of the room, between the inner walls
    createEnemies(NUMBER_OF_ENEMIES);
    setUpEnemy(0, WANDERER, 9, 5, 4.0f);

    createInterface();
}
