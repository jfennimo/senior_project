#include "Game.h"
#include "TextureManager.h"
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
#include <vector> // For word lists and zombie count
#include <cstdlib> // For rand() and srand()
#include <ctime>   // For time()

// For switching game states (modes)
GameState gameState;
GameState prevState;

// Map / Managers
Map* map;
Manager manager;
UIManager* uiManager;
WordListManager wordManager;
WordListManager::Difficulty difficulty;

// Renderer and Event structures
SDL_Renderer* Game::renderer = nullptr;
SDL_Event Game::event;

// Frame timer
Uint32 currentTime;

// Lessons Mode Entities
Entity* zombie1 = nullptr;
Entity* zombie2 = nullptr;
Entity* zombie3 = nullptr;
Entity* zombie4 = nullptr;

// Arcade Mode Entities
auto& player(manager.addEntity());
Entity* barrier = nullptr;
Entity* laserLeft = nullptr;
Entity* laserRight = nullptr;
Entity* comboMeter = nullptr;
Entity* laserPowerup = nullptr;
Entity* exclamation = nullptr;

// Shared Entities
Entity* leftHand = nullptr;
Entity* rightHand = nullptr;
Entity* laserMiddle = nullptr;
Entity* crosshair = nullptr;

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
TTF_Font* wpmFont;

// Wordlists
std::vector<std::string> lessonWords;
std::vector<std::string> arcadeWords;
std::vector<std::string> bonusLeft;
std::vector<std::string> bonusRight;

// Holds current lesson difficulty
WordListManager::Difficulty currentLessonDifficulty;

// Arcade mode zombie entities and active zombie index
std::vector<Entity*> zombies;
std::vector<Entity*> leftToRight;
std::vector<Entity*> rightToLeft;
std::vector<Entity*> tombstones;
size_t currentZombieIndex = 0; // Tracks the currently active zombie

// Zombie logic booleans
bool allZombiesTransformed = false;
bool barrierUnderAttack = false; // Track if zombies are attacking

Game::Game()
{
	uiManager = nullptr;
}
Game::~Game()
{
	delete uiManager;
}

// Arcade mode initiliazation (may move to reset game function)
void Game::init(const char* title, int width, int height, bool fullscreen)
{
	int flags = 0;

	if (fullscreen)
	{
		flags = SDL_WINDOW_FULLSCREEN;
	}

	if (SDL_Init(SDL_INIT_EVERYTHING) == 0)
	{
		std::cout << "Subsystems Intialized..." << std::endl;

		window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, flags);
		if (window)
		{
			std::cout << "Window created!" << std::endl;
		}

		renderer = SDL_CreateRenderer(window, -1, 0);
		if (renderer)
		{
			SDL_SetRenderDrawColor(renderer, 160, 160, 160, 0);
			std::cout << "Renderer created!" << std::endl;
		}

		if (TTF_Init() == -1) {
			std::cout << "Failed to initialize SDL_ttf... " << TTF_GetError() << std::endl;
			return;
		}

		isRunning = true;
	}
	else {
		std::cout << "SDL Initialization Failed!" << std::endl;
		isRunning = false;
	}

	// Load current save if there is one, otherwise create new save
	loadProgress();

	gameState = GameState::TITLE_SCREEN; // Initial state

	uiManager = new UIManager(renderer);
	map = new Map();

	screenWidth = width;
	screenHeight = height;

	// Ensuring player and barrier are centered for arcade mode
	playerX = screenWidth / 2;
	barrierX = (screenWidth / 2) - ((barrierWidth * barrierScale) / 2);

	laserX = (screenWidth / 2) - ((68 * 2) / 2);

	titleFont = TTF_OpenFont("assets/PressStart2P.ttf", 30);
	menuFont = TTF_OpenFont("assets/PressStart2P.ttf", 20);
	healthFont = TTF_OpenFont("assets/PressStart2P.ttf", 20);
	roundFont = TTF_OpenFont("assets/PressStart2P.ttf", 16);
	gameOverFont = TTF_OpenFont("assets/PressStart2P.ttf", 100);
	controlPanelFont = TTF_OpenFont("assets/Square.ttf", 30);
	statusFont = TTF_OpenFont("assets/Technology-BoldItalic.TTF", 40);
	threatLvlFont = TTF_OpenFont("assets/Technology-BoldItalic.TTF", 50);
	comboStatusFont = TTF_OpenFont("assets/Technology-BoldItalic.TTF", 30);
	wpmFont = TTF_OpenFont("assets/PressStart2P.ttf", 25);
}


void Game::handleEvents()
{
	SDL_PollEvent(&event);
	switch (event.type) {
	case SDL_QUIT:
		isRunning = false;
		break;

	case SDL_KEYDOWN:
		switch (event.key.keysym.sym) {

		// For pressing enter/return (for menus)
		case SDLK_RETURN:
			if (gameState == GameState::TITLE_SCREEN) {
				gameState = GameState::MAIN_MENU; // Transition main menu
				std::cout << "Navigating to main menu!" << std::endl;
			}
			else if (gameState == GameState::MAIN_MENU) {
				if (mainMenuSelection == 0) {
					exitLessonsMode();
					exitArcadeMode();
					gameState = GameState::LESSONS_TITLE; // Transition to lessons mode
					std::cout << "Navigating to lessons title!" << std::endl;
				}
				else if (mainMenuSelection == 1) {
					exitLessonsMode();
					exitArcadeMode();
					gameState = GameState::ARCADE_TITLE; // Transition to arcade mode
					resetArcadeMode(); // Reset/initialize arcade mode as state is changing to arcade title
					std::cout << "Navigating to arcade title!" << std::endl;
				}
				else if (mainMenuSelection == 2) {
					exitLessonsMode();
					exitArcadeMode();
					calculateAverageRecords();
					gameState = GameState::RECORDS;
					std::cout << "Navigating to records screen!" << std::endl;
				}
				else if (mainMenuSelection == 3) {
					exitLessonsMode();
					exitArcadeMode();
					gameState = GameState::WPM_TEST;
					resetWPMTest();
					std::cout << "Navigating to WPM test!" << std::endl;
				}
			}
			else if (gameState == GameState::LESSONS_TITLE) {
				if (lessonsMenuSelection == 0) {
					gameState = GameState::LESSONS_HTP;
					std::cout << "Navigating to lessons how to play!" << std::endl;
				}
				else if (lessonsMenuSelection == 1) {
					gameState = GameState::LESSONS_SELECTION;
					std::cout << "Navigating to lessons level selection!" << std::endl;
				}
			}
			else if (gameState == GameState::LESSONS_SELECTION) {
				if (lessonsLevelSelection == 0) {
					currentLessonDifficulty = WordListManager::LESSON_0;
					resetLessonsMode(currentLessonDifficulty);
					gameState = GameState::LESSONS_MODE;
					std::cout << "Navigating to lesson 0!" << std::endl;
				}
				else if (lessonsLevelSelection == 1) {
					currentLessonDifficulty = WordListManager::LESSON_1;
					resetLessonsMode(currentLessonDifficulty);
					gameState = GameState::LESSONS_MODE;
					std::cout << "Navigating to lesson 1!" << std::endl;
				}
			}
			else if (gameState == GameState::LESSONS_RESULTS) {
				// Updating overall accuracy stats
				lessonGamesPlayed++;
				lessonAccuracyTotal += lessonCompletion;
				saveProgress(); // Save game
				gameState = GameState::LESSONS_SELECTION;
				std::cout << "Navigating back to lessons level selection!" << std::endl;
			}
			else if (gameState == GameState::ARCADE_TITLE) {
				if (arcadeMenuSelection == 0) {
					gameState = GameState::ARCADE_HTP;
					std::cout << "Navigating to arcade how to play!" << std::endl;
				}
				else if (arcadeMenuSelection == 1) {
					gameState = GameState::ARCADE_MODE;
					std::cout << "Navigating to arcade mode!" << std::endl;
				}
			}
			else if (gameState == GameState::ARCADE_RESULTS) {
				gameState = GameState::ARCADE_MODE; // Start next level of arcade mode
				nextLevel();
				std::cout << "Starting next round!" << std::endl;
			}
			else if (gameState == GameState::BONUS_TITLE) {
				gameState = GameState::BONUS_STAGE; // Start bonus round
				bonusStage();
				std::cout << "Starting bonus round!" << std::endl;
			}
			else if (gameState == GameState::BONUS_RESULTS) {
				gameState = GameState::ARCADE_MODE; // Start next level of arcade mode
				nextLevel();
				std::cout << "Starting next round!" << std::endl;
			}
			else if (gameState == GameState::GAME_OVER) {
				// Updating overall accuracy stats
				arcadeGamesPlayed++;
				arcadeAccuracyTotal += arcadeSessionAccuracy;
				saveProgress(); // Save progress after arcade game over
				gameState = GameState::MAIN_MENU;
				std::cout << "Returning to main menu!" << std::endl;
			}
			else if (gameState == GameState::WPM_RESULTS) {
				wpmGamesPlayed++;
				wpmAccuracyTotal += wpmAccuracy * 100.0f; // Converting to percent when storing
				saveProgress(); // Save progress after test
				gameState = GameState::MAIN_MENU;
			}
			else if (gameState == GameState::PAUSE) {
				if (pauseMenuSelection == 0) {
					// Resume to previous mode
					gameState = prevState;
					std::cout << "Resuming gameplay!" << std::endl;
				}
				else if (pauseMenuSelection == 1) {
					pauseMenuSelection = 0;
					gameState = GameState::MAIN_MENU;
					std::cout << "Navigating back to main menu!" << std::endl;
				}
			}
			break;

		case SDLK_UP:
			if (gameState == GameState::LESSONS_SELECTION) {
				lessonsLevelSelection = std::max(0, lessonsLevelSelection - 1);
			}
			else if (gameState == GameState::PAUSE) {
				pauseMenuSelection = std::max(0, pauseMenuSelection - 1);
			}
			break;

		case SDLK_DOWN:
			if (gameState == GameState::LESSONS_SELECTION) {
				lessonsLevelSelection = std::min(1, lessonsLevelSelection + 1);
			}
			else if (gameState == GameState::PAUSE) {
				pauseMenuSelection = std::min(1, pauseMenuSelection + 1);
			}
			break;

		case SDLK_LEFT:
			if (gameState == GameState::MAIN_MENU) {
				mainMenuSelection = std::max(0, mainMenuSelection - 1);  // Prevent going below 0
			}
			else if (gameState == GameState::LESSONS_TITLE) {
				lessonsMenuSelection = std::max(0, lessonsMenuSelection - 1);  // Prevent going below 0
			}
			else if (gameState == GameState::ARCADE_TITLE) {
				arcadeMenuSelection = std::max(0, arcadeMenuSelection - 1);  // Prevent going below 0
			} 
			break;

		case SDLK_RIGHT:
			if (gameState == GameState::MAIN_MENU) {
				mainMenuSelection = std::min(3, mainMenuSelection + 1); // 0, 1, or 2
			}
			else if (gameState == GameState::LESSONS_TITLE) {
				lessonsMenuSelection = std::min(1, lessonsMenuSelection + 1);  // Prevent going above 1
			}
			else if (gameState == GameState::ARCADE_TITLE) {
				arcadeMenuSelection = std::min(1, arcadeMenuSelection + 1);  // Prevent going above 1
			}
			break;

		// To backup a menu
		case SDLK_ESCAPE:
			if (gameState == GameState::MAIN_MENU) {
				gameState = GameState::TITLE_SCREEN;
			}
			else if (gameState == GameState::LESSONS_TITLE) {
				gameState = GameState::MAIN_MENU;
			}
			else if (gameState == GameState::LESSONS_HTP) {
				gameState = GameState::LESSONS_TITLE;
			}
			else if (gameState == GameState::LESSONS_SELECTION) {
				gameState = GameState::LESSONS_TITLE;
			}
			else if (gameState == GameState::ARCADE_TITLE) {
				gameState = GameState::MAIN_MENU;
			}
			else if (gameState == GameState::ARCADE_HTP) {
				gameState = GameState::ARCADE_TITLE;
			}
			else if (gameState == GameState::WPM_TEST) {
				gameState = GameState::MAIN_MENU;
			}
			else if (gameState == GameState::RECORDS) {
				gameState = GameState::MAIN_MENU;
			}
			else if (gameState == GameState::LESSONS_MODE || gameState == GameState::ARCADE_MODE) {
				prevState = gameState; // Store the current mode
				gameState = GameState::PAUSE;
				std::cout << "Game paused!" << std::endl;
			}
			break;

		case SDLK_SPACE:
			if (gameState == GameState::ARCADE_MODE) {
				if (laserReady) {
					fireLaser();  // Fire laser power-up
					laserReady = false; // Consumes laser charge
					comboLevel = 0;
					comboStatus = ""; // Reset status display
				}
			}
			break;

		case SDLK_BACKSPACE:
			if (gameState == GameState::ARCADE_MODE || gameState == GameState::BONUS_STAGE) {
				if (!userInput.empty()) {
					userInput.pop_back(); // Remove last character
				}
			}
			else if (gameState == GameState::WPM_TEST && !wpmUserInput.empty()) {
				wpmUserInput.pop_back();
			}
			break;
		}
		break;

	case SDL_TEXTINPUT:
		if (gameState == GameState::LESSONS_MODE) {
			char typedChar = event.text.text[0];

			// Prevent typing beyond target line
			if (lessonUserInput.size() >= lessonCurrentLine.size()) {
				return;
			}

			// Add typed character
			lessonUserInput += typedChar;
			lessonTotalTypedChars++;

			// Determine correctness
			isCorrect = (typedChar == lessonCurrentLine[lessonUserInput.size() - 1]);
			if (isCorrect) {
				lessonCorrectChars++;
			}
			else {
				lessonIncorrectChars++;
				typedWrong[typedChar]++;
			}

			// Cache texture (placing this here rather than in draw for frame stability)
			std::string letter(1, typedChar);
			SDL_Color color = isCorrect ? correct : wrong;
			SDL_Surface* surface = TTF_RenderText_Solid(menuFont, letter.c_str(), color);
			if (surface) {
				SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
				if (texture) {
					typedCharTextures.push_back(texture);
					typedCharWidths.push_back(surface->w);
					typedCharColors.push_back(color);
				}
				SDL_FreeSurface(surface);
			}
		}

		if (gameState == GameState::ARCADE_MODE) {
			// Prevent spacebar from being typed as part of input
			if (event.text.text[0] == ' ') {
				break; // Skip input
			}

			// Prevent typing if word is fully typed AND incorrect
			if (userInput.size() >= targetText.size() && userInput != targetText) {
				break; // Lock input until user hits backspace
			}

			userInput += event.text.text; // Append typed text
			processedInput.assign(userInput.size(), false);

			if (userInput.back() != targetText[userInput.size() - 1]) {
				brokenCombo = true;
				comboStatus = "X";
				comboLevel = 0;

				char wrongChar = targetText[userInput.size() - 1];
				typedWrong[wrongChar]++; // Count every wrong keypress
			}

			// Increment total number of typed letters
			levelTotalLetters++;
			sessionTotalLetters++;

			// Check if typed letter matches target letter
			if (userInput.size() <= targetText.size() && event.text.text[0] == targetText[userInput.size() - 1]) {
				levelCorrectLetters++; // Increment correct letters
				sessionCorrectLetters++; // Increment total correct letters for game over screen
			}
		}

		if (gameState == GameState::BONUS_STAGE) {
			// Prevent spacebar from being typed as part of input
			if (event.text.text[0] == ' ') {
				break; // Skip input
			}

			// Prevent typing if word is fully typed AND incorrect
			if (userInput.size() >= targetText.size() && userInput != targetText) {
				break; // Lock input until user deletes
			}

			userInput += event.text.text; // Append typed text
			processedInput.assign(userInput.size(), false);

			if (userInput.back() != targetText[userInput.size() - 1]) {
				char wrongChar = targetText[userInput.size() - 1];
				typedWrong[wrongChar]++; // Count every wrong keypress
			}

			// Increment total number of typed letters
			levelTotalLetters++;
			sessionTotalLetters++;

			// Check if typed letter matches target letter
			if (userInput.size() <= targetText.size() && event.text.text[0] == targetText[userInput.size() - 1]) {
				levelCorrectLetters++; // Increment correct letters
				sessionCorrectLetters++; // Increment total correct letters for game over screen
			}
		}

		if (gameState == GameState::WPM_TEST) {
			char typedChar = event.text.text[0];

			// Start timer on first keypress
			if (!wpmTestStarted) {
				wpmTestStarted = true;
				lastSecondTick = SDL_GetTicks();
			}

			// Prevent typing beyond line length
			if (wpmUserInput.size() >= wpmCurrentLine.size()) {
				return; // Ignore extra input
			}

			// Check correctness before appending char
			size_t index = wpmUserInput.size();
			if (typedChar == wpmCurrentLine[index]) {
				wpmCorrectChars++;
			}
			else {
				typedWrong[typedChar]++;
			}

			// Add typed character
			wpmUserInput += typedChar;
			wpmTotalTypedChars++;
		}
		break;

	default:
		break;
	}
}


