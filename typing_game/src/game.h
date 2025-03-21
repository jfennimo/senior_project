#ifndef Game_h
#define Game_h
#include "SDL.h"
#include "SDL_image.h"
#include "UIManager.h"
#include "GameState.h"
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

	void nextLevel();
	void bonusStage();
	void resetGame();

	void updateHandSprites();

	void resetHandSprites();

	//void updateBarrierSprite();

	bool running() {
		return isRunning;
	}

	static SDL_Renderer *renderer;
	static SDL_Event event;

	TTF_Font* titleFont;
	TTF_Font* menuFont;
	TTF_Font* healthFont;
	TTF_Font* gameOverFont;

	GameState gameState;

private:
	bool isRunning = false;
	int cnt = 0;
	bool showBlinkText = true;       // Controls whether the text is visible
	Uint32 lastBlinkTime = 0;        // Tracks the last time the blink toggled
	const Uint32 BLINK_DELAY = 1000; // 1000 ms = 1 second
	Uint32 currentTime;

	SDL_Window* window;
	UIManager* uiManager;
	std::string userInput = ""; // for storing typed text
	bool isZombieTransformed = false; // to prevent multiple transformations!

	// For barrier UI
	bool barrierUnderAttack = false; // Track if zombies are attacking
	Uint32 lastFlashTime = 0; // Store last time the sprite switched
	bool flashState = false;  // Track if the barrier is currently in its "flashed" state

	// Zombie speed!!
	float speed = 0.5f; // How fast the zombies move towards the player

	// Results screen variables
	int level = 1;
	int zombieCount = 0;
	int zombiesDefeated = 0;

	// Barrier HP
	std::string hpResults;
	int resultsHP;
	int barrierHP;
	int bonusHP;
	const int maxHP = 500;

	// Letters typed incorrectly
	std::vector<char> typedWrong;
	std::vector<bool> processedInput;
	std::string wrongResults;
	std::ostringstream formattedResults;
	std::string finalWrongResults;

	// Key-to-finger sprite mapping
	std::unordered_map<char, std::string> keyToFingerMap;

	// Overall accuracy
	double levelAccuracy = 0.0;
	double totalAccuracy = 0.0;
	std::ostringstream levelAccuracyStream;
	std::string overallAccuracy;
	bool resultsCalculated = false;
	int levelCorrectLetters = 0;
	int levelTotalLetters = 0;
	int finalCorrectLetters = 0;
	int finalTotalLetters = 0;

	// Bonus stage
	int bonusLevel = 0;
	bool inBonusStage = false;
	bool leftGroupDefeated = false;
	float bonusSpeed = 2.0f;
	int bonusZombiesDefeated;
	int totalBonusZombies;
	std::string totalBonusZombiesDefeated;
};

#endif
