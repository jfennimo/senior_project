#include "game.h"
#include "TextureManager.h"
#include "GameObject.h"

GameObject* zombie;

Game::Game()
{}
Game::~Game()
{}

void Game::init(const char* title, int xpos, int ypos, int width, int height, bool fullscreen)
{
	int flags = 0;

	if (fullscreen)
	{
		flags = SDL_WINDOW_FULLSCREEN;
	}

	if (SDL_Init(SDL_INIT_EVERYTHING) == 0)
	{
		std::cout << "Subsystems Intialized..." << std::endl;

		window = SDL_CreateWindow(title, xpos, ypos, width, height, flags);
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
	} else {
		isRunning = false;
	}

	// testing...
	//SDL_Surface* tmpSurface = IMG_Load("Assets/Zombie.png");

	//zombieTex = SDL_CreateTextureFromSurface(renderer, tmpSurface);
	//SDL_FreeSurface(tmpSurface);

	/*zombieTex = TextureManager::LoadTexture("assets/zombie.png", renderer);*/

	zombie = new GameObject("assets/Zombie.png", renderer, 0, 0);
}

void Game::handleEvents()
{
	SDL_Event event;
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
	// Testing to get this right
	//cnt++;
	//destR.h = 128;
	//destR.w = 128;
	//destR.x = cnt;
	////destR.x = 350;
	////destR.y = 120;

	//std::cout << cnt << std::endl;

	zombie->update();
}

void Game::render()
{
	SDL_RenderClear(renderer);
	// here ya add stuff to render
	zombie->render();
	//SDL_RenderCopy(renderer, zombieTex, NULL, &destR);
	SDL_RenderPresent(renderer);
}

void Game::clean()
{
	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);
	SDL_Quit();
	std::cout << "Game Cleaned" << std::endl;
}