void Game::update() {
	manager.refresh();
	manager.update();

	auto& playerTransform = player.getComponent<TransformComponent>();

	switch (gameState) {
	case GameState::TITLE_SCREEN:
		// Title screen logic

		// Blink counter logic
		currentTime = SDL_GetTicks(); // Get current time in milliseconds

		if (currentTime > lastBlinkTime + BLINK_DELAY) {
			showBlinkText = !showBlinkText;  // Toggle visibility
			lastBlinkTime = currentTime;    // Update last blink time
		}

		break;

	case GameState::MAIN_MENU:
		// Main menu logic

		// Blink counter logic
		currentTime = SDL_GetTicks(); // Get current time in milliseconds

		if (currentTime > lastBlinkTime + BLINK_DELAY) {
			showBlinkText = !showBlinkText;  // Toggle visibility
			lastBlinkTime = currentTime;    // Update last blink time
		}

		break;

	case GameState::LESSONS_TITLE:
		// Lessons mode title screen logic

		// Blink counter logic
		currentTime = SDL_GetTicks(); // Get current time in milliseconds

		if (currentTime > lastBlinkTime + BLINK_DELAY) {
			showBlinkText = !showBlinkText;  // Toggle visibility
			lastBlinkTime = currentTime;    // Update last blink time
		}

		break;

	case GameState::LESSONS_HTP:
		// Lessons mode "how to play" screen logic

		// Blink counter logic
		currentTime = SDL_GetTicks(); // Get current time in milliseconds

		if (currentTime > lastBlinkTime + BLINK_DELAY) {
			showBlinkText = !showBlinkText;  // Toggle visibility
			lastBlinkTime = currentTime;    // Update last blink time
		}

		break;

	case GameState::LESSONS_SELECTION:
		// TODO (?!)

		break;

	case GameState::LESSONS_MODE:
		// Lessons Mode logic

		currentTime = SDL_GetTicks(); // Get current time in milliseconds

		// Update hand sprites to reflect the key needed to be pressed
		updateHandSprites(lessonCurrentLine, lessonUserInput);

		// Basic laser logic (as in, laser that shoots when a prompt is typed correctly)
		for (auto& laser : activeLasers) {
			laser.duration--;
		}

		activeLasers.erase(
			std::remove_if(activeLasers.begin(), activeLasers.end(),
				[](const LaserStrike& l) { return l.duration <= 0; }),
			activeLasers.end());

		// Count correct characters
		correctChars = 0;
		for (size_t i = 0; i < lessonUserInput.size() && i < lessonCurrentLine.size(); ++i) {
			if (lessonUserInput[i] == lessonCurrentLine[i]) {
				correctChars++;
			}
		}

		// Track % completion
		lessonTargetCompletion = (lessonCurrentLine.empty()) ? 0.0f :
			(static_cast<float>(correctChars) / lessonCurrentLine.size()) * 100.0f;

		// Move lessonCompletion toward lessonTargetCompletion, to fill "accuracy" gauge
		fillSpeed = 0.75f; // For animation speed
		if (lessonCompletion < lessonTargetCompletion) {
			lessonCompletion += fillSpeed;
			if (lessonCompletion > lessonTargetCompletion) {
				lessonCompletion = lessonTargetCompletion;
			}
		}
		else if (lessonCompletion > lessonTargetCompletion) {
			lessonCompletion -= fillSpeed;
			if (lessonCompletion < lessonTargetCompletion) {
				lessonCompletion = lessonTargetCompletion;
			}
		}

		// Update crosshair's position to zombie 1's position
		if (!zombie1Defeated) {
			crosshair->getComponent<TransformComponent>().position = zombie1->getComponent<TransformComponent>().position;
		}

		// Handle zombie defeats (based on percentage, each count as 25%)
		if (!zombie1Defeated && lessonTargetCompletion >= 25.0f) {
			// Basic laser animation for eliminating zombie
			int cannonX = laserMiddle->getComponent<TransformComponent>().position.x + 68; // Center of cannon
			int cannonY = laserMiddle->getComponent<TransformComponent>().position.y + 128; // Bottom of cannon

			int zombieX = zombie1->getComponent<TransformComponent>().position.x + 32;
			int zombieY = zombie1->getComponent<TransformComponent>().position.y + 32;

			LaserStrike laser;
			laser.startX = cannonX;
			laser.startY = cannonY;
			laser.endX = zombieX;
			laser.endY = zombieY;
			laser.duration = 6;

			activeLasers.push_back(laser);

			zombie1Defeated = true;
			zombie1->getComponent<SpriteComponent>().setTex("assets/Tombstone.png");
			crosshair->getComponent<TransformComponent>().position = zombie2->getComponent<TransformComponent>().position;
			zombiesRemaining = 3;
		}
		if (!zombie2Defeated && lessonTargetCompletion >= 50.0f) {
			// Basic laser animation for eliminating zombie
			int cannonX = laserMiddle->getComponent<TransformComponent>().position.x + 68; // Center of cannon
			int cannonY = laserMiddle->getComponent<TransformComponent>().position.y + 128; // Bottom of cannon

			int zombieX = zombie2->getComponent<TransformComponent>().position.x + 32;
			int zombieY = zombie2->getComponent<TransformComponent>().position.y + 32;

			LaserStrike laser;
			laser.startX = cannonX;
			laser.startY = cannonY;
			laser.endX = zombieX;
			laser.endY = zombieY;
			laser.duration = 6;

			activeLasers.push_back(laser);

			zombie2Defeated = true;
			zombie2->getComponent<SpriteComponent>().setTex("assets/Tombstone.png");
			crosshair->getComponent<TransformComponent>().position = zombie3->getComponent<TransformComponent>().position;
			zombiesRemaining = 2;
		}
		if (!zombie3Defeated && lessonTargetCompletion >= 75.0f) {
			// Basic laser animation for eliminating zombie
			int cannonX = laserMiddle->getComponent<TransformComponent>().position.x + 68; // Center of cannon
			int cannonY = laserMiddle->getComponent<TransformComponent>().position.y + 128; // Bottom of cannon

			int zombieX = zombie3->getComponent<TransformComponent>().position.x + 32;
			int zombieY = zombie3->getComponent<TransformComponent>().position.y + 32;

			LaserStrike laser;
			laser.startX = cannonX;
			laser.startY = cannonY;
			laser.endX = zombieX;
			laser.endY = zombieY;
			laser.duration = 6;

			activeLasers.push_back(laser);

			zombie3Defeated = true;
			lessonPassed = true; // they've earned a pass!
			zombie3->getComponent<SpriteComponent>().setTex("assets/Tombstone.png");
			crosshair->getComponent<TransformComponent>().position = zombie4->getComponent<TransformComponent>().position;
			zombiesRemaining = 1;
		}
		if (!zombie4Defeated && lessonTargetCompletion >= 100.0f) {
			// Basic laser animation for eliminating zombie
			int cannonX = laserMiddle->getComponent<TransformComponent>().position.x + 68; // Center of cannon
			int cannonY = laserMiddle->getComponent<TransformComponent>().position.y + 128; // Bottom of cannon

			int zombieX = zombie4->getComponent<TransformComponent>().position.x + 32;
			int zombieY = zombie4->getComponent<TransformComponent>().position.y + 32;

			LaserStrike laser;
			laser.startX = cannonX;
			laser.startY = cannonY;
			laser.endX = zombieX;
			laser.endY = zombieY;
			laser.duration = 6;

			activeLasers.push_back(laser);

			zombie4Defeated = true;
			lessonFullyCompleted = true;
			zombie4->getComponent<SpriteComponent>().setTex("assets/Tombstone.png");
			zombiesRemaining = 0;
		}

		// If full line typed, go to results after delay
		if (lessonUserInput.size() >= lessonCurrentLine.size()) {
			if (!lessonsDelayTimerStarted) {
				lessonsDelayTimerStarted = true;
				lessonsResultsDelayTimer = 120;

				// Freeze time and store it for results
				lessonTimeFrozen = true;
				lessonResultTime = lessonTimeElapsed;
			}

			if (lessonsResultsDelayTimer > 0) {
				lessonsResultsDelayTimer--;
			}
			else {
				lessonsDelayTimerStarted = false; // Reset for next level
				calculateLessonResults();
				gameState = GameState::LESSONS_RESULTS;
			}
		}

		break;

	case GameState::LESSONS_RESULTS:
		// Lessons results screen logic

		// Blink counter logic
		currentTime = SDL_GetTicks(); // Get current time

		if (currentTime > lastBlinkTime + BLINK_DELAY) {
			showBlinkText = !showBlinkText;  // Toggle visibility
			lastBlinkTime = currentTime;    // Update last blink time
		}

		// Update lifetime stats of letters typed correctly
		if (!lessonResultsStatsUpdated) {
			for (const auto& [ch, count] : typedWrong) {
				lifetimeWrongCharacters[ch] += count;
			}
			lessonResultsStatsUpdated = true;
		}

		break;

	case GameState::ARCADE_TITLE:
		// Arcade mode title screen logic

		// Blink counter logic
		currentTime = SDL_GetTicks(); // Get current time in milliseconds

		if (currentTime > lastBlinkTime + BLINK_DELAY) {
			showBlinkText = !showBlinkText;  // Toggle visibility
			lastBlinkTime = currentTime;    // Update last blink time
		}

		break;

	case GameState::ARCADE_HTP:
		// Arcade "how to play" screen logic

		// Blink counter logic
		currentTime = SDL_GetTicks(); // Get current time in milliseconds

		if (currentTime > lastBlinkTime + BLINK_DELAY) {
			showBlinkText = !showBlinkText;  // Toggle visibility
			lastBlinkTime = currentTime;    // Update last blink time
		}

		break;

	case GameState::ARCADE_MODE:
		// Arcade mode logic

		currentTime = SDL_GetTicks(); // Get current time in milliseconds

		// Reset brokenCombo at the start of a new word
		if (userInput.empty()) {
			brokenCombo = false;
		}

		// Check if word is fully typed and wrong, for status alert
		if (userInput.size() == targetText.size() && userInput != targetText) {
			wordTypedWrong = true;
		}
		else {
			wordTypedWrong = false;
		}

		// Update status text
		if (barrierUnderAttack) {
			statusText = "DANGER";
			if (laserReady) {
				statusText = "LASER READY";
			}
		}
		else {
			if (wordTypedWrong) {
				statusText = "ERROR";
			}
			else if (laserReady) {
				statusText = "LASER READY";
			}
			else if (barrierHP <= 50) {
				statusText = "CAUTION";
			}
			else if (barrierHP <= 20) {
				statusText = "CRITICAL";
			}
			else {
				statusText = "OK";
			}
		}

		// Screen shake logic, for when zombies attack barrier
		if (shakeDuration > 0) {
			shakeOffsetX = (std::rand() % (shakeMagnitude * 2)) - shakeMagnitude;
			shakeOffsetY = (std::rand() % (shakeMagnitude * 2)) - shakeMagnitude;
			shakeDuration--;
		}
		else {
			shakeOffsetX = 0;
			shakeOffsetY = 0;
		}

		// Basic laser logic (as in, laser that shoots when a prompt is typed correctly)
		for (auto& laser : activeLasers) {
			laser.duration--;
		}

		activeLasers.erase(
			std::remove_if(activeLasers.begin(), activeLasers.end(),
				[](const LaserStrike& l) { return l.duration <= 0; }),
			activeLasers.end());


		// Update crosshair position if zombies are present
		if (!zombies.empty() && currentZombieIndex < zombies.size()) {
			Entity* activeZombie = zombies[currentZombieIndex];
			auto& zombieTransform = activeZombie->getComponent<TransformComponent>();

			// Update crosshair's position to zombie's position
			auto& crosshairTransform = crosshair->getComponent<TransformComponent>();
			crosshairTransform.position = zombieTransform.position;
		}

		// Update hand sprites to reflect the key needed to be pressed
		updateHandSprites(targetText, userInput);

		// To update barrier attack status
		barrierUnderAttack = false;

		// Iterate through all zombies
		for (size_t i = 0; i < zombies.size(); ++i) {
			Entity* zombie = zombies[i];
			auto& zombieTransform = zombie->getComponent<TransformComponent>();
			auto& transformStatus = zombie->getComponent<TransformStatusComponent>();

			// Update stun status and timer
			transformStatus.updateStun();

			// If stunned and not transformed, play stun animation (but still allow prompt to be typed)
			if (transformStatus.isStunned() && !transformStatus.getTransformed()) {
				zombie->getComponent<SpriteComponent>().Play("Stun");
			}

			// Check if zombie is transformed
			if (!transformStatus.getTransformed() && !transformStatus.isStunned()) {
				// Move zombie toward the player if not stunned
				float dx = playerTransform.position.x - zombieTransform.position.x;
				float dy = playerTransform.position.y - zombieTransform.position.y;

				float magnitude = sqrt(dx * dx + dy * dy);
				if (magnitude > 0) {
					dx /= magnitude;
					dy /= magnitude;
				}

				zombieTransform.position.x += dx * speed;
				zombieTransform.position.y += dy * speed;

				// Directional animation
				auto& sprite = zombie->getComponent<SpriteComponent>();

				if (std::abs(dx) > std::abs(dy)) {
					if (dx > 0) {
						sprite.Play("Walk Right");
					}
					else {
						sprite.Play("Walk Left");
					}
				}
				else {
					if (dy > 0) {
						sprite.Play("Walk Down");
					}
				}

				// Check for wall collisions
				if (Collision::AABB(zombie->getComponent<ColliderComponent>().collider,
					barrier->getComponent<ColliderComponent>().collider)) {
					zombieTransform.position.x -= dx * speed;
					zombieTransform.position.y -= dy * speed;

					// Wall collision is true
					barrierUnderAttack = true; // Track if zombies are attacking

					// Attacking animation
					auto& sprite = zombie->getComponent<SpriteComponent>();

					if (std::abs(dx) > std::abs(dy)) {
						if (dx > 0) {
							sprite.Play("Attack Right");
						}
						else {
							sprite.Play("Attack Left");
						}
					}
					else {
						if (dy > 0) {
							sprite.Play("Attack Down");
						}
					}

					// Wall hit detected
					std::cout << "Barrier hit! HP: " << barrierHP << std::endl;
				}
			}

			// Lower HP by 10 every second the zombies are attacking the barrier
			if (barrierUnderAttack && currentTime - lastAttackTime >= 1000) {
				barrierHP -= 10;
				lastAttackTime = currentTime;

				updateBarrierDamage(barrierHP);

				// Trigger screen shake!
				shakeDuration = 3;    // frames to shake
				shakeMagnitude = 3;    // how far to shake

				if (barrierHP < 0) {
					barrierHP = 0;

					updateBarrierDamage(barrierHP); // Updates barrier sprite based on amount of damage taken
				}
			}

			// Check if zombie's prompt matches user input
			if (i == currentZombieIndex && userInput == arcadeWords[i] && !transformStatus.getTransformed()) {

				// Check if user types in prompt correctly without errors, to update combo
				checkCombo(userInput, targetText);

				// Basic laser animation for eliminating zombie
				int cannonX = laserMiddle->getComponent<TransformComponent>().position.x + 68; // Center of cannon
				int cannonY = laserMiddle->getComponent<TransformComponent>().position.y + 128; // Bottom of cannon

				int zombieX = zombie->getComponent<TransformComponent>().position.x + 32;
				int zombieY = zombie->getComponent<TransformComponent>().position.y + 32;

				LaserStrike laser;
				laser.startX = cannonX;
				laser.startY = cannonY;
				laser.endX = zombieX;
				laser.endY = zombieY;
				laser.duration = 6;

				activeLasers.push_back(laser);

				// Transform zombie and play defeat animation
				auto& sprite = zombie->getComponent<SpriteComponent>();
				sprite.Play("Defeat");

				// Update transformation status and account for how many zombies are inactive
				transformStatus.setTransformed(true);
				tombstones.push_back(zombie);

				// Update zombie count (for threat level) / zombies defeated
				zombieCount--;
				zombiesDefeated++;

				// Clear user input
				userInput.clear();

				// Move to next closest zombie
				if (i == currentZombieIndex) {
					// Find closest remaining zombie
					float closestDistance = std::numeric_limits<float>::max();
					size_t closestZombieIndex = currentZombieIndex;

					for (size_t j = 0; j < zombies.size(); ++j) {
						if (!zombies[j]->getComponent<TransformStatusComponent>().getTransformed()) {
							auto& targetZombieTransform = zombies[j]->getComponent<TransformComponent>();
							float dx = playerTransform.position.x - targetZombieTransform.position.x;
							float dy = playerTransform.position.y - targetZombieTransform.position.y;
							float distance = sqrt(dx * dx + dy * dy);

							if (distance < closestDistance) {
								closestDistance = distance;
								closestZombieIndex = j;
							}
						}
					}

					// Update current zombie to the closest one
					currentZombieIndex = closestZombieIndex;
					targetText = arcadeWords[currentZombieIndex];
				}
			}
		}
		// Check if all zombies are transformed
		allZombiesTransformed = true; // Assume all are defeated
		for (auto* zombie : zombies) {
			if (!zombie->getComponent<TransformStatusComponent>().getTransformed()) {
				allZombiesTransformed = false;
				break;
			}
		}

		// Laser power-up logic
		if (laserActive) {
			auto& laserTransform = laserPowerup->getComponent<TransformComponent>();
			auto& laserCollider = laserPowerup->getComponent<ColliderComponent>();

			// Move laser down!
			laserTransform.position.y += laserSpeed;

			// Check collision with zombies
			for (auto* zombie : zombies) {
				if (!zombie->getComponent<TransformStatusComponent>().getTransformed()) {
					if (Collision::AABB(zombie->getComponent<ColliderComponent>().collider, laserCollider.collider)) {

						// Get zapped, zambie! (stuns the zombie, making it unable to move or attack for 5 seconds)
						auto& status = zombie->getComponent<TransformStatusComponent>();
						if (!status.isStunned()) {
							auto& sprite = zombie->getComponent<SpriteComponent>();
							sprite.Play("Stun");

							status.setStunned(true, 300); // 5 seconds at 60 FPS
						}
					}
				}
			}

			// Remove laser power-up when it reaches bottom of screen
			if (laserTransform.position.y > 700) {
				laserPowerup->destroy();
				laserPowerup = nullptr;
				laserActive = false;
			}
		}

		// Add delay to results screen
		if (allZombiesTransformed && gameState == GameState::ARCADE_MODE) {
			if (!nextLevelDelayStarted) {
				nextLevelDelayStarted = true;
				nextLevelDelayTimer = 120;
			}

			if (nextLevelDelayTimer > 0) {
				nextLevelDelayTimer--;
			}
			else {
				nextLevelDelayStarted = false; // Reset for next level
				gameState = GameState::ARCADE_RESULTS;
			}
		}

		// Barrier destroyed!
		if (!barrierDestroyed && barrierHP <= 0) {
			barrierDestroyed = true;
			gameOverDelayTimer = 120; // 2 seconds at 60 FPS

			// Create exclamation point above player
			exclamation = &manager.addEntity();

			int exclaimX = player.getComponent<TransformComponent>().position.x - 17; // Centered..?!
			int exclaimY = player.getComponent<TransformComponent>().position.y - 5; // Slightly above player
			exclamation->addComponent<TransformComponent>(exclaimX, exclaimY, 17, 16, 2);
			exclamation->addComponent<SpriteComponent>("assets/Exclamation.png");
		}

		// If the delay timer is counting down
		if (barrierDestroyed) {
			if (gameOverDelayTimer > 0) {
				gameOverDelayTimer--;
			}
			else {
				// Destroy exclamation point
				if (exclamation) {
					exclamation->destroy();
					exclamation = nullptr;
				}

				barrierDestroyed = false;
				gameState = GameState::GAME_OVER;
			}
		}

		break;

	case GameState::BONUS_TITLE:
		// Bonus title logic

		// Blink counter logic
		currentTime = SDL_GetTicks(); // Get current time in milliseconds

		if (currentTime > lastBlinkTime + BLINK_DELAY) {
			showBlinkText = !showBlinkText;  // Toggle visibility
			lastBlinkTime = currentTime;    // Update last blink time
		}

		break;

	case GameState::BONUS_STAGE:
		// Bonus stage logic

		currentTime = SDL_GetTicks(); // Get current time in milliseconds

		// Check if word is fully typed and wrong
		if (userInput.size() == targetText.size() && userInput != targetText) {
			wordTypedWrong = true;
		}
		else {
			wordTypedWrong = false;
		}

		// Update status text (without some of the other status options, for bonus mode simplicity)
		if (wordTypedWrong) {
			statusText = "ERROR";
		}
		else if (barrierHP <= 50) {
			statusText = "CAUTION";
		}
		else if (barrierHP <= 20) {
			statusText = "CRITICAL";
		}
		else {
			statusText = "OK";
		}

		// Basic laser logic
		for (auto& laser : activeLasers) {
			laser.duration--;
		}

		activeLasers.erase(
			std::remove_if(activeLasers.begin(), activeLasers.end(),
				[](const LaserStrike& l) { return l.duration <= 0; }),
			activeLasers.end());


		// Update crosshair position if zombies are present in the left group
		if (!leftToRight.empty() && currentZombieIndex < leftToRight.size()) {
			Entity* activeZombie = leftToRight[currentZombieIndex];
			auto& zombieTransform = activeZombie->getComponent<TransformComponent>();

			// Update crosshair's position to zombie's position
			auto& crosshairTransform = crosshair->getComponent<TransformComponent>();
			crosshairTransform.position = zombieTransform.position;
		}

		// Update crosshair position if zombies are present in the right group
		if (!rightToLeft.empty() && currentZombieIndex < rightToLeft.size()) {
			Entity* activeZombie = rightToLeft[currentZombieIndex];
			auto& zombieTransform = activeZombie->getComponent<TransformComponent>();

			// Update crosshair's position to zombie's position
			auto& crosshairTransform = crosshair->getComponent<TransformComponent>();
			crosshairTransform.position = zombieTransform.position;
		}

		// Update hand sprites to reflect the key needed to be pressed
		updateHandSprites(targetText, userInput);

		// Check if all left-to-right zombies are transformed before moving right-to-left zombies
		leftGroupDefeated = true;
		for (auto* zombie : leftToRight) {
			if (!zombie->getComponent<TransformStatusComponent>().getTransformed()) {
				leftGroupDefeated = false;
				break;
			}
		}

		// Iterate through left-to-right zombies
		for (size_t i = 0; i < leftToRight.size(); ++i) {
			Entity* zombie = leftToRight[i];
			auto& zombieTransform = zombie->getComponent<TransformComponent>();
			auto& transformStatus = zombie->getComponent<TransformStatusComponent>();

			// Check if zombie moves past screen, then eliminate if so (arcade-style!)
			if (zombieTransform.position.x > 1600 && !transformStatus.getTransformed()) {
				// Transform zombie
				transformStatus.setTransformed(true);
				tombstones.push_back(zombie);

				// Update zombie count
				zombieCount--;

				// Clear user input
				userInput.clear();

				// Move to the zombie to the left (next in the group)
				if (i == currentZombieIndex) {
					float currentX = leftToRight[currentZombieIndex]->getComponent<TransformComponent>().position.x;
					float nextX = -std::numeric_limits<float>::max();
					int nextZombieIndex = -1;

					for (size_t j = 0; j < leftToRight.size(); ++j) {
						if (!leftToRight[j]->getComponent<TransformStatusComponent>().getTransformed()) {
							float candidateX = leftToRight[j]->getComponent<TransformComponent>().position.x;

							// Looking for left of current zombie
							if (candidateX < currentX && candidateX > nextX) {
								nextX = candidateX;
								nextZombieIndex = static_cast<int>(j);
							}
						}
					}

					// Found next zombie, update index and targetText
					if (nextZombieIndex != -1) {
						currentZombieIndex = nextZombieIndex;
						targetText = bonusLeft[currentZombieIndex];
					}
				}

			}

			// Check if zombie is transformed, then move if not
			if (!transformStatus.getTransformed()) {
				auto& sprite = zombie->getComponent<SpriteComponent>();
				sprite.Play("Walk Right");

				zombieTransform.position.x += bonusSpeed;
			}

			// Check if zombie's prompt matches user input
			if (i == currentZombieIndex && userInput == bonusLeft[i] && !transformStatus.getTransformed()) {

				// Basic laser animation for eliminating zombie
				int cannonX = laserMiddle->getComponent<TransformComponent>().position.x + 68; // Center of cannon
				int cannonY = laserMiddle->getComponent<TransformComponent>().position.y + 128; // Bottom of cannon

				int zombieX = zombie->getComponent<TransformComponent>().position.x + 32;
				int zombieY = zombie->getComponent<TransformComponent>().position.y + 32;

				LaserStrike laser;
				laser.startX = cannonX;
				laser.startY = cannonY;
				laser.endX = zombieX;
				laser.endY = zombieY;
				laser.duration = 6;

				activeLasers.push_back(laser);

				// Transform zombie, play defeat animation
				auto& sprite = zombie->getComponent<SpriteComponent>();
				sprite.Play("Defeat");
				transformStatus.setTransformed(true);
				tombstones.push_back(zombie);

				// Update zombie count / zombies defeated
				zombieCount--;
				zombiesDefeated++;

				// Update bonus zombie count / zombies defeated
				bonusZombiesDefeated++;

				// Rewards HP that will be added to barrierHP after results screen
				bonusHP += 10;

				// Clear user input
				userInput.clear();

				// Move to the zombie to the left
				if (i == currentZombieIndex) {
					float currentX = leftToRight[currentZombieIndex]->getComponent<TransformComponent>().position.x;
					float nextX = -std::numeric_limits<float>::max();
					int nextZombieIndex = -1;

					for (size_t j = 0; j < leftToRight.size(); ++j) {
						if (!leftToRight[j]->getComponent<TransformStatusComponent>().getTransformed()) {
							float candidateX = leftToRight[j]->getComponent<TransformComponent>().position.x;

							// Looking for left of current zombie
							if (candidateX < currentX && candidateX > nextX) {
								nextX = candidateX;
								nextZombieIndex = static_cast<int>(j);
							}
						}
					}

					// Found next zombie, update index and targetText
					if (nextZombieIndex != -1) {
						currentZombieIndex = nextZombieIndex;
						targetText = bonusLeft[currentZombieIndex];
					}
				}
			}
		}

		// Iterate through right-to-left zombies, only if left group is defeated
		if (leftGroupDefeated) {
			targetText = bonusRight[currentZombieIndex]; // Use words from the right group
			for (size_t i = 0; i < rightToLeft.size(); ++i) {
				Entity* zombie = rightToLeft[i];
				auto& zombieTransform = zombie->getComponent<TransformComponent>();
				auto& transformStatus = zombie->getComponent<TransformStatusComponent>();

				// Check if zombie moves past screen, then eliminate if so
				if (zombieTransform.position.x < -75 && !transformStatus.getTransformed()) {
					// Transform zombie
					transformStatus.setTransformed(true);
					tombstones.push_back(zombie);

					// Update zombie count
					zombieCount--;

					// Clear user input
					userInput.clear();

					// Move to the zombie to the right
					if (i == currentZombieIndex) {
						float currentX = rightToLeft[currentZombieIndex]->getComponent<TransformComponent>().position.x;
						float nextX = std::numeric_limits<float>::max();
						int nextZombieIndex = -1;

						for (size_t j = 0; j < rightToLeft.size(); ++j) {
							if (!rightToLeft[j]->getComponent<TransformStatusComponent>().getTransformed()) {
								float candidateX = rightToLeft[j]->getComponent<TransformComponent>().position.x;

								// Looking for right of current zombie
								if (candidateX > currentX && candidateX < nextX) {
									nextX = candidateX;
									nextZombieIndex = static_cast<int>(j);
								}
							}
						}

						// Found next zombie, update index and targetText
						if (nextZombieIndex != -1) {
							currentZombieIndex = nextZombieIndex;
							targetText = bonusRight[currentZombieIndex];
						}
					}

				}

				// Check if zombie is transformed, then move if not
				if (!transformStatus.getTransformed()) {
					auto& sprite = zombie->getComponent<SpriteComponent>();
					sprite.Play("Walk Left");

					zombieTransform.position.x -= bonusSpeed;
				}

				// Check if zombie's prompt matches user input
				if (i == currentZombieIndex && userInput == bonusRight[i] && !transformStatus.getTransformed()) {

					// Basic laser animation for eliminating zombie
					int cannonX = laserMiddle->getComponent<TransformComponent>().position.x + 68; // Center of cannon
					int cannonY = laserMiddle->getComponent<TransformComponent>().position.y + 128; // Bottom of cannon

					int zombieX = zombie->getComponent<TransformComponent>().position.x + 32;
					int zombieY = zombie->getComponent<TransformComponent>().position.y + 32;

					LaserStrike laser;
					laser.startX = cannonX;
					laser.startY = cannonY;
					laser.endX = zombieX;
					laser.endY = zombieY;
					laser.duration = 6;

					activeLasers.push_back(laser);

					// Transform zombie and play defeat animation
					auto& sprite = zombie->getComponent<SpriteComponent>();
					sprite.Play("Defeat");
					transformStatus.setTransformed(true);
					tombstones.push_back(zombie);

					// Update zombie count / zombies defeated
					zombieCount--;
					zombiesDefeated++;

					// Update bonus zombie count / zombies defeated
					bonusZombiesDefeated++;

					// Rewards HP that will be added to barrierHP after results screen
					bonusHP += 10;

					// Clear user input
					userInput.clear();

					// Move to the zombie to the right
					if (i == currentZombieIndex) {
						float currentX = rightToLeft[currentZombieIndex]->getComponent<TransformComponent>().position.x;
						float nextX = std::numeric_limits<float>::max();
						int nextZombieIndex = -1;

						for (size_t j = 0; j < rightToLeft.size(); ++j) {
							if (!rightToLeft[j]->getComponent<TransformStatusComponent>().getTransformed()) {
								float candidateX = rightToLeft[j]->getComponent<TransformComponent>().position.x;

								// Looking for right of current zombie
								if (candidateX > currentX && candidateX < nextX) {
									nextX = candidateX;
									nextZombieIndex = static_cast<int>(j);
								}
							}
						}

						// Found next zombie, update index and targetText
						if (nextZombieIndex != -1) {
							currentZombieIndex = nextZombieIndex;
							targetText = bonusRight[currentZombieIndex];
						}
					}

				}
			}
		}

		// Check if all zombies are transformed
		allZombiesTransformed = true; // Assume all are defeated

		for (auto* zombie : leftToRight) {
			if (!zombie->getComponent<TransformStatusComponent>().getTransformed()) {
				allZombiesTransformed = false;
				break;
			}
		}

		for (auto* zombie : rightToLeft) {
			if (!zombie->getComponent<TransformStatusComponent>().getTransformed()) {
				allZombiesTransformed = false;
				break;
			}
		}

		// Add delay to results screen
		if (allZombiesTransformed && gameState == GameState::BONUS_STAGE) {
			if (!nextLevelDelayStarted) {
				nextLevelDelayStarted = true;
				nextLevelDelayTimer = 120;
			}

			if (nextLevelDelayTimer > 0) {
				nextLevelDelayTimer--;
			}
			else {
				barrierHP += bonusHP;
				nextLevelDelayStarted = false; // Reset for next level
				gameState = GameState::BONUS_RESULTS; // Transition to results state
			}
		}

		break;

	case GameState::ARCADE_RESULTS:
		// Results screen logic

		// Blink counter logic
		currentTime = SDL_GetTicks(); // Get current time

		if (currentTime > lastBlinkTime + BLINK_DELAY) {
			showBlinkText = !showBlinkText;  // Toggle visibility
			lastBlinkTime = currentTime;    // Update last blink time
		}

		// Update lifetime stats of letters typed incorrectly
		if (!arcadeResultsStatsUpdated) {
			for (const auto& [ch, count] : typedWrong) {
				lifetimeWrongCharacters[ch] += count;
			}
			arcadeResultsStatsUpdated = true;
		}

		break;

	case GameState::BONUS_RESULTS:
		// Results screen logic

		// Blink counter logic
		currentTime = SDL_GetTicks(); // Get current time

		if (currentTime > lastBlinkTime + BLINK_DELAY) {
			showBlinkText = !showBlinkText;  // Toggle visibility
			lastBlinkTime = currentTime;    // Update last blink time
		}

		// Update lifetime stats of letter typed incorrectly
		if (!arcadeResultsStatsUpdated) {
			for (const auto& [ch, count] : typedWrong) {
				lifetimeWrongCharacters[ch] += count;
			}
			arcadeResultsStatsUpdated = true;
		}

		break;

	case GameState::GAME_OVER:
		// Game over screen logic

		// Blink counter logic
		currentTime = SDL_GetTicks(); // Get current time

		if (currentTime > lastBlinkTime + BLINK_DELAY) {
			showBlinkText = !showBlinkText;  // Toggle visibility
			lastBlinkTime = currentTime;    // Update last blink time
		}

		// Update highest level achieved
		if (level > arcadeHighestLevel) {
			arcadeHighestLevel = level;
		}

		// Update lifetime stats of letter typed incorrectly
		if (!arcadeResultsStatsUpdated) {
			for (const auto& [ch, count] : typedWrong) {
				lifetimeWrongCharacters[ch] += count;
			}
			arcadeResultsStatsUpdated = true;
		}

		break;

	case GameState::RECORDS:
		// Records screen logic

		currentTime = SDL_GetTicks();

		// Update blinking text
		if (currentTime > lastBlinkTime + BLINK_DELAY) {
			showBlinkText = !showBlinkText;
			lastBlinkTime = currentTime;
		}

		break;

	case GameState::WPM_TEST:
		// Words per minute test logic

		currentTime = SDL_GetTicks();

		// Update blinking text
		if (currentTime > lastBlinkTime + BLINK_DELAY) {
			showBlinkText = !showBlinkText;
			lastBlinkTime = currentTime;
		}

		// decrement wpmTimeRemaining
		if (wpmTestStarted && currentTime > lastSecondTick + 1000 && wpmTimeRemaining > 0) {
			wpmTimeRemaining--;
			lastSecondTick = currentTime;
		}

		// when timer hits 0, switch to wpm_results
		if (wpmTimeRemaining <= 0 && !wpmTestEnded) {
			wpmTestEnded = true;

			calculateWPM(); // Calculate results

			gameState = GameState::WPM_RESULTS;
			break;
		}

		// If line is fully typed, shift nextLine to currentLine and load a new one
		if (wpmUserInput.size() >= wpmCurrentLine.size()) {
			//wpmTypedWords += countWords(wpmCurrentLine); // May keep for results

			shiftWpmLines(); // Handles shifting logic
		}
		break;

	case GameState::WPM_RESULTS:
		// Words per minute test results logic

		currentTime = SDL_GetTicks();

		// Update blinking text
		if (currentTime > lastBlinkTime + BLINK_DELAY) {
			showBlinkText = !showBlinkText;
			lastBlinkTime = currentTime;
		}

		// Update lifetime stats of letter typed incorrectly
		if (!wpmResultsStatsUpdated) {
			for (const auto& [ch, count] : typedWrong) {
				lifetimeWrongCharacters[ch] += count;
			}
			wpmResultsStatsUpdated = true;
		}

		break;

	case GameState::PAUSE:
		// Pause screen logic...

		// No logic because game is paused!
		break;

	default:
		break;
	}
}


