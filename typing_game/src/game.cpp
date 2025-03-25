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

auto& player(manager.addEntity());
auto& leftHand(manager.addEntity());
auto& rightHand(manager.addEntity());
auto& barrier1(manager.addEntity());
auto& barrier2(manager.addEntity());
auto& barrier3(manager.addEntity());
auto& greenSiren1(manager.addEntity());
auto& greenSiren2(manager.addEntity());
auto& crosshair(manager.addEntity());

// Fonts
TTF_Font* titleFont;
TTF_Font* menuFont;
TTF_Font* healthFont;
TTF_Font* gameOverFont;

// Frame timer
Uint32 currentTime;

// Wordlist stuff
//std::vector<std::string> wordList = { "test", "brains" };
//std::vector<std::string> bonusList = { "a", "b", "c" , "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z" };

std::vector<std::string> words = wordManager.getRandomWords(WordListManager::EASY, 3);
//std::vector<std::string> mediumWords = wordManager.getRandomWords(WordListManager::MEDIUM, 3);
//std::vector<std::string> hardWords = wordManager.getRandomWords(WordListManager::HARD, 3);
std::vector<std::string> bonusLeft = wordManager.getRandomWords(WordListManager::BONUSLEFT, 3);
std::vector<std::string> bonusRight = wordManager.getRandomWords(WordListManager::BONUSRIGHT, 3);


//std::vector<std::string> wordList = { "test", "brains", "yum", "howdy", "yuck" };
size_t currentPromptIndex = 0; // Track the current word prompt
std::string targetText; // Holds current target prompt


// Zombie entities and active zombie index
std::vector<Entity*> zombies;
std::vector<Entity*> leftToRight;
std::vector<Entity*> rightToLeft;
 
