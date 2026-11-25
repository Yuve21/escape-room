/**
* Author: Yuvraj Chandyok
* Assignment: Rising Heat
* Date due: December 7, 2026, 2:00pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#include "Scene.h"

Scene::Scene() : mGameState{}, mOrigin{} {}

Scene::Scene(Vector2 origin, const char *bgHexCode)
    : mGameState{}, mOrigin{ origin }, mBGColourHexCode{ bgHexCode }
{}