void Game::render()
{
	SDL_RenderClear(renderer);

	switch (gameState) {
	case GameState::TITLE_SCREEN:
		// Draw title screen

		if (!titleFont) {
			std::cout << "Failed to load font: " << TTF_GetError() << std::endl;
			return;
		}

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		uiManager->drawText("Letter RIP", 650, 450, { 255, 255, 255, 255 }, titleFont);

		if (showBlinkText) {
			uiManager->drawText("Press Enter to Start!", 600, 525, { 255, 255, 255, 255 }, menuFont);
		}
		SDL_RenderPresent(renderer);
		break;

	case GameState::MAIN_MENU:
		// Draw main menu

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		uiManager->drawText("Main Menu!", 660, 60, { 255, 255, 255, 255 }, titleFont);

		// Changes color of text on screen based on which menu selection is currently chosen
		SDL_Color lessonsColor = mainMenuSelection == 0 ? SDL_Color{ 255, 255, 0, 255 } : SDL_Color{ 255, 255, 255, 255 };
		SDL_Color arcadeColor = mainMenuSelection == 1 ? SDL_Color{ 255, 255, 0, 255 } : SDL_Color{ 255, 255, 255, 255 };
		SDL_Color recordsColor = mainMenuSelection == 2 ? SDL_Color{ 255, 255, 0, 255 } : SDL_Color{ 255, 255, 255, 255 };
		SDL_Color wpmColor = mainMenuSelection == 3 ? SDL_Color{ 255, 255, 0, 255 } : SDL_Color{ 255, 255, 255, 255 };

		uiManager->drawText("Lessons", 400, 400, lessonsColor, titleFont);
		uiManager->drawText("Arcade", 800, 400, arcadeColor, titleFont);
		uiManager->drawText("Records", 400, 500, recordsColor, titleFont);
		uiManager->drawText("WPM Test", 800, 500, wpmColor, titleFont);

		SDL_RenderPresent(renderer);
		break;

	case GameState::LESSONS_TITLE:
		// Draw lessons mode title screen

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		uiManager->drawText("Lessons Mode", 660, 60, { 255, 255, 255, 255 }, titleFont);

		howToColor = lessonsMenuSelection == 0 ? SDL_Color{ 255, 255, 0, 255 } : SDL_Color{ 255, 255, 255, 255 };
		startColor = lessonsMenuSelection == 1 ? SDL_Color{ 255, 255, 0, 255 } : SDL_Color{ 255, 255, 255, 255 };

		uiManager->drawText("How To Play", 500, 450, howToColor, titleFont);
		uiManager->drawText("Lesson Selection", 900, 450, startColor, titleFont);

		SDL_RenderPresent(renderer);
		break;

	case GameState::LESSONS_HTP:
		// Draw lessons mode "how to play" screen

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		uiManager->drawText("How To Play", 660, 60, { 255, 255, 255, 255 }, titleFont);

		SDL_RenderPresent(renderer);
		break;

	case GameState::LESSONS_SELECTION:
		// Draw lessons mode selection screen

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		uiManager->drawText("Lesson Selection", 660, 60, { 255, 255, 255, 255 }, titleFont);

		baseY = 250; // Vertical spacing between lesson rows
		for (int i = 0; i < totalLessons; ++i) {
			// Highlight the selected lesson
			SDL_Color color = (lessonsLevelSelection == i) ? SDL_Color{ 255, 255, 0, 255 } : SDL_Color{ 255, 255, 255, 255 };

			// Display lesson title
			std::string label = "Lesson " + std::to_string(i);
			uiManager->drawText(label, 200, baseY + (i * 100), color, menuFont);

			// Get progress data for each lesson
			WordListManager::Difficulty lessonDiff = static_cast<WordListManager::Difficulty>(i);
			if (lessonProgressMap.contains(lessonDiff)) {
				SaveSystem::LessonProgress& progress = lessonProgressMap[lessonDiff];

				// Status text displayed next to each lesson
				std::string status = "";
				if (progress.fullyCompleted) {
					status = "Fully Completed";
				}
				else if (progress.passed) {
					status = "Lesson Passed";
				}

				// Best accuracy and time displayed next to each lesson
				std::string stats = " | Best Accuracy: " + std::to_string(progress.bestAccuracy) + "%";
				if (progress.bestTime > 0) {
					stats += " | Best Time: " + std::to_string(progress.bestTime) + "s";
				}

				if (!status.empty()) {
					uiManager->drawText(status + stats, 500, baseY + (i * 100), { 102, 255, 105, 255 }, menuFont);
				}
			}
		}

		SDL_RenderPresent(renderer);
		break;

	case GameState::LESSONS_MODE:
		// Draw lessons mode

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		if (!lessonTimeFrozen) {
			lessonTimeElapsed = (SDL_GetTicks() - lessonStartTime) / 1000; // Store lessonStartTime at lesson init
		}

		// Draw middle laser cannon at top
		laserMiddle->getComponent<SpriteComponent>().draw();

		// Dark blue screen on top of gray panel
		panelColor = { 160, 160, 160, 255 };
		screenColor = { 102, 102, 255, 255 };

		// Top panel
		uiManager->drawRectangle(0, 0, 1600, 150, panelColor);

		// Centered screen rectangle with black outline
		uiManager->drawRectangle(-20, 20, 1620, 110, {0, 0, 0, 255}); // Black outline
		uiManager->drawRectangle(0, 30, 1600, 90, screenColor);  // Blue "screen"

		// Bottom "control panel"
		uiManager->drawRectangle(0, 750, 1600, 200, panelColor);

		// Positions and Setup
		lessonTargetY = 45;
		lessonInputY = 85;
		lessonLetterX = 800;
		typedWidth = 0;

		correct = { 0, 255, 0, 255 };
		wrong = { 255, 0, 0, 255 };
		neutral = { 255, 255, 255, 255 };

		// Draw reference line (centered and scrolling)
		refLineLetterX = 800;

		for (size_t i = 0; i < lessonCharTextures.size(); ++i) {
			int drawX = refLineLetterX - lessonScrollX;
			SDL_Rect dst = { drawX, lessonTargetY, lessonCharWidths[i], 24 };
			SDL_RenderCopy(renderer, lessonCharTextures[i], nullptr, &dst);
			refLineLetterX += lessonCharWidths[i] + 1;
		}


		// Draw typed characters
		lessonLetterX = 800;
		for (size_t i = 0; i < typedCharTextures.size(); ++i) {
			int drawX = lessonLetterX - lessonScrollX;
			SDL_Rect dst = { drawX, lessonInputY, typedCharWidths[i], 24 };
			SDL_RenderCopy(renderer, typedCharTextures[i], nullptr, &dst);

			lessonLetterX += typedCharWidths[i] + 1;
			typedWidth += typedCharWidths[i] + 1;
		}


		// Fixed blinking cursor
		if (lessonUserInput.size() <= lessonCurrentLine.size()) {
			if ((SDL_GetTicks() / 500) % 2 == 0) {
				SDL_Color caretColor = (lessonUserInput == lessonCurrentLine) ? neutral : wrong;
				SDL_Rect caretRect = {
					lessonFixedCursorX,
					lessonInputY,
					2,
					18
				};
				SDL_SetRenderDrawColor(renderer, caretColor.r, caretColor.g, caretColor.b, caretColor.a);
				SDL_RenderFillRect(renderer, &caretRect);
			}
		}

		// Apply scroll
		lessonScrollX = typedWidth;

		// Draw game objects
		manager.draw();

		zombie1->getComponent<SpriteComponent>().draw();
		zombie2->getComponent<SpriteComponent>().draw();
		zombie3->getComponent<SpriteComponent>().draw();
		zombie4->getComponent<SpriteComponent>().draw();

		crosshair->getComponent<SpriteComponent>().draw();

		leftHand->getComponent<SpriteComponent>().draw();
		rightHand->getComponent<SpriteComponent>().draw();

		// Draw basic laser
		for (const auto& laser : activeLasers) {
			SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red

			int thickness = 4;

			// Draw 'thickness' of laser (number of lines offset horizontally)
			for (int i = -thickness / 2; i <= thickness / 2; ++i) {
				SDL_RenderDrawLine(
					renderer,
					laser.startX + i,
					laser.startY,
					laser.endX + i,
					laser.endY
				);
			}
		}

		// Draw assets on control panel
		if (uiManager) {
			outlineColor = { 255, 255, 255, 255 };
			fgColor = { 102, 255, 105, 255 };
			bgColor = { 255, 102, 102, 255 };
			textColor = { 0, 0, 0, 255 };

			uiManager->drawHealthbar(185, 810, 320, 40, static_cast<int>(lessonCompletion), 100, "ACCURACY:", outlineColor, fgColor, bgColor, controlPanelFont, textColor);
			uiManager->drawThreatLvl(1140, 785, 70, 70, zombiesRemaining, "THREAT LVL", outlineColor, bgColor, controlPanelFont, threatLvlFont, textColor);
			uiManager->drawTimeElapsed(1335, 785, 160, 70, lessonTimeElapsed, "TIME ELAPSED", outlineColor, fgColor, controlPanelFont, threatLvlFont, textColor);
		}

		SDL_RenderPresent(renderer);
		break;

	case GameState::LESSONS_RESULTS:
		// Draw lessons results screen

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		uiManager->drawText("Lessons Results", 660, 60, { 255, 255, 255, 255 }, titleFont);

		// Completion Message
		if (lessonFullyCompleted) {
			uiManager->drawText("Perfect Completion!", 400, 200, { 102, 255, 105, 255 }, menuFont);  // green
		}
		else if (lessonPassed) {
			uiManager->drawText("Lesson Passed! (Try for perfect!)", 400, 200, { 255, 255, 255, 255 }, menuFont);  // white
		}
		else {
			uiManager->drawText("Lesson Failed... Try Again!", 400, 200, { 255, 80, 80, 255 }, menuFont);  // red
		}

		// Stats
		uiManager->drawText("Time: " + std::to_string(lessonResultTime) + " seconds", 400, 300, { 255, 255, 255, 255 }, menuFont);
		uiManager->drawText("Accuracy: " + std::to_string((int)(lessonCompletion)) + "%", 400, 500, { 255, 255, 255, 255 }, menuFont);
		uiManager->drawText("Characters: " + std::to_string(lessonCorrectChars) + " / " + std::to_string(lessonIncorrectChars) + " (correct / incorrect)", 400, 700, { 255, 255, 255, 255 }, menuFont);

		if (showBlinkText) {
			uiManager->drawText("Press Enter to Return to the Lesson Selection Screen!", 400, 800, { 255, 255, 255, 255 }, menuFont);
		}

		SDL_RenderPresent(renderer);
		break;

	case GameState::ARCADE_TITLE:
		// Draw arcade mode title screen

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		uiManager->drawText("Arcade Mode", 660, 60, { 255, 255, 255, 255 }, titleFont);

		howToColor = arcadeMenuSelection == 0 ? SDL_Color{ 255, 255, 0, 255 } : SDL_Color{ 255, 255, 255, 255 };
		startColor = arcadeMenuSelection == 1 ? SDL_Color{ 255, 255, 0, 255 } : SDL_Color{ 255, 255, 255, 255 };

		uiManager->drawText("How To Play", 500, 450, howToColor, titleFont);
		uiManager->drawText("Start", 900, 450, startColor, titleFont);

		SDL_RenderPresent(renderer);
		break;

	case GameState::ARCADE_HTP:
		// Draw arcade mode "how to play" screen

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		uiManager->drawText("How To Play", 660, 60, { 255, 255, 255, 255 }, titleFont);

		SDL_RenderPresent(renderer);
		break;

	case GameState::ARCADE_MODE:
		// Draw arcade mode

		SDL_SetRenderDrawColor(renderer, 160, 160, 160, 255);
		SDL_RenderClear(renderer);

		// MAKE SURE TO REMOVE THIS BIT WHEN READY
		// 
		// 
		// 
		// 
		// 
		// 
		// TESTING LINES TO ENSURE DIMENSIONS ARE CORRECT
		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // red
		SDL_RenderDrawLine(renderer, 800, 0, 800, 900);   // vertical center line
		SDL_RenderDrawLine(renderer, 1599, 0, 1599, 900); // far right edge

		// Cursor rendering
		cursorBlinkSpeed = 500; // Milliseconds
		showCursor = (SDL_GetTicks() / cursorBlinkSpeed) % 2 == 0;

		// Draw map and game objects
		map->drawMap(shakeOffsetX, shakeOffsetY);
		manager.draw();

		// Render sprite hands over tombstones
		leftHand->getComponent<SpriteComponent>().draw();
		rightHand->getComponent<SpriteComponent>().draw();

		// Render crosshair
		if (!zombies.empty() && currentZombieIndex < zombies.size()) {
			Entity* activeZombie = zombies[currentZombieIndex];
			auto& zombieTransform = activeZombie->getComponent<TransformComponent>();

			// Place crosshair on top of current zombie
			auto& crosshairTransform = crosshair->getComponent<TransformComponent>();
			crosshairTransform.position = zombieTransform.position;

			// Draw crosshair sprite
			crosshair->getComponent<SpriteComponent>().draw();
		}

		if (!allZombiesTransformed && currentZombieIndex < zombies.size()) {
			// Only render prompt if there are still zombies to be defeated
			Entity* activeZombie = zombies[currentZombieIndex];
			auto& zombieTransform = activeZombie->getComponent<TransformComponent>();

			int zombieWidth = 32;
			int zombieCenterX = static_cast<int>(zombieTransform.position.x + (zombieWidth));
			int textY = static_cast<int>(zombieTransform.position.y - 20); // Slightly above zombie

			if (uiManager) {
				// HERE ya goob
				//
				//
				// 
				// 
				// 
				// NEED TO UPDATE PROMPT BOX AND POSSIBLY TEXT SIZE !
				SDL_Color rectColor = { 255, 178, 102, 255 };
				TTF_Font* font = TTF_OpenFont("assets/PressStart2P.ttf", 16);

				if (font) {
					// Calculate total text width
					int totalTextWidth = 0;
					for (char c : targetText) {
						int w, h;
						TTF_SizeText(font, std::string(1, c).c_str(), &w, &h);
						totalTextWidth += w;
					}

					// Center textX based on zombie sprite's center and the text width
					int textX = zombieCenterX - (totalTextWidth / 2);

					// Center background rectangle
					int rectWidth = totalTextWidth + 20; // with some padding
					uiManager->drawRectangle(textX - 10, textY - 5, rectWidth, 25, rectColor);

					// Render letters with spacing
					int letterX = textX;
					int cursorX = textX;

					for (size_t i = 0; i < targetText.size(); ++i) {
						SDL_Color color = { 255, 255, 255, 255 }; // Default to white
						if (i < userInput.size()) {
							if (userInput[i] == targetText[i]) {
								color = { 0, 255, 0, 255 }; // Green for correct input
							}
							else if (!processedInput[i]) {
								color = { 255, 0, 0, 255 }; // Red for incorrect input

								processedInput[i] = true;
							}
							else if (processedInput[i]) {
								color = { 255, 0, 0, 255 }; // Keep red for already processed incorrect input
							}
						}

						std::string letter(1, targetText[i]);
						SDL_Surface* surface = TTF_RenderText_Solid(font, letter.c_str(), color);
						if (surface) {
							SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
							if (texture) {
								SDL_Rect dst = { letterX, textY, surface->w, surface->h };
								SDL_RenderCopy(renderer, texture, nullptr, &dst);

								letterX += surface->w + 1;

								// Update cursorX after rendering the letter
								if (i + 1 == userInput.size()) {
									cursorX = letterX - 2;
								}

								SDL_DestroyTexture(texture);
							}
							SDL_FreeSurface(surface);
						}
					}

					// Handle fully typed case cursor at end
					if (userInput.size() == targetText.size()) {
						cursorX = letterX; // After last letter
					}

					// Draw cursor
					if (showCursor && userInput.size() <= targetText.size()) {
						// Change caret color if input is fully typed but incorrect
						SDL_Color caretColor = { 255, 255, 255, 255 }; // Default to white

						if (userInput.size() == targetText.size() && userInput != targetText) {
							caretColor = { 255, 0, 0, 255 }; // Red for incorrect full word
						}

						int caretWidth = 2;
						int caretHeight = 18;

						SDL_Rect caretRect = {
							cursorX,
							textY,
							caretWidth,
							caretHeight
						};

						SDL_SetRenderDrawColor(renderer, caretColor.r, caretColor.g, caretColor.b, caretColor.a);
						SDL_RenderFillRect(renderer, &caretRect);
					}

					TTF_CloseFont(font);
				}
			}
		}

		// These are lower so they are drawn over the zombies!

		// Draw basic laser
		for (const auto& laser : activeLasers) {
			SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red

			int thickness = 4;

			// Draw 'thickness' of laser (number of lines offset horizontally)
			for (int i = -thickness / 2; i <= thickness / 2; ++i) {
				SDL_RenderDrawLine(
					renderer,
					laser.startX + i,
					laser.startY,
					laser.endX + i,
					laser.endY
				);
			}
		}

		// Draw laser cannons LAST (or else the zombies walk over them and that just looks plain silly)
		laserLeft->getComponent<SpriteComponent>().draw();
		laserRight->getComponent<SpriteComponent>().draw();
		laserMiddle->getComponent<SpriteComponent>().draw();

		// Draw assets on control panel
		if (uiManager) {
			SDL_Color outlineColor = { 255, 255, 255, 255 };
			SDL_Color fgColor = { 102, 255, 105, 255 };
			SDL_Color bgColor = { 255, 102, 102, 255 };
			SDL_Color comboColor = { 255, 255, 102, 255 };
			SDL_Color textColor = { 0, 0, 0, 255 };

			uiManager->drawHealthbar(130, 780, 320, 40, barrierHP, maxHP, "SHIELD:", outlineColor, fgColor, bgColor, controlPanelFont, textColor);
			uiManager->drawStatusBar(130, 840, 320, 40, "STATUS:", statusText, outlineColor, bgColor, controlPanelFont, statusFont, textColor);
			uiManager->drawThreatLvl(1140, 785, 70, 70, zombieCount, "THREAT LVL", outlineColor, bgColor, controlPanelFont, threatLvlFont, textColor);
			uiManager->drawComboAlert(1335, 860, 60, 30, comboLevel, "COMBO:", comboStatus, outlineColor, comboColor, controlPanelFont, comboStatusFont, textColor);
		}

		// Draw level (round) text at top of screen in the middle
		uiManager->drawCenteredText("Round " + std::to_string(level), 10, { 0, 0, 0, 255 }, roundFont, screenWidth);

		// Draw laser power-up!
		if (laserActive) {
			laserPowerup->getComponent<SpriteComponent>().Play("Laser");
		}

		// Draw exclamation point above player when barrier is destroyed
		if (barrierDestroyed) {
			exclamation->getComponent<SpriteComponent>().draw();
		}

		SDL_RenderPresent(renderer);
		break;

	case GameState::BONUS_TITLE:
		// Draw bonus title screen

		SDL_SetRenderDrawColor(renderer, 255, 51, 51, 255);
		SDL_RenderClear(renderer);

		uiManager->drawText("BONUS", 600, 100, { 255, 255, 255, 255 }, gameOverFont);
		uiManager->drawText("STAGE!", 565, 350, { 255, 255, 255, 255 }, gameOverFont);


		if (showBlinkText) {
			uiManager->drawText("Press Enter to Start Bonus Round...", 500, 750, { 255, 255, 255, 255 }, menuFont);
		}

		SDL_RenderPresent(renderer);
		break;

	case GameState::BONUS_STAGE:
		// Draw bonus stage

		SDL_SetRenderDrawColor(renderer, 160, 160, 160, 255);
		SDL_RenderClear(renderer);

		// Cursor rendering
		cursorBlinkSpeed = 500; // Milliseconds
		showCursor = (SDL_GetTicks() / cursorBlinkSpeed) % 2 == 0;

		// Draw map and game objects
		map->drawMap();
		manager.draw();

		// Draw sprite hands
		leftHand->getComponent<SpriteComponent>().draw();
		rightHand->getComponent<SpriteComponent>().draw();

		// Render crosshair on left group first
		if (!leftGroupDefeated) {
			if (!leftToRight.empty() && currentZombieIndex < leftToRight.size()) {
				Entity* activeZombie = leftToRight[currentZombieIndex];
				auto& zombieTransform = activeZombie->getComponent<TransformComponent>();

				// Place crosshair on top of current zombie
				auto& crosshairTransform = crosshair->getComponent<TransformComponent>();
				crosshairTransform.position = zombieTransform.position;

				// Draw crosshair sprite
				crosshair->getComponent<SpriteComponent>().draw();
			}
		}
		else {
			if (!rightToLeft.empty() && currentZombieIndex < rightToLeft.size()) {
				Entity* activeZombie = rightToLeft[currentZombieIndex];
				auto& zombieTransform = activeZombie->getComponent<TransformComponent>();

				// Place crosshair on top of current zombie
				auto& crosshairTransform = crosshair->getComponent<TransformComponent>();
				crosshairTransform.position = zombieTransform.position;

				// Draw crosshair sprite
				crosshair->getComponent<SpriteComponent>().draw();
			}
		}

		// Render prompt on left group first
		if (!leftGroupDefeated) {
			if (!allZombiesTransformed && currentZombieIndex < leftToRight.size()) {
				// Only render prompt if there are still zombies to be defeated
				Entity* activeZombie = leftToRight[currentZombieIndex];
				auto& zombieTransform = activeZombie->getComponent<TransformComponent>();

				int zombieWidth = 32;
				int zombieCenterX = static_cast<int>(zombieTransform.position.x + (zombieWidth));
				int textY = static_cast<int>(zombieTransform.position.y - 20); // Slightly above zombie

				if (uiManager) {
					SDL_Color rectColor = { 255, 178, 102, 255 };
					TTF_Font* font = TTF_OpenFont("assets/PressStart2P.ttf", 16);

					if (font) {
						// Calculate total text width
						int totalTextWidth = 0;
						for (char c : targetText) {
							int w, h;
							TTF_SizeText(font, std::string(1, c).c_str(), &w, &h);
							totalTextWidth += w;
						}

						// Center textX based on zombie sprite's center and the text width
						int textX = zombieCenterX - (totalTextWidth / 2);

						// Center background rectangle
						int rectWidth = totalTextWidth + 20; // With some padding
						uiManager->drawRectangle(textX - 10, textY - 5, rectWidth, 25, rectColor);

						// Render letters with spacing
						int letterX = textX;
						int cursorX = textX;

						for (size_t i = 0; i < targetText.size(); ++i) {
							SDL_Color color = { 255, 255, 255, 255 }; // Default to white
							if (i < userInput.size()) {
								if (userInput[i] == targetText[i]) {
									color = { 0, 255, 0, 255 }; // Green for correct input
								}
								else if (!processedInput[i]) {
									color = { 255, 0, 0, 255 }; // Red for incorrect input

									processedInput[i] = true;
								}
								else if (processedInput[i]) {
									color = { 255, 0, 0, 255 }; // Keep red for already processed incorrect input
								}
							}

							std::string letter(1, targetText[i]);
							SDL_Surface* surface = TTF_RenderText_Solid(font, letter.c_str(), color);
							if (surface) {
								SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
								if (texture) {
									SDL_Rect dst = { letterX, textY, surface->w, surface->h };
									SDL_RenderCopy(renderer, texture, nullptr, &dst);

									letterX += surface->w + 1;

									// Update cursorX AFTER rendering the letter
									if (i + 1 == userInput.size()) {
										cursorX = letterX - 2;
									}

									SDL_DestroyTexture(texture);
								}
								SDL_FreeSurface(surface);
							}
						}

						// Handle fully typed case cursor at end
						if (userInput.size() == targetText.size()) {
							cursorX = letterX; // After last letter
						}

						// Draw cursor
						if (showCursor && userInput.size() <= targetText.size()) {
							// Change caret color if input is fully typed but incorrect
							SDL_Color caretColor = { 255, 255, 255, 255 }; // Default to white

							if (userInput.size() == targetText.size() && userInput != targetText) {
								caretColor = { 255, 0, 0, 255 }; // Red for incorrect full word
							}

							int caretWidth = 2;
							int caretHeight = 18;

							SDL_Rect caretRect = {
								cursorX,
								textY,
								caretWidth,
								caretHeight
							};

							SDL_SetRenderDrawColor(renderer, caretColor.r, caretColor.g, caretColor.b, caretColor.a);
							SDL_RenderFillRect(renderer, &caretRect);
						}

						TTF_CloseFont(font);
					}
				}
			}
		}
		else {
			if (!allZombiesTransformed && currentZombieIndex < rightToLeft.size()) {
				// Only render prompt if there are still zombies to be defeated
				Entity* activeZombie = rightToLeft[currentZombieIndex];
				auto& zombieTransform = activeZombie->getComponent<TransformComponent>();

				int zombieWidth = 32;
				int zombieCenterX = static_cast<int>(zombieTransform.position.x + (zombieWidth));
				int textY = static_cast<int>(zombieTransform.position.y - 20); // Slightly above zombie

				if (uiManager) {
					SDL_Color rectColor = { 255, 178, 102, 255 };
					TTF_Font* font = TTF_OpenFont("assets/PressStart2P.ttf", 16);

					if (font) {
						// Calculate total text width
						int totalTextWidth = 0;
						for (char c : targetText) {
							int w, h;
							TTF_SizeText(font, std::string(1, c).c_str(), &w, &h);
							totalTextWidth += w;
						}

						// Center textX based on zombie sprite's center and the text width
						int textX = zombieCenterX - (totalTextWidth / 2);

						// Center background rectangle
						int rectWidth = totalTextWidth + 20; // With some padding
						uiManager->drawRectangle(textX - 10, textY - 5, rectWidth, 25, rectColor);

						// Render letters with spacing
						int letterX = textX;
						int cursorX = textX;

						for (size_t i = 0; i < targetText.size(); ++i) {
							SDL_Color color = { 255, 255, 255, 255 }; // Default to white
							if (i < userInput.size()) {
								if (userInput[i] == targetText[i]) {
									color = { 0, 255, 0, 255 }; // Green for correct input
								}
								else if (!processedInput[i]) {
									color = { 255, 0, 0, 255 }; // Red for incorrect input

									processedInput[i] = true;
								}
								else if (processedInput[i]) {
									color = { 255, 0, 0, 255 }; // Keep red for already processed incorrect input
								}
							}

							std::string letter(1, targetText[i]);
							SDL_Surface* surface = TTF_RenderText_Solid(font, letter.c_str(), color);
							if (surface) {
								SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
								if (texture) {
									SDL_Rect dst = { letterX, textY, surface->w, surface->h };
									SDL_RenderCopy(renderer, texture, nullptr, &dst);

									letterX += surface->w + 1;

									// Update cursorX AFTER rendering the letter
									if (i + 1 == userInput.size()) {
										cursorX = letterX - 2;
									}

									SDL_DestroyTexture(texture);
								}
								SDL_FreeSurface(surface);
							}
						}

						// Handle fully typed case cursor at end
						if (userInput.size() == targetText.size()) {
							cursorX = letterX; // After last letter
						}

						// Draw cursor
						if (showCursor && userInput.size() <= targetText.size()) {
							// Change caret color if input is fully typed but incorrect
							SDL_Color caretColor = { 255, 255, 255, 255 }; // Default to white

							if (userInput.size() == targetText.size() && userInput != targetText) {
								caretColor = { 255, 0, 0, 255 }; // Red for incorrect full word
							}

							int caretWidth = 2;
							int caretHeight = 18;

							SDL_Rect caretRect = {
								cursorX,
								textY,
								caretWidth,
								caretHeight
							};

							SDL_SetRenderDrawColor(renderer, caretColor.r, caretColor.g, caretColor.b, caretColor.a);
							SDL_RenderFillRect(renderer, &caretRect);
						}

						TTF_CloseFont(font);
					}
				}
			}
		}

		// Moving down here so this is drawn over the zombies

		// Draw basic laser
		for (const auto& laser : activeLasers) {
			SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red

			int thickness = 4;

			// Draw 'thickness' of laser (number of lines offset horizontally)
			for (int i = -thickness / 2; i <= thickness / 2; ++i) {
				SDL_RenderDrawLine(
					renderer,
					laser.startX + i,
					laser.startY,
					laser.endX + i,
					laser.endY
				);
			}
		}

		// Draw laser cannons LAST
		laserLeft->getComponent<SpriteComponent>().draw();
		laserRight->getComponent<SpriteComponent>().draw();
		laserMiddle->getComponent<SpriteComponent>().draw();

		// Draw assets on control panel
		if (uiManager) {
			SDL_Color outlineColor = { 255, 255, 255, 255 };
			SDL_Color fgColor = { 102, 255, 105, 255 };
			SDL_Color bgColor = { 255, 102, 102, 255 };
			SDL_Color comboColor = { 255, 255, 102, 255 };
			SDL_Color textColor = { 0, 0, 0, 255 };

			uiManager->drawHealthbar(130, 780, 320, 40, barrierHP, maxHP, "SHIELD:", outlineColor, fgColor, bgColor, controlPanelFont, textColor);
			uiManager->drawStatusBar(130, 840, 320, 40, "STATUS:", statusText, outlineColor, bgColor, controlPanelFont, statusFont, textColor);
			uiManager->drawThreatLvl(1140, 785, 70, 70, zombieCount, "THREAT LVL", outlineColor, bgColor, controlPanelFont, threatLvlFont, textColor);
			uiManager->drawComboAlert(1335, 860, 60, 30, comboLevel, "COMBO:", comboStatus, outlineColor, comboColor, controlPanelFont, comboStatusFont, textColor);
		}

		// Draw bonus text at top of screen in the center
		uiManager->drawCenteredText("BONUS", 10, { 0, 0, 0, 255 }, roundFont, screenWidth);

		SDL_RenderPresent(renderer);
		break;

	case GameState::ARCADE_RESULTS:
		// Draw results screen

		SDL_SetRenderDrawColor(renderer, 255, 178, 102, 255);
		SDL_RenderClear(renderer);

		hpResults = "Barrier HP Remaining: " + std::to_string(barrierHP);

		// Move unique values from typedWrong to wrongResults
		wrongResults.clear();
		for (const auto& [ch, count] : typedWrong) {
			wrongResults.push_back(ch);
		}

		// Format wrongResults with commas
		formattedResults.str(""); // Clear previous results
		formattedResults.clear(); // Reset state
		for (size_t i = 0; i < wrongResults.size(); ++i) {
			formattedResults << wrongResults[i];

			if (i < wrongResults.size() - 1) { // Add comma for all but the last character
				formattedResults << ", ";
			}
		}

		finalWrongResults = "Letters Typed Incorrectly: " + formattedResults.str();

		// Calculate accuracy
		if (levelTotalLetters > 0) {
			levelAccuracy = (static_cast<double>(levelCorrectLetters) / levelTotalLetters) * 100;
		}

		overallAccuracy = "Level Accuracy: " + formatPercentage(levelAccuracy);

		uiManager->drawText("Level " + std::to_string(level) + " Results!", 625, 50, { 255, 255, 255, 255 }, titleFont);
		uiManager->drawText(hpResults, 40, 200, { 255, 255, 255, 255 }, menuFont);
		uiManager->drawText(finalWrongResults, 40, 400, { 255, 255, 255, 255 }, menuFont);
		uiManager->drawText(overallAccuracy, 40, 600, { 255, 255, 255, 255 }, menuFont);

		// heeeree
		//
		//
		//
		//
		//
		//
		// Add alert when another zombie is spawning

		if (showBlinkText) {
			uiManager->drawText("Press Enter to Start the Next Level!", 500, 750, { 255, 255, 255, 255 }, menuFont);
		}

		SDL_RenderPresent(renderer);
		break;

	case GameState::BONUS_RESULTS:
		// Draw results screen

		SDL_SetRenderDrawColor(renderer, 255, 178, 102, 255);
		SDL_RenderClear(renderer);

		hpResults = "Barrier HP restored: " + std::to_string(bonusHP);

		// heeerreeeee
		// 
		// 
		// 
		// 
		// 
		// 
		// maybe add total Barrier HP here

		// Move unique values from typedWrong to wrongResults
		wrongResults.clear();
		for (const auto& [ch, count] : typedWrong) {
			wrongResults.push_back(ch); 
		}

		// Format wrongResults with commas
		formattedResults.str(""); // Clear previous results
		formattedResults.clear(); // Reset state
		for (size_t i = 0; i < wrongResults.size(); ++i) {
			formattedResults << wrongResults[i];

			if (i < wrongResults.size() - 1) { // Add comma for all but the last character
				formattedResults << ", ";
			}
		}

		finalWrongResults = "Letters Typed Incorrectly: " + formattedResults.str();

		// Calculate accuracy
		if (levelTotalLetters > 0) {
			levelAccuracy = (static_cast<double>(levelCorrectLetters) / levelTotalLetters) * 100;
		}

		totalBonusZombiesDefeated = "Zombies defeated: " + std::to_string(bonusZombiesDefeated) + "/" + std::to_string(totalBonusZombies);

		overallAccuracy = "Level Accuracy: " + formatPercentage(levelAccuracy);

		uiManager->drawText("Bonus Stage Results!", 560, 50, { 255, 255, 255, 255 }, titleFont);
		uiManager->drawText(hpResults, 40, 200, { 255, 255, 255, 255 }, menuFont);
		uiManager->drawText(finalWrongResults, 40, 300, { 255, 255, 255, 255 }, menuFont);
		uiManager->drawText(totalBonusZombiesDefeated, 40, 400, { 255, 255, 255, 255 }, menuFont);
		uiManager->drawText(overallAccuracy, 40, 500, { 255, 255, 255, 255 }, menuFont);

		if (showBlinkText) {
			uiManager->drawText("Press Enter to Start the Next Level!", 500, 750, { 255, 255, 255, 255 }, menuFont);
		}

		SDL_RenderPresent(renderer);
		break;

	case GameState::GAME_OVER:
		// Draw game over screen

		SDL_SetRenderDrawColor(renderer, 255, 51, 51, 255);
		SDL_RenderClear(renderer);

		// Calculate accuracy
		if (sessionTotalLetters > 0) {
			arcadeSessionAccuracy = (static_cast<double>(sessionCorrectLetters) / sessionTotalLetters) * 100;
		}

		overallAccuracy = "Overall Accuracy: " + formatPercentage(arcadeSessionAccuracy);

		uiManager->drawText("GAME", 600, 100, { 255, 255, 255, 255 }, gameOverFont);
		uiManager->drawText("OVER!", 575, 300, { 255, 255, 255, 255 }, gameOverFont);
		if (showBlinkText && level == arcadeHighestLevel) {
			uiManager->drawText("NEW RECORD!", 575, 450, { 255, 255, 255, 255 }, menuFont);
		}
		uiManager->drawText("Highest Level Reached: " + std::to_string(level), 600, 500, { 255, 255, 255, 255 }, menuFont);
		uiManager->drawText("Total Zombies Defeated: " + std::to_string(zombiesDefeated), 600, 550, { 255, 255, 255, 255 }, menuFont);
		uiManager->drawText(overallAccuracy, 600, 600, { 255, 255, 255, 255 }, menuFont);

		if (showBlinkText) {
			uiManager->drawText("Press Enter to Return to the Title Screen...", 400, 750, { 255, 255, 255, 255 }, menuFont);
		}

		SDL_RenderPresent(renderer);
		break;

	case GameState::RECORDS:
		// Draw records screen

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		uiManager->drawText("Records", 700, 100, { 255, 255, 255, 255 }, titleFont);

		title = getTypingTitle(highestWpm);

		uiManager->drawText("Current Title: " + title, 600, 200, { 255, 255, 255, 255 }, menuFont);

		// Add in how many time each mode was played

		// Lessons Mode Accuracy
		uiManager->drawText("Lessons Mode Total Accuracy: " + formatPercentage(recordsLessonAccuracy), 600, 250, {255, 255, 255, 255}, menuFont);

		// Arcade Mode Accuracy
		uiManager->drawText("Arcade Mode Total Accuracy: " + formatPercentage(recordsArcadeAccuracy), 600, 300, {255, 255, 255, 255}, menuFont);

		// WPM Test Accuracy
		uiManager->drawText("WPM Test Total Accuracy: " + formatPercentage(recordsWpmAccuracy), 600, 350, {255, 255, 255, 255}, menuFont);

		// Overall accuracy of every mode
		uiManager->drawText("Overall Accuracy: " + formatPercentage(recordsOverallAccuracy), 600, 400, {255, 255, 255, 255}, menuFont);

		// Highest WPM test score
		uiManager->drawText("Highest WPM Score: " + std::to_string(highestWpm), 600, 450, { 255, 255, 255, 255 }, menuFont);

		// Lessons completed
		lessonsCompleted = 0;
		for (const auto& [difficulty, progress] : saveData.lessonProgressMap) {
			if (progress.passed || progress.fullyCompleted) {
				lessonsCompleted++;
			}
		}

		lessonSummary = "Lessons Completed: (" + std::to_string(lessonsCompleted) + "/" + std::to_string(totalLessons) + ")";
		uiManager->drawText(lessonSummary, 600, 500, { 255, 255, 255, 255 }, menuFont);

		// Characters typed wrong in every mode
		//
		//
		//
		//
		uiManager->drawText("Incorrect Characters:", 600, 550, { 255, 255, 255, 255 }, menuFont);

		y = 600;
		for (const auto& [ch, count] : lifetimeWrongCharacters) {
			//std::string displaySpace = (ch == ' ') ? "<space>" : std::string(1, ch);
			//std::string entry = displaySpace + ": " + std::to_string(count);
			std::string entry = std::string(1, ch) + ": " + std::to_string(count);
			uiManager->drawText(entry, 620, y, { 255, 100, 100, 255 }, menuFont);
			y += 30;
		}

		if (showBlinkText) {
			uiManager->drawText("Press ESC to return to the Main Menu!", 580, 800, { 255, 255, 255, 255 }, menuFont);
		}

		SDL_RenderPresent(renderer);
		break;

	case GameState::WPM_TEST:
		// Draw WPM test

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		if (!wpmTestStarted && showBlinkText) {
			uiManager->drawText(
				"Start typing to begin!",
				620, 250,
				{ 255, 255, 255, 255 },
				menuFont
			);
		}

		uiManager->drawText("Words Per Minute Test", 600, 100, { 255, 255, 255, 255 }, titleFont);

		// Draw timer
		uiManager->drawText("Time: " + std::to_string(wpmTimeRemaining), 50, 50, { 255, 255, 255, 255 }, wpmFont);

		// Typing box
		uiManager->drawRectangle(0, 300, 1600, 150, { 255, 255, 255, 255 });

		// Set positions
		lineStartX = 220;
		topLineY = 280;
		middleLineY = 320;
		bottomLineY = 360;

		letterX = lineStartX;
		cursorX = lineStartX;

		correct = { 0, 255, 0, 255 };
		wrong = { 255, 0, 0, 255 };
		neutral = { 0, 0, 0, 255 };

		if (!wpmTopLine.empty()) {
			uiManager->drawText(wpmTopLine, lineStartX, topLineY, { 160, 160, 160, 255 }, menuFont);
		}

		// Render current line
		for (size_t i = 0; i < wpmCurrentLine.size(); ++i) {
			SDL_Color color = neutral;

			if (i < wpmUserInput.size()) {
				if (wpmUserInput[i] == wpmCurrentLine[i]) color = correct;
				else color = wrong;
			}

			std::string letter(1, wpmCurrentLine[i]);
			SDL_Surface* surface = TTF_RenderText_Solid(menuFont, letter.c_str(), color);
			if (surface) {
				SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
				if (texture) {
					SDL_Rect dst = { letterX, middleLineY, surface->w, surface->h };
					SDL_RenderCopy(renderer, texture, nullptr, &dst);

					letterX += surface->w + 1;

					// Update cursor position after last typed character
					if (i + 1 == wpmUserInput.size()) {
						cursorX = letterX - 2;
					}

					SDL_DestroyTexture(texture);
				}
				SDL_FreeSurface(surface);
			}
		}

		// Handle case where full line is typed
		if (wpmUserInput.size() == wpmCurrentLine.size()) {
			cursorX = letterX;
		}

		// Draw cursor
		if (wpmUserInput.size() <= wpmCurrentLine.size()) {
			SDL_Color caretColor = (wpmUserInput == wpmCurrentLine) ? neutral : wrong;

			SDL_Rect caretRect = {
				cursorX,
				middleLineY,
				2,
				18
			};

			SDL_SetRenderDrawColor(renderer, caretColor.r, caretColor.g, caretColor.b, caretColor.a);
			SDL_RenderFillRect(renderer, &caretRect);
		}

		// Render next line
		uiManager->drawText(wpmNextLine, lineStartX, bottomLineY, { 160, 160, 160, 255 }, menuFont);

		SDL_RenderPresent(renderer);
		break;

	case GameState::WPM_RESULTS:
		// Draw WPM results screen

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		uiManager->drawText("Words Per Minute Test Results", 500, 100, { 255, 255, 255, 255 }, titleFont);

		uiManager->drawText("Time: 60 seconds", 400, 300, { 255, 255, 255, 255 }, menuFont);
		uiManager->drawText("Raw WPM: " + std::to_string((int)rawWpm), 400, 400, { 255, 255, 255, 255 }, menuFont);
		uiManager->drawText("Accuracy: " + std::to_string((int)(wpmAccuracy * 100)) + "%", 400, 500, { 255, 255, 255, 255 }, menuFont);
		uiManager->drawText("Overall WPM: " + std::to_string((int)wpm), 400, 600, { 255, 255, 255, 255 }, menuFont);
		uiManager->drawText("Characters: " + std::to_string(wpmCorrectChars) + " / " + std::to_string(wpmIncorrectChars) + " (correct / incorrect)", 400, 700, {255, 255, 255, 255}, menuFont);

		if (showBlinkText) {
			uiManager->drawText(
				"Press Enter to Return to the Main Menu!",
				400, 800,
				{ 255, 255, 255, 255 },
				menuFont
			);
		}

		SDL_RenderPresent(renderer);
		break;

	case GameState::PAUSE:
		// Draw pause screen

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180); 
		SDL_RenderFillRect(renderer, nullptr); // Fill entire screen

		SDL_Color resumeColor = pauseMenuSelection == 0 ? SDL_Color{ 255, 255, 0, 255 } : SDL_Color{ 255, 255, 255, 255 };
		SDL_Color quitColor = pauseMenuSelection == 1 ? SDL_Color{ 255, 255, 0, 255 } : SDL_Color{ 255, 255, 255, 255 };

		uiManager->drawCenteredText("Paused", 300, { 255, 255, 255, 255 }, titleFont, screenWidth);
		uiManager->drawCenteredText("Resume", 400, resumeColor, menuFont, screenWidth);
		uiManager->drawCenteredText("Quit to Main Menu", 500, quitColor, menuFont, screenWidth);

		SDL_RenderPresent(renderer);
		break;

	default:
		break;
	}

	SDL_RenderPresent(renderer);
}

