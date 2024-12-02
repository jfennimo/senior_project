#include "Game.h"
#include "TextureManager.h"
#include "Map.h"
#include "ECS/Components.h"
#include "Vector2D.h"
#include "Collision.h"
#include <vector> // For wordlist and zombie count
#include <cstdlib> // For rand() and srand()
#include <ctime>   // For time()

Map* map;
Manager manager;
UIManager* uiManager;

SDL_Renderer* Game::renderer = nullptr;
SDL_Event Game::event;

auto& player(manager.addEntity());
//auto& zombie(manager.addEntity());
auto& wall1(manager.addEntity());
auto& wall2(manager.addEntity());
auto& wall3(manager.addEntity());

// Wordlist stuff
std::vector<std::string> wordList = { "test", "brains", "yum", "howdy", "yuck"};
size_t currentPromptIndex = 0; // Track the current word prompt
std::string targetText; // Holds the current target prompt

// Zombie entities and active zombie index
std::vector<Entity*> zombies;
size_t currentZombieIndex = 0; // Tracks the currently active zombie


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

    uiManager = new UIManager(renderer);
    map = new Map();

    // Setting player position
    player.addComponent<TransformComponent>(615, 640);
    player.addComponent<SpriteComponent>("assets/Player.png");

    // Walls around player
    wall1.addComponent<TransformComponent>(520.0f, 600.0f, 30, 225, 1);
    wall2.addComponent<TransformComponent>(520.0f, 600.0f, 150, 30, 1);
    wall3.addComponent<TransformComponent>(745.0f, 600.0f, 150, 30, 1);
    wall1.addComponent<SpriteComponent>("assets/Wall.png");
    wall1.addComponent<ColliderComponent>("wall");
    wall2.addComponent<SpriteComponent>("assets/Wall.png");
    wall2.addComponent<ColliderComponent>("wall");
    wall3.addComponent<SpriteComponent>("assets/Wall.png");
    wall3.addComponent<ColliderComponent>("wall");

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

            // Ensure the spawn is not too close to the player
            auto& playerTransform = player.getComponent<TransformComponent>();
            float dx = playerTransform.position.x - x;
            float dy = playerTransform.position.y - y;
            if (sqrt(dx * dx + dy * dy) < 200.0f) { // Adjust the threshold as needed
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
}


void Game::handleEvents()
{
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
}


void Game::update() {
    manager.refresh();
    manager.update();

    auto& playerTransform = player.getComponent<TransformComponent>();

    // Iterate through all zombies in order of the word list
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

            float speed = 1.0f; // Adjust speed as necessary
            zombieTransform.position.x += dx * speed;
            zombieTransform.position.y += dy * speed;

            // Check for wall collisions
            if (Collision::AABB(zombie->getComponent<ColliderComponent>().collider,
                wall1.getComponent<ColliderComponent>().collider) ||
                Collision::AABB(zombie->getComponent<ColliderComponent>().collider,
                    wall2.getComponent<ColliderComponent>().collider) ||
                Collision::AABB(zombie->getComponent<ColliderComponent>().collider,
                    wall3.getComponent<ColliderComponent>().collider)) {
                zombieTransform.position.x -= dx * speed;
                zombieTransform.position.y -= dy * speed;

                // Wall hit detected
                std::cout << "Wall hit!" << std::endl;
            }
        }

        // Check if this zombie's prompt matches the user input
        if (userInput == wordList[i] && !transformStatus.getTransformed()) {
            zombie->getComponent<SpriteComponent>().setTex("assets/Tombstone.png");
            transformStatus.setTransformed(true);

            // Move to the next target word if applicable
            if (i == currentZombieIndex) {
                currentZombieIndex++;
                if (currentZombieIndex < zombies.size()) {
                    targetText = wordList[currentZombieIndex];
                }
            }

            userInput.clear();
        }
    }
}


void Game::render()
{
    SDL_RenderClear(renderer);

    // Draw map and game objects
    map->drawMap();
    manager.draw();

    if (currentZombieIndex < zombies.size()) {
        Entity* activeZombie = zombies[currentZombieIndex];
        auto& zombieTransform = activeZombie->getComponent<TransformComponent>();

        int textX = static_cast<int>(zombieTransform.position.x - 60); // Zombie's x position
        int textY = static_cast<int>(zombieTransform.position.y - 20); // Slightly above the zombie

        if (uiManager) {
            SDL_Color rectColor = { 153, 255, 153, 255 };
            uiManager->drawRectangle(textX - 10, textY - 5, 200, 25, rectColor);

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

    SDL_RenderPresent(renderer);
}


void Game::clean()
{
	delete uiManager;
	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);
	TTF_Quit();
	SDL_Quit();
	std::cout << "Game Cleaned" << std::endl;
}


