#ifndef Game_h
#define Game_h
#include "SDL.h"
#include "SDL_image.h"
#include "UIManager.h"
#include "GameState.h"
#include <vector>
#include <iostream>
#include <sstream>

class Game {

public:
	Game();
	~Game();

	void init(const char* title, int width, int height, bool fullscreen);
	
	void handleEvents();
	void update();
	void render();
	void clean();

	bool running() {
		return isRunning;
	}

	static SDL_Renderer *renderer;
	static SDL_Event event;

	TTF_Font* titleFont;
	TTF_Font* menuFont;
	TTF_Font* healthFont;

	GameState gameState;

private:
	bool isRunning = false;
	int cnt = 0;
	bool showBlinkText = true;       // Controls whether the text is visible
	Uint32 lastBlinkTime = 0;        // Tracks the last time the blink toggled
	const Uint32 BLINK_DELAY = 1000; // 1000 ms = 1 second

	SDL_Window* window;
	UIManager* uiManager;
	std::string userInput = ""; // for storing typed text
	bool isZombieTransformed = false; // to prevent multiple transformations!


	// Results screen variables

	// Barrier HP
	std::string hpResults;
	int barrierHP;
	const int maxHP = 500;

	// Letters typed incorrectly
	std::vector<char> typedWrong;
	std::vector<bool> processedInput;
	std::string wrongResults;
	std::ostringstream formattedResults;
	std::string finalWrongResults;

	// Overall accuracy
	std::ostringstream accuracyStream;
	std::string overallAccuracy;
	bool resultsCalculated = false;
	int correctLetters = 0;
	int totalLetters = 0;

};

#endif
