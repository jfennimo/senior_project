#include "Game.h"
#include "TextureManager.h"
#include "Map.h"
#include "ECS/Components.h"
#include "Vector2D.h"
#include "Collision.h"
#include "WordListManager.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <format>
#include <sstream>
#include <unordered_map>
#include <vector> // For wordlist and zombie count
#include <cstdlib> // For rand() and srand()
#include <ctime>   // For time()

GameState gameState;

Map* map;
Manager manager;
UIManager* uiManager;

WordListManager wordManager;
WordListManager::Difficulty difficulty;

SDL_Renderer* Game::renderer = nullptr;
SDL_Event Game::event;

// Frame timer
Uint32 currentTime;

// Arcade Mode Entities
auto& player(manager.addEntity());
auto& barrier(manager.addEntity());
auto& leftHand(manager.addEntity());
auto& rightHand(manager.addEntity());
auto& barrier1(manager.addEntity());
auto& barrier2(manager.addEntity());
auto& barrier3(manager.addEntity());
auto& crosshair(manager.addEntity());
auto& laserMiddle(manager.addEntity());
auto& laserLeft(manager.addEntity());
auto& laserRight(manager.addEntity());
auto& comboMeter(manager.addEntity());

Entity* laserPowerup = nullptr;
Entity* exclamation = nullptr;

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

// Wordlists
std::vector<std::string> words = wordManager.getRandomWords(WordListManager::EASY, 3);
std::vector<std::string> bonusLeft = wordManager.getRandomWords(WordListManager::BONUSLEFT, 3);
std::vector<std::string> bonusRight = wordManager.getRandomWords(WordListManager::BONUSRIGHT, 3);

// Holds current target prompt
std::string targetText;

// Zombie entities and active zombie index
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

	gameState = GameState::TITLE_SCREEN; // Initial state

	uiManager = new UIManager(renderer);
	map = new Map();

	screenWidth = width;
	screenHeight = height;

	// Ensuring player and barrier are centered
	playerX = screenWidth / 2;
	barrierX = (screenWidth / 2) - ((barrierWidth * barrierScale) / 2);

	laserX = (screenWidth / 2) - ((68 * 2) / 2);

	// Setting player position (No sprite for player because player is inside barrier orb)
	player.addComponent<TransformComponent>(playerX, 660);

	// Setting hand sprites
	leftHand.addComponent<TransformComponent>(545, 770, 64, 64, 2);
	rightHand.addComponent<TransformComponent>(930, 770, 64, 64, 2);

	leftHand.addComponent<SpriteComponent>("assets/Left_Hand.png");
	rightHand.addComponent<SpriteComponent>("assets/Right_Hand.png");

	// Barrier orb
	barrier.addComponent<TransformComponent>(barrierX, 640, 64, 64, 2);
	barrier.addComponent<SpriteComponent>("assets/Barrier_Orb_0.png");
	barrier.addComponent<ColliderComponent>("barrier");

	// Initialize crosshair entity
	crosshair.addComponent<TransformComponent>(0, 0); // Initial position of crosshair
	crosshair.addComponent<SpriteComponent>("assets/Crosshair.png");

	// Middle laser cannon
	laserMiddle.addComponent<TransformComponent>(laserX, 0, 68, 68, 2);
	laserMiddle.addComponent<SpriteComponent>("assets/Laser_Cannon_Middle.png");

	// Left and right laser cannons
	laserLeft.addComponent<TransformComponent>(0, 0, 64, 64, 2);
	laserRight.addComponent<TransformComponent>(1472, 0, 64, 64, 2);

	laserLeft.addComponent<SpriteComponent>("assets/Laser_Cannon_Left.png");
	laserRight.addComponent<SpriteComponent>("assets/Laser_Cannon_Right.png");

	// Combo meter
	comboMeter.addComponent<TransformComponent>(1350, 785, 64, 32, 2);
	comboMeter.addComponent<SpriteComponent>("assets/Combo_Meter_0.png");

	// Initialize random seed for zombie spawn
	std::srand(static_cast<unsigned int>(std::time(nullptr)));

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
				x = rand() % width; // Full width range
				y = -spawnBuffer;
				break;
			case 1: // Left
				x = -spawnBuffer;
				y = rand() % 650; // Ensures zombies spawn above the barrier orb
				break;
			case 2: // Right
				x = width + spawnBuffer; // Force outside screen bounds
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
	targetText = words[currentZombieIndex];

	// Intilalize zombies remaining
	zombieCount = zombies.size();

	// Intialize barrier health and health font
	barrierHP = maxHP;

	titleFont = TTF_OpenFont("assets/PressStart2P.ttf", 30);
	menuFont = TTF_OpenFont("assets/PressStart2P.ttf", 20);
	healthFont = TTF_OpenFont("assets/PressStart2P.ttf", 20);
	roundFont = TTF_OpenFont("assets/PressStart2P.ttf", 16);
	gameOverFont = TTF_OpenFont("assets/PressStart2P.ttf", 100);
	controlPanelFont = TTF_OpenFont("assets/Square.ttf", 30);
	statusFont = TTF_OpenFont("assets/Technology-BoldItalic.TTF", 40);
	threatLvlFont = TTF_OpenFont("assets/Technology-BoldItalic.TTF", 50);
	comboStatusFont = TTF_OpenFont("assets/Technology-BoldItalic.TTF", 30);
}


