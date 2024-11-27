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
	player.addComponent<KeyboardController>();
	player.addComponent<ColliderComponent>("player");

	// Setting zombie position
	zombie.addComponent<TransformComponent>(350, 300);
	zombie.addComponent<SpriteComponent>("assets/Zombie.png");

	// Placing wall into area for testing
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

	// Move zombie
	//zombie.getComponent<TransformComponent>().position.Add(Vector2D(2, 0));
	//// zombie will turn into tombstone if they go past x300
	//// for texture filtering
	//if (zombie.getComponent<TransformComponent>().position.x > 300)
	//{
	//	zombie.getComponent<SpriteComponent>().setTex("assets/Tombstone.png");
	//}
	//std::cout << player.getComponent<PositionComponent>().x() << "," <<
	//	player.getComponent<PositionComponent>().y() << std::endl;

	if (Collision::AABB(player.getComponent<ColliderComponent>().collider,
		wall.getComponent<ColliderComponent>().collider))
	{
		//player.getComponent<TransformComponent>().scale = 1;
		player.getComponent<TransformComponent>().velocity * -1;
		std::cout << "Wall Hit!" << std::endl;
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
		TTF_Font* font = TTF_OpenFont("assets/PressStart2P.ttf", 16); // Smaller font size
		if (font) {
			SDL_Color color = { 255, 255, 255, 255 }; // White color
			uiManager->drawText("hello world", textX, textY, color, font);
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


