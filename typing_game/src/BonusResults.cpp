#include "BonusResults.h"
#include "Game.h"

BonusResults::BonusResults(Game* game)
	: game(game),
	lastBlinkTime(SDL_GetTicks()),
	showBlinkText(true)
{}

void BonusResults::update() {
	// Bonus results screen logic
	Uint32 currentTime = SDL_GetTicks(); // Get current time

	if (currentTime > lastBlinkTime + BLINK_DELAY) {
		showBlinkText = !showBlinkText;  // Toggle visibility
		lastBlinkTime = currentTime;    // Update last blink time
	}
}

void BonusResults::render() {
	// Draw Bonus results screen
	TTF_Font* titleFont = game->titleFont;
	TTF_Font* menuFont = game->menuFont;

	if (!titleFont || !menuFont) {
		std::cout << "Failed to load font: " << TTF_GetError() << std::endl;
		return;
	}

	SDL_SetRenderDrawColor(Game::renderer, 255, 178, 102, 255);
	SDL_RenderClear(Game::renderer);

	// heeerreeeee
	// 
	// 
	// 
	// 
	// 
	// 
	// maybe add total Barrier HP here

	game->uiManager->drawText("Bonus Stage Results!", 560, 50, { 255, 255, 255, 255 }, titleFont);
	game->uiManager->drawText(hpResults, 40, 200, { 255, 255, 255, 255 }, menuFont);
	game->uiManager->drawText(finalWrongResults, 40, 300, { 255, 255, 255, 255 }, menuFont);
	game->uiManager->drawText(totalBonusZombiesDefeated, 40, 400, { 255, 255, 255, 255 }, menuFont);
	game->uiManager->drawText(overallAccuracy, 40, 500, { 255, 255, 255, 255 }, menuFont);

	if (showBlinkText) {
		game->uiManager->drawText("Press Enter to Start the Next Level!", 500, 750, { 255, 255, 255, 255 }, menuFont);
	}

	SDL_RenderPresent(Game::renderer);
}

void BonusResults::handleEvent(SDL_Event& event) {
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_RETURN) {
		game->nextArcadeLevel();
		std::cout << "Starting next round!" << std::endl;
		game->changeState(GameState::ARCADE_MODE);
	}
}

void BonusResults::setResults(const std::string& hp, const std::string& wrong, std::string& zombDefeat, const std::string& accuracy) {
	hpResults = hp;
	finalWrongResults = wrong;
	totalBonusZombiesDefeated = zombDefeat;
	overallAccuracy = accuracy;
}