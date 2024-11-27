#include "UIManager.h"

UIManager::UIManager(SDL_Renderer* ren) : renderer(ren) {}

// Draw text on screen
void UIManager::drawText(const std::string& text, int x, int y, SDL_Color color, TTF_Font* font) {
	if (!font) {
		SDL_Log("No font loaded...");
	}

	SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

	SDL_Rect destRect = { x, y, surface->w, surface->h };
	SDL_RenderCopy(renderer, texture, nullptr, &destRect);

	SDL_FreeSurface(surface);
	SDL_DestroyTexture(texture);
}

// Draw rectangle
void UIManager::drawRectangle(int x, int y, int width, int height, SDL_Color color) {
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

	SDL_Rect rect = { x, y, width, height };
	SDL_RenderFillRect(renderer, &rect);
}