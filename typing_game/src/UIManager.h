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
	void drawCenteredText(const std::string& text, int y, SDL_Color color, TTF_Font* font, int screenWidth);
	void drawHealthbar(int x, int y, int width, int height, int currentHealth, int maxHealth, const std::string& labelText, SDL_Color outlineColor, SDL_Color fgColor, SDL_Color bgColor, TTF_Font* font, SDL_Color textColor);
	void drawStatusBar(int x, int y, int width, int height, const std::string& labelText, const std::string& statusText, SDL_Color outlineColor, SDL_Color bgColor, TTF_Font* labelFont, TTF_Font* statusFont, SDL_Color textColor);
	void drawThreatLvl(int x, int y, int width, int height, int threatLvl, const std::string& labelText, SDL_Color outlineColor, SDL_Color bgColor, TTF_Font* labelFont, TTF_Font* digitFont, SDL_Color textColor);
	void drawComboAlert(int x, int y, int width, int height, int comboLevel, const std::string& labelText, const std::string& statusText, SDL_Color outlineColor, SDL_Color bgColor, TTF_Font* labelFont, TTF_Font* statusFont, SDL_Color textColor);
	void drawTimeElapsed(int x, int y, int width, int height, int elapsedSeconds, const std::string& labelText, SDL_Color outlineColor, SDL_Color bgColor, TTF_Font* labelFont, TTF_Font* digitFont, SDL_Color textColor);
};