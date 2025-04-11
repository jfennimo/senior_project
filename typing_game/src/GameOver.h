#pragma once
#include "GameMode.h"
#include "GameState.h"
#include "SDL.h"
#include "SDL_ttf.h"
#include <iostream>

// Forward declarations
class Game;
class UIManager;

class GameOver : public GameMode {
public:
    GameOver(Game* game);

    void update() override;
    void render() override;
    void handleEvent(SDL_Event& event) override;
    void setResults(std::string& lvl, std::string& zombDefeat, const std::string& accuracy);

private:
    Game* game;
    Uint32 lastBlinkTime = 0;
    bool showBlinkText;
    const Uint32 BLINK_DELAY = 1200;

    std::string level;
    std::string zombiesDefeated;
    std::string overallAccuracy;
};