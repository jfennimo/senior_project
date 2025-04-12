#pragma once
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

	bool running() {
		return isRunning;
	}

	// Arcade Mode methods
	void nextLevel();
	void bonusStage();
	void resetArcadeMode();
	void updateHandSprites();
	void resetHandSprites();
	void updateBarrierDamage(int barrierHP);
	void checkCombo(const std::string& input, const std::string& target);
	void fireLaser();

	static SDL_Renderer* renderer;
	static SDL_Event event;

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

	GameState gameState;

private:
	bool isRunning = false;
	int cnt = 0;
	bool showBlinkText = true;       // Controls whether the text is visible
	Uint32 lastBlinkTime = 0;        // Tracks the last time the blink toggled
	const Uint32 BLINK_DELAY = 1200; // 1000 ms = 1 second
	Uint32 currentTime;

	SDL_Window* window;

	// MAIN MENU VARIABLES
	int mainMenuSelection = 0; // 0 = Arcade, 1 = Records
	int arcadeModeSelection = 0;  // 0 = "How To Play", 1 = "Start"
	int pauseMenuSelection = 0; // 0 = Resume, 1 = Quit



	// ARCADE MODE VARIABLES BELOW:
	// 
	// 
	// 
	// For storing typed text
	std::string userInput = "";

	// Screen size
	int screenWidth;
	int screenHeight;
	int centerX = 800;

	// Barrier orb (player) dimensions / placement
	const int barrierWidth = 64;
	const int barrierScale = 2;
	int playerX;
	int barrierX;

	// Middle laser cannon placement
	int laserX;

	// For barrier UI / logic
	bool barrierUnderAttack = false; // Track if zombies are attacking
	Uint32 lastFlashTime = 0; // Store last time the sprite switched
	bool flashState = false;  // Track if the barrier is currently in its "flashed" state
	Uint32 lastAttackTime = 0;

	// Control Panel UI
	std::string statusText = "OK";
	bool wordTypedWrong = false;

	// For rendering cursor
	int cursorBlinkSpeed;
	bool showCursor;

	// Combo
	bool brokenCombo = false;
	bool laserReady = false;
	int comboLevel = 0;
	std::string comboStatus;

	// Basic laser
	struct LaserStrike {
		int startX, startY; // Laser source (bottom of middle cannon)
		int endX, endY;     // Target position (zombie's center)
		int duration;       // How long the beam lasts (in frames)
	};

	std::vector<LaserStrike> activeLasers;

	// Laser power up
	bool laserActive = false;
	float laserSpeed = 2.0f; // may need to adjust

	// Zombie variables
	float speed = 0.5f; // How fast the zombies move toward the player
	bool isZombieTransformed = false; // to prevent multiple transformations!

	// Results screen variables
	int level = 1;
	int zombieCount = 0;
	int zombiesDefeated = 0;

	// Barrier variables
	std::string hpResults;
	//int resultsHP;
	int barrierHP;
	int bonusHP;
	const int maxHP = 100;
	int damageLevel = 0;

	// For screen shake
	int shakeDuration = 0;
	int shakeMagnitude = 0;
	int shakeOffsetX = 0;
	int shakeOffsetY = 0;

	// For game pause before screen transition to level results
	bool nextLevelDelayStarted = false;
	int nextLevelDelayTimer = 0;

	// For game pause before screen transition to game over
	bool barrierDestroyed = false;
	int gameOverDelayTimer = 0; // in frames

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
	int sessionCorrectLetters = 0;
	int sessionTotalLetters = 0;

	// Bonus stage
	int bonusLevel = 0;
	bool inBonusStage = false;
	bool leftGroupDefeated = false;
	float bonusSpeed = 3.0f; // bonus zombie speed!
	int bonusZombiesDefeated;
	int totalBonusZombies;
	std::string totalBonusZombiesDefeated;

	// Records screen variables
	bool gameOverStatsUpdated = false;

	// Accuracy
	int y;
	float recordsAccuracy = 0.0f;
	std::string accuracyText;
	int finalCorrectLetters = 0; // lifetime total
	int finalTotalLetters = 0;
	std::unordered_map<char, int> lifetimeWrongLetters; // Track how often each wrong letter is typed
};