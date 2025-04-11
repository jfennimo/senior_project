#pragma once
#include "GameMode.h"
#include "GameState.h"
#include "SDL.h"
#include "SDL_ttf.h"
#include <iostream>

// Forward declarations
class Game;
class UIManager;

class ArcadeResults : public GameMode {
public:
    ArcadeResults(Game* game);

    void update() override;
    void render() override;
    void handleEvent(SDL_Event& event) override;
    void setResults(const std::string& hp, const std::string& wrong, const std::string& accuracy, const std::string& levelNum);

private:
    Game* game;
    Uint32 lastBlinkTime = 0;
    bool showBlinkText;
    const Uint32 BLINK_DELAY = 1200;

    std::string hpResults;
    std::string finalWrongResults;
    std::string overallAccuracy;
    std::string level;
};