void Game::clean()
{
	// Clean game/free memory on exit

	// heerree
	// 
	// 
	// 
	// 
	// TODO (add more items to free?!)
	delete uiManager;
	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);
	TTF_Quit();
	SDL_Quit();
	std::cout << "Game Cleaned" << std::endl;
}

// Lessons Mode Methods
// 
// Reset all elements of lessons mode for a fresh playthrough
void Game::resetLessonsMode(WordListManager::Difficulty lessonDifficulty)
{
	// Clean up previous entities if they exist
	if (zombie1) { zombie1->destroy(); zombie1 = nullptr; }
	if (zombie2) { zombie2->destroy(); zombie2 = nullptr; }
	if (zombie3) { zombie3->destroy(); zombie3 = nullptr; }
	if (zombie4) { zombie4->destroy(); zombie4 = nullptr; }
	if (leftHand) { leftHand->destroy(); leftHand = nullptr; }
	if (rightHand) { rightHand->destroy(); rightHand = nullptr; }
	if (crosshair) { crosshair->destroy(); crosshair = nullptr; }
	if (laserMiddle) { laserMiddle->destroy(); laserMiddle = nullptr; }

	// Create fresh zombies
	zombie1 = &manager.addEntity();
	zombie1->addComponent<TransformComponent>(300, 600, 32, 32, 2);
	zombie1->addComponent<SpriteComponent>("assets/Zombie.png");

	zombie2 = &manager.addEntity();
	zombie2->addComponent<TransformComponent>(700, 600, 32, 32, 2);
	zombie2->addComponent<SpriteComponent>("assets/Zombie.png");

	zombie3 = &manager.addEntity();
	zombie3->addComponent<TransformComponent>(900, 600, 32, 32, 2);
	zombie3->addComponent<SpriteComponent>("assets/Zombie.png");

	zombie4 = &manager.addEntity();
	zombie4->addComponent<TransformComponent>(1300, 600, 32, 32, 2);
	zombie4->addComponent<SpriteComponent>("assets/Zombie.png");

	// Setting hand sprites
	leftHand = &manager.addEntity();
	leftHand->addComponent<TransformComponent>(545, 770, 64, 64, 2);
	leftHand->addComponent<SpriteComponent>("assets/Left_Hand.png");

	rightHand = &manager.addEntity();
	rightHand->addComponent<TransformComponent>(930, 770, 64, 64, 2);
	rightHand->addComponent<SpriteComponent>("assets/Right_Hand.png");

	// Initialize crosshair entity
	crosshair = &manager.addEntity();
	crosshair->addComponent<TransformComponent>(0, 0); // Initial position of crosshair
	crosshair->addComponent<SpriteComponent>("assets/Crosshair.png");

	// Middle laser cannon
	laserMiddle = &manager.addEntity();
	laserMiddle->addComponent<TransformComponent>(laserX, 150, 68, 68, 2);
	laserMiddle->addComponent<SpriteComponent>("assets/Laser_Cannon_Middle.png");

	// Clear word list and current line (in case they have words loaded in)
	lessonWords.clear();
	lessonCurrentLine.clear();

	lessonWords = wordManager.getWords(lessonDifficulty, 50);

	// Join them into a single string
	std::ostringstream oss;
	for (size_t i = 0; i < lessonWords.size(); ++i) {
		oss << lessonWords[i];
		if (i < lessonWords.size() - 1) oss << " ";
	}
	lessonCurrentLine = oss.str();

	std::cout << "Lesson initialized with " << lessonWords.size() << " words." << std::endl;

	// Reset hand sprites
	currentLeftTex = "";
	currentRightTex = "";

	// Reset user input
	lessonUserInput.clear();

	// Reset target completion
	lessonCompletion = 0.0f;
	lessonTargetCompletion = 0.0f;

	// Reset lesson passed
	lessonPassed = false;
	lessonFullyCompleted = false;

	// Reset typed chars
	lessonCorrectChars = 0;
	lessonTotalTypedChars = 0;
	lessonIncorrectChars = 0;

	// Reset letters typed incorrectly
	typedWrong.clear();

	// For stats
	lessonResultsStatsUpdated = false;

	// Reset zombies defeated
	zombie1Defeated = false;
	zombie2Defeated = false;
	zombie3Defeated = false;
	zombie4Defeated = false;
	zombiesRemaining = 4;

	// Clear active basic lasers
	activeLasers.clear();

	// Reset timers
	lessonTimeFrozen = false;
	lessonTimeElapsed = 0;
	lessonStartTime = SDL_GetTicks(); // marks the fresh start of the lesson

	lessonCharTextures.clear();
	lessonCharWidths.clear();

	for (char c : lessonCurrentLine) {
		std::string letter(1, c);
		SDL_Surface* surface = TTF_RenderText_Solid(menuFont, letter.c_str(), { 192, 192, 192, 255 });
		if (surface) {
			SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
			if (texture) {
				lessonCharTextures.push_back(texture);
				lessonCharWidths.push_back(surface->w);
			}
			SDL_FreeSurface(surface);
		}
	}

	for (auto* tex : typedCharTextures) {
		SDL_DestroyTexture(tex);
	}
	typedCharTextures.clear();
	typedCharWidths.clear();
	typedCharColors.clear();

	std::cout << "Lessons mode setup/reset!" << std::endl;
}

