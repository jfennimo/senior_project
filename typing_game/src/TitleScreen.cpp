#include "TitleScreen.h"
#include "Game.h"

TitleScreen::TitleScreen(Game* game)
    : game(game),
    lastBlinkTime(SDL_GetTicks()),
    showBlinkText(true)
{}

void TitleScreen::update() {
    // Title screen logic
    Uint32 currentTime = SDL_GetTicks();

    if (currentTime - lastBlinkTime >= BLINK_DELAY) { // Blinking text logic
        showBlinkText = !showBlinkText;
        lastBlinkTime = currentTime;
    }
}

void TitleScreen::render() {
    // Draw title screen
    TTF_Font* titleFont = game->titleFont;
    TTF_Font* menuFont = game->menuFont;

    if (!titleFont || !menuFont) {
        std::cout << "Failed to load font: " << TTF_GetError() << std::endl;
        return;
    }

    SDL_SetRenderDrawColor(Game::renderer, 0, 0, 0, 255);
    SDL_RenderClear(Game::renderer);

    game->uiManager->drawText("Letter RIP", 650, 450, { 255, 255, 255, 255 }, titleFont);

    if (showBlinkText) {
        game->uiManager->drawText("Press Enter to Start!", 600, 525, { 255, 255, 255, 255 }, menuFont);
    }

    SDL_RenderPresent(Game::renderer);

}

void TitleScreen::handleEvent(SDL_Event& event) {
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_RETURN) {
        std::cout << "Navigating to main menu!" << std::endl;
        game->changeState(GameState::MAIN_MENU);
    }
}