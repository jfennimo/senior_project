#pragma once
#include "GameMode.h"
#include "GameState.h"
#include "SDL.h"
#include "SDL_image.h"
#include "SDL_ttf.h"
#include "TextureManager.h"
#include "WordListManager.h"
#include "Map.h"
#include "ECS/Components.h"
#include "Vector2D.h"
#include "Collision.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <format>
#include <sstream>
#include <unordered_map>
#include <vector> // For wordlist and zombie count
#include <cstdlib> // For rand() and srand()
#include <ctime>   // For time()

// Forward declarations
class Game;
class UIManager;

class ArcadeMode : public GameMode {
public:
	ArcadeMode(Game* game);

	void update() override;
	void render() override;
	void handleEvent(SDL_Event& event) override;

	// Arcade mode switch statement methods

	// Update methods
	void updateArcadeStage();
	void updateBonusStage();

	// Render methods
	void renderArcadeStage();
	void renderBonusStage();

	// Handle Event methods
	void handleArcadeEvent(SDL_Event& event);
	void handleBonusEvent(SDL_Event& event);

	// Arcade Mode public methods..!
	void resetGame();
	void nextLevel();
	void bonusStage();
	void updateHandSprites();
	void resetHandSprites();
	void updateBarrierDamage(int barrierHP);
	void checkCombo(const std::string& input, const std::string& target);
	void fireLaser();
	
	// Phase enum
	enum class Phase {
		ARCADE,
		BONUS
	};

	void setPhase(Phase newPhase);

private:
	Game* game;
	Phase currentPhase = Phase::ARCADE;

	// Managers
	Manager manager;
	Map* map = nullptr;
	WordListManager wordManager;
	WordListManager::Difficulty difficulty;

	// Entities
	Entity* player = nullptr;
	Entity* barrier = nullptr;
	Entity* leftHand = nullptr;
	Entity* rightHand = nullptr;
	Entity* crosshair = nullptr;
	Entity* laserMiddle = nullptr;
	Entity* laserLeft = nullptr;
	Entity* laserRight = nullptr;
	Entity* comboMeter = nullptr;
	Entity* laserPowerUp = nullptr;
	Entity* exclamation = nullptr;

	// Arcade Mode Wordlists
	std::vector<std::string> words;
	std::vector<std::string> bonusLeft;
	std::vector<std::string> bonusRight;

	// Holds current target prompt
	std::string targetText;

	// Zombie entities and active zombie index
	std::vector<Entity*> zombies;
	std::vector<Entity*> leftToRight;
	std::vector<Entity*> rightToLeft;
	std::vector<Entity*> tombstones;
	size_t currentZombieIndex = 0; // Tracks the currently active zombie
	bool allZombiesTransformed = false; // Tracks if all zombies are defeated (transformed) or not
	bool barrierUnderAttack = false; // Track if zombies are attacking

	// Logic variables
	//Uint32 currentTime; ???
	
	// Blinking text variables
	Uint32 lastBlinkTime = 0;
	bool showBlinkText;
	const Uint32 BLINK_DELAY = 1200;

	// For storing typed text
	std::string userInput = "";

	// Barrier orb (player) dimensions / placement
	const int barrierWidth = 64;
	const int barrierScale = 2;
	int playerX;
	int barrierX;

	// Middle laser cannon placement
	int laserX;

	// For barrier UI / logic
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
	int finalCorrectLetters = 0;
	int finalTotalLetters = 0;

	// Bonus stage
	int bonusLevel = 0;
	bool inBonusStage = false;
	bool leftGroupDefeated = false;
	float bonusSpeed = 3.0f; // bonus zombie speed!
	int bonusZombiesDefeated;
	int totalBonusZombies;
	std::string totalBonusZombiesDefeated;

	// Private helper methods
	std::string getFormattedWrongLetters();
	std::string getFormattedAccuracy();
	std::string getOverallAccuracy();

	// BUG TESTING
	//bool exclamationShouldBeDestroyed = false;
};
