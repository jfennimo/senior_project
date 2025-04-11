#pragma once
#include "GameMode.h"
#include "GameState.h"
#include "SDL.h"
#include "SDL_ttf.h"
#include <iostream>

// Forward declarations
class Game;
class UIManager;

class BonusTitle : public GameMode {
public:
    BonusTitle(Game* game);

    void update() override;
    void render() override;
    void handleEvent(SDL_Event& event) override;

private:
    Game* game;
    Uint32 lastBlinkTime = 0;
    bool showBlinkText;
    const Uint32 BLINK_DELAY = 1200;
};