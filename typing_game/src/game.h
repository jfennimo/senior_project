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

	void updateBarrierDamage(int barrierHP);

	void checkCombo(const std::string& input, const std::string& target);

	void fireLaser();

	//void transformZombie(Entity* zombie);


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
	TTF_Font* controlPanelFont;
	TTF_Font* statusFont;
	TTF_Font* threatLvlFont;

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

	// Screen size
	int screenWidth;
	int screenHeight;
	int centerX = 800;

	// Barrier orb (player) dimensions / placement
	const int barrierWidth = 64;
	const int barrierScale = 2;
	int barrierX;

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

	// Laser
	//Entity* laser = nullptr;
	bool laserActive = false;
	float laserSpeed = 1.0f; // may need to adjust

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
	const int maxHP = 100;
	int damageLevel = 0;

	// For screen shake
	int shakeDuration = 0;
	int shakeMagnitude = 0;
	int shakeOffsetX = 0;
	int shakeOffsetY = 0;

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
