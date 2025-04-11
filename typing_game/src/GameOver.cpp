#include "GameOver.h"
#include "Game.h"

GameOver::GameOver(Game* game)
    : game(game),
    lastBlinkTime(SDL_GetTicks()),
    showBlinkText(true)
{}

void GameOver::update() {
    // Game over screen logic

    // Blink counter logic
    Uint32 currentTime = SDL_GetTicks(); // Get current time in milliseconds

    if (currentTime > lastBlinkTime + BLINK_DELAY) {
        showBlinkText = !showBlinkText;  // Toggle visibility
        lastBlinkTime = currentTime;    // Update last blink time
    }
}

void GameOver::render() {
	// Draw game over screen
	TTF_Font* titleFont = game->titleFont;
    TTF_Font* menuFont = game->menuFont;
    TTF_Font* gameOverFont = game->gameOverFont;

    if (!titleFont || gameOverFont) {
        std::cout << "Failed to load font: " << TTF_GetError() << std::endl;
        return;
    }

	SDL_SetRenderDrawColor(Game::renderer, 255, 51, 51, 255);
	SDL_RenderClear(Game::renderer);

	game->uiManager->drawText("GAME", 600, 100, { 255, 255, 255, 255 }, gameOverFont);
    game->uiManager->drawText("OVER!", 575, 350, { 255, 255, 255, 255 }, gameOverFont);
    game->uiManager->drawText(level , 600, 500, { 255, 255, 255, 255 }, menuFont);
    game->uiManager->drawText(zombiesDefeated, 600, 550, { 255, 255, 255, 255 }, menuFont);
    game->uiManager->drawText(overallAccuracy, 600, 600, { 255, 255, 255, 255 }, menuFont);

	if (showBlinkText) {
        game->uiManager->drawText("Press Enter to Return to the Title Screen...", 400, 750, { 255, 255, 255, 255 }, menuFont);
	}

	SDL_RenderPresent(Game::renderer);
}

void GameOver::handleEvent(SDL_Event& event) {
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_RETURN) {
        std::cout << "Returning to title screen!" << std::endl;
        game->resetArcadeMode();
        game->changeState(GameState::TITLE_SCREEN);
    }
}

void GameOver::setResults(std::string& lvl, std::string& zombDefeat, const std::string& accuracy) {
	level = lvl;
	zombiesDefeated = zombDefeat;
	overallAccuracy = accuracy;
}