// Removing entities when exiting lessons mode (just so they don't accidentally show up on other screens!)
void Game::exitLessonsMode()
{
	// Clean up previous entities if they exist
	if (zombie1) { zombie1->destroy(); zombie1 = nullptr; }
	if (zombie2) { zombie2->destroy(); zombie2 = nullptr; }
	if (zombie3) { zombie3->destroy(); zombie3 = nullptr; }
	if (zombie4) { zombie4->destroy(); zombie4 = nullptr; }
	if (leftHand) { leftHand->destroy(); leftHand = nullptr; }
	if (rightHand) { rightHand->destroy(); rightHand = nullptr; }
	if (crosshair) { crosshair->destroy(); crosshair = nullptr; }
	if (laserMiddle) { laserMiddle->destroy(); laserMiddle = nullptr; }

	// Clear laser effects
	activeLasers.clear();

	// Clear input
	lessonUserInput.clear();

	// Reset flags
	zombie1Defeated = zombie2Defeated = zombie3Defeated = zombie4Defeated = false;
	zombiesRemaining = 4;
	lessonsDelayTimerStarted = false;
	lessonsResultsDelayTimer = 0;

	// Deleting char textures
	for (SDL_Texture* tex : lessonCharTextures) {
		SDL_DestroyTexture(tex);
	}
	lessonCharTextures.clear();
	lessonCharWidths.clear();

	// Reset timers
	lessonTimeFrozen = false;
	lessonTimeElapsed = 0;
	//lessonStartTime = SDL_GetTicks(); // Marks the fresh start of the lesson

	// Reset hand sprites
	currentLeftTex = "";
	currentRightTex = "";

	// Reset typed chars
	lessonCorrectChars = 0;
	lessonTotalTypedChars = 0;
	lessonIncorrectChars = 0;

	// Reset letters typed incorrectly
	typedWrong.clear();

	// For stats
	lessonResultsStatsUpdated = false;

	// Anything else to delete?!
}

