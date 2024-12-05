#include "Game.h"
#include "TextureManager.h"
#include "Map.h"
#include "ECS/Components.h"
#include "Vector2D.h"
#include "Collision.h"
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
auto& barrier1(manager.addEntity());
auto& barrier2(manager.addEntity());
auto& barrier3(manager.addEntity());
auto& crosshair(manager.addEntity());

// Health bar / HP
int barrierHP;
const int maxHP = 500;

// Fonts
TTF_Font* titleFont;
TTF_Font* healthFont;

// Wordlist stuff
std::vector<std::string> wordList = { "test", "brains", "yum", "howdy", "yuck" };
size_t currentPromptIndex = 0; // Track the current word prompt
std::string targetText; // Holds the current target prompt

// Zombie entities and active zombie index
std::vector<Entity*> zombies;
size_t currentZombieIndex = 0; // Tracks the currently active zombie

//TTF_Font* titleFont = TTF_OpenFont("assets/PressStart2P.ttf", 30);

bool allZombiesTransformed = false;


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

    gameState = GameState::ARCADE_MODE; // Initial state

    uiManager = new UIManager(renderer);
    map = new Map();

    // Setting player position
    player.addComponent<TransformComponent>(615, 640);
    player.addComponent<SpriteComponent>("assets/Player.png");

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

    // Initialize the crosshair entity only once with its components
    crosshair.addComponent<TransformComponent>(0, 0); // Initial position of the crosshair (0, 0)
    crosshair.addComponent<SpriteComponent>("assets/Crosshair.png");

    // Initialize random seed
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    // Spawn zombies at random off-screen positions, ensuring they are not too close to the player
    int spawnBuffer = 100; // Distance beyond the game window for spawning
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

            // Ensure zombie spawn is not too close to the player
            auto& playerTransform = player.getComponent<TransformComponent>();
            float dx = playerTransform.position.x - x;
            float dy = playerTransform.position.y - y;
            if (sqrt(dx * dx + dy * dy) < 200.0f) { // Adjust threshold as needed
                validSpawn = false;
            }
        }

        newZombie->addComponent<TransformComponent>(x, y);
        newZombie->addComponent<SpriteComponent>("assets/Zombie.png");
        newZombie->addComponent<ColliderComponent>("zombie");
        newZombie->addComponent<TransformStatusComponent>(); // Add transformation status
        zombies.push_back(newZombie);

    }

    // Initialize the first target prompt
    targetText = wordList[currentPromptIndex];

    // Intialize barrier health and health font
    barrierHP = maxHP;

    titleFont = TTF_OpenFont("assets/PressStart2P.ttf", 30);
    //TTF_Font* healthFont = TTF_OpenFont("assets/PressStart2P.ttf", 16);
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
                gameState = GameState::MAIN_MENU; // Transition from title screen
                std::cout << "Navigating to main menu!" << std::endl;
            }
            else if (gameState == GameState::MAIN_MENU) {
                gameState = GameState::ARCADE_MODE; // Transition to arcade mode
                std::cout << "Navigating to arcade mode!" << std::endl;
            }
        }
        if (gameState == GameState::ARCADE_MODE) {
            if (event.key.keysym.sym == SDLK_BACKSPACE && !userInput.empty()) {
                userInput.pop_back(); // Remove last character in ARCADE_MODE
            }
        }
        
        break;
    case SDL_TEXTINPUT:
        if (gameState == GameState::ARCADE_MODE) {
            userInput += event.text.text; // Append typed text only in ARCADE_MODE
        }
        break;
    
    default:
        break;
    }
    /*
	SDL_PollEvent(&event);
	switch (event.type)
	{
	case SDL_QUIT:
		isRunning = false;
		break;
	case SDL_TEXTINPUT: // Capture text input
		userInput += event.text.text; // Append typed text
		break;
	case SDL_KEYDOWN:
		if (event.key.keysym.sym == SDLK_BACKSPACE && !userInput.empty()) {
			userInput.pop_back(); // Remove last character on backspace
		}
		break;
	default:
		break;
	}
    */
}