std::vector<Entity*> tombstones;
size_t currentZombieIndex = 0; // Tracks the currently active zombie

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


	// previously 1280x720
		// now 1600x900
	// Setting player position
	player.addComponent<TransformComponent>(770, 820);
	player.addComponent<SpriteComponent>("assets/Player.png");

	// Setting hand sprites
	leftHand.addComponent<TransformComponent>(515, 770, 64, 64, 2);
	rightHand.addComponent<TransformComponent>(970, 770, 64, 64, 2);

	leftHand.addComponent<SpriteComponent>("assets/Left_Hand.png");
	rightHand.addComponent<SpriteComponent>("assets/Right_Hand.png");

	// Walls around player
	barrier1.addComponent<TransformComponent>(650.0f, 768.0f, 32, 280, 1);
	barrier2.addComponent<TransformComponent>(650.0f, 768.0f, 190, 32, 1);
	barrier3.addComponent<TransformComponent>(930.0f, 768.0f, 190, 32, 1);

	barrier1.addComponent<SpriteComponent>("assets/Barrier1.png");
	barrier1.addComponent<ColliderComponent>("barrier");
	barrier2.addComponent<SpriteComponent>("assets/Barrier2.png");
	barrier2.addComponent<ColliderComponent>("barrier");
	barrier3.addComponent<SpriteComponent>("assets/Barrier3.png");
	barrier3.addComponent<ColliderComponent>("barrier");

	// Green and red sirens on barrier
	greenSiren1.addComponent<TransformComponent>(645, 728, 10, 10, 4);
	greenSiren2.addComponent<TransformComponent>(926, 728, 10, 10, 4);

	greenSiren1.addComponent<SpriteComponent>("assets/Siren_Green.png");
	greenSiren2.addComponent<SpriteComponent>("assets/Siren_Green.png");

	// Initialize crosshair entity
	crosshair.addComponent<TransformComponent>(0, 0); // Initial position of crosshair
	crosshair.addComponent<SpriteComponent>("assets/Crosshair.png");

	// Initialize random seed
	std::srand(static_cast<unsigned int>(std::time(nullptr)));

	// TESTING
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
				y = rand() % 650; // Keep it within valid height
				break;
			case 2: // Right
				x = width + spawnBuffer; // Force outside screen bounds
				y = rand() % 650; // Keep within valid height
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
				if (sqrt(odx * odx + ody * ody) < 60.0f) { // May need to adjust radius
					validSpawn = false;
					break;
				}
			}
		}

		newZombie->addComponent<TransformComponent>(x, y);
		newZombie->addComponent<SpriteComponent>("assets/Zombie.png");
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

	// Initialize target prompt
	//targetText = easyWords[currentPromptIndex];

	// Intialize barrier health and health font
	barrierHP = maxHP;

	titleFont = TTF_OpenFont("assets/PressStart2P.ttf", 30);
	menuFont = TTF_OpenFont("assets/PressStart2P.ttf", 20);
	healthFont = TTF_OpenFont("assets/PressStart2P.ttf", 20);
	gameOverFont = TTF_OpenFont("assets/PressStart2P.ttf", 100);
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
				//allZombiesTransformed = false;
				nextLevel();
				std::cout << "Starting next round!" << std::endl;
			}
			else if (gameState == GameState::BONUS_TITLE) {
				gameState = GameState::BONUS_STAGE; // Start bonus round
				//allZombiesTransformed = false;
				bonusStage();
				std::cout << "Starting bonus round!" << std::endl;
			}
			else if (gameState == GameState::BONUS_RESULTS) {
				gameState = GameState::ARCADE_MODE; // Start next level of arcade mode
				//allZombiesTransformed = false;
				nextLevel();
				std::cout << "Starting next round!" << std::endl;
			}
			else if (gameState == GameState::GAME_OVER) {
				gameState = GameState::TITLE_SCREEN;
				resetGame();
				// Need to update final correct letters / total correct letters...
				std::cout << "Returning to title screen!" << std::endl;
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

		if (gameState == GameState::BONUS_STAGE) {
			// Prevent typing if word is fully typed AND incorrect
			if (userInput.size() >= targetText.size() && userInput != targetText) {
				break; // Lock input until user deletes
			}

			userInput += event.text.text; // Append typed text
			processedInput.assign(userInput.size(), false);

			// NEED TO UPDATE (with wut? idk)
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
		currentTime = SDL_GetTicks(); // Get current time

		if (currentTime > lastBlinkTime + BLINK_DELAY) {
			showBlinkText = !showBlinkText;  // Toggle visibility
			lastBlinkTime = currentTime;    // Update last blink time
		}
		break;

	case GameState::MAIN_MENU:
		// Main menu logic

		// Blink counter logic
		currentTime = SDL_GetTicks(); // Get current time

		if (currentTime > lastBlinkTime + BLINK_DELAY) {
			showBlinkText = !showBlinkText;  // Toggle visibility
			lastBlinkTime = currentTime;    // Update last blink time
		}
		break;

	case GameState::ARCADE_MODE:
		// Game logic

		currentTime = SDL_GetTicks(); // Get current time in milliseconds

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

		// Iterate through all zombies
		for (size_t i = 0; i < zombies.size(); ++i) {
			Entity* zombie = zombies[i];
			auto& zombieTransform = zombie->getComponent<TransformComponent>();
			auto& transformStatus = zombie->getComponent<TransformStatusComponent>();

			// Check if zombie is transformed
			if (!transformStatus.getTransformed()) {
				// Move zombie toward the player
				float dx = playerTransform.position.x - zombieTransform.position.x;
				float dy = playerTransform.position.y - zombieTransform.position.y;

				float magnitude = sqrt(dx * dx + dy * dy);
				if (magnitude > 0) {
					dx /= magnitude;
					dy /= magnitude;
				}

				zombieTransform.position.x += dx * speed;
				zombieTransform.position.y += dy * speed;

				// Check for wall collisions
				if (Collision::AABB(zombie->getComponent<ColliderComponent>().collider,
					barrier1.getComponent<ColliderComponent>().collider) ||
					Collision::AABB(zombie->getComponent<ColliderComponent>().collider,
						barrier2.getComponent<ColliderComponent>().collider) ||
					Collision::AABB(zombie->getComponent<ColliderComponent>().collider,
						barrier3.getComponent<ColliderComponent>().collider)) {
					zombieTransform.position.x -= dx * speed;
					zombieTransform.position.y -= dy * speed;

					// Wall collision is true
					barrierUnderAttack = true; // Track if zombies are attacking

					// Lower hp if zombie touches barrier
					barrierHP--;
					if (barrierHP < 0) barrierHP = 0;

					// Wall hit detected
					std::cout << "Barrier hit! HP: " << barrierHP << std::endl;

					if (currentTime - lastFlashTime > 200) // Flash every 200ms
					{
						flashState = !flashState; // Toggle flash state
						const char* newTexture = flashState ? "assets/Siren_Green.png" : "assets/Siren_Red.png";

						// Ensure greenSiren1 has a SpriteComponent before setting texture
						if (greenSiren1.hasComponent<SpriteComponent>())
						{
							greenSiren1.getComponent<SpriteComponent>().setTex(newTexture);
						}

						if (greenSiren2.hasComponent<SpriteComponent>())
						{
							greenSiren2.getComponent<SpriteComponent>().setTex(newTexture);
						}

						lastFlashTime = currentTime;
					}
				} else
				{
					if (barrierUnderAttack && (currentTime - lastFlashTime > 300)) {
						barrierUnderAttack = false; // Track if zombies are attacking
						
						greenSiren1.getComponent<SpriteComponent>().setTex("assets/Siren_Green.png");
						greenSiren2.getComponent<SpriteComponent>().setTex("assets/Siren_Green.png");
					}
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

				// Transform zombie
				zombie->getComponent<SpriteComponent>().setTex("assets/Tombstone.png");
				transformStatus.setTransformed(true);
				tombstones.push_back(zombie);

				// Update zombie count / zombies defeated
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

		if (allZombiesTransformed && gameState == GameState::ARCADE_MODE) {
			gameState = GameState::RESULTS; // Transition to results state
		}

		if (barrierHP <= 0) {
			gameState = GameState::GAME_OVER;
		}

		break;

	case GameState::BONUS_TITLE:
		// Bonus title logic

		// Blink counter logic
		currentTime = SDL_GetTicks(); // Get current time

		if (currentTime > lastBlinkTime + BLINK_DELAY) {
			showBlinkText = !showBlinkText;  // Toggle visibility
			lastBlinkTime = currentTime;    // Update last blink time
		}
		break;

	case GameState::BONUS_STAGE:
		// Bonus stage logic
		currentTime = SDL_GetTicks(); // Get current time in milliseconds

		// Update crosshair position if zombies are present
		if (!leftToRight.empty() &&  currentZombieIndex < leftToRight.size()) {
			Entity* activeZombie = leftToRight[currentZombieIndex];
			auto& zombieTransform = activeZombie->getComponent<TransformComponent>();

			// Update crosshair's position to zombie's position
			auto& crosshairTransform = crosshair.getComponent<TransformComponent>();
			crosshairTransform.position = zombieTransform.position;
		}

		// Update crosshair position if zombies are present
		if (!rightToLeft.empty() && currentZombieIndex < rightToLeft.size()) {
			Entity* activeZombie = rightToLeft[currentZombieIndex];
			auto& zombieTransform = activeZombie->getComponent<TransformComponent>();

			// Update crosshair's position to zombie's position
			auto& crosshairTransform = crosshair.getComponent<TransformComponent>();
			crosshairTransform.position = zombieTransform.position;
		}

		// Update hand sprites to reflect the key needed to be pressed
		updateHandSprites();

		//if (!leftToRight.empty()) {
		//	targetText = bonusLeft[currentPromptIndex]; // Use words from the left group
		//}
		//else if (!rightToLeft.empty()) {
		//	targetText = bonusRight[currentPromptIndex]; // Use words from the right group
		//}

		// Check if all left-to-right zombies are transformed before moving right-to-left zombies
		leftGroupDefeated = true;
		for (auto* zombie : leftToRight) {
			if (!zombie->getComponent<TransformStatusComponent>().getTransformed()) {
				leftGroupDefeated = false;
				break;
			}
		}

		// Iterate through left to right zombies
		for (size_t i = 0; i < leftToRight.size(); ++i) {
			Entity* zombie = leftToRight[i];
			auto& zombieTransform = zombie->getComponent<TransformComponent>();
			auto& transformStatus = zombie->getComponent<TransformStatusComponent>();

			// Check if zombie moves past screen, then eliminate if so
			if (zombieTransform.position.x > 1600 && !transformStatus.getTransformed()) {
				// Transform zombie
				transformStatus.setTransformed(true);
				zombie->getComponent<SpriteComponent>().setTex("assets/Tombstone.png");
				tombstones.push_back(zombie);

				// Update zombie count
				zombieCount--;

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

			// Check if zombie is transformed, then move if not
			if (!transformStatus.getTransformed()) {
				zombieTransform.position.x += bonusSpeed;
			}
			// Possible update to bonus game?
			// Create random choice of zombie movement, x+= being default

			/*
				int randLeft = rand() % 1;
				if (randLeft == 0) {
					zombieTransform.position.x += bonusSpeed;
				}
				else if (randLeft == 1) {
					zombieTransform.position.x += bonusSpeed;
					zombieTransform.position.y += bonusSpeed;
				}
			}
			*/

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

				// Transform zombie
				zombie->getComponent<SpriteComponent>().setTex("assets/Tombstone.png");
				transformStatus.setTransformed(true);
				tombstones.push_back(zombie);

				// Update zombie count / zombies defeated
				zombieCount--;
				zombiesDefeated++;

				// Update bonus zombie count / zombies defeated
				bonusZombiesDefeated++;

				// Restore some HP or... add this at the results screen...
				bonusHP += 5;

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

		// Iterate through right to left zombies, only if left group is defeated
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
					zombie->getComponent<SpriteComponent>().setTex("assets/Tombstone.png");
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
						float nextX = std::numeric_limits<float>::max(); // Look for the smallest `x` greater than `currentX`
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

					// Transform zombie
					zombie->getComponent<SpriteComponent>().setTex("assets/Tombstone.png");
					transformStatus.setTransformed(true);
					tombstones.push_back(zombie);

					// Update zombie count / zombies defeated
					zombieCount--;
					zombiesDefeated++;

					// Update bonus zombie count / zombies defeated
					bonusZombiesDefeated++;

					// Restore some HP or... add this at the results screen...
					bonusHP += 5;

					// Clear user input
					userInput.clear();

					// Resetting hand sprites
					resetHandSprites();

					// Move to the zombie to the right
					if (i == currentZombieIndex) {
						float currentX = rightToLeft[currentZombieIndex]->getComponent<TransformComponent>().position.x;
						float nextX = std::numeric_limits<float>::max(); // Look for the smallest `x` greater than `currentX`
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

		if (allZombiesTransformed && gameState == GameState::BONUS_STAGE) {
			barrierHP += bonusHP;
			gameState = GameState::BONUS_RESULTS; // Transition to results state
		}

		if (barrierHP <= 0) {
			gameState = GameState::BONUS_RESULTS;
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

		//inBonusStage = false; // Reset the flag when exiting the bonus stage

		break;

	case GameState::GAME_OVER:
		// Game over logic

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

		// Cursor rendering
		cursorBlinkSpeed = 500; // milliseconds
		showCursor = (SDL_GetTicks() / cursorBlinkSpeed) % 2 == 0;

		// Draw map and game objects
		map->drawMap();
		manager.draw();

		// Render sprite hands over tombstones
		leftHand.getComponent<SpriteComponent>().draw();
		rightHand.getComponent<SpriteComponent>().draw();

		// Update hand sprites to reflect the key needed to be pressed
		//updateHandSprites();

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

		// Check if all zombies have been defeated (transformed)
		//allZombiesTransformed = true;
		//for (size_t i = 0; i < zombies.size(); ++i) {
		//    if (!zombies[i]->getComponent<TransformStatusComponent>().getTransformed()) {
		//        allZombiesTransformed = false;
		//        break;
		//    }
		//}

		if (!allZombiesTransformed && currentZombieIndex < zombies.size()) {
			// Only render prompt if there are still zombies to be defeated
			Entity* activeZombie = zombies[currentZombieIndex];
			auto& zombieTransform = activeZombie->getComponent<TransformComponent>();

			//int textX = static_cast<int>(zombieTransform.position.x); // Zombie's x position
			int zombieWidth = 32;
			int zombieCenterX = static_cast<int>(zombieTransform.position.x + (zombieWidth));
			int textY = static_cast<int>(zombieTransform.position.y - 20); // Slightly above zombie

			if (uiManager) {
				SDL_Color rectColor = { 255, 178, 102, 255 };
				//uiManager->drawRectangle(textX - 25, textY - 5, 125, 25, rectColor);

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

					// Handle fully typed case — cursor at end
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

		// Moving down here so this is drawn over the zombies
		
		// Draw HP bar
		if (uiManager && healthFont) {
			SDL_Color outlineColor = { 255, 255, 255, 255 };
			SDL_Color fgColor = { 102, 255, 105, 255 };
			SDL_Color bgColor = { 255, 102, 102, 255 };
			//SDL_Color textColor = { 255, 255, 51, 255 };
			SDL_Color textColor = { 0, 0, 0, 255 };
			uiManager->drawHealthbar(50, 35, 250, 30, barrierHP, maxHP, outlineColor, fgColor, bgColor, "Barrier HP", healthFont, textColor);
		}

		// Draw level text
		uiManager->drawText("Level " + std::to_string(level), 715, 10, { 0, 0, 0, 255 }, healthFont);

		// Draw zombies remaining text
		uiManager->drawText("Zombies Remaining: " + std::to_string(zombieCount), 1090, 10, { 0, 0, 0, 255 }, healthFont);

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

		// Render sprite hands over tombstones
		leftHand.getComponent<SpriteComponent>().draw();
		rightHand.getComponent<SpriteComponent>().draw();

		// Update hand sprites to reflect the key needed to be pressed
		//updateHandSprites();

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

				//int textX = static_cast<int>(zombieTransform.position.x); // Zombie's x position
				//int textY = static_cast<int>(zombieTransform.position.y - 20); // Slightly above zombie

				if (uiManager) {
					SDL_Color rectColor = { 255, 178, 102, 255 };
					//uiManager->drawRectangle(textX - 25, textY - 5, 125, 25, rectColor);

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

						// Handle fully typed case — cursor at end
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

				//int textX = static_cast<int>(zombieTransform.position.x); // Zombie's x position
				//int textY = static_cast<int>(zombieTransform.position.y - 20); // Slightly above zombie

				if (uiManager) {
					SDL_Color rectColor = { 255, 178, 102, 255 };
					//uiManager->drawRectangle(textX - 25, textY - 5, 125, 25, rectColor);

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

						// Handle fully typed case — cursor at end
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

		// Draw HP bar
		if (uiManager && healthFont) {
			SDL_Color outlineColor = { 255, 255, 255, 255 };
			SDL_Color fgColor = { 102, 255, 105, 255 };
			SDL_Color bgColor = { 255, 102, 102, 255 };
			//SDL_Color textColor = { 255, 255, 51, 255 };
			SDL_Color textColor = { 0, 0, 0, 255 };
			uiManager->drawHealthbar(50, 35, 250, 30, barrierHP, maxHP, outlineColor, fgColor, bgColor, "Barrier HP", healthFont, textColor);
		}

		// Draw level text
		uiManager->drawText("BONUS STAGE", 700, 10, { 0, 0, 0, 255 }, healthFont);

		// Draw zombies remaining text
		uiManager->drawText("Zombies Remaining: " + std::to_string(zombieCount), 1090, 10, { 0, 0, 0, 255 }, healthFont);

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

		// Calculate accuracy (avoiding division by zero for testing)
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

		// Calculate accuracy (avoiding division by zero for testing)
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

		// Calculate accuracy (avoiding division by zero for testing)
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

	// TODO (add more items to free)
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
	if (level % 10 == 0 && !inBonusStage) {
		gameState = GameState::BONUS_TITLE; // Transition to bonus title screen
		inBonusStage = true;
		return;
	}

	// Ensuring barrierHP doesn't surpass 500
	if (barrierHP >= 500) {
		barrierHP = 500;
	}

	zombies.clear(); // Clear the previous round's zombies
	currentZombieIndex = 0;
	allZombiesTransformed = false;

	// Setting number of zombies to spawn, with a new one appearing every 5 levels
	int numZombies = 3 + (level / 5);

	// Randomizing words and updating difficulty every 10 levels 
	int cycleLevel = (level % 30) + 1; // Ensures levels 1–30 -> 1–30

	if (cycleLevel <= 10) {
		difficulty = WordListManager::EASY;
	}
	else if (cycleLevel <= 20) {
		difficulty = WordListManager::MEDIUM;
	}
	else {
		difficulty = WordListManager::HARD;
	}

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
				y = rand() % 650; // Keep it within valid height
				break;
			case 2: // Right
				x = 1600 + spawnBuffer; // Force outside screen bounds
				y = rand() % 650; // Keep within valid height
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
				if (sqrt(odx * odx + ody * ody) < 60.0f) { // May need to adjust radius
					validSpawn = false;
					break;
				}
			}
		}

		newZombie->addComponent<TransformComponent>(x, y);
		newZombie->addComponent<SpriteComponent>("assets/Zombie.png");
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

	// Reset the typing target
	currentPromptIndex = 0;
	//targetText = easyWords[currentPromptIndex];

	// Reset hand sprites
	//resetHandSprites();

	// Reset letters typed incorrectly
	typedWrong.clear();

	// Reset accuracy
	levelCorrectLetters = 0;
	levelTotalLetters = 0;

	// Increase zambie speed!! Also decrease slightly every 10 levels...
	speed += 0.1f;

	if (level % 10 == 0) {
		speed -= 0.5f;
	}

	// Increment level
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

	// Increasing bonus level
	bonusLevel++;

	// Keeping track of zombie count and increases each bonus round
	int numZombiesLeft = 3 + (bonusLevel - 1);
	int numZombiesRight = 3 + (bonusLevel - 1);

	// Randomize letters every new bonus round
	bonusLeft = wordManager.getRandomWords(WordListManager::BONUSLEFT, numZombiesLeft);
	bonusRight = wordManager.getRandomWords(WordListManager::BONUSRIGHT, numZombiesRight);

	int spacing = 120; // Space between zombies

	// Generate random y-coordinate for left to right zombie row
	int yLeft = 200 + (rand() % 500);

	// Left to Right group
	for (int i = 0; i < numZombiesLeft; ++i)
	{
		Entity* newZombie = &manager.addEntity();
		int x = -150 - (i * spacing); // Start just outside the left edge
		int y = yLeft;
		//int y = 200 + (rand() % 500);

		newZombie->addComponent<TransformComponent>(x, y);
		newZombie->addComponent<SpriteComponent>("assets/Zombie.png");
		newZombie->addComponent<ColliderComponent>("zombie");
		newZombie->addComponent<TransformStatusComponent>(); // Add transformation status
		leftToRight.push_back(newZombie);
		totalBonusZombies++;
	}

	// Generate random y-coordinate for right to left zombie row
	int yRight = 200 + (rand() % 500);

	// Right to Left group
	for (int i = 0; i < numZombiesRight; ++i)
	{
		Entity* newZombie = &manager.addEntity();
		int x = 1600 + (i * spacing); // Start just outside the right edge
		int y = yRight;

		newZombie->addComponent<TransformComponent>(x, y);
		newZombie->addComponent<SpriteComponent>("assets/Zombie.png");
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

	// Reset the typing target
	currentPromptIndex = 0;
	targetText = bonusLeft[currentPromptIndex];

	// Reset hand sprites
	//resetHandSprites();

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

	// Reset zombie spawn mechanics
	currentZombieIndex = 0;
	allZombiesTransformed = false;

	// TESTING
	int numZombies = 3;

	// Randomizing words
	words = wordManager.getRandomWords(WordListManager::EASY, numZombies);

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
				y = rand() % 650; // Keep it within valid height
				break;
			case 2: // Right
				x = 1600 + spawnBuffer; // Force outside screen bounds
				y = rand() % 650; // Keep within valid height
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
				if (sqrt(odx * odx + ody * ody) < 60.0f) { // May need to adjust radius
					validSpawn = false;
					break;
				}
			}
		}

		newZombie->addComponent<TransformComponent>(x, y);
		newZombie->addComponent<SpriteComponent>("assets/Zombie.png");
		newZombie->addComponent<ColliderComponent>("zombie");
		newZombie->addComponent<TransformStatusComponent>(); // Add transformation status
		zombies.push_back(newZombie);
	}

	// Reset game variables

	// Reset zombies remaining / zombies defeated
	zombieCount = zombies.size();
	zombiesDefeated = 0;

	// Reset HP
	barrierHP = maxHP;

	// Reset the typing target
	currentPromptIndex = 0;
	targetText = words[currentPromptIndex];

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

	std::cout << "Arcade mode reset!" << std::endl;
}

// Key-to-finger sprite mapping
void Game::updateHandSprites() 
{
	// Get next letter to be typed and update sprite fingers accordingly
	for (size_t i = 0; i < targetText.size(); i++) {
		//std::cout << "Checking key: " << targetText[i] << std::endl;
		if (i <= userInput.size()) {
			// Left pinky
			if (targetText[i] == 'q') {
				//std::cout << "Checking key: " << targetText[i] << std::endl;
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Pinky.png");
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			else if (targetText[i] == 'a') {
				//std::cout << "Checking key: " << targetText[i] << std::endl;
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Pinky.png");
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			else if (targetText[i] == 'z') {
				//std::cout << "Checking key: " << targetText[i] << std::endl;
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Pinky.png");
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			// Left ring
			else if (targetText[i] == 'w') {
				//std::cout << "Checking key: " << targetText[i] << std::endl;
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Ring.png");
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			else if (targetText[i] == 's') {
				//std::cout << "Checking key: " << targetText[i] << std::endl;
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Ring.png");
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			else if (targetText[i] == 'x') {
				//std::cout << "Checking key: " << targetText[i] << std::endl;
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Ring.png");
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			// Left middle
			else if (targetText[i] == 'e') {
				//std::cout << "Checking key: " << targetText[i] << std::endl;
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Middle.png");
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			else if (targetText[i] == 'd') {
				//std::cout << "Checking key: " << targetText[i] << std::endl;
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Middle.png");
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			else if (targetText[i] == 'c') {
				//std::cout << "Checking key: " << targetText[i] << std::endl;
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Middle.png");
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			// Left index
			else if (targetText[i] == 'r') {
				//std::cout << "Checking key: " << targetText[i] << std::endl;
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Index.png");
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			else if (targetText[i] == 'f') {
				//std::cout << "Checking key: " << targetText[i] << std::endl;
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Index.png");
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			else if (targetText[i] == 'v') {
				//std::cout << "Checking key: " << targetText[i] << std::endl;
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Index.png");
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			else if (targetText[i] == 't') {
				//std::cout << "Checking key: " << targetText[i] << std::endl;
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Index.png");
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			else if (targetText[i] == 'g') {
				//std::cout << "Checking key: " << targetText[i] << std::endl;
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Index.png");
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			else if (targetText[i] == 'b') {
				//std::cout << "Checking key: " << targetText[i] << std::endl;
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Index.png");
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}

			// Right index
			else if (targetText[i] == 'y') {
				//std::cout << "Checking key: " << targetText[i] << std::endl;
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Index.png");
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			else if (targetText[i] == 'h') {
				//std::cout << "Checking key: " << targetText[i] << std::endl;
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Index.png");
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			else if (targetText[i] == 'n') {
				//std::cout << "Checking key: " << targetText[i] << std::endl;
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Index.png");
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			else if (targetText[i] == 'u') {
				//std::cout << "Checking key: " << targetText[i] << std::endl;
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Index.png");
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			else if (targetText[i] == 'j') {
				//std::cout << "Checking key: " << targetText[i] << std::endl;
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Index.png");
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			else if (targetText[i] == 'm') {
				//std::cout << "Checking key: " << targetText[i] << std::endl;
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Index.png");
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			// Right middle
			else if (targetText[i] == 'i') {
				//std::cout << "Checking key: " << targetText[i] << std::endl;
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Middle.png");
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			else if (targetText[i] == 'k') {
				//std::cout << "Checking key: " << targetText[i] << std::endl;
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Middle.png");
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			else if (targetText[i] == ',') {
				//std::cout << "Checking key: " << targetText[i] << std::endl;
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Middle.png");
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			// Right ring
			else if (targetText[i] == 'o') {
				//std::cout << "Checking key: " << targetText[i] << std::endl;
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Ring.png");
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			else if (targetText[i] == 'l') {
				//std::cout << "Checking key: " << targetText[i] << std::endl;
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Ring.png");
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			else if (targetText[i] == '.') {
				//std::cout << "Checking key: " << targetText[i] << std::endl;
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Ring.png");
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			// Right pinky
			else if (targetText[i] == 'p') {
				//std::cout << "Checking key: " << targetText[i] << std::endl;
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Pinky.png");
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			// Left thumb / Right thumb
			else if (targetText[i] == ' ') {
				//std::cout << "Checking key: " << targetText[i] << std::endl;
				leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Thumb.png");
				rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Thumb.png");
			}
		}
	}
}

void Game::resetHandSprites() {
	leftHand.getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
	rightHand.getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
}
