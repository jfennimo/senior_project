#pragma once
#include <string>
#include "SDL.h"
#include "SDL_ttf.h"

class UIManager {
private:
	SDL_Renderer* renderer;

public:
	// Constructor
	UIManager(SDL_Renderer* ren);

	void drawText(const std::string& text, int x, int y, SDL_Color color, TTF_Font* font);
	void drawRectangle(int x, int y, int width, int height, SDL_Color color);
	void drawHealthbar(int x, int y, int width, int height, int currentHealth, int maxHealth, SDL_Color outlineColor, SDL_Color fgColor, SDL_Color bgColor, const std::string& labelText, TTF_Font* font, SDL_Color textColor);
};
