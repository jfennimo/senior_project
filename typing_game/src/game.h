#pragma once
#include "SDL.h"
#include "SDL_image.h"
#include "UIManager.h"
#include "GameState.h"
#include "GameMode.h"
//#include "ECS/Components.h"
#include <memory>
#include <vector>
#include <iostream>
#include <sstream>
#include <unordered_map>

class Game {

public:
	Game();
	~Game();

	void init(const char* title, int width, int height, bool fullscreen);
	
	void handleEvents();
	void update();
	void render();
	void clean();

	void nextArcadeLevel();
	void resetArcadeMode();

	bool running() {
		return isRunning;
	}

	void changeState(GameState newState);

	static SDL_Renderer *renderer;
	static SDL_Event event;

	//Manager manager;
	UIManager* uiManager;

	// Fonts
	TTF_Font* titleFont;
	TTF_Font* menuFont;
	TTF_Font* healthFont;
	TTF_Font* roundFont;
	TTF_Font* gameOverFont;
	TTF_Font* controlPanelFont;
	TTF_Font* statusFont;
	TTF_Font* threatLvlFont;
	TTF_Font* comboStatusFont;

	// Game state / Game mode members
	GameState gameState;
	std::unique_ptr<GameMode> currentMode;
	bool startArcadeInBonus = false;
	
	// Cached results screen variables
	std::string cachedHpResults;
	std::string cachedWrongResults;
	std::string cachedAccuracy;
	std::string cachedLevel;

	// Cached bonus results screen variables
	std::string cachedBonusHpResults;
	std::string cachedBonusWrongResults;
	std::string cachedBonusZombiesDefeated;
	std::string cachedBonusAccuracy;

	// Cached game over results screen variables
	std::string cachedHighestLevel;
	std::string cachedZombiesDefeated;
	std::string cachedOverallAccuracy;

	// TESTING
	bool arcadeJustStarted = false;
	bool nextLevelToStart = false;


private:
	bool isRunning = false;
	int cnt = 0;
	bool showBlinkText = true;       // Controls whether the text is visible
	Uint32 lastBlinkTime = 0;        // Tracks the last time the blink toggled
	const Uint32 BLINK_DELAY = 1200; // 1000 ms = 1 second
	Uint32 currentTime;

	SDL_Window* window;

	// Screen size
	int screenWidth;
	int screenHeight;
};