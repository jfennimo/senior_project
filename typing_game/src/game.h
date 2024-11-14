#ifndef game_h
#define game_h
#include "SDL.h"
#include "SDL_image.h"
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

private:
	bool isRunning = false;
	int cnt = 0;
	SDL_Window* window;
};

#endif
