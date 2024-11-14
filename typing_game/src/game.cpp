#include "game.h"
#include "TextureManager.h"
#include "map.h"
#include "ECS/Components.h"
#include "Vector2D.h"


//#include "GameObject.h"
//#include "ECS.h"
//#include "Components.h"
//GameObject* player;
//GameObject* zombie;
Map* map;
Manager manager;

SDL_Renderer* Game::renderer = nullptr;
SDL_Event Game::event;

auto& player(manager.addEntity());

Game::Game()
{}
Game::~Game()
{}

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

		isRunning = true;
	}

	//player = new GameObject("assets/Player.png", 400, 300);
	//zombie = new GameObject("assets/Zombie.png", 0, 0);
	map = new Map();

	player.addComponent<TransformComponent>();
	player.addComponent<SpriteComponent>("assets/Player.png");
	player.addComponent<KeyboardController>();
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
	//player.getComponent<TransformComponent>().position.Add(Vector2D(5, 0));
	//// player will turn into zombie if they go past x100
	//// for texture filtering
	//if (player.getComponent<TransformComponent>().position.x > 100)
	//{
	//	player.getComponent<SpriteComponent>().setTex("assets/Zombie.png");
	//}
	//std::cout << player.getComponent<PositionComponent>().x() << "," <<
	//	player.getComponent<PositionComponent>().y() << std::endl;
}

void Game::render()
{
	SDL_RenderClear(renderer);
	map->drawMap();
	manager.draw();
	//player->render();
	//zombie->render();
	SDL_RenderPresent(renderer);
}

void Game::clean()
{
	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);
	SDL_Quit();
	std::cout << "Game Cleaned" << std::endl;
}


