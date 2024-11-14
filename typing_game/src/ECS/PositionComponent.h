#pragma once
#include "Components.h"

class PositionComponent : public Component
{
private:
	int xpos;
	int ypos;

public:

	PositionComponent()
	{
		xpos = 0;
		ypos = 0;
	}

	PositionComponent(int x, int y)
	{
		xpos = x;
		ypos = y;
	}

	void update() override
	{
		xpos++;
		ypos++;
	}

	int x() const { return xpos; }
	void x(int x) { xpos = x; }
	int y() const { return ypos; }
	void y(int y) { ypos = y; }

	void setPos(int x, int y)
	{
		xpos = x;
		ypos = y;
	}
};