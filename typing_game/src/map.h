#pragma once
#include "game.h"

class Map
{
public:
	Map();
	~Map();

	void loadMap(int arr[20][25]);
	void drawMap();

private:

	SDL_Rect src, dest;
	SDL_Texture* wall;
	SDL_Texture* floor;

	int map[20][25];

};