void Game::handleEvents()
{
	SDL_PollEvent(&event);
	switch (event.type) {
	case SDL_QUIT:
		isRunning = false;
		break;

	case SDL_KEYDOWN:
		if (event.key.keysym.sym == SDLK_RETURN) {
			if (gameState == GameState::TITLE_SCREEN) {
				gameState = GameState::MAIN_MENU; // Transition main menu
				std::cout << "Navigating to main menu!" << std::endl;
			}
			else if (gameState == GameState::MAIN_MENU) {
				gameState = GameState::ARCADE_MODE; // Transition to arcade mode
				std::cout << "Navigating to arcade mode!" << std::endl;
			}
			else if (gameState == GameState::RESULTS) {
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
				gameState = GameState::TITLE_SCREEN;
				resetGame();
				// Need to update final correct letters / total correct letters... (?!)
				std::cout << "Returning to title screen!" << std::endl;
			}
		}
		if (gameState == GameState::ARCADE_MODE) {
			if (event.key.keysym.sym == SDLK_SPACE && laserReady) {
				fireLaser();  // Fire laser power-up
				laserReady = false; // Consumes laser charge
				comboLevel = 0;
				comboStatus = ""; // Reset display
			}
		}
		if (gameState == GameState::ARCADE_MODE || gameState == GameState::BONUS_STAGE) {
			if (event.key.keysym.sym == SDLK_BACKSPACE && !userInput.empty()) {
				userInput.pop_back(); // Remove last character
			}
		}

		break;

	case SDL_TEXTINPUT:
		if (gameState == GameState::ARCADE_MODE) {
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

			// Check for mistakes immediately, for combo system
			if (userInput.back() != targetText[userInput.size() - 1]) {
				brokenCombo = true;
				comboStatus = "X";
				comboLevel = 0;
			}

			// Increment total number of typed letters
			levelTotalLetters++;
			finalTotalLetters++;

			// Check if typed letter matches target letter
			if (userInput.size() <= targetText.size() && event.text.text[0] == targetText[userInput.size() - 1]) {
				levelCorrectLetters++; // Increment correct letters
				finalCorrectLetters++; // Increment total correct letters for game over screen

				// Resetting hand sprites
				resetHandSprites();
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

			// Increment total number of typed letters
			levelTotalLetters++;
			finalTotalLetters++;

			// Check if typed letter matches target letter
			if (userInput.size() <= targetText.size() && event.text.text[0] == targetText[userInput.size() - 1]) {
				levelCorrectLetters++; // Increment correct letters
				finalCorrectLetters++; // Increment total correct letters for game over screen

				// Resetting hand sprites
				resetHandSprites();
			}
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

	case GameState::ARCADE_MODE:
		// Game logic

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
			auto& crosshairTransform = crosshair.getComponent<TransformComponent>();
			crosshairTransform.position = zombieTransform.position;
		}

		// Update hand sprites to reflect the key needed to be pressed
		updateHandSprites();

		// To update barrier attack status
		barrierUnderAttack = false;

		// Iterate through all zombies
		for (size_t i = 0; i < zombies.size(); ++i) {
			Entity* zombie = zombies[i];
			auto& zombieTransform = zombie->getComponent<TransformComponent>();
			auto& transformStatus = zombie->getComponent<TransformStatusComponent>();

			// Update stun status and timer
			transformStatus.updateStun();

			// If stunned and not transformed, play stun animation (but still allow prompt logic)
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
					barrier.getComponent<ColliderComponent>().collider)) {
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
			if (i == currentZombieIndex && userInput == words[i] && !transformStatus.getTransformed()) {
				for (size_t j = 0; j < userInput.size(); ++j) {
					if (j >= targetText.size() || userInput[j] != targetText[j]) {
						// Append to typedWrong only if not already processed
						if (std::find(typedWrong.begin(), typedWrong.end(), userInput[j]) == typedWrong.end()) {
							typedWrong.push_back(targetText[j]);
						}
					}
				}

				// Check if user types in prompt correctly without errors, to update combo
				checkCombo(userInput, targetText);

				// Basic laser animation for eliminating zombie
				int cannonX = laserMiddle.getComponent<TransformComponent>().position.x + 68; // center of 68px cannon
				int cannonY = laserMiddle.getComponent<TransformComponent>().position.y + 128; // bottom of cannon

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

				// Resetting hand sprites
				resetHandSprites();

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
					targetText = words[currentZombieIndex];
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
				gameState = GameState::RESULTS;
				nextLevelDelayStarted = false; // Reset for next level
			}
		}

		// Barrier destroyed!
		if (!barrierDestroyed && barrierHP <= 0) {
			barrierDestroyed = true;
			gameOverDelayTimer = 120; // 2 seconds at 60 FPS

			// Create exclamation point above player
			exclamation = &manager.addEntity();

			int exclaimX = player.getComponent<TransformComponent>().position.x - 17; // centered..?!
			int exclaimY = player.getComponent<TransformComponent>().position.y - 5; // slightly above player
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
			auto& crosshairTransform = crosshair.getComponent<TransformComponent>();
			crosshairTransform.position = zombieTransform.position;
		}

		// Update crosshair position if zombies are present in the right group
		if (!rightToLeft.empty() && currentZombieIndex < rightToLeft.size()) {
			Entity* activeZombie = rightToLeft[currentZombieIndex];
			auto& zombieTransform = activeZombie->getComponent<TransformComponent>();

			// Update crosshair's position to zombie's position
			auto& crosshairTransform = crosshair.getComponent<TransformComponent>();
			crosshairTransform.position = zombieTransform.position;
		}

		// Update hand sprites to reflect the key needed to be pressed
		updateHandSprites();

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

				// Resetting hand sprites
				resetHandSprites();

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
				for (size_t j = 0; j < userInput.size(); ++j) {
					if (j >= targetText.size() || userInput[j] != targetText[j]) {
						// Append to typedWrong only if not already processed
						if (std::find(typedWrong.begin(), typedWrong.end(), userInput[j]) == typedWrong.end()) {
							typedWrong.push_back(targetText[j]);
						}
					}
				}

				// Basic laser animation for eliminating zombie
				int cannonX = laserMiddle.getComponent<TransformComponent>().position.x + 68; // center of cannon
				int cannonY = laserMiddle.getComponent<TransformComponent>().position.y + 128; // bottom of cannon

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

				// Resetting hand sprites
				resetHandSprites();

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

					// Resetting hand sprites
					resetHandSprites();

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
					for (size_t j = 0; j < userInput.size(); ++j) {
						if (j >= targetText.size() || userInput[j] != targetText[j]) {
							// Append to typedWrong only if not already processed
							if (std::find(typedWrong.begin(), typedWrong.end(), userInput[j]) == typedWrong.end()) {
								typedWrong.push_back(targetText[j]);
							}
						}
					}

					// Basic laser animation for eliminating zombie
					int cannonX = laserMiddle.getComponent<TransformComponent>().position.x + 68; // center of 68px cannon
					int cannonY = laserMiddle.getComponent<TransformComponent>().position.y + 128; // bottom of cannon

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

					// Resetting hand sprites
					resetHandSprites();

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
				gameState = GameState::BONUS_RESULTS; // Transition to results state
				nextLevelDelayStarted = false; // Reset for next level
			}
		}

		break;

	case GameState::RESULTS:
		// Results screen logic

		// Blink counter logic
		currentTime = SDL_GetTicks(); // Get current time

		if (currentTime > lastBlinkTime + BLINK_DELAY) {
			showBlinkText = !showBlinkText;  // Toggle visibility
			lastBlinkTime = currentTime;    // Update last blink time
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

		break;

	case GameState::GAME_OVER:
		// Game over screen logic

		// Blink counter logic
		currentTime = SDL_GetTicks(); // Get current time

		if (currentTime > lastBlinkTime + BLINK_DELAY) {
			showBlinkText = !showBlinkText;  // Toggle visibility
			lastBlinkTime = currentTime;    // Update last blink time
		}

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
		if (!titleFont) {
			std::cout << "Failed to load font: " << TTF_GetError() << std::endl;
			return;
		}

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		uiManager->drawText("Main Menu!", 660, 60, { 255, 255, 255, 255 }, titleFont);

		if (showBlinkText) {
			uiManager->drawText("Arcade Mode", 640, 450, { 255, 255, 255, 255 }, titleFont);
		}

		SDL_RenderPresent(renderer);
		break;

	case GameState::ARCADE_MODE:
		// Draw game

		SDL_SetRenderDrawColor(renderer, 160, 160, 160, 255);
		SDL_RenderClear(renderer);

		// TESTING LINES TO ENSURE DIMENSIONS ARE CORRECT
		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // red
		SDL_RenderDrawLine(renderer, 800, 0, 800, 900);   // vertical center line
		SDL_RenderDrawLine(renderer, 1599, 0, 1599, 900); // far right edge

		// Cursor rendering
		cursorBlinkSpeed = 500; // milliseconds
		showCursor = (SDL_GetTicks() / cursorBlinkSpeed) % 2 == 0;

		// Draw map and game objects
		map->drawMap(shakeOffsetX, shakeOffsetY);
		manager.draw();

		// Render sprite hands over tombstones
		leftHand.getComponent<SpriteComponent>().draw();
		rightHand.getComponent<SpriteComponent>().draw();

		// Render crosshair
		if (!zombies.empty() && currentZombieIndex < zombies.size()) {
			Entity* activeZombie = zombies[currentZombieIndex];
			auto& zombieTransform = activeZombie->getComponent<TransformComponent>();

			// Place crosshair on top of current zombie
			auto& crosshairTransform = crosshair.getComponent<TransformComponent>();
			crosshairTransform.position = zombieTransform.position;

			// Draw crosshair sprite
			crosshair.getComponent<SpriteComponent>().draw();
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
					int cursorX = textX; // default in case userInput is empty (for whatever reason !)

					for (size_t i = 0; i < targetText.size(); ++i) {
						SDL_Color color = { 255, 255, 255, 255 }; // Default to white
						if (i < userInput.size()) {
							if (userInput[i] == targetText[i]) {
								color = { 0, 255, 0, 255 }; // Green for correct input
							}
							else if (!processedInput[i]) {
								color = { 255, 0, 0, 255 }; // Red for incorrect input

								// Append to typedWrong only if not already processed
								if (std::find(typedWrong.begin(), typedWrong.end(), userInput[i]) == typedWrong.end()) {
									typedWrong.push_back(targetText[i]);
								}

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
						cursorX = letterX; // after last letter
					}

					// Draw cursor
					if (showCursor && userInput.size() <= targetText.size()) {
						// Change caret color if input is fully typed but incorrect
						SDL_Color caretColor = { 255, 255, 255, 255 }; // default: white

						if (userInput.size() == targetText.size() && userInput != targetText) {
							caretColor = { 255, 0, 0, 255 }; // red for incorrect full word
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
		laserLeft.getComponent<SpriteComponent>().draw();
		laserRight.getComponent<SpriteComponent>().draw();
		laserMiddle.getComponent<SpriteComponent>().draw();

		// Draw control panel
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
		if (!titleFont) {
			std::cout << "Failed to load font: " << TTF_GetError() << std::endl;
			return;
		}

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
		// Draw game

		SDL_SetRenderDrawColor(renderer, 160, 160, 160, 255);
		SDL_RenderClear(renderer);

		// Cursor rendering
		cursorBlinkSpeed = 500; // milliseconds
		showCursor = (SDL_GetTicks() / cursorBlinkSpeed) % 2 == 0;

		// Draw map and game objects
		map->drawMap();
		manager.draw();

		// Draw sprite hands
		leftHand.getComponent<SpriteComponent>().draw();
		rightHand.getComponent<SpriteComponent>().draw();

		// Render crosshair on left group first
		if (!leftGroupDefeated) {
			if (!leftToRight.empty() && currentZombieIndex < leftToRight.size()) {
				Entity* activeZombie = leftToRight[currentZombieIndex];
				auto& zombieTransform = activeZombie->getComponent<TransformComponent>();

				// Place crosshair on top of current zombie
				auto& crosshairTransform = crosshair.getComponent<TransformComponent>();
				crosshairTransform.position = zombieTransform.position;

				// Draw crosshair sprite
				crosshair.getComponent<SpriteComponent>().draw();
			}
		}
		else {
			if (!rightToLeft.empty() && currentZombieIndex < rightToLeft.size()) {
				Entity* activeZombie = rightToLeft[currentZombieIndex];
				auto& zombieTransform = activeZombie->getComponent<TransformComponent>();

				// Place crosshair on top of current zombie
				auto& crosshairTransform = crosshair.getComponent<TransformComponent>();
				crosshairTransform.position = zombieTransform.position;

				// Draw crosshair sprite
				crosshair.getComponent<SpriteComponent>().draw();
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
						int rectWidth = totalTextWidth + 20; // with some padding
						uiManager->drawRectangle(textX - 10, textY - 5, rectWidth, 25, rectColor);

						// Render letters with spacing
						int letterX = textX;
						int cursorX = textX; // default in case userInput is empty

						for (size_t i = 0; i < targetText.size(); ++i) {
							SDL_Color color = { 255, 255, 255, 255 }; // Default to white
							if (i < userInput.size()) {
								if (userInput[i] == targetText[i]) {
									color = { 0, 255, 0, 255 }; // Green for correct input
								}
								else if (!processedInput[i]) {
									color = { 255, 0, 0, 255 }; // Red for incorrect input

									// Append to typedWrong only if not already processed
									if (std::find(typedWrong.begin(), typedWrong.end(), userInput[i]) == typedWrong.end()) {
										typedWrong.push_back(targetText[i]);
									}

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
							cursorX = letterX; // after last letter
						}

						// Draw cursor
						if (showCursor && userInput.size() <= targetText.size()) {
							// Change caret color if input is fully typed but incorrect
							SDL_Color caretColor = { 255, 255, 255, 255 }; // default: white

							if (userInput.size() == targetText.size() && userInput != targetText) {
								caretColor = { 255, 0, 0, 255 }; // red for incorrect full word
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
						int rectWidth = totalTextWidth + 20; // with some padding
						uiManager->drawRectangle(textX - 10, textY - 5, rectWidth, 25, rectColor);

						// Render letters with spacing
						int letterX = textX;
						int cursorX = textX; // default in case userInput is empty

						for (size_t i = 0; i < targetText.size(); ++i) {
							SDL_Color color = { 255, 255, 255, 255 }; // Default to white
							if (i < userInput.size()) {
								if (userInput[i] == targetText[i]) {
									color = { 0, 255, 0, 255 }; // Green for correct input
								}
								else if (!processedInput[i]) {
									color = { 255, 0, 0, 255 }; // Red for incorrect input

									// Append to typedWrong only if not already processed
									if (std::find(typedWrong.begin(), typedWrong.end(), userInput[i]) == typedWrong.end()) {
										typedWrong.push_back(targetText[i]);
									}

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

						// Handle fully typed case   cursor at end
						if (userInput.size() == targetText.size()) {
							cursorX = letterX; // after last letter
						}

						// Draw cursor
						if (showCursor && userInput.size() <= targetText.size()) {
							// Change caret color if input is fully typed but incorrect
							SDL_Color caretColor = { 255, 255, 255, 255 }; // default: white

							if (userInput.size() == targetText.size() && userInput != targetText) {
								caretColor = { 255, 0, 0, 255 }; // red for incorrect full word
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
		laserLeft.getComponent<SpriteComponent>().draw();
		laserRight.getComponent<SpriteComponent>().draw();
		laserMiddle.getComponent<SpriteComponent>().draw();

		// Draw control panel
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

	case GameState::RESULTS:
		// Draw results screen
		if (!titleFont) {
			std::cout << "Failed to load font: " << TTF_GetError() << std::endl;
			return;
		}

		SDL_SetRenderDrawColor(renderer, 255, 178, 102, 255);
		SDL_RenderClear(renderer);

		hpResults = "Barrier HP Remaining: " + std::to_string(barrierHP);

		// Move unique values from typedWrong to wrongResults
		wrongResults.clear(); // Clear previous values
		for (auto i : typedWrong) {
			if (wrongResults.find(i) == std::string::npos) { // Ensure no duplicates
				wrongResults.push_back(i);
			}
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

		levelAccuracyStream.str(""); // Clear previous accuracy
		levelAccuracyStream.clear(); // Reset state
		levelAccuracyStream << "Level Accuracy: " << std::fixed << std::setprecision(2) << levelAccuracy << "%";
		overallAccuracy = levelAccuracyStream.str(); // Assign the formatted string

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
		if (!titleFont) {
			std::cout << "Failed to load font: " << TTF_GetError() << std::endl;
			return;
		}

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
		wrongResults.clear(); // Clear previous values
		for (auto i : typedWrong) {
			if (wrongResults.find(i) == std::string::npos) { // Ensure no duplicates
				wrongResults.push_back(i);
			}
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

		levelAccuracyStream.str(""); // Clear previous accuracy
		levelAccuracyStream.clear(); // Reset state
		levelAccuracyStream << "Level Accuracy: " << std::fixed << std::setprecision(2) << levelAccuracy << "%";
		overallAccuracy = levelAccuracyStream.str(); // Assign the formatted string

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
		if (!titleFont) {
			std::cout << "Failed to load font: " << TTF_GetError() << std::endl;
			return;
		}

		SDL_SetRenderDrawColor(renderer, 255, 51, 51, 255);
		SDL_RenderClear(renderer);

		// Calculate accuracy
		if (finalTotalLetters > 0) {
			levelAccuracy = (static_cast<double>(finalCorrectLetters) / finalTotalLetters) * 100;
		}

		levelAccuracyStream.str(""); // Clear previous accuracy
		levelAccuracyStream.clear(); // Reset state
		levelAccuracyStream << "Overall Accuracy: " << std::fixed << std::setprecision(2) << levelAccuracy << "%";
		overallAccuracy = levelAccuracyStream.str(); // Assign the formatted string

		uiManager->drawText("GAME", 600, 100, { 255, 255, 255, 255 }, gameOverFont);
		uiManager->drawText("OVER!", 575, 350, { 255, 255, 255, 255 }, gameOverFont);
		uiManager->drawText("Highest Level Reached: " + std::to_string(level), 600, 500, { 255, 255, 255, 255 }, menuFont);
		uiManager->drawText("Total Zombies Defeated: " + std::to_string(zombiesDefeated), 600, 550, { 255, 255, 255, 255 }, menuFont);
		uiManager->drawText(overallAccuracy, 600, 600, { 255, 255, 255, 255 }, menuFont);

		if (showBlinkText) {
			uiManager->drawText("Press Enter to Return to the Title Screen...", 400, 750, { 255, 255, 255, 255 }, menuFont);
		}

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
	words = wordManager.getRandomWords(difficulty, numZombies);

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
	targetText = words[currentZombieIndex];

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

	std::cout << "Zombies reset for new round!" << std::endl;
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
	int yLeft = 150 + (rand() % 360); // 150-559

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
	int yRight = 150 + (rand() % 360); // 150-559

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
	bonusSpeed += 1.0;

	std::cout << "Zombies reset for bonus round!" << std::endl;
}

// Reset all elements of arcade mode for a fresh playthrough
void Game::resetGame()
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

	// Clear active basic lasers
	activeLasers.clear();

	// Clear laser power-up if still active
	if (laserActive) {
		laserPowerup->destroy();
		laserPowerup = nullptr;
		laserActive = false;
	}

	// Reset zombie spawn mechanics
	currentZombieIndex = 0;
	allZombiesTransformed = false;

	// Reset zombies that spawn
	int numZombies = 3;

	// Randomizing words
	words = wordManager.getRandomWords(WordListManager::EASY, numZombies);

	// Reset map visual
	map->setDifficulty(MapLevel::EASY);

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

	// Reset game variables

	// Reset zombies remaining / zombies defeated
	zombieCount = zombies.size();
	zombiesDefeated = 0;

	// Reset HP / barrier damage
	barrierHP = maxHP;
	updateBarrierDamage(barrierHP);

	// Reset the typing target
	targetText = words[currentZombieIndex];

	// Clear user input from last game
	userInput.clear();

	// Reset letters typed incorrectly
	typedWrong.clear();

	// Reset accuracy
	levelCorrectLetters = 0;
	levelTotalLetters = 0;
	finalCorrectLetters = 0;
	finalTotalLetters = 0;

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
	bonusSpeed = 2.0f;

	std::cout << "Arcade mode reset!" << std::endl;
}

// Key-to-finger sprite mapping
void Game::updateHandSprites()
{
	// Get next letter to be typed and update sprite fingers accordingly
	for (size_t i = 0; i < targetText.size(); i++) {
		if (i <= userInput.size()) {
			// Left pinky
			if (targetText[i] == 'q') {
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Pinky.png");
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			else if (targetText[i] == 'a') {
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Pinky.png");
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			else if (targetText[i] == 'z') {
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Pinky.png");
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			// Left ring
			else if (targetText[i] == 'w') {
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Ring.png");
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			else if (targetText[i] == 's') {
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Ring.png");
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			else if (targetText[i] == 'x') {
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Ring.png");
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			// Left middle
			else if (targetText[i] == 'e') {
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Middle.png");
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			else if (targetText[i] == 'd') {
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Middle.png");
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			else if (targetText[i] == 'c') {
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Middle.png");
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			// Left index
			else if (targetText[i] == 'r') {
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Index.png");
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			else if (targetText[i] == 'f') {
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Index.png");
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			else if (targetText[i] == 'v') {
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Index.png");
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			else if (targetText[i] == 't') {
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Index.png");
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			else if (targetText[i] == 'g') {
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Index.png");
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			else if (targetText[i] == 'b') {
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Index.png");
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}

			// Right index
			else if (targetText[i] == 'y') {
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Index.png");
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			else if (targetText[i] == 'h') {
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Index.png");
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			else if (targetText[i] == 'n') {
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Index.png");
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			else if (targetText[i] == 'u') {
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Index.png");
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			else if (targetText[i] == 'j') {
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Index.png");
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			else if (targetText[i] == 'm') {
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Index.png");
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			// Right middle
			else if (targetText[i] == 'i') {
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Middle.png");
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			else if (targetText[i] == 'k') {
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Middle.png");
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			else if (targetText[i] == ',') {
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Middle.png");
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			// Right ring
			else if (targetText[i] == 'o') {
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Ring.png");
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			else if (targetText[i] == 'l') {
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Ring.png");
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			else if (targetText[i] == '.') {
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Ring.png");
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			// Right pinky
			else if (targetText[i] == 'p') {
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Pinky.png");
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			// Left thumb / Right thumb
			else if (targetText[i] == ' ') {
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Thumb.png");
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Thumb.png");
			}
		}
	}
}

// To reset hand sprites back to their default state (no fingers highlighted)
void Game::resetHandSprites() {
	leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
	rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
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
	barrier.getComponent<SpriteComponent>().setTex(texturePath.c_str());
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
		comboMeter.getComponent<SpriteComponent>().setTex("assets/Combo_Meter_0.png");
		break;
	case 1: comboStatus = "x1";
		comboMeter.getComponent<SpriteComponent>().setTex("assets/Combo_Meter_1.png");
		break;
	case 2: comboStatus = "x2";
		comboMeter.getComponent<SpriteComponent>().setTex("assets/Combo_Meter_2.png");
		break;
	case 3: comboStatus = "x3";
		comboMeter.getComponent<SpriteComponent>().setTex("assets/Combo_Meter_3.png");
		break;
	case 4: comboStatus = "x4";
		comboMeter.getComponent<SpriteComponent>().setTex("assets/Combo_Meter_4.png");
		break;
	case 5: comboStatus = "x5";
		comboMeter.getComponent<SpriteComponent>().setTex("assets/Combo_Meter_5.png");
		break;
	case 6: comboStatus = "MAX!";
		comboMeter.getComponent<SpriteComponent>().setTex("assets/Combo_Meter_6.png");
		break;
	}
}

// Fires laser power-up
void Game::fireLaser() {
	if (laserActive) return; // to prevent multiple lasers...

	laserPowerup = &manager.addEntity();
	laserPowerup->addComponent<TransformComponent>(60, 32, 1472, 64, 1);
	laserPowerup->addComponent<SpriteComponent>("assets/Laser-Sheet.png", true);
	laserPowerup->addComponent<ColliderComponent>("laser");

	laserActive = true;
}