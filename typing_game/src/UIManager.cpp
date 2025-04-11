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

// Center text on the screen
void UIManager::drawCenteredText(const std::string& text, int y, SDL_Color color, TTF_Font* font, int screenWidth) {
	int textWidth, textHeight;
	TTF_SizeText(font, text.c_str(), &textWidth, &textHeight);
	int x = (screenWidth / 2) - (textWidth / 2);
	drawText(text, x, y, color, font);
}

// Health bar
void UIManager::drawHealthbar(int x, int y, int width, int height, int currentHealth, int maxHealth, const std::string& labelText, SDL_Color outlineColor, SDL_Color fgColor, SDL_Color bgColor, TTF_Font* font, SDL_Color textColor) {
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

	// Render text to the left of the health bar
	if (font) {
		SDL_Surface* textSurface = TTF_RenderText_Solid(font, labelText.c_str(), textColor);
		if (textSurface) {
			SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
			if (textTexture) {
				int textWidth = textSurface->w;
				int textHeight = textSurface->h;

				// Align text to the left of the health bar, vertically centered
				SDL_Rect textRect = {
					x - textWidth - 10,                  // 10px padding to the left
					y + (height / 2) - (textHeight / 2), // Vertically centered with the bar
					textWidth,
					textHeight
				};

				SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);
				SDL_DestroyTexture(textTexture);
			}
			SDL_FreeSurface(textSurface);
		}
	}
}

// Status bar
void UIManager::drawStatusBar(int x, int y, int width, int height, const std::string& labelText, const std::string& statusText, SDL_Color outlineColor, SDL_Color bgColor, TTF_Font* labelFont, TTF_Font* statusFont, SDL_Color textColor) {
	Uint32 ticks = SDL_GetTicks();
	bool showStatusText = true;

	// Make status text flash on/off every 300 ms
	if (statusText == "LASER READY") {
		showStatusText = (ticks / 300) % 2 == 0;
	}
	else if (statusText == "ERROR") {
		showStatusText = (ticks / 300) % 2 == 0;
	}
	else if (statusText == "DANGER") {
		showStatusText = (ticks / 300) % 2 == 0;
	}

	// Determine fill color based on status
	SDL_Color fillColor;

	if (statusText == "OK") {
		fillColor = { 0, 255, 0, 255 }; // Green
	}
	else if (statusText == "CAUTION") {
		fillColor = { 255, 255, 0, 255 }; // Yellow
	}
	else if (statusText == "CRITICAL") {
		fillColor = { 255, 165, 0, 255 }; // Orange
	}
	else if (statusText == "DANGER") {
		fillColor = { 255, 0, 0, 255 }; // Red
	}
	else if (statusText == "ERROR") {
		fillColor = { 255, 0, 0, 255 }; // Red
	}
	else if (statusText == "LASER READY") {
		fillColor = { 0, 0, 255, 255 }; // Blue
	}
	else {
		fillColor = { 128, 128, 128, 255 }; // Default: gray
	}

	// Outline around status bar
	SDL_Rect outlineRect = { x - 2, y - 2, width + 4, height + 4 };
	SDL_SetRenderDrawColor(renderer, outlineColor.r, outlineColor.g, outlineColor.b, outlineColor.a);
	SDL_RenderFillRect(renderer, &outlineRect);

	// Background
	SDL_Rect bgRect = { x, y, width, height };
	SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
	SDL_RenderFillRect(renderer, &bgRect);

	// Fill color
	SDL_Rect fillRect = { x, y, width, height };
	SDL_SetRenderDrawColor(renderer, fillColor.r, fillColor.g, fillColor.b, fillColor.a);
	SDL_RenderFillRect(renderer, &fillRect);

	// Render text to the left of the status bar
	if (labelFont) {
		SDL_Surface* textSurface = TTF_RenderText_Solid(labelFont, labelText.c_str(), textColor);
		if (textSurface) {
			SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
			if (textTexture) {
				int textWidth = textSurface->w;
				int textHeight = textSurface->h;

				// Align text to the left of the health bar, vertically centered
				SDL_Rect textRect = {
					x - textWidth - 10,                  // 10px padding to the left
					y + (height / 2) - (textHeight / 2), // Vertically centered with the bar
					textWidth,
					textHeight
				};

				SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);
				SDL_DestroyTexture(textTexture);
			}
			SDL_FreeSurface(textSurface);
		}
	}

	// Render status text centered in the bar
	if (statusFont && showStatusText) {
		SDL_Surface* textSurface = TTF_RenderText_Blended(statusFont, statusText.c_str(), textColor);

		if (textSurface) {
			SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
			if (textTexture) {
				int textWidth = textSurface->w;
				int textHeight = textSurface->h;

				int verticalOffset = 4;

				SDL_Rect textRect = {
					x + (width / 2) - (textWidth / 2),
					y + (height / 2) - (textHeight / 2) + verticalOffset,
					textWidth,
					textHeight
				};

				SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);
				SDL_DestroyTexture(textTexture);
			}
			SDL_FreeSurface(textSurface);
		}
	}
}