void Game::update() {
    manager.refresh();
    manager.update();

    auto& playerTransform = player.getComponent<TransformComponent>();

    switch (gameState) {
    case GameState::TITLE_SCREEN:
        // Title screen logic
        //if (Game::event.type == SDL_KEYDOWN) {


        //    // Transition to main menu on any key press
        //std::cout << "Title screen updated!" << std::endl;
        //    gameState = GameState::MAIN_MENU;
        //}
        break;

    case GameState::MAIN_MENU:
        // Main menu logic
        //if (Game::event.type == SDL_KEYDOWN) {


        //    // Transition to arade mode on any key press
        //std::cout << "Main menu updated!" << std::endl;
        //    gameState = GameState::ARCADE_MODE;
        //}
        break;


    case GameState::ARCADE_MODE:
        // Game logic

        // Update crosshair position if zombies are present
        if (!zombies.empty() && currentZombieIndex < zombies.size()) {
            Entity* activeZombie = zombies[currentZombieIndex];
            auto& zombieTransform = activeZombie->getComponent<TransformComponent>();

            // Update the crosshair's position to the zombie's position
            auto& crosshairTransform = crosshair.getComponent<TransformComponent>();
            crosshairTransform.position = zombieTransform.position;
        }

        // Iterate through all zombies
        for (size_t i = 0; i < zombies.size(); ++i) {
            Entity* zombie = zombies[i];
            auto& zombieTransform = zombie->getComponent<TransformComponent>();
            auto& transformStatus = zombie->getComponent<TransformStatusComponent>();

            // Check if the zombie is transformed
            if (!transformStatus.getTransformed()) {
                // Move the zombie toward the player
                float dx = playerTransform.position.x - zombieTransform.position.x;
                float dy = playerTransform.position.y - zombieTransform.position.y;

                float magnitude = sqrt(dx * dx + dy * dy);
                if (magnitude > 0) {
                    dx /= magnitude;
                    dy /= magnitude;
                }

                float speed = 0.5f; // May need to adjust this for difficulty
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

                    // Lower hp if zombie touches barrier
                    barrierHP--;
                    if (barrierHP < 0) barrierHP = 0;

                    // Wall hit detected
                    std::cout << "Barrier hit! HP: " << barrierHP << std::endl;
                }
            }

            // Check if this zombie's prompt matches the user input
            if (userInput == wordList[i] && !transformStatus.getTransformed()) {
                zombie->getComponent<SpriteComponent>().setTex("assets/Tombstone.png");
                transformStatus.setTransformed(true);

                // Move to the next closest zombie
                if (i == currentZombieIndex) {
                    // Find the closest remaining zombie
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

                    // Update the current zombie to the closest one
                    currentZombieIndex = closestZombieIndex;
                    targetText = wordList[currentZombieIndex];
                }

                userInput.clear();
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

        if (allZombiesTransformed) {
            gameState = GameState::RESULTS; // Transition to results state
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
        //TTF_Font* titleFont = TTF_OpenFont("assets/PressStart2P.ttf", 30);
        if (!titleFont) {
            std::cout << "Failed to load font: " << TTF_GetError() << std::endl;
            return;
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        uiManager->drawText("Letter RIP", 520, 360, { 255, 255, 255, 255 }, titleFont);

        //SDL_RenderPresent(game->renderer);
        //std::cout << "Title screen rendered!" << std::endl;
        break;

    case GameState::MAIN_MENU:
        // Draw main menu
        //TTF_Font* titleFont = TTF_OpenFont("assets/PressStart2P.ttf", 30);
        if (!titleFont) {
            std::cout << "Failed to load font: " << TTF_GetError() << std::endl;
            return;
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        uiManager->drawText("Main menu!", 520, 360, { 255, 255, 255, 255 }, titleFont);

        //SDL_RenderPresent(game->renderer);
        //std::cout << "Main menu rendered!" << std::endl;
        break;

    case GameState::ARCADE_MODE:
        // Draw game
        // Draw map and game objects
        map->drawMap();
        manager.draw();

        healthFont = TTF_OpenFont("assets/PressStart2P.ttf", 20);
        // Draw HP bar
        if (uiManager && healthFont) {
            SDL_Color outlineColor = { 255, 255, 255, 255 };
            SDL_Color fgColor = { 102, 255, 105, 255 };
            SDL_Color bgColor = { 255, 102, 102, 255 };
            SDL_Color textColor = { 255, 255, 51, 255 };
            uiManager->drawHealthbar(40, 30, 200, 25, barrierHP, maxHP, outlineColor, fgColor, bgColor, "Barrier HP", healthFont, textColor);
        }

        // Render the crosshair
        if (!zombies.empty() && currentZombieIndex < zombies.size()) {
            Entity* activeZombie = zombies[currentZombieIndex];
            auto& zombieTransform = activeZombie->getComponent<TransformComponent>();

            // Place crosshair on top of the current zombie
            auto& crosshairTransform = crosshair.getComponent<TransformComponent>();
            crosshairTransform.position = zombieTransform.position;

            // Draw the crosshair sprite
            crosshair.getComponent<SpriteComponent>().draw();
        }

        // Check if all zombies have been defeated (transformed)
        allZombiesTransformed = true;
        for (size_t i = 0; i < zombies.size(); ++i) {
            if (!zombies[i]->getComponent<TransformStatusComponent>().getTransformed()) {
                allZombiesTransformed = false;
                break;
            }
        }

        if (!allZombiesTransformed && currentZombieIndex < zombies.size()) {
            // Only render the prompt if there are still zombies to be defeated
            Entity* activeZombie = zombies[currentZombieIndex];
            auto& zombieTransform = activeZombie->getComponent<TransformComponent>();

            int textX = static_cast<int>(zombieTransform.position.x + 5); // Zombie's x position
            int textY = static_cast<int>(zombieTransform.position.y - 20); // Slightly above the zombie

            if (uiManager) {
                SDL_Color rectColor = { 255, 178, 102, 255 };
                uiManager->drawRectangle(textX - 20, textY - 5, 125, 25, rectColor);

                TTF_Font* font = TTF_OpenFont("assets/PressStart2P.ttf", 16);
                if (font) {
                    int letterX = textX;
                    for (size_t i = 0; i < targetText.size(); ++i) {
                        SDL_Color color = { 255, 255, 255, 255 };
                        if (i < userInput.size() && userInput[i] == targetText[i]) {
                            color = { 255, 0, 0, 255 };
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

        break;

    case GameState::RESULTS:
        // Draw results screen

        break;


    default:
        break;
    }

    SDL_RenderPresent(renderer);
}

void Game::clean()
{
	delete uiManager;
	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);
    //if (healthFont) {
    //    TTF_CloseFont(healthFont);
    //    healthFont = nullptr;
    //}
	TTF_Quit();
	SDL_Quit();
	std::cout << "Game Cleaned" << std::endl;
}


