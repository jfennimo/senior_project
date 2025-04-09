#include "MainMenu.h"
#include "Game.h"

MainMenu::MainMenu(Game* game)
    : game(game),
    lastBlinkTime(SDL_GetTicks()),
    showBlinkText(true)
{
}

void MainMenu::update() {
    // Main menu logic
    Uint32 currentTime = SDL_GetTicks();

    if (currentTime - lastBlinkTime >= BLINK_DELAY) { // Blinking text logic
        showBlinkText = !showBlinkText;
        lastBlinkTime = currentTime;
    }
}

void MainMenu::render() {
    // Draw main menu screen
    TTF_Font* titleFont = game->titleFont;
    TTF_Font* menuFont = game->menuFont;

    if (!titleFont || !menuFont) {
        std::cout << "Failed to load font: " << TTF_GetError() << std::endl;
        return;
    }

    SDL_SetRenderDrawColor(Game::renderer, 0, 0, 0, 255);
    SDL_RenderClear(Game::renderer);

    game->uiManager->drawText("Main Menu!", 660, 60, { 255, 255, 255, 255 }, titleFont);

    if (showBlinkText) {
        game->uiManager->drawText("Arcade Mode", 640, 450, { 255, 255, 255, 255 }, titleFont);
    }

    SDL_RenderPresent(Game::renderer);
}

void MainMenu::handleEvent(SDL_Event& event) {
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_RETURN) {
        std::cout << "Navigating to arcade mode!" << std::endl;
        game->changeState(GameState::ARCADE_MODE);
    }
}