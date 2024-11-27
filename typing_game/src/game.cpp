#include "Game.h"
#include "TextureManager.h"
#include "Map.h"
#include "ECS/Components.h"
#include "Vector2D.h"
#include "Collision.h"

//#include "GameObject.h"
//#include "ECS.h"
//#include "Components.h"
//GameObject* player;
//GameObject* zombie;
Map* map;
Manager manager;
UIManager* uiManager;

SDL_Renderer* Game::renderer = nullptr;
SDL_Event Game::event;

auto& player(manager.addEntity());
auto& zombie(manager.addEntity());
auto& wall(manager.addEntity());

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
	} else {
		std::cout << "SDL Initialization Failed!" << std::endl;
		isRunning = false;
	}


	uiManager = new UIManager(renderer);

	//player = new GameObject("assets/Player.png", 400, 300);
	//zombie = new GameObject("assets/Zombie.png", 0, 0);
	map = new Map();

	// Setting player position
	player.addComponent<TransformComponent>(350, 550);
	player.addComponent<SpriteComponent>("assets/Player.png");
	//player.addComponent<KeyboardController>();

	// Setting zombie position
	zombie.addComponent<TransformComponent>(350, 100);
	zombie.addComponent<SpriteComponent>("assets/Zombie.png");
	zombie.addComponent<ColliderComponent>("zombie");

	// Wall around player
	wall.addComponent<TransformComponent>(00.0f, 500.0f, 30, 800, 1);
	wall.addComponent<SpriteComponent>("assets/Wall.png");
	wall.addComponent<ColliderComponent>("wall");
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
			else if (event.key.keysym.sym == SDLK_RETURN) { // Check for Enter key
				if (userInput == "hello world" && !isZombieTransformed) {
					zombie.getComponent<SpriteComponent>().setTex("assets/Tombstone.png");
					isZombieTransformed = true; // Prevent further changes
				}
			}
			break;
		default:
			break;
	}
}

void Game::update()
{
	//player->update();
	//zombie->update();
	manager.refresh();
	manager.update();

	// Referencing zombie's transform component
	auto& zombieTransform = zombie.getComponent<TransformComponent>();

	// Only move zombie if it has not transformed
	if (!isZombieTransformed) {
		// Move zombie down
		Vector2D movement(0, 1); // Move downwards
		zombieTransform.position.Add(movement);

		// Checking for wall collision
		if (Collision::AABB(zombie.getComponent<ColliderComponent>().collider,
			wall.getComponent<ColliderComponent>().collider))
		{
			zombieTransform.position.Subtract(movement); // Reverse (stop) the movement
			std::cout << "Wall Hit!" << std::endl;
		}
	}
}

void Game::render()
{
	SDL_RenderClear(renderer);

	// Draw map and game objects
	map->drawMap();
	manager.draw();

	// Access zombie's position
	auto& zombieTransform = zombie.getComponent<TransformComponent>();
	int textX = static_cast<int>(zombieTransform.position.x - 60); // Zombie's x position
	int textY = static_cast<int>(zombieTransform.position.y - 20); // Slightly above the zombie

	// Render the prompt bubble and text
	if (uiManager) {
		SDL_Color rectColor = { 153, 255, 153, 255 };
		uiManager->drawRectangle(textX - 10, textY - 5, 200, 25, rectColor);

		TTF_Font* font = TTF_OpenFont("assets/PressStart2P.ttf", 16);
		if (font) {
			std::string targetText = "hello world";

			int letterX = textX; // Starting position for letters
			for (size_t i = 0; i < targetText.size(); ++i) {
				// Determine color based on what the user has typed
				SDL_Color color = { 255, 255, 255, 255 }; // Default: white
				if (i < userInput.size() && userInput[i] == targetText[i]) {
					color = { 255, 0, 0, 255 }; // Correctly typed: red
				}

				// Render each letter individually
				std::string letter(1, targetText[i]); // Convert char to string
				SDL_Surface* surface = TTF_RenderText_Solid(font, letter.c_str(), color);
				if (surface) {
					SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
					if (texture) {
						SDL_Rect dst = { letterX, textY, surface->w, surface->h };
						SDL_RenderCopy(renderer, texture, nullptr, &dst);
						letterX += surface->w; // Move to the right for the next letter
						SDL_DestroyTexture(texture);
					}
					SDL_FreeSurface(surface);
				}
			}
			TTF_CloseFont(font);
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