// To quickly calculate the lessons results and display it on the results screen/lesson select screen
void Game::calculateLessonResults() {
	SaveSystem::LessonProgress& progress = lessonProgressMap[currentLessonDifficulty];

	if (lessonPassed) {
		progress.passed = true;
	}
	if (lessonFullyCompleted) {
		progress.fullyCompleted = true;
	}
	progress.bestAccuracy = std::max(progress.bestAccuracy, static_cast<int>(lessonCompletion));
	progress.bestTime = (progress.bestTime == 0 || lessonResultTime < progress.bestTime) ? lessonResultTime : progress.bestTime;

	lessonIncorrectChars = lessonTotalTypedChars - lessonCorrectChars;
}

// Arcade Mode Methods
// 
// Reset all elements of arcade mode for a fresh playthrough
void Game::resetArcadeMode()
{
	// Remove zombie entities
	for (auto* zombie : zombies) {
		// Reset zombie sprite and transformation status
		zombie->getComponent<SpriteComponent>().setTex("assets/Zombie.png");  // Reset to normal zombie sprite
		zombie->getComponent<TransformStatusComponent>().setTransformed(false); // Reset transformation status
		zombie->destroy(); // Mark zombie entity for removal
	}
	zombies.clear(); // Clear the zombies vector

	for (auto* zombie : leftToRight) {
		// Reset zombie sprite and transformation status
		zombie->getComponent<SpriteComponent>().setTex("assets/Zombie.png");  // Reset to normal zombie sprite
		zombie->getComponent<TransformStatusComponent>().setTransformed(false); // Reset transformation status
		zombie->destroy(); // Mark zombie entity for removal
	}
	leftToRight.clear(); // Clear the zombies vector

	for (auto* zombie : rightToLeft) {
		// Reset zombie sprite and transformation status
		zombie->getComponent<SpriteComponent>().setTex("assets/Zombie.png");  // Reset to normal zombie sprite
		zombie->getComponent<TransformStatusComponent>().setTransformed(false); // Reset transformation status
		zombie->destroy(); // Mark zombie entity for removal
	}
	rightToLeft.clear(); // Clear the zombies vector

	// Remove tombstone entities
	for (auto* tombstone : tombstones) {
		tombstone->destroy(); // Mark tombstone entity for removal
	}
	tombstones.clear(); // Clear the tombstone vector

	// Clean up previous entities if they exist
	if (barrier) { barrier->destroy(); barrier = nullptr; }
	if (leftHand) { leftHand->destroy(); leftHand = nullptr; }
	if (rightHand) { rightHand->destroy(); rightHand = nullptr; }
	if (crosshair) { crosshair->destroy(); crosshair = nullptr; }
	if (laserMiddle) { laserMiddle->destroy(); laserMiddle = nullptr; }
	if (laserLeft) { laserLeft->destroy(); laserLeft = nullptr; }
	if (laserRight) { laserRight->destroy(); laserRight = nullptr; }
	if (comboMeter) { comboMeter->destroy(); comboMeter = nullptr; }

	// Setting player position (No sprite for player because player is inside barrier orb)
	player.addComponent<TransformComponent>(playerX, 660);

	// Setting hand sprites
	leftHand = &manager.addEntity();
	leftHand->addComponent<TransformComponent>(545, 770, 64, 64, 2);
	leftHand->addComponent<SpriteComponent>("assets/Left_Hand.png");

	rightHand = &manager.addEntity();
	rightHand->addComponent<TransformComponent>(930, 770, 64, 64, 2);
	rightHand->addComponent<SpriteComponent>("assets/Right_Hand.png");

	// Barrier orb
	barrier = &manager.addEntity();
	barrier->addComponent<TransformComponent>(barrierX, 640, 64, 64, 2);
	barrier->addComponent<SpriteComponent>("assets/Barrier_Orb_0.png");
	barrier->addComponent<ColliderComponent>("barrier");

	// Initialize crosshair entity
	crosshair = &manager.addEntity();
	crosshair->addComponent<TransformComponent>(0, 0); // Initial position of crosshair
	crosshair->addComponent<SpriteComponent>("assets/crosshair.png");

	// Middle laser cannon
	laserMiddle = &manager.addEntity();
	laserMiddle->addComponent<TransformComponent>(laserX, 0, 68, 68, 2);
	laserMiddle->addComponent<SpriteComponent>("assets/Laser_Cannon_Middle.png");

	// Left and right laser cannons
	laserLeft = &manager.addEntity();
	laserLeft->addComponent<TransformComponent>(0, 0, 64, 64, 2);
	laserLeft->addComponent<SpriteComponent>("assets/Laser_Cannon_Left.png");

	laserRight = &manager.addEntity();
	laserRight->addComponent<TransformComponent>(1472, 0, 64, 64, 2);
	laserRight->addComponent<SpriteComponent>("assets/Laser_Cannon_Right.png");

	// Combo meter
	comboMeter = &manager.addEntity();
	comboMeter->addComponent<TransformComponent>(1350, 785, 64, 32, 2);
	comboMeter->addComponent<SpriteComponent>("assets/Combo_Meter_0.png");

	// Initialize random seed for zombie spawn
	std::srand(static_cast<unsigned int>(std::time(nullptr)));

	// Reset zombie spawn mechanics
	currentZombieIndex = 0;
	allZombiesTransformed = false;

	// Initial number of zombies to spawn
	int numZombies = 3;

	// Spawn zombies at random off-screen positions but not too close to player
	int spawnBuffer = 150; // Distance beyond game window for spawning
	for (size_t i = 0; i < numZombies; ++i)
	{
		Entity* newZombie = &manager.addEntity();

		int spawnEdge = rand() % 3; // 0: top, 1: left, 2: right
		int x, y;
		bool validSpawn = false;

		while (!validSpawn) {
			validSpawn = true;
			switch (spawnEdge)
			{
			case 0: // Top
				x = rand() % screenWidth; // Full width range
				y = -spawnBuffer;
				break;
			case 1: // Left
				x = -spawnBuffer;
				y = rand() % 650; // Ensures zombies spawn above the barrier orb
				break;
			case 2: // Right
				x = screenWidth + spawnBuffer; // Force outside screen bounds
				y = rand() % 650; // Ensures zombies spawn above the barrier orb
				break;
			}

			// Ensure zombie spawn is not too close to player
			auto& playerTransform = player.getComponent<TransformComponent>();
			float dx = playerTransform.position.x - x;
			float dy = playerTransform.position.y - y;
			if (sqrt(dx * dx + dy * dy) < 400.0f) {
				validSpawn = false;
				continue;
			}

			// Check distance to other zombies
			for (Entity* otherZombie : zombies) {
				auto& otherTransform = otherZombie->getComponent<TransformComponent>();
				float odx = otherTransform.position.x - x;
				float ody = otherTransform.position.y - y;
				if (sqrt(odx * odx + ody * ody) < 70.0f) { // Radius for how far zombies spawn from each other
					validSpawn = false;
					break;
				}
			}
		}

		newZombie->addComponent<TransformComponent>(x, y);
		newZombie->addComponent<SpriteComponent>("assets/Zambie_Test-Sheet.png", true);
		newZombie->addComponent<ColliderComponent>("zombie");
		newZombie->addComponent<TransformStatusComponent>(); // Add transformation status
		zombies.push_back(newZombie);
	}

	// Find the closest zombie to the player at game start
	float closestDistance = std::numeric_limits<float>::max();
	size_t closestZombieIndex = 0;

	for (size_t i = 0; i < zombies.size(); ++i) {
		if (!zombies[i]->getComponent<TransformStatusComponent>().getTransformed()) {
			auto& zombieTransform = zombies[i]->getComponent<TransformComponent>();
			float dx = player.getComponent<TransformComponent>().position.x - zombieTransform.position.x;
			float dy = player.getComponent<TransformComponent>().position.y - zombieTransform.position.y;
			float distance = sqrt(dx * dx + dy * dy);

			if (distance < closestDistance) {
				closestDistance = distance;
				closestZombieIndex = i;
			}
		}
	}

	// Set starting target
	currentZombieIndex = closestZombieIndex;

	// Intilalize zombies remaining

	// Intialize barrier health and health font

	// Clear active basic lasers
	activeLasers.clear();

	// Clear laser power-up if still active
	if (laserActive) {
		laserPowerup->destroy();
		laserPowerup = nullptr;
		laserActive = false;
	}

	// For stats
	arcadeResultsStatsUpdated = false;

	// Randomizing words
	arcadeWords = wordManager.getRandomWords(WordListManager::EASY, numZombies);

	// Reset map visual
	map->setDifficulty(MapLevel::EASY);

	// Reset hand sprites
	currentLeftTex = "";
	currentRightTex = "";

	// Reset game variables

	// Reset zombies remaining / zombies defeated
	zombieCount = zombies.size();
	zombiesDefeated = 0;

	// Reset HP / barrier damage
	barrierHP = maxHP;
	updateBarrierDamage(barrierHP);

	// Reset the typing target
	targetText = arcadeWords[currentZombieIndex];

	// Clear user input from last game
	userInput.clear();

	// Reset letters typed incorrectly
	typedWrong.clear();

	// Reset accuracy
	levelCorrectLetters = 0;
	levelTotalLetters = 0;
	sessionCorrectLetters = 0;
	sessionTotalLetters = 0;

	// Reset zambie speed!!
	speed = 0.5f;

	// Reset level
	level = 1;

	// Reset combo variables
	brokenCombo = false;
	laserReady = false;
	comboLevel = 0;
	checkCombo("", targetText);

	// Reset bonus stage variables
	bonusLevel = 0;
	inBonusStage = false;
	bonusSpeed = 4.0f;

	std::cout << "Arcade mode reset!" << std::endl;
}

