#pragma once
#include "Game.h"

enum class MapLevel {
	EASY,
	MEDIUM,
	HARD
};

class Map
{
public:
	Map();
	~Map();

	//void loadMap(int** arr, int rows, int columns);
	void loadMap(int arr[24][50]);
	void drawMap(int offsetX = 0, int offsetY = 0);

	void setDifficulty(MapLevel currentDifficulty);

	//int map[23][40];


private:
	MapLevel currentDifficulty = MapLevel::EASY;

	SDL_Rect src, dest;
	SDL_Texture* wall;
	SDL_Texture* floorEasy;
	SDL_Texture* floorMedium;
	SDL_Texture* floorHard;
	SDL_Texture* floorCaution;

	int map[24][50];

};