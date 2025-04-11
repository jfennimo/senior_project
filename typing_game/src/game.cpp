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

#include "TitleScreen.h"
#include "MainMenu.h"
#include "ArcadeMode.h"
#include "ArcadeResults.h"
#include "BonusTitle.h"
#include "BonusResults.h"
#include "GameOver.h"

GameState gameState;
UIManager* uiManager;
SDL_Renderer* Game::renderer = nullptr;
SDL_Event Game::event;


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

		// For random spawns
		std::srand(static_cast<unsigned int>(std::time(nullptr)));

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

	uiManager = new UIManager(renderer);
	screenWidth = width;
	screenHeight = height;



	titleFont = TTF_OpenFont("assets/PressStart2P.ttf", 30);
	menuFont = TTF_OpenFont("assets/PressStart2P.ttf", 20);
	healthFont = TTF_OpenFont("assets/PressStart2P.ttf", 20);
	roundFont = TTF_OpenFont("assets/PressStart2P.ttf", 16);
	gameOverFont = TTF_OpenFont("assets/PressStart2P.ttf", 100);
	controlPanelFont = TTF_OpenFont("assets/Square.ttf", 30);
	statusFont = TTF_OpenFont("assets/Technology-BoldItalic.TTF", 40);
	threatLvlFont = TTF_OpenFont("assets/Technology-BoldItalic.TTF", 50);
	comboStatusFont = TTF_OpenFont("assets/Technology-BoldItalic.TTF", 30);

	changeState(GameState::TITLE_SCREEN);
}

void Game::changeState(GameState newState)
{
	gameState = newState;

	switch (newState) {
	case GameState::TITLE_SCREEN:
		currentMode = std::make_unique<TitleScreen>(this);
		break;
	case GameState::MAIN_MENU:
		currentMode = std::make_unique<MainMenu>(this);
		break;
	case GameState::ARCADE_MODE: {
		auto arcade = std::make_unique<ArcadeMode>(this);
		if (startArcadeInBonus) {
			arcade->setPhase(ArcadeMode::Phase::BONUS);
			startArcadeInBonus = false; // Reset flag
		}
		currentMode = std::move(arcade);
		break;
	}
	case GameState::ARCADE_RESULTS: {
		auto resultsScreen = std::make_unique<ArcadeResults>(this);
		resultsScreen->setResults(cachedHpResults, cachedWrongResults, cachedAccuracy, cachedLevel);
		currentMode = std::move(resultsScreen);
		break;
	}
	case GameState::BONUS_TITLE:
		currentMode = std::make_unique<BonusTitle>(this);
		break;
	case GameState::BONUS_RESULTS: {
		currentMode = std::make_unique<BonusResults>(this);
		auto bonusResultsScreen = std::make_unique<BonusResults>(this);
		bonusResultsScreen->setResults(cachedBonusHpResults, cachedBonusWrongResults, cachedBonusZombiesDefeated, cachedBonusAccuracy);
		currentMode = std::move(bonusResultsScreen);
		break;
	}
	case GameState::GAME_OVER: {
		currentMode = std::make_unique<GameOver>(this);
		auto gameOverResults = std::make_unique<GameOver>(this);
		gameOverResults->setResults(cachedHighestLevel, cachedZombiesDefeated, cachedOverallAccuracy);
		currentMode = std::move(gameOverResults);
		break;
	}
	default:
		currentMode = nullptr;
		break;
	}
}


void Game::handleEvents()
{
	SDL_PollEvent(&event);
	if (event.type == SDL_QUIT) {
		isRunning = false;
	}
	if (currentMode) currentMode->handleEvent(event);
}


void Game::update() {
	if (currentMode) {
		currentMode->update();
	}
}


void Game::render()
{
	SDL_RenderClear(renderer);
	if (currentMode) currentMode->render();

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

// Wrapper
void Game::nextArcadeLevel() {
	if (auto* arcade = dynamic_cast<ArcadeMode*>(currentMode.get())) {
		arcade->nextLevel();
	}
	else {
		std::cerr << "[Error] Tried to call nextLevel() on non-ArcadeMode\n";
	}
}

// Wrapper
void Game::resetArcadeMode() {
	if (auto* arcade = dynamic_cast<ArcadeMode*>(currentMode.get())) {
		arcade->resetGame();
	}
	else {
		std::cerr << "[Error] Tried to call resetGame() on non-ArcadeMode\n";
	}
}