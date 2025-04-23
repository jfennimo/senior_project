#pragma once
#include "SDL.h"
#include "SDL_image.h"
#include "UIManager.h"
#include "WordListManager.h"
#include "GameState.h"
#include "SaveSystem.h"
#include <vector>
#include <iostream>
#include <sstream>
#include <unordered_map>

class Game {

public:
	Game();
	~Game();

	void init(const char* title, int width, int height, bool fullscreen);

	// Game methods for main method
	void handleEvents();
	void update();
	void render();
	void clean();

	bool running() {
		return isRunning;
	}

	// Public methods
	//
	//
	//

	// Lessons Mode methods
	void resetLessonsMode(WordListManager::Difficulty lessonDifficulty);
	void exitLessonsMode();
	void calculateLessonResults();

	// Arcade Mode methods
	void resetArcadeMode();
	void exitArcadeMode();
	void nextLevel();
	void bonusStage();
	void updateBarrierDamage(int barrierHP);
	void checkCombo(const std::string& input, const std::string& target);
	void fireLaser();

	// WPM Test methods
	void resetWPMTest();
	void shiftWpmLines();
	int countWords(const std::string& line);
	std::string generateRandomLine();
	void calculateWPM();
	std::string getTypingTitle(int highestWpm);

	// Results method(s)
	void calculateAverageRecords();

	// Shared method(s)
	void updateHandSprites(const std::string& targetText, const std::string& userInput);
	std::string formatPercentage(float value);

	// Save/Load methods
	void syncToSaveData();
	void syncFromSaveData();
	void saveProgress();
	void loadProgress();


	// Public Members
	//
	//
	//

	// Holds saved lesson progress
	std::unordered_map<WordListManager::Difficulty, SaveSystem::LessonProgress> lessonProgressMap;

	static SDL_Renderer* renderer;
	static SDL_Event event;

	SaveSystem::SaveData saveData;

private:
	bool isRunning = false;
	int cnt = 0;
	bool showBlinkText = true;       // Controls whether the text is visible
	Uint32 lastBlinkTime = 0;        // Tracks the last time the blink toggled
	const Uint32 BLINK_DELAY = 1200; // 1000 ms = 1 second
	Uint32 currentTime;

	SDL_Window* window;

	// Main menu variables:
	//
	//
	//
	int mainMenuSelection = 0; // 0 = Lessons, 1 = Arcade, 2 = Records, 3 = WPM Test
	int lessonsMenuSelection = 0; // 0 = How To Play, 1 = Start
	int lessonsLevelSelection = 0; // 0 - ? = Lesson selection
	int arcadeMenuSelection = 0;  // 0 = How To Play, 1 = Start
	int pauseMenuSelection = 0; // 0 = Resume, 1 = Quit

	SDL_Color howToColor;
	SDL_Color startColor;

	// Lessons mode variables:
	//
	//
	//
	SDL_Color panelColor;
	SDL_Color screenColor;
	SDL_Color outlineColor;

	bool lessonTimeFrozen = false;
	int lessonsCompleted;
	int totalLessons = 10;
	int baseY;
	int lessonScrollX = 0;
	const int lessonFixedCursorX = 800; // X position of cursor
	int typedWidth;
	int refLineLetterX = 800;

	std::vector<SDL_Texture*> lessonCharTextures; // For reference line
	std::vector<int> lessonCharWidths;

	std::vector<SDL_Texture*> typedCharTextures;  // For typed line
	std::vector<int> typedCharWidths;
	std::vector<SDL_Color> typedCharColors;

	int lessonTargetY;
	int lessonInputY;
	int lessonLetterX;

	std::string lessonCurrentLine;
	std::string lessonUserInput;

	bool zombie1Defeated = false;
	bool zombie2Defeated = false;
	bool zombie3Defeated = false;
	bool zombie4Defeated = false;

	bool lessonPassed = false;
	bool lessonFullyCompleted = false;

	float lessonCompletion = 0.0f;
	float lessonTargetCompletion = 0.0f;
	int correctChars = 0;
	float fillSpeed = 0.0f;
	int zombiesRemaining = 4;

	bool isCorrect;
	int lessonCorrectChars = 0;
	int lessonTotalTypedChars = 0;
	int lessonIncorrectChars = 0;

	bool lessonsDelayTimerStarted = false;
	int lessonsResultsDelayTimer = 0;

	int lessonTimeElapsed = 0;
	int lessonStartTime = 0;
	int lessonResultTime = 0;

	std::string lessonSummary;


	// Arcade mode variables:
	// 
	// 
	//
	// For storing typed text
	std::string userInput;

	// Holds current target prompt
	std::string targetText;

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
	std::vector<bool> processedInput;
	std::string wrongResults;
	std::ostringstream formattedResults;
	std::string finalWrongResults;

	// Key-to-finger sprite mapping
	std::unordered_map<char, std::string> keyToFingerMap;

	// Overall accuracy
	double levelAccuracy = 0.0;
	double totalAccuracy = 0.0;
	double arcadeSessionAccuracy = 0.0;
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


	// Records screen variables:
	//
	//
	//
	bool lessonResultsStatsUpdated = false;
	bool arcadeResultsStatsUpdated = false;
	bool wpmResultsStatsUpdated = false;

	// Games played
	int lessonGamesPlayed = 0;
	int arcadeGamesPlayed = 0;
	int wpmGamesPlayed = 0;

	// Accuracy for each time a game mode was played
	float lessonAccuracyTotal = 0.0f;
	float arcadeAccuracyTotal = 0.0f;
	float wpmAccuracyTotal = 0.0f;

	// Overall accuracy
	float recordsLessonAccuracy = 0.0f;
	float recordsArcadeAccuracy = 0.0f;
	float recordsWpmAccuracy = 0.0f;
	float recordsOverallAccuracy = 0.0f;

	// Highest WPM achieved / WPM title
	int highestWpm;
	std::string title;

	// Arcade highest level
	int arcadeHighestLevel = 0;

	// Lifetime wrong characters (characters typed incorrectly throughout playtime)
	// Just for draw positioning
	int column;
	int row;
	int itemsPerColumn;
	int startX;
	int startY;
	int spacingY;
	int spacingX;
	int y; 
	int entryIndex;
	std::unordered_map<char, int> lifetimeWrongCharacters; // Track how often each wrong letter is typed
	std::vector<std::pair<char, int>> sortedWrongCharacters; // Sorts characters by most common to least common

	// WPM Test variables:
	//
	//
	//
	Uint32 lastSecondTick = 0;

	std::string wpmTopLine;
	std::string wpmCurrentLine;
	std::string wpmNextLine;
	std::string wpmUserInput;

	bool wpmTestStarted = false;
	bool wpmTestEnded = false;

	int wpmTimeRemaining = 60;
	//int wpmTypedWords = 0; // Can safely delete, but may keep for results
	float rawWpm = 0.0f;
	float wpmAccuracy = 0.0f;
	float wpm = 0.0f;
	int wpmCorrectChars = 0;
	int wpmTotalTypedChars = 0;
	int wpmIncorrectChars = 0;

	// WPM Render Variables
	int lineStartX;
	int topLineY;
	int middleLineY;
	int bottomLineY;
	int letterX;
	int cursorX;


	// Shared variables:
	//
	//
	//
	SDL_Color fgColor;
	SDL_Color bgColor;
	SDL_Color comboColor;
	SDL_Color textColor;
	SDL_Color correct;
	SDL_Color wrong;
	SDL_Color neutral;

	std::unordered_map<char, int> typedWrong;
	std::string currentLeftTex;
	std::string currentRightTex;
};