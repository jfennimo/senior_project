#pragma once
#include "SDL.h"

class Game; // Forward declaration so GameMode can refer to it

class GameMode {
public:
    virtual void update() = 0;
    virtual void render() = 0;
    virtual void handleEvent(SDL_Event& event) = 0;
    virtual ~GameMode() = default;
};