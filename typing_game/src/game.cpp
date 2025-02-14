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
#include <vector> // For wordlist and zombie count
#include <cstdlib> // For rand() and srand()
#include <ctime>   // For time()

GameState gameState;

Map* map;
Manager manager;
UIManager* uiManager;

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
std::vector<std::string> wordList = { "test", "brains" };
//std::vector<std::string> wordList = { "test", "brains", "yum", "howdy", "yuck" };
size_t currentPromptIndex = 0; // Track the current word prompt
std::string targetText; // Holds current target prompt

// Zombie entities and active zombie index
std::vector<Entity*> zombies;
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
			SDL_SetRenderDrawColor(renderer, 148, 148, 148, 0);
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

	// Setting player position
	player.addComponent<TransformComponent>(615, 640);
	player.addComponent<SpriteComponent>("assets/Player.png");

	// Setting hand sprites
	leftHand.addComponent<TransformComponent>(380, 580, 64, 64, 2);
	rightHand.addComponent<TransformComponent>(790, 580, 64, 64, 2);

	leftHand.addComponent<SpriteComponent>("assets/Left_Hand.png");
	rightHand.addComponent<SpriteComponent>("assets/Right_Hand.png");

	// Walls around player
	barrier1.addComponent<TransformComponent>(520.0f, 600.0f, 32, 225, 1);
	barrier2.addComponent<TransformComponent>(520.0f, 600.0f, 150, 32, 1);
	barrier3.addComponent<TransformComponent>(745.0f, 600.0f, 150, 32, 1);

	barrier1.addComponent<SpriteComponent>("assets/Barrier1.png");
	barrier1.addComponent<ColliderComponent>("barrier");
	barrier2.addComponent<SpriteComponent>("assets/Barrier2.png");
	barrier2.addComponent<ColliderComponent>("barrier");
	barrier3.addComponent<SpriteComponent>("assets/Barrier3.png");
	barrier3.addComponent<ColliderComponent>("barrier");

	// Green and red sirens on barrier
	greenSiren1.addComponent<TransformComponent>(515, 560, 10, 10, 4);
	greenSiren2.addComponent<TransformComponent>(740, 560, 10, 10, 4);

	greenSiren1.addComponent<SpriteComponent>("assets/Siren_Green.png");
	greenSiren2.addComponent<SpriteComponent>("assets/Siren_Green.png");

	// Initialize crosshair entity
	crosshair.addComponent<TransformComponent>(0, 0); // Initial position of crosshair
	crosshair.addComponent<SpriteComponent>("assets/Crosshair.png");

	// Initialize random seed
	std::srand(static_cast<unsigned int>(std::time(nullptr)));

	// Spawn zombies at random off-screen positions but not too close to player
	int spawnBuffer = 100; // Distance beyond game window for spawning
	for (size_t i = 0; i < wordList.size(); ++i)
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
				x = rand() % width;
				y = -spawnBuffer;
				break;
			case 1: // Left
				x = -spawnBuffer;
				y = rand() % height;
				break;
			case 2: // Right
				x = width + spawnBuffer;
				y = rand() % height;
				break;
			}

			// Ensure zombie spawn is not too close to player
			auto& playerTransform = player.getComponent<TransformComponent>();
			float dx = playerTransform.position.x - x;
			float dy = playerTransform.position.y - y;
			if (sqrt(dx * dx + dy * dy) < 200.0f) {
				validSpawn = false;
			}
		}

		newZombie->addComponent<TransformComponent>(x, y);
		newZombie->addComponent<SpriteComponent>("assets/Zombie.png");
		newZombie->addComponent<ColliderComponent>("zombie");
		newZombie->addComponent<TransformStatusComponent>(); // Add transformation status
		zombies.push_back(newZombie);
	}
	
	// Intilalize zombies remaining
	zombieCount = zombies.size();

	// Initialize target prompt
	targetText = wordList[currentPromptIndex];

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
			else if (gameState == GameState::GAME_OVER) {
				gameState = GameState::TITLE_SCREEN;
				resetGame();
				// Need to update final correct letters / total correct letters...
				std::cout << "Returning to title screen!" << std::endl;
			}
		}
		if (gameState == GameState::ARCADE_MODE) {
			if (event.key.keysym.sym == SDLK_BACKSPACE && !userInput.empty()) {
				userInput.pop_back(); // Remove last character
			}
		}
		break;

	case SDL_TEXTINPUT:
		if (gameState == GameState::ARCADE_MODE) {
			userInput += event.text.text; // Append typed text
			processedInput.assign(userInput.size(), false);

			// Increment total number of typed letters
			levelTotalLetters++;
			finalTotalLetters++;

			// Check if typed letter matches target letter
			if (userInput.size() <= targetText.size() && event.text.text[0] == targetText[userInput.size() - 1]) {
				levelCorrectLetters++; // Increment correct letters
				finalCorrectLetters++; // Increment total correct letters for game over screen
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
			if (userInput == wordList[i] && !transformStatus.getTransformed()) {
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
					targetText = wordList[currentZombieIndex];
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

	case GameState::RESULTS:
		// Results screen logic
		
		// Blink counter logic
		currentTime = SDL_GetTicks(); // Get current time

		if (currentTime > lastBlinkTime + BLINK_DELAY) {
			showBlinkText = !showBlinkText;  // Toggle visibility
			lastBlinkTime = currentTime;    // Update last blink time
		}
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

		uiManager->drawText("Letter RIP", 520, 360, { 255, 255, 255, 255 }, titleFont);

		if (showBlinkText) {
			uiManager->drawText("Press Enter to Start!", 470, 420, { 255, 255, 255, 255 }, menuFont);
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

		uiManager->drawText("Main Menu!", 520, 50, { 255, 255, 255, 255 }, titleFont);

		if (showBlinkText) {
			uiManager->drawText("Arcade Mode", 500, 360, { 255, 255, 255, 255 }, titleFont);
		}

		SDL_RenderPresent(renderer);
		break;

	case GameState::ARCADE_MODE:
		// Draw game

		// Draw map and game objects
		map->drawMap();
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

			int textX = static_cast<int>(zombieTransform.position.x); // Zombie's x position
			int textY = static_cast<int>(zombieTransform.position.y - 20); // Slightly above zombie

			if (uiManager) {
				SDL_Color rectColor = { 255, 178, 102, 255 };
				uiManager->drawRectangle(textX - 25, textY - 5, 125, 25, rectColor);

				TTF_Font* font = TTF_OpenFont("assets/PressStart2P.ttf", 16);
				if (font) {
					int letterX = textX;
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
								letterX += surface->w;
								SDL_DestroyTexture(texture);
							}
							SDL_FreeSurface(surface);
						}
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
			uiManager->drawHealthbar(40, 30, 200, 25, barrierHP, maxHP, outlineColor, fgColor, bgColor, "Barrier HP", healthFont, textColor);
		}

		// Draw level text
		uiManager->drawText("Level " + std::to_string(level), 570, 10, { 0, 0, 0, 255 }, healthFont);

		// Draw zombies remaining text
		uiManager->drawText("Zombies Remaining: " + std::to_string(zombieCount), 870, 10, { 0, 0, 0, 255 }, healthFont);

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

		uiManager->drawText("Level " + std::to_string(level) + " Results!", 430, 50, { 255, 255, 255, 255 }, titleFont);
		uiManager->drawText(hpResults, 20, 150, { 255, 255, 255, 255 }, menuFont);
		uiManager->drawText(finalWrongResults, 20, 300, { 255, 255, 255, 255 }, menuFont);
		uiManager->drawText(overallAccuracy, 20, 450, { 255, 255, 255, 255 }, menuFont);

		if (showBlinkText) {
			uiManager->drawText("Press Enter to Start the Next Level!", 290, 600, { 255, 255, 255, 255 }, menuFont);
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

		uiManager->drawText("GAME", 450, 80, { 255, 255, 255, 255 }, gameOverFont);
		uiManager->drawText("OVER!", 425, 280, { 255, 255, 255, 255 }, gameOverFont);
		uiManager->drawText("Highest Level Reached: " + std::to_string(level), 420, 450, { 255, 255, 255, 255 }, menuFont);
		uiManager->drawText("Total Zombies Defeated: " + std::to_string(zombiesDefeated), 420, 500, { 255, 255, 255, 255 }, menuFont);
		uiManager->drawText(overallAccuracy, 420, 550, { 255, 255, 255, 255 }, menuFont);

		if (showBlinkText) {
			uiManager->drawText("Press Enter to Return to the Title Screen...", 225, 630, { 255, 255, 255, 255 }, menuFont);
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
	zombies.clear(); // Clear the previous round's zombies
	currentZombieIndex = 0;
	allZombiesTransformed = false;

	int spawnBuffer = 100; // Distance beyond game window for spawning
	for (size_t i = 0; i < wordList.size(); ++i)
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
				x = rand() % 1280;
				y = -spawnBuffer;
				break;
			case 1: // Left
				x = -spawnBuffer;
				y = rand() % 720;
				break;
			case 2: // Right
				x = 1280 + spawnBuffer;
				y = rand() % 720;
				break;
			}

			// Ensure zombie spawn is not too close to player
			auto& playerTransform = player.getComponent<TransformComponent>();
			float dx = playerTransform.position.x - x;
			float dy = playerTransform.position.y - y;
			if (sqrt(dx * dx + dy * dy) < 200.0f) {
				validSpawn = false;
			}
		}

		newZombie->addComponent<TransformComponent>(x, y);
		newZombie->addComponent<SpriteComponent>("assets/Zombie.png");
		newZombie->addComponent<ColliderComponent>("zombie");
		newZombie->addComponent<TransformStatusComponent>(); // Add transformation status
		
		zombies.push_back(newZombie);
	}

	// Intilalize zombies remaining
	zombieCount = zombies.size();

	// Reset the typing target
	currentPromptIndex = 0;
	targetText = wordList[currentPromptIndex];

	// Reset letters typed incorrectly
	typedWrong.clear();

	// Reset accuracy
	levelCorrectLetters = 0;
	levelTotalLetters = 0;

	// Increase zambie speed!!
	speed += 0.1;

	// Increment level
	level++;

	std::cout << "Zombies reset for new round!" << std::endl;
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

	// Remove tombstone entities
	for (auto* tombstone : tombstones) {
		tombstone->destroy(); // Mark tombstone entity for removal
	}
	tombstones.clear(); // Clear the tombstone vector

	// Reset zombie spawn mechanics
	currentZombieIndex = 0;
	allZombiesTransformed = false;

	int spawnBuffer = 100; // Distance beyond game window for spawning
	for (size_t i = 0; i < wordList.size(); ++i)
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
				x = rand() % 1280;
				y = -spawnBuffer;
				break;
			case 1: // Left
				x = -spawnBuffer;
				y = rand() % 720;
				break;
			case 2: // Right
				x = 1280 + spawnBuffer;
				y = rand() % 720;
				break;
			}

			// Ensure zombie spawn is not too close to player
			auto& playerTransform = player.getComponent<TransformComponent>();
			float dx = playerTransform.position.x - x;
			float dy = playerTransform.position.y - y;
			if (sqrt(dx * dx + dy * dy) < 200.0f) {
				validSpawn = false;
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
	targetText = wordList[currentPromptIndex];

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

	std::cout << "Game reset!" << std::endl;
}