// Threat level counter
void UIManager::drawThreatLvl(int x, int y, int width, int height, int threatLvl, const std::string& labelText, SDL_Color outlineColor, SDL_Color bgColor, TTF_Font* labelFont, TTF_Font* digitFont, SDL_Color textColor) {
	// Outline around the square
	SDL_Rect outlineRect = { x - 2, y - 2, width + 4, height + 4 };
	SDL_SetRenderDrawColor(renderer, outlineColor.r, outlineColor.g, outlineColor.b, outlineColor.a);
	SDL_RenderFillRect(renderer, &outlineRect);

	// Background inside the square
	SDL_Rect bgRect = { x, y, width, height };
	SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
	SDL_RenderFillRect(renderer, &bgRect);

	// === Render the THREAT LEVEL digit in the center ===
	if (digitFont) {
		std::string lvlStr = std::to_string(threatLvl);
		SDL_Surface* lvlSurface = TTF_RenderText_Blended(digitFont, lvlStr.c_str(), textColor);
		if (lvlSurface) {
			SDL_Texture* lvlTexture = SDL_CreateTextureFromSurface(renderer, lvlSurface);
			if (lvlTexture) {
				int textW = lvlSurface->w;
				int textH = lvlSurface->h;

				SDL_Rect textRect = {
					x + (width / 2) - (textW / 2),
					y + (height / 2) - (textH / 2),
					textW,
					textH
				};

				SDL_RenderCopy(renderer, lvlTexture, nullptr, &textRect);
				SDL_DestroyTexture(lvlTexture);
			}
			SDL_FreeSurface(lvlSurface);
		}
	}

	// === Render the label text BELOW the square ===
	if (labelFont) {
		SDL_Surface* labelSurface = TTF_RenderText_Blended(labelFont, labelText.c_str(), textColor);
		if (labelSurface) {
			SDL_Texture* labelTexture = SDL_CreateTextureFromSurface(renderer, labelSurface);
			if (labelTexture) {
				int labelW = labelSurface->w;
				int labelH = labelSurface->h;

				SDL_Rect labelRect = {
					x + (width / 2) - (labelW / 2),
					y + height + 6, // 6px padding below the square
					labelW,
					labelH
				};

				SDL_RenderCopy(renderer, labelTexture, nullptr, &labelRect);
				SDL_DestroyTexture(labelTexture);
			}
			SDL_FreeSurface(labelSurface);
		}
	}
}

void UIManager::drawComboAlert(int x, int y, int width, int height, int comboLevel, const std::string& labelText, const std::string& statusText, SDL_Color outlineColor, SDL_Color bgColor, TTF_Font* labelFont, TTF_Font* statusFont, SDL_Color textColor)
{
	if (statusText == "MAX!") {
		bgColor = { 102, 255, 105, 255 }; // green
	}
	else if (statusText == "X") {
		bgColor = { 255, 102, 102, 255 }; // red
	}

	// === Render the label text on the left ===
	if (labelFont) {
		SDL_Surface* labelSurface = TTF_RenderText_Blended(labelFont, labelText.c_str(), textColor);
		if (labelSurface) {
			SDL_Texture* labelTexture = SDL_CreateTextureFromSurface(renderer, labelSurface);
			if (labelTexture) {
				int labelW = labelSurface->w;
				int labelH = labelSurface->h;

				SDL_Rect labelRect = {
					x,
					y + (height / 2) - (labelH / 2),
					labelW,
					labelH
				};

				SDL_RenderCopy(renderer, labelTexture, nullptr, &labelRect);
				SDL_DestroyTexture(labelTexture);

				// === Draw the rectangle next to the label ===
				int rectX = labelRect.x + labelRect.w + 10; // 10px padding between label and box

				SDL_Rect outlineRect = { rectX - 2, y - 2, width + 4, height + 4 };
				SDL_SetRenderDrawColor(renderer, outlineColor.r, outlineColor.g, outlineColor.b, outlineColor.a);
				SDL_RenderFillRect(renderer, &outlineRect);

				SDL_Rect bgRect = { rectX, y, width, height };
				SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
				SDL_RenderFillRect(renderer, &bgRect);

				// === Render the statusText centered inside the box ===
				SDL_Surface* statusSurface = TTF_RenderText_Blended(statusFont, statusText.c_str(), textColor);
				if (statusSurface) {
					SDL_Texture* statusTexture = SDL_CreateTextureFromSurface(renderer, statusSurface);
					if (statusTexture) {
						int statusW = statusSurface->w;
						int statusH = statusSurface->h;

						SDL_Rect statusRect = {
							rectX + (width / 2) - (statusW / 2),
							y + (height / 2) - (statusH / 2),
							statusW,
							statusH
						};

						SDL_RenderCopy(renderer, statusTexture, nullptr, &statusRect);
						SDL_DestroyTexture(statusTexture);
					}
					SDL_FreeSurface(statusSurface);
				}
			}
			SDL_FreeSurface(labelSurface);
		}
	}
}