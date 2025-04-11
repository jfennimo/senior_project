#include "ArcadeResults.h"
#include "Game.h"

ArcadeResults::ArcadeResults(Game* game)
    : game(game),
    lastBlinkTime(SDL_GetTicks()),
    showBlinkText(true)
{}

void ArcadeResults::update() {
    // Arcade results screen logic
    Uint32 currentTime = SDL_GetTicks(); // Get current time

    if (currentTime > lastBlinkTime + BLINK_DELAY) {
        showBlinkText = !showBlinkText;  // Toggle visibility
        lastBlinkTime = currentTime;    // Update last blink time
    }
}

void ArcadeResults::render() {
	// Draw Arcade results screen
	TTF_Font* titleFont = game->titleFont;
	TTF_Font* menuFont = game->menuFont;

	if (!titleFont || !menuFont) {
		std::cout << "Failed to load font: " << TTF_GetError() << std::endl;
		return;
	}

	SDL_SetRenderDrawColor(Game::renderer, 255, 178, 102, 255);
	SDL_RenderClear(Game::renderer);

	game->uiManager->drawText(level, 625, 50, { 255, 255, 255, 255 }, titleFont);
	game->uiManager->drawText(hpResults, 40, 200, { 255, 255, 255, 255 }, menuFont);
	game->uiManager->drawText(finalWrongResults, 40, 400, { 255, 255, 255, 255 }, menuFont);
	game->uiManager->drawText(overallAccuracy, 40, 600, { 255, 255, 255, 255 }, menuFont);

	// heeeree
	//
	//
	//
	//
	//
	//
	// Add alert when another zombie is spawning

	if (showBlinkText) {
		game->uiManager->drawText("Press Enter to Start the Next Level!", 500, 750, { 255, 255, 255, 255 }, menuFont);
	}

	SDL_RenderPresent(Game::renderer);
}

void ArcadeResults::handleEvent(SDL_Event& event) {
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_RETURN) {
		std::cout << "Starting next round!" << std::endl;
		game->nextLevelToStart = true;
        game->changeState(GameState::ARCADE_MODE);
    }
}

void ArcadeResults::setResults(const std::string& hp, const std::string& wrong, const std::string& accuracy, const std::string& levelNum) {
	hpResults = hp;
	finalWrongResults = wrong;
	overallAccuracy = accuracy;
	level = levelNum;
}