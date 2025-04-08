#pragma once
#include "Components.h"
#include "SDL.h"
#include "../TextureManager.h"
#include "Animation.h"
#include <map>

class SpriteComponent : public Component
{
private:
	TransformComponent *transform;
	SDL_Texture *texture;
	SDL_Rect srcRect, destRect;
	Uint32 animationStartTime = 0;

	bool animated = false;
	int frames = 0;
	int speed = 100;
	bool loop = true;
	int currentFrame = 0;

public:

	int animIndex = 0;

	std::map<const char*, Animation> animations;

	SpriteComponent() = default;
	SpriteComponent(const char* path)
	{
		setTex(path);
	}

	SpriteComponent(const char* path, bool isAnimated)
	{
		animated = isAnimated;

		// Zombie animations
		Animation walkDown = Animation(0, 4, 100);
		Animation attackDown = Animation(1, 4, 100);
		Animation walkRight = Animation(2, 4, 100);
		Animation attackRight = Animation(3, 4, 100);
		Animation walkLeft = Animation(4, 4, 100);
		Animation attackLeft = Animation(5, 4, 100);
		Animation defeat = Animation(6, 19, 100, false);

		animations.emplace("Walk Down", walkDown);
		animations.emplace("Attack Down", attackDown);
		animations.emplace("Walk Right", walkRight);
		animations.emplace("Attack Right", attackRight);
		animations.emplace("Walk Left", walkLeft);
		animations.emplace("Attack Left", attackLeft);
		animations.emplace("Defeat", defeat);

		// Laser
		Animation laser = Animation(0, 3, 100);
		animations.emplace("Laser", laser);

		Play("Walk Down");

		//frames = nFrames;
		//speed = mSpeed;
		setTex(path);
	}

	// Deconstructor
	~SpriteComponent()
	{
		SDL_DestroyTexture(texture);
	}

	void setFrames(int framess) {
		frames = framess;
	}

	void setTex(const char* path)
	{
		texture = TextureManager::LoadTexture(path);
	}

	void init() override
	{
		transform = &entity->getComponent<TransformComponent>();

		srcRect.x = srcRect.y = 0;
		srcRect.w = transform->width;
		srcRect.h = transform->height;
	}

	void update() override
	{
		if (animated) {
			if (loop) {
				// Looping animation: use total time
				currentFrame = static_cast<int>((SDL_GetTicks() / speed) % frames);
			}
			else {
				// Non-looping: use time since animation started
				Uint32 elapsedTime = SDL_GetTicks() - animationStartTime;
				int frameIndex = elapsedTime / speed;
				currentFrame = std::min(frameIndex, frames - 1);
			}

			srcRect.x = srcRect.w * currentFrame;
		}

		srcRect.y = animIndex * transform->height;

		destRect.x = static_cast<int>(transform->position.x);
		destRect.y = static_cast<int>(transform->position.y);
		destRect.w = transform->width * transform->scale;
		destRect.h = transform->height * transform->scale;
	}

	void draw() override
	{
		TextureManager::Draw(texture, srcRect, destRect);
	}

	// New render function for custom position rendering
	void render(int x, int y)
	{
		destRect.x = x;
		destRect.y = y;
		TextureManager::Draw(texture, srcRect, destRect);
	}

	void Play(const char* animName)
	{
		const Animation& anim = animations[animName];
		frames = anim.frames;
		animIndex = anim.index;
		speed = anim.speed;
		loop = anim.loop;
		currentFrame = 0;

		animationStartTime = SDL_GetTicks();
	}

};

class TransformStatusComponent : public Component {
public:
	bool isTransformed = false; // Default to false

	void setTransformed(bool status) {
		isTransformed = status;
	}
	bool getTransformed() const {
		return isTransformed;
	}
};