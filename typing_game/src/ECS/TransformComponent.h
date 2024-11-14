#pragma once
#include "Components.h"
#include "../Vector2D.h"

class TransformComponent : public Component
{
public:

	Vector2D position;

	TransformComponent()
	{
		position.x = 0.0f;
		position.y = 0.0f;
	}

	TransformComponent(float x, float y)
	{
		position.x = 0;
		position.y = 0;
	}

	TransformComponent(int x, int y)
	{
		position.x = x;
		position.y = y;
	}

	void update() override
	{

	}

	//int x() const { return xpos; }
	//void x(int x) { xpos = x; }
	//int y() const { return ypos; }
	//void y(int y) { ypos = y; }

	//void setPos(int x, int y)
	//{
	//	xpos = x;
	//	ypos = y;
	//}
};