// Removing entities when exiting arcade mode (just so they don't accidentally show up on other screens!)
void Game::exitArcadeMode()
{
	// Remove zombie entities
	for (auto* zombie : zombies) {
		// Reset zombie sprite and transformation status
		zombie->getComponent<SpriteComponent>().setTex("assets/Zombie.png");  // Reset to normal zombie sprite
		zombie->getComponent<TransformStatusComponent>().setTransformed(false); // Reset transformation status
		zombie->destroy(); // Mark zombie entity for removal
	}
	zombies.clear(); // Clear the zombies vector

	for (auto* zombie : leftToRight) {
		// Reset zombie sprite and transformation status
		zombie->getComponent<SpriteComponent>().setTex("assets/Zombie.png");  // Reset to normal zombie sprite
		zombie->getComponent<TransformStatusComponent>().setTransformed(false); // Reset transformation status
		zombie->destroy(); // Mark zombie entity for removal
	}
	leftToRight.clear(); // Clear the zombies vector

	for (auto* zombie : rightToLeft) {
		// Reset zombie sprite and transformation status
		zombie->getComponent<SpriteComponent>().setTex("assets/Zombie.png");  // Reset to normal zombie sprite
		zombie->getComponent<TransformStatusComponent>().setTransformed(false); // Reset transformation status
		zombie->destroy(); // Mark zombie entity for removal
	}
	rightToLeft.clear(); // Clear the zombies vector

	// Remove tombstone entities
	for (auto* tombstone : tombstones) {
		tombstone->destroy(); // Mark tombstone entity for removal
	}
	tombstones.clear(); // Clear the tombstone vector

	// Cleaning up other entities...
	if (barrier) { barrier->destroy(); barrier = nullptr; }
	if (leftHand) { leftHand->destroy(); leftHand = nullptr; }
	if (rightHand) { rightHand->destroy(); rightHand = nullptr; }
	if (crosshair) { crosshair->destroy(); crosshair = nullptr; }
	if (laserMiddle) { laserMiddle->destroy(); laserMiddle = nullptr; }
	if (laserLeft) { laserLeft->destroy(); laserLeft = nullptr; }
	if (laserRight) { laserRight->destroy(); laserRight = nullptr; }
	if (comboMeter) { comboMeter->destroy(); comboMeter = nullptr; }

	// Cleanup anything else necessary...

	// For stats
	arcadeResultsStatsUpdated = false;

	// Reset letters typed incorrectly
	typedWrong.clear();

	// Reset hand sprites
	currentLeftTex = "";
	currentRightTex = "";
}

