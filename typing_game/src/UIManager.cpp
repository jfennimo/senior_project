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

// Health bar
void UIManager::drawHealthbar(int x, int y, int width, int height, int currentHealth, int maxHealth, SDL_Color outlineColor, SDL_Color fgColor, SDL_Color bgColor, const std::string& labelText, TTF_Font* font, SDL_Color textColor) {
	// Outline around bar
	SDL_Rect outlineRect = { x - 2, y - 2, width + 4, height + 4 };
	SDL_SetRenderDrawColor(renderer, outlineColor.r, outlineColor.g, outlineColor.b, outlineColor.a);
	SDL_RenderFillRect(renderer, &outlineRect);

	// Background bar
	SDL_Rect bgRect = { x, y, width, height };
	SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
	SDL_RenderFillRect(renderer, &bgRect);

	// Foreground bar (scaled by current health)
	int healthWidth = static_cast<int>((static_cast<float>(currentHealth) / maxHealth) * width);
	SDL_Rect fgRect = { x, y, healthWidth, height };
	SDL_SetRenderDrawColor(renderer, fgColor.r, fgColor.g, fgColor.b, fgColor.a);
	SDL_RenderFillRect(renderer, &fgRect);

	// Render text above health bar
	if (font) {
		SDL_Surface* textSurface = TTF_RenderText_Solid(font, labelText.c_str(), textColor);
		if (textSurface) {
			SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
			if (textTexture) {
				int textWidth = textSurface->w;
				int textHeight = textSurface->h;

				// Center the text above the health bar
				SDL_Rect textRect = { x + (width / 2) - (textWidth / 2), y - textHeight - 5, textWidth, textHeight };
				SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);

				SDL_DestroyTexture(textTexture);
			}
			SDL_FreeSurface(textSurface);
		}
	}
}