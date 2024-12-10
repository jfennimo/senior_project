#pragma once
#include "Game.h"

class Map
{
public:
	Map();
	~Map();

	//void loadMap(int** arr, int rows, int columns);
	void loadMap(int arr[23][40]);
	void drawMap();

	//int map[23][40];


private:

	SDL_Rect src, dest;
	SDL_Texture* wall;
	SDL_Texture* floor;

	int map[23][40];

};