// To set up next level of arcade mode
void Game::nextLevel()
{
	// Go to bonus stage every ten rounds
	if (level % 10 == 0 && !inBonusStage) {
		gameState = GameState::BONUS_TITLE; // Transition to bonus title screen
		inBonusStage = true;
		return;
	}

	// Ensuring barrierHP doesn't surpass 100
	if (barrierHP >= 100) {
		barrierHP = 100;
	}

	// Update barrier sprite damage
	updateBarrierDamage(barrierHP);

	// Clear the previous round's zombies and reset zombie index and transformation status
	zombies.clear();
	currentZombieIndex = 0;
	allZombiesTransformed = false;

	// Clear active basic lasers
	activeLasers.clear();

	// Clear laser power-up if still active
	if (laserActive) {
		laserPowerup->destroy();
		laserPowerup = nullptr;
		laserActive = false;
	}

	// Setting number of zombies to spawn, with a new one appearing every 5 levels
	int numZombies = 3 + (level / 5);

	// Randomizing words and updating difficulty every 10 levels 
	int cycleLevel = (level % 30) + 1; // Ensures difficulty cycles every 30 rounds

	if (cycleLevel <= 10) {
		difficulty = WordListManager::EASY;
		map->setDifficulty(MapLevel::EASY);
	}
	else if (cycleLevel <= 20) {
		difficulty = WordListManager::MEDIUM;
		map->setDifficulty(MapLevel::MEDIUM);
	}
	else {
		difficulty = WordListManager::HARD;
		map->setDifficulty(MapLevel::HARD);
	}

	// Get random words for the next level, based on the current difficulty and number of zombies spawning
	arcadeWords = wordManager.getRandomWords(difficulty, numZombies);

	// Spawn zombies at random off-screen positions but not too close to player
	int spawnBuffer = 150; // Distance beyond game window for spawning
	for (size_t i = 0; i < numZombies; ++i)
	{
		Entity* newZombie = &manager.addEntity();

		int spawnEdge = rand() % 3; // 0: top, 1: left, 2: right
		int x, y;
		bool validSpawn = false;

		while (!validSpawn) {
			validSpawn = true;
			switch (spawnEdge)
			{
			case 0: // Top
				x = rand() % 1600; // Full width range
				y = -spawnBuffer;
				break;
			case 1: // Left
				x = -spawnBuffer;
				y = rand() % 650; // Ensures zombies spawn above the barrier orb
				break;
			case 2: // Right
				x = 1600 + spawnBuffer; // Force outside screen bounds
				y = rand() % 650; // Ensures zombies spawn above the barrier orb
				break;
			}

			// Ensure zombie spawn is not too close to player
			auto& playerTransform = player.getComponent<TransformComponent>();
			float dx = playerTransform.position.x - x;
			float dy = playerTransform.position.y - y;
			if (sqrt(dx * dx + dy * dy) < 400.0f) {
				validSpawn = false;
				continue;
			}

			// Check distance to other zombies
			for (Entity* otherZombie : zombies) {
				auto& otherTransform = otherZombie->getComponent<TransformComponent>();
				float odx = otherTransform.position.x - x;
				float ody = otherTransform.position.y - y;
				if (sqrt(odx * odx + ody * ody) < 70.0f) { // May need to adjust radius
					validSpawn = false;
					break;
				}
			}
		}

		newZombie->addComponent<TransformComponent>(x, y);
		newZombie->addComponent<SpriteComponent>("assets/Zambie_Test-Sheet.png", true);
		newZombie->addComponent<ColliderComponent>("zombie");
		newZombie->addComponent<TransformStatusComponent>(); // Add transformation status
		zombies.push_back(newZombie);
	}

	// Find the closest zombie to the player at game start
	float closestDistance = std::numeric_limits<float>::max();
	size_t closestZombieIndex = 0;

	for (size_t i = 0; i < zombies.size(); ++i) {
		if (!zombies[i]->getComponent<TransformStatusComponent>().getTransformed()) {
			auto& zombieTransform = zombies[i]->getComponent<TransformComponent>();
			float dx = player.getComponent<TransformComponent>().position.x - zombieTransform.position.x;
			float dy = player.getComponent<TransformComponent>().position.y - zombieTransform.position.y;
			float distance = sqrt(dx * dx + dy * dy);

			if (distance < closestDistance) {
				closestDistance = distance;
				closestZombieIndex = i;
			}
		}
	}

	// Set starting target
	currentZombieIndex = closestZombieIndex;
	targetText = arcadeWords[currentZombieIndex];

	// For stats
	arcadeResultsStatsUpdated = false;

	// Intilalize zombies remaining
	zombieCount = zombies.size();

	// Clear user input
	userInput.clear();

	// Reset letters typed incorrectly
	typedWrong.clear();

	// Reset accuracy
	levelCorrectLetters = 0;
	levelTotalLetters = 0;

	// Increase zambie speed!!
	speed += 0.1f;

	// Reset speed every 10 levels
	if (level % 10 == 0) {
		speed = 0.5f;
	}

	// Increment level (round)
	level++;

	// Reset bonus total zombies
	totalBonusZombies = 0;

	inBonusStage = false; // Reset the flag when exiting the bonus stage

	// Reset hand sprites
	currentLeftTex = "";
	currentRightTex = "";

	std::cout << "Arcade mode setup for new round!" << std::endl;
}

// To set up bonus stage
void Game::bonusStage()
{
	// Clear the previous round's zombies
	leftToRight.clear();
	rightToLeft.clear();
	currentZombieIndex = 0;
	allZombiesTransformed = false;

	// Clear active basic lasers
	activeLasers.clear();

	// Clear laser power-up if still active
	if (laserActive) {
		laserPowerup->destroy();
		laserPowerup = nullptr;
		laserActive = false;
	}

	// Increasing bonus level
	bonusLevel++;

	// Keeping track of zombie count and increases each bonus round
	int numZombiesLeft = 3 + (bonusLevel - 1);
	int numZombiesRight = 3 + (bonusLevel - 1);

	// Randomize letters every new bonus round
	bonusLeft = wordManager.getRandomWords(WordListManager::BONUSLEFT, numZombiesLeft);
	bonusRight = wordManager.getRandomWords(WordListManager::BONUSRIGHT, numZombiesRight);

	int spacing = 120; // Space between zombies

	// Generate random y-coordinate for left-to-right zombie row
	int yLeft = 150 + (rand() % 360);

	// Left-to-Right group
	for (int i = 0; i < numZombiesLeft; ++i)
	{
		Entity* newZombie = &manager.addEntity();
		int x = -150 - (i * spacing); // Start just outside the left edge
		int y = yLeft;

		newZombie->addComponent<TransformComponent>(x, y);
		newZombie->addComponent<SpriteComponent>("assets/Zambie_Test-Sheet.png", true);
		newZombie->addComponent<ColliderComponent>("zombie");
		newZombie->addComponent<TransformStatusComponent>(); // Add transformation status
		leftToRight.push_back(newZombie);
		totalBonusZombies++;
	}

	// Generate random y-coordinate for right-to-left zombie row
	int yRight = 150 + (rand() % 360);

	// Right-to-Left group
	for (int i = 0; i < numZombiesRight; ++i)
	{
		Entity* newZombie = &manager.addEntity();
		int x = 1600 + (i * spacing); // Start just outside the right edge
		int y = yRight;

		newZombie->addComponent<TransformComponent>(x, y);
		newZombie->addComponent<SpriteComponent>("assets/Zambie_Test-Sheet.png", true);
		newZombie->addComponent<ColliderComponent>("zombie");
		newZombie->addComponent<TransformStatusComponent>(); // Add transformation status
		rightToLeft.push_back(newZombie);
		totalBonusZombies++;
	}

	// Make sure the leftmost zombie is first in the list
	std::reverse(rightToLeft.begin(), rightToLeft.end());

	// Intilalize zombies remaining
	zombieCount += leftToRight.size();
	zombieCount += rightToLeft.size();

	// Set starting target
	targetText = bonusLeft[currentZombieIndex];

	// For stats
	arcadeResultsStatsUpdated = false;

	// Reset letters typed incorrectly
	typedWrong.clear();

	// Reset accuracy
	levelCorrectLetters = 0;
	levelTotalLetters = 0;

	// Reset bonus HP
	bonusHP = 0;

	// Reset bonus stage zombies defeated
	bonusZombiesDefeated = 0;

	// Increase zambie speed!!
	bonusSpeed += 2.0;

	std::cout << "Zombies reset for bonus round!" << std::endl;
}

// To update the barrier orb sprite based on how much damage has been taken
void Game::updateBarrierDamage(int barrierHP) {
	// Convert HP (0-100) to damage level (0-10)
	int damageLevel = 10 - (barrierHP / 10);

	// Clamp damage level at 10
	damageLevel = std::max(0, std::min(damageLevel, 10));

	// Build file path
	std::string texturePath = "assets/Barrier_Orb_" + std::to_string(damageLevel) + ".png";

	// Set texture using c_str()
	barrier->getComponent<SpriteComponent>().setTex(texturePath.c_str());
}

// To check the current combo and update the on screen UI accordingly, and to activate the laser power-up when combo is at max
void Game::checkCombo(const std::string& input, const std::string& target) {
	if (brokenCombo || input != target) {
		comboLevel = 0;
		laserReady = false;
	}
	else {
		comboLevel = std::min(comboLevel + 1, 6);
		if (comboLevel == 6) {
			laserReady = true;
		}
	}

	switch (comboLevel) {
	case 0: comboStatus = "";
		comboMeter->getComponent<SpriteComponent>().setTex("assets/Combo_Meter_0.png");
		break;
	case 1: comboStatus = "x1";
		comboMeter->getComponent<SpriteComponent>().setTex("assets/Combo_Meter_1.png");
		break;
	case 2: comboStatus = "x2";
		comboMeter->getComponent<SpriteComponent>().setTex("assets/Combo_Meter_2.png");
		break;
	case 3: comboStatus = "x3";
		comboMeter->getComponent<SpriteComponent>().setTex("assets/Combo_Meter_3.png");
		break;
	case 4: comboStatus = "x4";
		comboMeter->getComponent<SpriteComponent>().setTex("assets/Combo_Meter_4.png");
		break;
	case 5: comboStatus = "x5";
		comboMeter->getComponent<SpriteComponent>().setTex("assets/Combo_Meter_5.png");
		break;
	case 6: comboStatus = "MAX!";
		comboMeter->getComponent<SpriteComponent>().setTex("assets/Combo_Meter_6.png");
		break;
	}
}

// Fires laser power-up
void Game::fireLaser() {
	if (laserActive) return; // to prevent multiple lasers...

	laserPowerup = &manager.addEntity();
	laserPowerup->addComponent<TransformComponent>(65, 32, 1472, 64, 1);
	laserPowerup->addComponent<SpriteComponent>("assets/Laser-Sheet.png", true);
	laserPowerup->addComponent<ColliderComponent>("laser");

	laserActive = true;
}

// WPM Test Methods
//
// Reset/initialze WPM test
void Game::resetWPMTest() {
	std::cout << "WPM Test Reset!" << std::endl;
	wpmTestStarted = false;
	wpmTestEnded = false;
	wpmTimeRemaining = 60;
	lastSecondTick = 0;
	rawWpm = 0.0f;
	wpmAccuracy = 0.0f;
	wpm = 0.0f;
	wpmCorrectChars = 0;
	wpmTotalTypedChars = 0;
	wpmIncorrectChars = 0;
	wpmTopLine = ""; // Nothing typed yet
	wpmCurrentLine = generateRandomLine();
	wpmNextLine = generateRandomLine();
	wpmUserInput.clear();
	// Reset letters typed incorrectly
	typedWrong.clear();
}

// Handles line shifting logic
void Game::shiftWpmLines() {
	wpmTopLine = wpmCurrentLine;
	wpmCurrentLine = wpmNextLine;
	wpmNextLine = generateRandomLine();
	wpmUserInput.clear();
}

// Counts words (unneeded atm, may add back in tho)
int Game::countWords(const std::string& line) {
	std::istringstream iss(line);
	return std::distance(std::istream_iterator<std::string>(iss), std::istream_iterator<std::string>());
}

// Generates a random line of words for the WPM test
std::string Game::generateRandomLine() {
	std::vector<std::string> wpmWords = wordManager.getRandomWords(WordListManager::WPM, 12); // May need to adjust count so it doesn't overflow!
	std::string line;
	for (const auto& word : wpmWords) {
		line += word + " ";
	}
	if (!line.empty()) line.pop_back(); // Remove trailing space
	return line;
}

// To quickly calculate the WPM results and display it on the results screen
void Game::calculateWPM() {
	rawWpm = (float)wpmTotalTypedChars / 5.0f;

	wpmAccuracy = wpmTotalTypedChars > 0 ? (float)wpmCorrectChars / wpmTotalTypedChars : 0.0f;

	wpm = (float)wpmCorrectChars / 5.0f;

	wpmIncorrectChars = wpmTotalTypedChars - wpmCorrectChars;

	// Update lifetime stat
	if (wpm > highestWpm) {
		highestWpm = wpm;
	}
}

// To get the user's "typing title" based on their highest WPM score!
std::string Game::getTypingTitle(int highestWpm) {
	if (highestWpm < 21) return "Keyboard Confused";
	else if (highestWpm < 31) return "Home Row Tourist";
	else if (highestWpm < 41) return "Hunter and Pecker";
	else if (highestWpm < 51) return "Average Joe";
	else if (highestWpm < 61) return "Speedy Scribbler";
	else if (highestWpm < 71) return "Office Favorite";
	else if (highestWpm < 81) return "Word Machine";
	else if (highestWpm < 91) return "Precision Professional";
	else if (highestWpm < 101) return "Keyboard Wizard";
	else if (highestWpm < 121) return "Terrific Typist";
	else return "QWERTY OVERLORD";
}

// Results method(s)
//
// To calculate the average for each mode
void Game::calculateAverageRecords()
{
	recordsLessonAccuracy = lessonGamesPlayed > 0
		? lessonAccuracyTotal / lessonGamesPlayed
		: 0.0f;

	recordsArcadeAccuracy = arcadeGamesPlayed > 0
		? arcadeAccuracyTotal / arcadeGamesPlayed
		: 0.0f;

	recordsWpmAccuracy = wpmGamesPlayed > 0
		? wpmAccuracyTotal / wpmGamesPlayed
		: 0.0f;

	int totalGames = lessonGamesPlayed + arcadeGamesPlayed + wpmGamesPlayed;
	float totalAccuracy = lessonAccuracyTotal + arcadeAccuracyTotal + wpmAccuracyTotal;

	recordsOverallAccuracy = totalGames > 0
		? totalAccuracy / totalGames
		: 0.0f;
}

// Shared method(s)
// 
// Key-to-finger sprite mapping
void Game::updateHandSprites(const std::string& targetText, const std::string& userInput)
{
	if (userInput.size() >= targetText.size()) return;

	char nextChar = targetText[userInput.size()];
	std::string leftTex = "assets/Left_Hand.png";
	std::string rightTex = "assets/Right_Hand.png";

	// Determine which key was pressed and what fingers to show
	switch (nextChar) {
		// Left pinky
	case 'q': case 'a': case 'z':
		leftTex = "assets/Left_Pinky.png";
		break;

		// Left ring
	case 'w': case 's': case 'x':
		leftTex = "assets/Left_Ring.png";
		break;

		// Left middle
	case 'e': case 'd': case 'c':
		leftTex = "assets/Left_Middle.png";
		break;

		// Left index
	case 'r': case 'f': case 'v':
	case 't': case 'g': case 'b':
		leftTex = "assets/Left_Index.png";
		break;

		// Right index
	case 'y': case 'h': case 'n':
	case 'u': case 'j': case 'm':
		rightTex = "assets/Right_Index.png";
		break;

		// Right middle
	case 'i': case 'k': case ',':
		rightTex = "assets/Right_Middle.png";
		break;

		// Right ring
	case 'o': case 'l': case '.':
		rightTex = "assets/Right_Ring.png";
		break;

		// Right pinky
	case 'p':
		rightTex = "assets/Right_Pinky.png";
		break;

		// Thumbs for space
	case ' ':
		leftTex = "assets/Left_Thumb.png";
		rightTex = "assets/Right_Thumb.png";
		break;

	default:
		break;
	}

	// Only update textures if they have changed
	currentLeftTex = "";
	currentRightTex = "";

	if (leftTex != currentLeftTex) {
		leftHand->getComponent<SpriteComponent>().setTex(leftTex.c_str());
		currentLeftTex = leftTex;
	}

	if (rightTex != currentRightTex) {
		rightHand->getComponent<SpriteComponent>().setTex(rightTex.c_str());
		currentRightTex = rightTex;
	}
}

std::string Game::formatPercentage(float value) {
	std::ostringstream oss;
	oss << std::fixed << std::setprecision(2) << value;
	return oss.str() + "%";
}

// Save/Loads Methods
//
// Store current game stats into saveData
void Game::syncToSaveData() {
	// Lessons mode Stats
	saveData.lessonGamesPlayed = lessonGamesPlayed;
	saveData.lessonAccuracyTotal = lessonAccuracyTotal;

	// Lessons mode progress
	saveData.lessonProgressMap = lessonProgressMap;

	// Arcade mode stats
	saveData.arcadeGamesPlayed = arcadeGamesPlayed;
	saveData.arcadeAccuracyTotal = arcadeAccuracyTotal;
	saveData.arcadeHighestLevel = arcadeHighestLevel;

	// WPM mode stats
	saveData.wpmGamesPlayed = wpmGamesPlayed;
	saveData.wpmAccuracyTotal = wpmAccuracyTotal;
	saveData.highestWpm = highestWpm;

	// Letters typed incorrectly
	saveData.lifetimeWrongCharacters = lifetimeWrongCharacters;
}

// Load saved stats back into the game
void Game::syncFromSaveData() {
	// Lessons mode Stats
	lessonGamesPlayed = saveData.lessonGamesPlayed;
	lessonAccuracyTotal = saveData.lessonAccuracyTotal;

	// Lessons mode progress
	lessonProgressMap = saveData.lessonProgressMap;

	// Arcade mode stats
	arcadeGamesPlayed = saveData.arcadeGamesPlayed;
	arcadeAccuracyTotal = saveData.arcadeAccuracyTotal;
	arcadeHighestLevel = saveData.arcadeHighestLevel;

	// WPM mode stats
	wpmGamesPlayed = saveData.wpmGamesPlayed;
	wpmAccuracyTotal = saveData.wpmAccuracyTotal;
	highestWpm = saveData.highestWpm;

	// Letters Typed Incorrectly
	lifetimeWrongCharacters = saveData.lifetimeWrongCharacters;
}

// Full autosave wrapper
void Game::saveProgress() {
	syncToSaveData();
	if (SaveSystem::saveToFile("autosave.txt", saveData)) {
		std::cout << "Progress autosaved.\n";
	}
	else {
		std::cerr << "Autosave failed!\n";
	}
}

// Full load wrapper
void Game::loadProgress() {
	if (SaveSystem::loadFromFile("autosave.txt", saveData)) {
		syncFromSaveData();
		std::cout << "Save file loaded.\n";
	}
	else {
		std::cerr << "No save file found. Using defaults.\n";
	}
}