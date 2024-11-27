#ifndef Game_h
#define Game_h
#include "SDL.h"
#include "SDL_image.h"
#include "UIManager.h"
#include <iostream>

class Game {

public:
	Game();
	~Game();

	void init(const char* title, int width, int height, bool fullscreen);
	
	void handleEvents();
	void update();
	void render();
	void clean();

	bool running() {
		return isRunning;
	}

	static SDL_Renderer *renderer;
	static SDL_Event event;

private:
	bool isRunning = false;
	int cnt = 0;
	SDL_Window* window;
	UIManager* uiManager;
	std::string userInput = ""; // for storing typed text
	bool isZombieTransformed = false; // to prevent multiple transformations!
};

#endif
