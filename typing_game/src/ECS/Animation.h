#pragma once

struct Animation
{
	int index;
	int frames;
	int speed;
	bool loop;

	Animation() = default;
	Animation(int i, int f, int s, bool l = true)
	{
		index = i;
		frames = f;
		speed = s;
		loop = l;
	}
};