#include "BonusTitle.h"
#include "Game.h"

BonusTitle::BonusTitle(Game* game)
    : game(game),
    lastBlinkTime(SDL_GetTicks()),
    showBlinkText(true)
{}

void BonusTitle::update() {
    // Bonus title logic

    // Blink counter logic
    Uint32 currentTime = SDL_GetTicks(); // Get current time in milliseconds

    if (currentTime > lastBlinkTime + BLINK_DELAY) {
        showBlinkText = !showBlinkText;  // Toggle visibility
        lastBlinkTime = currentTime;    // Update last blink time
    }
}

void BonusTitle::render() {
    // Draw bonus title screen
    TTF_Font* titleFont = game->titleFont;
    TTF_Font* menuFont = game->menuFont;
    TTF_Font* gameOverFont = game->gameOverFont;

    if (!titleFont || gameOverFont) {
        std::cout << "Failed to load font: " << TTF_GetError() << std::endl;
        return;
    }

    SDL_SetRenderDrawColor(Game::renderer, 255, 51, 51, 255);
    SDL_RenderClear(Game::renderer);

    game->uiManager->drawText("BONUS", 600, 100, { 255, 255, 255, 255 }, gameOverFont);
    game->uiManager->drawText("STAGE!", 565, 350, { 255, 255, 255, 255 }, gameOverFont);


    if (showBlinkText) {
        game->uiManager->drawText("Press Enter to Start Bonus Round...", 500, 750, { 255, 255, 255, 255 }, menuFont);
    }

    SDL_RenderPresent(Game::renderer);
}

void BonusTitle::handleEvent(SDL_Event& event) {
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_RETURN) {
        std::cout << "Starting bonus round!" << std::endl;

        game->startArcadeInBonus = true;
        game->changeState(GameState::ARCADE_MODE);
    }
}