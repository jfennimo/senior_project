#include "ArcadeMode.h"
#include "ArcadeResults.h"
#include "Game.h"

//Manager manager;
//auto& player(manager.addEntity());
//auto& barrier(manager.addEntity());
//auto& leftHand(manager.addEntity());
//auto& rightHand(manager.addEntity());
//auto& barrier1(manager.addEntity());
//auto& barrier2(manager.addEntity());
//auto& barrier3(manager.addEntity());
//auto& crosshair(manager.addEntity());
//auto& laserMiddle(manager.addEntity());
//auto& laserLeft(manager.addEntity());
//auto& laserRight(manager.addEntity());
//auto& comboMeter(manager.addEntity());

ArcadeMode::ArcadeMode(Game* game)
	: game(game),
	lastBlinkTime(SDL_GetTicks()),
	showBlinkText(true)
{
	if (game->arcadeJustStarted) {
		resetGame();
		game->arcadeJustStarted = false;
	}

	if (game->nextLevelToStart) {
		nextLevel();
		game->nextLevelToStart = false;
	}
}

void ArcadeMode::update() {
	// Refresh ECS
	manager.update();
	manager.refresh();

    switch (currentPhase) {
    case Phase::ARCADE:
        updateArcadeStage();
        break;
    case Phase::BONUS:
        updateBonusStage();
        break;
	default:
		break;
    }
}

void ArcadeMode::render() {
    switch (currentPhase) {
    case Phase::ARCADE:
        renderArcadeStage();
        break;
    case Phase::BONUS:
        renderBonusStage();
        break;
    default:
        break;
    }
}

void ArcadeMode::handleEvent(SDL_Event& event) {
    switch (currentPhase) {
    case Phase::ARCADE:
        handleArcadeEvent(event);
        break;
    case Phase::BONUS:
        handleBonusEvent(event);
        break;
    default:
        break;
    }
}

// Update methods
void ArcadeMode::updateArcadeStage() {
    // TODO: arcade mode logic
	// Arcade mode logic
	//if (exclamationShouldBeDestroyed) {
	//	manager.refresh();
	//	exclamation = nullptr;
	//	exclamationShouldBeDestroyed = false;
	//}

	//manager.refresh();
	//manager.update();

	// Find player position
	auto& playerTransform = player->getComponent<TransformComponent>();

	Uint32 currentTime = SDL_GetTicks(); // Get current time in milliseconds

	// Reset brokenCombo at the start of a new word
	if (userInput.empty()) {
		brokenCombo = false;
	}

	// Check if word is fully typed and wrong, for status alert
	if (userInput.size() == targetText.size() && userInput != targetText) {
		wordTypedWrong = true;
	}
	else {
		wordTypedWrong = false;
	}

	// Update status text
	if (barrierUnderAttack) {
		statusText = "DANGER";
		if (laserReady) {
			statusText = "LASER READY";
		}
	}
	else {
		if (wordTypedWrong) {
			statusText = "ERROR";
		}
		else if (laserReady) {
			statusText = "LASER READY";
		}
		else if (barrierHP <= 50) {
			statusText = "CAUTION";
		}
		else if (barrierHP <= 20) {
			statusText = "CRITICAL";
		}
		else {
			statusText = "OK";
		}
	}

	// Screen shake logic, for when zombies attack barrier
	if (shakeDuration > 0) {
		shakeOffsetX = (std::rand() % (shakeMagnitude * 2)) - shakeMagnitude;
		shakeOffsetY = (std::rand() % (shakeMagnitude * 2)) - shakeMagnitude;
		shakeDuration--;
	}
	else {
		shakeOffsetX = 0;
		shakeOffsetY = 0;
	}

	// Basic laser logic (as in, laser that shoots when a prompt is typed correctly)
	for (auto& laser : activeLasers) {
		laser.duration--;
	}

	activeLasers.erase(
		std::remove_if(activeLasers.begin(), activeLasers.end(),
			[](const LaserStrike& l) { return l.duration <= 0; }),
		activeLasers.end());


	// Update crosshair position if zombies are present
	if (!zombies.empty() && currentZombieIndex < zombies.size()) {
		Entity* activeZombie = zombies[currentZombieIndex];
		auto& zombieTransform = activeZombie->getComponent<TransformComponent>();

		// Update crosshair's position to zombie's position
		auto& crosshairTransform = crosshair->getComponent<TransformComponent>();
		crosshairTransform.position = zombieTransform.position;
	}

	// Update hand sprites to reflect the key needed to be pressed
	updateHandSprites();

	// To update barrier attack status
	barrierUnderAttack = false;


	player->getComponent<TransformComponent>();

	// Iterate through all zombies
	for (size_t i = 0; i < zombies.size(); ++i) {
		Entity* zombie = zombies[i];
		auto& zombieTransform = zombie->getComponent<TransformComponent>();
		auto& transformStatus = zombie->getComponent<TransformStatusComponent>();

		// Update stun status and timer
		transformStatus.updateStun();

		// If stunned and not transformed, play stun animation (but still allow prompt logic)
		if (transformStatus.isStunned() && !transformStatus.getTransformed()) {
			zombie->getComponent<SpriteComponent>().Play("Stun");
		}

		// Check if zombie is transformed
		if (!transformStatus.getTransformed() && !transformStatus.isStunned()) {
			// Move zombie toward the player if not stunned
			float dx = playerTransform.position.x - zombieTransform.position.x;
			float dy = playerTransform.position.y - zombieTransform.position.y;

			float magnitude = sqrt(dx * dx + dy * dy);
			if (magnitude > 0) {
				dx /= magnitude;
				dy /= magnitude;
			}

			zombieTransform.position.x += dx * speed;
			zombieTransform.position.y += dy * speed;

			// Directional animation
			auto& sprite = zombie->getComponent<SpriteComponent>();

			if (std::abs(dx) > std::abs(dy)) {
				if (dx > 0) {
					sprite.Play("Walk Right");
				}
				else {
					sprite.Play("Walk Left");
				}
			}
			else {
				if (dy > 0) {
					sprite.Play("Walk Down");
				}
			}

			// Check for wall collisions
			if (Collision::AABB(zombie->getComponent<ColliderComponent>().collider,
				barrier->getComponent<ColliderComponent>().collider)) {
				zombieTransform.position.x -= dx * speed;
				zombieTransform.position.y -= dy * speed;

				// Wall collision is true
				barrierUnderAttack = true; // Track if zombies are attacking

				// Attacking animation
				auto& sprite = zombie->getComponent<SpriteComponent>();

				if (std::abs(dx) > std::abs(dy)) {
					if (dx > 0) {
						sprite.Play("Attack Right");
					}
					else {
						sprite.Play("Attack Left");
					}
				}
				else {
					if (dy > 0) {
						sprite.Play("Attack Down");
					}
				}

				// Wall hit detected
				std::cout << "Barrier hit! HP: " << barrierHP << std::endl;
			}
		}

		// Lower HP by 10 every second the zombies are attacking the barrier
		if (barrierUnderAttack && currentTime - lastAttackTime >= 1000) {
			barrierHP -= 10;
			lastAttackTime = currentTime;

			updateBarrierDamage(barrierHP);

			// Trigger screen shake!
			shakeDuration = 3;    // frames to shake
			shakeMagnitude = 3;    // how far to shake

			if (barrierHP < 0) {
				barrierHP = 0;

				updateBarrierDamage(barrierHP); // Updates barrier sprite based on amount of damage taken
			}
		}

		// Check if zombie's prompt matches user input
		if (i == currentZombieIndex && userInput == words[i] && !transformStatus.getTransformed()) {
			for (size_t j = 0; j < userInput.size(); ++j) {
				if (j >= targetText.size() || userInput[j] != targetText[j]) {
					// Append to typedWrong only if not already processed
					if (std::find(typedWrong.begin(), typedWrong.end(), userInput[j]) == typedWrong.end()) {
						typedWrong.push_back(targetText[j]);
					}
				}
			}

			// Check if user types in prompt correctly without errors, to update combo
			checkCombo(userInput, targetText);

			// Basic laser animation for eliminating zombie
			int cannonX = laserMiddle->getComponent<TransformComponent>().position.x + 68; // center of 68px cannon
			int cannonY = laserMiddle->getComponent<TransformComponent>().position.y + 128; // bottom of cannon

			int zombieX = zombie->getComponent<TransformComponent>().position.x + 32;
			int zombieY = zombie->getComponent<TransformComponent>().position.y + 32;

			LaserStrike laser;
			laser.startX = cannonX;
			laser.startY = cannonY;
			laser.endX = zombieX;
			laser.endY = zombieY;
			laser.duration = 6;

			activeLasers.push_back(laser);

			// Transform zombie and play defeat animation
			auto& sprite = zombie->getComponent<SpriteComponent>();
			sprite.Play("Defeat");

			// Update transformation status and account for how many zombies are inactive
			transformStatus.setTransformed(true);
			tombstones.push_back(zombie);

			// Update zombie count (for threat level) / zombies defeated
			zombieCount--;
			zombiesDefeated++;

			// Clear user input
			userInput.clear();

			// Resetting hand sprites
			resetHandSprites();

			// Move to next closest zombie
			if (i == currentZombieIndex) {
				// Find closest remaining zombie
				float closestDistance = std::numeric_limits<float>::max();
				size_t closestZombieIndex = currentZombieIndex;

				for (size_t j = 0; j < zombies.size(); ++j) {
					if (!zombies[j]->getComponent<TransformStatusComponent>().getTransformed()) {
						auto& targetZombieTransform = zombies[j]->getComponent<TransformComponent>();
						float dx = playerTransform.position.x - targetZombieTransform.position.x;
						float dy = playerTransform.position.y - targetZombieTransform.position.y;
						float distance = sqrt(dx * dx + dy * dy);

						if (distance < closestDistance) {
							closestDistance = distance;
							closestZombieIndex = j;
						}
					}
				}

				// Update current zombie to the closest one
				currentZombieIndex = closestZombieIndex;
				targetText = words[currentZombieIndex];
			}
		}
	}
	// Check if all zombies are transformed
	allZombiesTransformed = true; // Assume all are defeated
	for (auto* zombie : zombies) {
		if (!zombie->getComponent<TransformStatusComponent>().getTransformed()) {
			allZombiesTransformed = false;
			break;
		}
	}

	// Laser power-up logic
	if (laserActive) {
		auto& laserTransform = laserPowerUp->getComponent<TransformComponent>();
		auto& laserCollider = laserPowerUp->getComponent<ColliderComponent>();

		// Move laser down!
		laserTransform.position.y += laserSpeed;

		// Check collision with zombies
		for (auto* zombie : zombies) {
			if (!zombie->getComponent<TransformStatusComponent>().getTransformed()) {
				if (Collision::AABB(zombie->getComponent<ColliderComponent>().collider, laserCollider.collider)) {

					// Get zapped, zambie! (stuns the zombie, making it unable to move or attack for 5 seconds)
					auto& status = zombie->getComponent<TransformStatusComponent>();
					if (!status.isStunned()) {
						auto& sprite = zombie->getComponent<SpriteComponent>();
						sprite.Play("Stun");

						status.setStunned(true, 300); // 5 seconds at 60 FPS
					}
				}
			}
		}

		// Remove laser power-up when it reaches bottom of screen
		if (laserTransform.position.y > 700) {
			laserPowerUp->destroy();
			laserPowerUp = nullptr;
			laserActive = false;
		}
	}

	// Add delay to results screen
	if (allZombiesTransformed) {
		if (!nextLevelDelayStarted) {
			nextLevelDelayStarted = true;
			nextLevelDelayTimer = 120;
		}

		if (nextLevelDelayTimer > 0) {
			nextLevelDelayTimer--;
		}
		else {
			nextLevelDelayStarted = false; // Reset for next level

			// Calculate results and hand them off to results screen
			game->cachedHpResults = "Barrier HP Remaining: " + std::to_string(barrierHP);
			game->cachedWrongResults = getFormattedWrongLetters();
			game->cachedAccuracy = getFormattedAccuracy();
			game->cachedLevel = "Level " + std::to_string(level) + " Results!";

			game->changeState(GameState::ARCADE_RESULTS);
			return;
		}
	}

	// Barrier destroyed!
	if (!barrierDestroyed && barrierHP <= 0) {
		barrierDestroyed = true;
		gameOverDelayTimer = 120; // 2 seconds at 60 FPS

		// Create exclamation point above player
		//exclamation = &manager.addEntity();

		//int exclaimX = player->getComponent<TransformComponent>().position.x - 17; // centered..?!
		//int exclaimY = player->getComponent<TransformComponent>().position.y - 5; // slightly above player
		//exclamation->addComponent<TransformComponent>(exclaimX, exclaimY, 17, 16, 2);
		//exclamation->addComponent<SpriteComponent>("assets/Exclamation.png");
	}

	// If the delay timer is counting down
	if (barrierDestroyed) {
		if (gameOverDelayTimer > 0) {
			gameOverDelayTimer--;
		}
		else {
			// Destroy exclamation point
			//if (exclamation && exclamation->hasComponent<TransformComponent>()) {
			//	std::cout << "[DEBUG] Destroying exclamation entity\n";
			//	exclamation->destroy();
			//	exclamationShouldBeDestroyed = true;
			//}


			barrierDestroyed = false;

			// Calculate results and hand them off to game over screen
			game->cachedHighestLevel = "Highest Level Reached: " + std::to_string(level);
			game->cachedZombiesDefeated = "Total Zombies Defeated: " + std::to_string(zombiesDefeated);
			game->cachedOverallAccuracy = getOverallAccuracy();

			game->changeState(GameState::GAME_OVER);
			return;
		}
	}
}

void ArcadeMode::updateBonusStage() {
    // TODO: bonus stage logic

	// Bonus stage logic
	Uint32 currentTime = SDL_GetTicks(); // Get current time in milliseconds

	// Check if word is fully typed and wrong
	if (userInput.size() == targetText.size() && userInput != targetText) {
		wordTypedWrong = true;
	}
	else {
		wordTypedWrong = false;
	}

	// Update status text (without some of the other status options, for bonus mode simplicity)
	if (wordTypedWrong) {
		statusText = "ERROR";
	}
	else if (barrierHP <= 50) {
		statusText = "CAUTION";
	}
	else if (barrierHP <= 20) {
		statusText = "CRITICAL";
	}
	else {
		statusText = "OK";
	}

	// Basic laser logic
	for (auto& laser : activeLasers) {
		laser.duration--;
	}

	activeLasers.erase(
		std::remove_if(activeLasers.begin(), activeLasers.end(),
			[](const LaserStrike& l) { return l.duration <= 0; }),
		activeLasers.end());


	// Update crosshair position if zombies are present in the left group
	if (!leftToRight.empty() && currentZombieIndex < leftToRight.size()) {
		Entity* activeZombie = leftToRight[currentZombieIndex];
		auto& zombieTransform = activeZombie->getComponent<TransformComponent>();

		// Update crosshair's position to zombie's position
		auto& crosshairTransform = crosshair->getComponent<TransformComponent>();
		crosshairTransform.position = zombieTransform.position;
	}

	// Update crosshair position if zombies are present in the right group
	if (!rightToLeft.empty() && currentZombieIndex < rightToLeft.size()) {
		Entity* activeZombie = rightToLeft[currentZombieIndex];
		auto& zombieTransform = activeZombie->getComponent<TransformComponent>();

		// Update crosshair's position to zombie's position
		auto& crosshairTransform = crosshair->getComponent<TransformComponent>();
		crosshairTransform.position = zombieTransform.position;
	}

	// Update hand sprites to reflect the key needed to be pressed
	updateHandSprites();

	// Check if all left-to-right zombies are transformed before moving right-to-left zombies
	leftGroupDefeated = true;
	for (auto* zombie : leftToRight) {
		if (!zombie->getComponent<TransformStatusComponent>().getTransformed()) {
			leftGroupDefeated = false;
			break;
		}
	}

	// Iterate through left-to-right zombies
	for (size_t i = 0; i < leftToRight.size(); ++i) {
		Entity* zombie = leftToRight[i];
		auto& zombieTransform = zombie->getComponent<TransformComponent>();
		auto& transformStatus = zombie->getComponent<TransformStatusComponent>();

		// Check if zombie moves past screen, then eliminate if so (arcade-style!)
		if (zombieTransform.position.x > 1600 && !transformStatus.getTransformed()) {
			// Transform zombie
			transformStatus.setTransformed(true);
			tombstones.push_back(zombie);

			// Update zombie count
			zombieCount--;

			// Clear user input
			userInput.clear();

			// Resetting hand sprites
			resetHandSprites();

			// Move to the zombie to the left (next in the group)
			if (i == currentZombieIndex) {
				float currentX = leftToRight[currentZombieIndex]->getComponent<TransformComponent>().position.x;
				float nextX = -std::numeric_limits<float>::max();
				int nextZombieIndex = -1;

				for (size_t j = 0; j < leftToRight.size(); ++j) {
					if (!leftToRight[j]->getComponent<TransformStatusComponent>().getTransformed()) {
						float candidateX = leftToRight[j]->getComponent<TransformComponent>().position.x;

						// Looking for left of current zombie
						if (candidateX < currentX && candidateX > nextX) {
							nextX = candidateX;
							nextZombieIndex = static_cast<int>(j);
						}
					}
				}

				// Found next zombie, update index and targetText
				if (nextZombieIndex != -1) {
					currentZombieIndex = nextZombieIndex;
					targetText = bonusLeft[currentZombieIndex];
				}
			}

		}

		// Check if zombie is transformed, then move if not
		if (!transformStatus.getTransformed()) {
			auto& sprite = zombie->getComponent<SpriteComponent>();
			sprite.Play("Walk Right");

			zombieTransform.position.x += bonusSpeed;
		}

		// Check if zombie's prompt matches user input
		if (i == currentZombieIndex && userInput == bonusLeft[i] && !transformStatus.getTransformed()) {
			for (size_t j = 0; j < userInput.size(); ++j) {
				if (j >= targetText.size() || userInput[j] != targetText[j]) {
					// Append to typedWrong only if not already processed
					if (std::find(typedWrong.begin(), typedWrong.end(), userInput[j]) == typedWrong.end()) {
						typedWrong.push_back(targetText[j]);
					}
				}
			}

			// Basic laser animation for eliminating zombie
			int cannonX = laserMiddle->getComponent<TransformComponent>().position.x + 68; // center of cannon
			int cannonY = laserMiddle->getComponent<TransformComponent>().position.y + 128; // bottom of cannon

			int zombieX = zombie->getComponent<TransformComponent>().position.x + 32;
			int zombieY = zombie->getComponent<TransformComponent>().position.y + 32;

			LaserStrike laser;
			laser.startX = cannonX;
			laser.startY = cannonY;
			laser.endX = zombieX;
			laser.endY = zombieY;
			laser.duration = 6;

			activeLasers.push_back(laser);

			// Transform zombie, play defeat animation
			auto& sprite = zombie->getComponent<SpriteComponent>();
			sprite.Play("Defeat");
			transformStatus.setTransformed(true);
			tombstones.push_back(zombie);

			// Update zombie count / zombies defeated
			zombieCount--;
			zombiesDefeated++;

			// Update bonus zombie count / zombies defeated
			bonusZombiesDefeated++;

			// Rewards HP that will be added to barrierHP after results screen
			bonusHP += 10;

			// Clear user input
			userInput.clear();

			// Resetting hand sprites
			resetHandSprites();

			// Move to the zombie to the left
			if (i == currentZombieIndex) {
				float currentX = leftToRight[currentZombieIndex]->getComponent<TransformComponent>().position.x;
				float nextX = -std::numeric_limits<float>::max();
				int nextZombieIndex = -1;

				for (size_t j = 0; j < leftToRight.size(); ++j) {
					if (!leftToRight[j]->getComponent<TransformStatusComponent>().getTransformed()) {
						float candidateX = leftToRight[j]->getComponent<TransformComponent>().position.x;

						// Looking for left of current zombie
						if (candidateX < currentX && candidateX > nextX) {
							nextX = candidateX;
							nextZombieIndex = static_cast<int>(j);
						}
					}
				}

				// Found next zombie, update index and targetText
				if (nextZombieIndex != -1) {
					currentZombieIndex = nextZombieIndex;
					targetText = bonusLeft[currentZombieIndex];
				}
			}
		}
	}

	// Iterate through right-to-left zombies, only if left group is defeated
	if (leftGroupDefeated) {
		targetText = bonusRight[currentZombieIndex]; // Use words from the right group
		for (size_t i = 0; i < rightToLeft.size(); ++i) {
			Entity* zombie = rightToLeft[i];
			auto& zombieTransform = zombie->getComponent<TransformComponent>();
			auto& transformStatus = zombie->getComponent<TransformStatusComponent>();

			// Check if zombie moves past screen, then eliminate if so
			if (zombieTransform.position.x < -75 && !transformStatus.getTransformed()) {
				// Transform zombie
				transformStatus.setTransformed(true);
				tombstones.push_back(zombie);

				// Update zombie count
				zombieCount--;

				// Clear user input
				userInput.clear();

				// Resetting hand sprites
				resetHandSprites();

				// Move to the zombie to the right
				if (i == currentZombieIndex) {
					float currentX = rightToLeft[currentZombieIndex]->getComponent<TransformComponent>().position.x;
					float nextX = std::numeric_limits<float>::max();
					int nextZombieIndex = -1;

					for (size_t j = 0; j < rightToLeft.size(); ++j) {
						if (!rightToLeft[j]->getComponent<TransformStatusComponent>().getTransformed()) {
							float candidateX = rightToLeft[j]->getComponent<TransformComponent>().position.x;

							// Looking for right of current zombie
							if (candidateX > currentX && candidateX < nextX) {
								nextX = candidateX;
								nextZombieIndex = static_cast<int>(j);
							}
						}
					}

					// Found next zombie, update index and targetText
					if (nextZombieIndex != -1) {
						currentZombieIndex = nextZombieIndex;
						targetText = bonusRight[currentZombieIndex];
					}
				}

			}

			// Check if zombie is transformed, then move if not
			if (!transformStatus.getTransformed()) {
				auto& sprite = zombie->getComponent<SpriteComponent>();
				sprite.Play("Walk Left");

				zombieTransform.position.x -= bonusSpeed;
			}

			// Check if zombie's prompt matches user input
			if (i == currentZombieIndex && userInput == bonusRight[i] && !transformStatus.getTransformed()) {
				for (size_t j = 0; j < userInput.size(); ++j) {
					if (j >= targetText.size() || userInput[j] != targetText[j]) {
						// Append to typedWrong only if not already processed
						if (std::find(typedWrong.begin(), typedWrong.end(), userInput[j]) == typedWrong.end()) {
							typedWrong.push_back(targetText[j]);
						}
					}
				}

				// Basic laser animation for eliminating zombie
				int cannonX = laserMiddle->getComponent<TransformComponent>().position.x + 68; // center of 68px cannon
				int cannonY = laserMiddle->getComponent<TransformComponent>().position.y + 128; // bottom of cannon

				int zombieX = zombie->getComponent<TransformComponent>().position.x + 32;
				int zombieY = zombie->getComponent<TransformComponent>().position.y + 32;

				LaserStrike laser;
				laser.startX = cannonX;
				laser.startY = cannonY;
				laser.endX = zombieX;
				laser.endY = zombieY;
				laser.duration = 6;

				activeLasers.push_back(laser);

				// Transform zombie and play defeat animation
				auto& sprite = zombie->getComponent<SpriteComponent>();
				sprite.Play("Defeat");
				transformStatus.setTransformed(true);
				tombstones.push_back(zombie);

				// Update zombie count / zombies defeated
				zombieCount--;
				zombiesDefeated++;

				// Update bonus zombie count / zombies defeated
				bonusZombiesDefeated++;

				// Rewards HP that will be added to barrierHP after results screen
				bonusHP += 10;

				// Clear user input
				userInput.clear();

				// Resetting hand sprites
				resetHandSprites();

				// Move to the zombie to the right
				if (i == currentZombieIndex) {
					float currentX = rightToLeft[currentZombieIndex]->getComponent<TransformComponent>().position.x;
					float nextX = std::numeric_limits<float>::max();
					int nextZombieIndex = -1;

					for (size_t j = 0; j < rightToLeft.size(); ++j) {
						if (!rightToLeft[j]->getComponent<TransformStatusComponent>().getTransformed()) {
							float candidateX = rightToLeft[j]->getComponent<TransformComponent>().position.x;

							// Looking for right of current zombie
							if (candidateX > currentX && candidateX < nextX) {
								nextX = candidateX;
								nextZombieIndex = static_cast<int>(j);
							}
						}
					}

					// Found next zombie, update index and targetText
					if (nextZombieIndex != -1) {
						currentZombieIndex = nextZombieIndex;
						targetText = bonusRight[currentZombieIndex];
					}
				}

			}
		}
	}

	// Check if all zombies are transformed
	allZombiesTransformed = true; // Assume all are defeated

	for (auto* zombie : leftToRight) {
		if (!zombie->getComponent<TransformStatusComponent>().getTransformed()) {
			allZombiesTransformed = false;
			break;
		}
	}

	for (auto* zombie : rightToLeft) {
		if (!zombie->getComponent<TransformStatusComponent>().getTransformed()) {
			allZombiesTransformed = false;
			break;
		}
	}

	// Add delay to results screen
	if (allZombiesTransformed) {
		if (!nextLevelDelayStarted) {
			nextLevelDelayStarted = true;
			nextLevelDelayTimer = 120;
		}

		if (nextLevelDelayTimer > 0) {
			nextLevelDelayTimer--;
		}
		else {
			barrierHP += bonusHP;
			nextLevelDelayStarted = false; // Reset for next level

			// Calculate results and hand them off to results screen
			game->cachedBonusHpResults = "Barrier HP restored: " + std::to_string(bonusHP);
			game->cachedBonusWrongResults = getFormattedWrongLetters();
			game->cachedBonusZombiesDefeated = "Zombies defeated: " + std::to_string(bonusZombiesDefeated) + "/" + std::to_string(totalBonusZombies);
			game->cachedAccuracy = getFormattedAccuracy();

			game->changeState(GameState::BONUS_RESULTS); // Transition to results state
		}
	}
}

// Render methods
void ArcadeMode::renderArcadeStage() {
    // TODO: draw zombies, UI, etc.

	TTF_Font* roundFont = game->roundFont;
	TTF_Font* controlPanelFont = game->controlPanelFont;
	TTF_Font* statusFont = game->statusFont;
	TTF_Font* threatLvlFont = game->threatLvlFont;
	TTF_Font* comboStatusFont = game->comboStatusFont;

	// Draw game

	SDL_SetRenderDrawColor(Game::renderer, 160, 160, 160, 255);
	SDL_RenderClear(Game::renderer);

	// TESTING LINES TO ENSURE DIMENSIONS ARE CORRECT
	SDL_SetRenderDrawColor(Game::renderer, 255, 0, 0, 255); // red
	SDL_RenderDrawLine(Game::renderer, 800, 0, 800, 900);   // vertical center line
	SDL_RenderDrawLine(Game::renderer, 1599, 0, 1599, 900); // far right edge

	// Cursor rendering
	cursorBlinkSpeed = 500; // milliseconds
	showCursor = (SDL_GetTicks() / cursorBlinkSpeed) % 2 == 0;

	// Draw map and game objects
	map->drawMap(shakeOffsetX, shakeOffsetY);
	manager.draw();

	// Render sprite hands over tombstones
	leftHand->getComponent<SpriteComponent>().draw();
	rightHand->getComponent<SpriteComponent>().draw();

	// Render crosshair
	if (!zombies.empty() && currentZombieIndex < zombies.size()) {
		Entity* activeZombie = zombies[currentZombieIndex];
		auto& zombieTransform = activeZombie->getComponent<TransformComponent>();

		// Place crosshair on top of current zombie
		auto& crosshairTransform = crosshair->getComponent<TransformComponent>();
		crosshairTransform.position = zombieTransform.position;

		// Draw crosshair sprite
		crosshair->getComponent<SpriteComponent>().draw();
	}

	if (!allZombiesTransformed && currentZombieIndex < zombies.size()) {
		// Only render prompt if there are still zombies to be defeated
		Entity* activeZombie = zombies[currentZombieIndex];
		auto& zombieTransform = activeZombie->getComponent<TransformComponent>();

		int zombieWidth = 32;
		int zombieCenterX = static_cast<int>(zombieTransform.position.x + (zombieWidth));
		int textY = static_cast<int>(zombieTransform.position.y - 20); // Slightly above zombie

		if (game->uiManager) {
			// HERE ya goob
			//
			//
			// 
			// 
			// 
			// NEED TO UPDATE PROMPT BOX AND POSSIBLY TEXT SIZE !
			SDL_Color rectColor = { 255, 178, 102, 255 };
			TTF_Font* font = TTF_OpenFont("assets/PressStart2P.ttf", 16);

			if (font) {
				// Calculate total text width
				int totalTextWidth = 0;
				for (char c : targetText) {
					int w, h;
					TTF_SizeText(font, std::string(1, c).c_str(), &w, &h);
					totalTextWidth += w;
				}

				// Center textX based on zombie sprite's center and the text width
				int textX = zombieCenterX - (totalTextWidth / 2);

				// Center background rectangle
				int rectWidth = totalTextWidth + 20; // with some padding
				game->uiManager->drawRectangle(textX - 10, textY - 5, rectWidth, 25, rectColor);

				// Render letters with spacing
				int letterX = textX;
				int cursorX = textX; // default in case userInput is empty (for whatever reason !)

				for (size_t i = 0; i < targetText.size(); ++i) {
					SDL_Color color = { 255, 255, 255, 255 }; // Default to white
					if (i < userInput.size()) {
						if (userInput[i] == targetText[i]) {
							color = { 0, 255, 0, 255 }; // Green for correct input
						}
						else if (!processedInput[i]) {
							color = { 255, 0, 0, 255 }; // Red for incorrect input

							// Append to typedWrong only if not already processed
							if (std::find(typedWrong.begin(), typedWrong.end(), userInput[i]) == typedWrong.end()) {
								typedWrong.push_back(targetText[i]);
							}

							processedInput[i] = true;
						}
						else if (processedInput[i]) {
							color = { 255, 0, 0, 255 }; // Keep red for already processed incorrect input
						}
					}

					std::string letter(1, targetText[i]);
					SDL_Surface* surface = TTF_RenderText_Solid(font, letter.c_str(), color);
					if (surface) {
						SDL_Texture* texture = SDL_CreateTextureFromSurface(Game::renderer, surface);
						if (texture) {
							SDL_Rect dst = { letterX, textY, surface->w, surface->h };
							SDL_RenderCopy(Game::renderer, texture, nullptr, &dst);

							letterX += surface->w + 1;

							// Update cursorX AFTER rendering the letter
							if (i + 1 == userInput.size()) {
								cursorX = letterX - 2;
							}

							SDL_DestroyTexture(texture);
						}
						SDL_FreeSurface(surface);
					}
				}

				// Handle fully typed case — cursor at end
				if (userInput.size() == targetText.size()) {
					cursorX = letterX; // after last letter
				}

				// Draw cursor
				if (showCursor && userInput.size() <= targetText.size()) {
					// Change caret color if input is fully typed but incorrect
					SDL_Color caretColor = { 255, 255, 255, 255 }; // default: white

					if (userInput.size() == targetText.size() && userInput != targetText) {
						caretColor = { 255, 0, 0, 255 }; // red for incorrect full word
					}

					int caretWidth = 2;
					int caretHeight = 18;

					SDL_Rect caretRect = {
						cursorX,
						textY,
						caretWidth,
						caretHeight
					};

					SDL_SetRenderDrawColor(Game::renderer, caretColor.r, caretColor.g, caretColor.b, caretColor.a);
					SDL_RenderFillRect(Game::renderer, &caretRect);
				}

				TTF_CloseFont(font);
			}
		}
	}

	// These are lower so they are drawn over the zombies!

	// Draw basic laser
	for (const auto& laser : activeLasers) {
		SDL_SetRenderDrawColor(Game::renderer, 255, 0, 0, 255); // Red

		int thickness = 4;

		// Draw 'thickness' of laser (number of lines offset horizontally)
		for (int i = -thickness / 2; i <= thickness / 2; ++i) {
			SDL_RenderDrawLine(
				Game::renderer,
				laser.startX + i,
				laser.startY,
				laser.endX + i,
				laser.endY
			);
		}
	}

	// Draw laser cannons LAST (or else the zombies walk over them and that just looks plain silly)
	laserLeft->getComponent<SpriteComponent>().draw();
	laserRight->getComponent<SpriteComponent>().draw();
	laserMiddle->getComponent<SpriteComponent>().draw();

	// Draw control panel
	if (game->uiManager) {
		SDL_Color outlineColor = { 255, 255, 255, 255 };
		SDL_Color fgColor = { 102, 255, 105, 255 };
		SDL_Color bgColor = { 255, 102, 102, 255 };
		SDL_Color comboColor = { 255, 255, 102, 255 };
		SDL_Color textColor = { 0, 0, 0, 255 };

		game->uiManager->drawHealthbar(130, 780, 320, 40, barrierHP, maxHP, "SHIELD:", outlineColor, fgColor, bgColor, controlPanelFont, textColor);
		game->uiManager->drawStatusBar(130, 840, 320, 40, "STATUS:", statusText, outlineColor, bgColor, controlPanelFont, statusFont, textColor);
		game->uiManager->drawThreatLvl(1140, 785, 70, 70, zombieCount, "THREAT LVL", outlineColor, bgColor, controlPanelFont, threatLvlFont, textColor);
		game->uiManager->drawComboAlert(1335, 860, 60, 30, comboLevel, "COMBO:", comboStatus, outlineColor, comboColor, controlPanelFont, comboStatusFont, textColor);
	}

	// Draw level (round) text at top of screen in the middle
	game->uiManager->drawCenteredText("Round " + std::to_string(level), 10, { 0, 0, 0, 255 }, roundFont, 1600);

	// Draw laser power-up!
	if (laserActive) {
		laserPowerUp->getComponent<SpriteComponent>().Play("Laser");
	}

	// Draw exclamation point above player when barrier is destroyed
	//if (barrierDestroyed && exclamation && exclamation->isActive()) {
	//	exclamation->getComponent<SpriteComponent>().draw();
	//}

	SDL_RenderPresent(Game::renderer);
}

void ArcadeMode::renderBonusStage() {
    // TODO: draw bonus stuff

	// Draw game

	TTF_Font* roundFont = game->roundFont;
	TTF_Font* controlPanelFont = game->controlPanelFont;
	TTF_Font* statusFont = game->statusFont;
	TTF_Font* threatLvlFont = game->threatLvlFont;
	TTF_Font* comboStatusFont = game->comboStatusFont;

	SDL_SetRenderDrawColor(Game::renderer, 160, 160, 160, 255);
	SDL_RenderClear(Game::renderer);

	// Cursor rendering
	cursorBlinkSpeed = 500; // milliseconds
	showCursor = (SDL_GetTicks() / cursorBlinkSpeed) % 2 == 0;

	// Draw map and game objects
	map->drawMap();
	manager.draw();

	// Draw sprite hands
	leftHand->getComponent<SpriteComponent>().draw();
	rightHand->getComponent<SpriteComponent>().draw();

	// Render crosshair on left group first
	if (!leftGroupDefeated) {
		if (!leftToRight.empty() && currentZombieIndex < leftToRight.size()) {
			Entity* activeZombie = leftToRight[currentZombieIndex];
			auto& zombieTransform = activeZombie->getComponent<TransformComponent>();

			// Place crosshair on top of current zombie
			auto& crosshairTransform = crosshair->getComponent<TransformComponent>();
			crosshairTransform.position = zombieTransform.position;

			// Draw crosshair sprite
			crosshair->getComponent<SpriteComponent>().draw();
		}
	}
	else {
		if (!rightToLeft.empty() && currentZombieIndex < rightToLeft.size()) {
			Entity* activeZombie = rightToLeft[currentZombieIndex];
			auto& zombieTransform = activeZombie->getComponent<TransformComponent>();

			// Place crosshair on top of current zombie
			auto& crosshairTransform = crosshair->getComponent<TransformComponent>();
			crosshairTransform.position = zombieTransform.position;

			// Draw crosshair sprite
			crosshair->getComponent<SpriteComponent>().draw();
		}
	}

	// Render prompt on left group first
	if (!leftGroupDefeated) {
		if (!allZombiesTransformed && currentZombieIndex < leftToRight.size()) {
			// Only render prompt if there are still zombies to be defeated
			Entity* activeZombie = leftToRight[currentZombieIndex];
			auto& zombieTransform = activeZombie->getComponent<TransformComponent>();

			int zombieWidth = 32;
			int zombieCenterX = static_cast<int>(zombieTransform.position.x + (zombieWidth));
			int textY = static_cast<int>(zombieTransform.position.y - 20); // Slightly above zombie

			if (game->uiManager) {
				SDL_Color rectColor = { 255, 178, 102, 255 };
				TTF_Font* font = TTF_OpenFont("assets/PressStart2P.ttf", 16);

				if (font) {
					// Calculate total text width
					int totalTextWidth = 0;
					for (char c : targetText) {
						int w, h;
						TTF_SizeText(font, std::string(1, c).c_str(), &w, &h);
						totalTextWidth += w;
					}

					// Center textX based on zombie sprite's center and the text width
					int textX = zombieCenterX - (totalTextWidth / 2);

					// Center background rectangle
					int rectWidth = totalTextWidth + 20; // with some padding
					game->uiManager->drawRectangle(textX - 10, textY - 5, rectWidth, 25, rectColor);

					// Render letters with spacing
					int letterX = textX;
					int cursorX = textX; // default in case userInput is empty

					for (size_t i = 0; i < targetText.size(); ++i) {
						SDL_Color color = { 255, 255, 255, 255 }; // Default to white
						if (i < userInput.size()) {
							if (userInput[i] == targetText[i]) {
								color = { 0, 255, 0, 255 }; // Green for correct input
							}
							else if (!processedInput[i]) {
								color = { 255, 0, 0, 255 }; // Red for incorrect input

								// Append to typedWrong only if not already processed
								if (std::find(typedWrong.begin(), typedWrong.end(), userInput[i]) == typedWrong.end()) {
									typedWrong.push_back(targetText[i]);
								}

								processedInput[i] = true;
							}
							else if (processedInput[i]) {
								color = { 255, 0, 0, 255 }; // Keep red for already processed incorrect input
							}
						}

						std::string letter(1, targetText[i]);
						SDL_Surface* surface = TTF_RenderText_Solid(font, letter.c_str(), color);
						if (surface) {
							SDL_Texture* texture = SDL_CreateTextureFromSurface(Game::renderer, surface);
							if (texture) {
								SDL_Rect dst = { letterX, textY, surface->w, surface->h };
								SDL_RenderCopy(Game::renderer, texture, nullptr, &dst);

								letterX += surface->w + 1;

								// Update cursorX AFTER rendering the letter
								if (i + 1 == userInput.size()) {
									cursorX = letterX - 2;
								}

								SDL_DestroyTexture(texture);
							}
							SDL_FreeSurface(surface);
						}
					}

					// Handle fully typed case — cursor at end
					if (userInput.size() == targetText.size()) {
						cursorX = letterX; // after last letter
					}

					// Draw cursor
					if (showCursor && userInput.size() <= targetText.size()) {
						// Change caret color if input is fully typed but incorrect
						SDL_Color caretColor = { 255, 255, 255, 255 }; // default: white

						if (userInput.size() == targetText.size() && userInput != targetText) {
							caretColor = { 255, 0, 0, 255 }; // red for incorrect full word
						}

						int caretWidth = 2;
						int caretHeight = 18;

						SDL_Rect caretRect = {
							cursorX,
							textY,
							caretWidth,
							caretHeight
						};

						SDL_SetRenderDrawColor(Game::renderer, caretColor.r, caretColor.g, caretColor.b, caretColor.a);
						SDL_RenderFillRect(Game::renderer, &caretRect);
					}

					TTF_CloseFont(font);
				}
			}
		}
	}
	else {
		if (!allZombiesTransformed && currentZombieIndex < rightToLeft.size()) {
			// Only render prompt if there are still zombies to be defeated
			Entity* activeZombie = rightToLeft[currentZombieIndex];
			auto& zombieTransform = activeZombie->getComponent<TransformComponent>();

			int zombieWidth = 32;
			int zombieCenterX = static_cast<int>(zombieTransform.position.x + (zombieWidth));
			int textY = static_cast<int>(zombieTransform.position.y - 20); // Slightly above zombie

			if (game->uiManager) {
				SDL_Color rectColor = { 255, 178, 102, 255 };
				TTF_Font* font = TTF_OpenFont("assets/PressStart2P.ttf", 16);

				if (font) {
					// Calculate total text width
					int totalTextWidth = 0;
					for (char c : targetText) {
						int w, h;
						TTF_SizeText(font, std::string(1, c).c_str(), &w, &h);
						totalTextWidth += w;
					}

					// Center textX based on zombie sprite's center and the text width
					int textX = zombieCenterX - (totalTextWidth / 2);

					// Center background rectangle
					int rectWidth = totalTextWidth + 20; // with some padding
					game->uiManager->drawRectangle(textX - 10, textY - 5, rectWidth, 25, rectColor);

					// Render letters with spacing
					int letterX = textX;
					int cursorX = textX; // default in case userInput is empty

					for (size_t i = 0; i < targetText.size(); ++i) {
						SDL_Color color = { 255, 255, 255, 255 }; // Default to white
						if (i < userInput.size()) {
							if (userInput[i] == targetText[i]) {
								color = { 0, 255, 0, 255 }; // Green for correct input
							}
							else if (!processedInput[i]) {
								color = { 255, 0, 0, 255 }; // Red for incorrect input

								// Append to typedWrong only if not already processed
								if (std::find(typedWrong.begin(), typedWrong.end(), userInput[i]) == typedWrong.end()) {
									typedWrong.push_back(targetText[i]);
								}

								processedInput[i] = true;
							}
							else if (processedInput[i]) {
								color = { 255, 0, 0, 255 }; // Keep red for already processed incorrect input
							}
						}

						std::string letter(1, targetText[i]);
						SDL_Surface* surface = TTF_RenderText_Solid(font, letter.c_str(), color);
						if (surface) {
							SDL_Texture* texture = SDL_CreateTextureFromSurface(Game::renderer, surface);
							if (texture) {
								SDL_Rect dst = { letterX, textY, surface->w, surface->h };
								SDL_RenderCopy(Game::renderer, texture, nullptr, &dst);

								letterX += surface->w + 1;

								// Update cursorX AFTER rendering the letter
								if (i + 1 == userInput.size()) {
									cursorX = letterX - 2;
								}

								SDL_DestroyTexture(texture);
							}
							SDL_FreeSurface(surface);
						}
					}

					// Handle fully typed case — cursor at end
					if (userInput.size() == targetText.size()) {
						cursorX = letterX; // after last letter
					}

					// Draw cursor
					if (showCursor && userInput.size() <= targetText.size()) {
						// Change caret color if input is fully typed but incorrect
						SDL_Color caretColor = { 255, 255, 255, 255 }; // default: white

						if (userInput.size() == targetText.size() && userInput != targetText) {
							caretColor = { 255, 0, 0, 255 }; // red for incorrect full word
						}

						int caretWidth = 2;
						int caretHeight = 18;

						SDL_Rect caretRect = {
							cursorX,
							textY,
							caretWidth,
							caretHeight
						};

						SDL_SetRenderDrawColor(Game::renderer, caretColor.r, caretColor.g, caretColor.b, caretColor.a);
						SDL_RenderFillRect(Game::renderer, &caretRect);
					}

					TTF_CloseFont(font);
				}
			}
		}
	}

	// Moving down here so this is drawn over the zombies

	// Draw basic laser
	for (const auto& laser : activeLasers) {
		SDL_SetRenderDrawColor(Game::renderer, 255, 0, 0, 255); // Red

		int thickness = 4;

		// Draw 'thickness' of laser (number of lines offset horizontally)
		for (int i = -thickness / 2; i <= thickness / 2; ++i) {
			SDL_RenderDrawLine(
				Game::renderer,
				laser.startX + i,
				laser.startY,
				laser.endX + i,
				laser.endY
			);
		}
	}

	// Draw laser cannons LAST
	laserLeft->getComponent<SpriteComponent>().draw();
	laserRight->getComponent<SpriteComponent>().draw();
	laserMiddle->getComponent<SpriteComponent>().draw();

	// Draw control panel
	if (game->uiManager) {
		SDL_Color outlineColor = { 255, 255, 255, 255 };
		SDL_Color fgColor = { 102, 255, 105, 255 };
		SDL_Color bgColor = { 255, 102, 102, 255 };
		SDL_Color comboColor = { 255, 255, 102, 255 };
		SDL_Color textColor = { 0, 0, 0, 255 };

		game->uiManager->drawHealthbar(130, 780, 320, 40, barrierHP, maxHP, "SHIELD:", outlineColor, fgColor, bgColor, controlPanelFont, textColor);
		game->uiManager->drawStatusBar(130, 840, 320, 40, "STATUS:", statusText, outlineColor, bgColor, controlPanelFont, statusFont, textColor);
		game->uiManager->drawThreatLvl(1140, 785, 70, 70, zombieCount, "THREAT LVL", outlineColor, bgColor, controlPanelFont, threatLvlFont, textColor);
		game->uiManager->drawComboAlert(1335, 860, 60, 30, comboLevel, "COMBO:", comboStatus, outlineColor, comboColor, controlPanelFont, comboStatusFont, textColor);
	}

	// Draw bonus text at top of screen in the center
	game->uiManager->drawCenteredText("BONUS", 10, { 0, 0, 0, 255 }, roundFont, 1600);

	SDL_RenderPresent(Game::renderer);
}

// Handle Event Methods
void ArcadeMode::handleArcadeEvent(SDL_Event& event) {
    // TODO: handle arcade mode inputs
	switch (event.type) {
	case SDL_KEYDOWN:
		if (event.key.keysym.sym == SDLK_SPACE && laserReady) {
			fireLaser();  // Fire laser power-up
			laserReady = false; // Consumes laser charge
			comboLevel = 0;
			comboStatus = ""; // Reset combo display
		}

		if (event.key.keysym.sym == SDLK_BACKSPACE && !userInput.empty()) {
			userInput.pop_back(); // Remove last character
		}

		// ADD PAUSE HERE!

		break;

	case SDL_TEXTINPUT:
		// Prevent spacebar from being typed as part of input
		if (event.text.text[0] == ' ') {
			break; // Skip input
		}

		// Prevent typing if word is fully typed AND incorrect
		if (userInput.size() >= targetText.size() && userInput != targetText) {
			break; // Lock input until user deletes
		}

		userInput += event.text.text; // Append typed text
		processedInput.assign(userInput.size(), false);

		// Check for mistakes immediately, for combo system
		if (userInput.back() != targetText[userInput.size() - 1]) {
			brokenCombo = true;
			comboStatus = "X";
			comboLevel = 0;
		}

		// Increment total number of typed letters
		levelTotalLetters++;
		finalTotalLetters++;

		// Check if typed letter matches target letter
		if (userInput.size() <= targetText.size() && event.text.text[0] == targetText[userInput.size() - 1]) {
			levelCorrectLetters++; // Increment correct letters
			finalCorrectLetters++; // Increment total correct letters for game over screen

			// Resetting hand sprites
			resetHandSprites();
		}
		break;

	default:
		break;
	}
}

void ArcadeMode::handleBonusEvent(SDL_Event& event) {
    // TODO: handle bonus stage inputs

	switch (event.type) {
	case SDL_KEYDOWN:
		if (event.key.keysym.sym == SDLK_BACKSPACE && !userInput.empty()) {
			userInput.pop_back(); // Remove last character
		}

		// ADD PAUSE HERE!

		break;

	case SDL_TEXTINPUT:
		// Prevent spacebar from being typed as part of input
		if (event.text.text[0] == ' ') {
			break; // Skip input
		}

		// Prevent typing if word is fully typed AND incorrect
		if (userInput.size() >= targetText.size() && userInput != targetText) {
			break; // Lock input until user deletes
		}

		userInput += event.text.text; // Append typed text
		processedInput.assign(userInput.size(), false);

		// Increment total number of typed letters
		levelTotalLetters++;
		finalTotalLetters++;

		// Check if typed letter matches target letter
		if (userInput.size() <= targetText.size() && event.text.text[0] == targetText[userInput.size() - 1]) {
			levelCorrectLetters++; // Increment correct letters
			finalCorrectLetters++; // Increment total correct letters for game over screen

			// Resetting hand sprites
			resetHandSprites();
		}
		break;

	default:
		break;
	}
}


// Public methods below
// 
// 
// Reset or setup all elements of arcade mode for a fresh playthrough
void ArcadeMode::resetGame()
{
	manager.refresh();

	map = new Map();

	// Ensuring player, barrier, and laser are centered
	playerX = 1600 / 2;
	barrierX = (1600 / 2) - ((barrierWidth * barrierScale) / 2);
	laserX = (1600 / 2) - ((68 * 2) / 2);

	// Player
	//if (player) player->destroy();
	player = &manager.addEntity();
	player->addComponent<TransformComponent>(playerX, 660);

	// Barrier
	//if (barrier) barrier->destroy();
	barrier = &manager.addEntity();
	barrier->addComponent<TransformComponent>(barrierX, 640, 64, 64, 2);
	barrier->addComponent<SpriteComponent>("assets/Barrier_Orb_0.png");
	barrier->addComponent<ColliderComponent>("barrier");

	// Left hand
	//if (leftHand) leftHand->destroy();
	leftHand = &manager.addEntity();
	leftHand->addComponent<TransformComponent>(545, 770, 64, 64, 2);
	leftHand->addComponent<SpriteComponent>("assets/Left_Hand.png");

	// Right hand
	//if (rightHand) rightHand->destroy();
	rightHand = &manager.addEntity();
	rightHand->addComponent<TransformComponent>(930, 770, 64, 64, 2);
	rightHand->addComponent<SpriteComponent>("assets/Right_Hand.png");

	// Crosshair
	//if (crosshair) crosshair->destroy();
	crosshair = &manager.addEntity();
	crosshair->addComponent<TransformComponent>(0, 0);
	crosshair->addComponent<SpriteComponent>("assets/Crosshair.png");

	// Middle laser
	//if (laserMiddle) laserMiddle->destroy();
	laserMiddle = &manager.addEntity();
	laserMiddle->addComponent<TransformComponent>(laserX, 0, 68, 68, 2);
	laserMiddle->addComponent<SpriteComponent>("assets/Laser_Cannon_Middle.png");

	// Left laser
	//if (laserLeft) laserLeft->destroy();
	laserLeft = &manager.addEntity();
	laserLeft->addComponent<TransformComponent>(0, 0, 64, 64, 2);
	laserLeft->addComponent<SpriteComponent>("assets/Laser_Cannon_Left.png");

	// Right laser
	//if (laserRight) laserRight->destroy();
	laserRight = &manager.addEntity();
	laserRight->addComponent<TransformComponent>(1472, 0, 64, 64, 2);
	laserRight->addComponent<SpriteComponent>("assets/Laser_Cannon_Right.png");

	// Combo meter
	//if (comboMeter) comboMeter->destroy();
	comboMeter = &manager.addEntity();
	comboMeter->addComponent<TransformComponent>(1350, 785, 64, 32, 2);
	comboMeter->addComponent<SpriteComponent>("assets/Combo_Meter_0.png");

	// Exclamation
	//if (exclamation) exclamation->destroy();
	//	exclamation = nullptr;


	// Remove zombie entities
	for (auto* zombie : zombies) {
		// Reset zombie sprite and transformation status
		zombie->getComponent<SpriteComponent>().setTex("assets/Zombie.png");  // Reset to normal zombie sprite
		zombie->getComponent<TransformStatusComponent>().setTransformed(false); // Reset transformation status
		zombie->destroy(); // Mark zombie entity for removal
	}
	zombies.clear(); // Clear the zombies vector

	for (auto* zombie : leftToRight) {
		// Reset zombie sprite and transformation status
		zombie->getComponent<SpriteComponent>().setTex("assets/Zombie.png");  // Reset to normal zombie sprite
		zombie->getComponent<TransformStatusComponent>().setTransformed(false); // Reset transformation status
		zombie->destroy(); // Mark zombie entity for removal
	}
	leftToRight.clear(); // Clear the zombies vector

	for (auto* zombie : rightToLeft) {
		// Reset zombie sprite and transformation status
		zombie->getComponent<SpriteComponent>().setTex("assets/Zombie.png");  // Reset to normal zombie sprite
		zombie->getComponent<TransformStatusComponent>().setTransformed(false); // Reset transformation status
		zombie->destroy(); // Mark zombie entity for removal
	}
	rightToLeft.clear(); // Clear the zombies vector

	// Remove tombstone entities
	for (auto* tombstone : tombstones) {
		tombstone->destroy(); // Mark tombstone entity for removal
	}
	tombstones.clear(); // Clear the tombstone vector

	// Clear active basic lasers
	activeLasers.clear();

	// Clear laser power-up if still active
	if (laserActive) {
		laserPowerUp->destroy();
		laserPowerUp = nullptr;
		laserActive = false;
	}

	// Reset zombie spawn mechanics
	currentZombieIndex = 0;
	allZombiesTransformed = false;

	// Initial number of zombies to spawn
	int numZombies = 3;

	// Randomizing words
	words = wordManager.getRandomWords(WordListManager::EASY, numZombies);

	// Reset map visual
	map->setDifficulty(MapLevel::EASY);

	// Spawn zombies at random off-screen positions but not too close to player
	int spawnBuffer = 150; // Distance beyond game window for spawning
	for (size_t i = 0; i < numZombies; ++i)
	{
		Entity* newZombie = &manager.addEntity();

		int spawnEdge = rand() % 3; // 0: top, 1: left, 2: right
		int x, y;
		bool validSpawn = false;

		while (!validSpawn) {
			validSpawn = true;
			switch (spawnEdge)
			{
			case 0: // Top
				x = rand() % 1600; // Full width range
				y = -spawnBuffer;
				break;
			case 1: // Left
				x = -spawnBuffer;
				y = rand() % 650; // Ensures zombies spawn above the barrier orb
				break;
			case 2: // Right
				x = 1600 + spawnBuffer; // Force outside screen bounds
				y = rand() % 650; // Ensures zombies spawn above the barrier orb
				break;
			}

			// Ensure zombie spawn is not too close to player
			auto& playerTransform = player->getComponent<TransformComponent>();
			float dx = playerTransform.position.x - x;
			float dy = playerTransform.position.y - y;
			if (sqrt(dx * dx + dy * dy) < 400.0f) {
				validSpawn = false;
				continue;
			}

			// Check distance to other zombies
			for (Entity* otherZombie : zombies) {
				auto& otherTransform = otherZombie->getComponent<TransformComponent>();
				float odx = otherTransform.position.x - x;
				float ody = otherTransform.position.y - y;
				if (sqrt(odx * odx + ody * ody) < 70.0f) { // Radius for how far zombies spawn from each other
					validSpawn = false;
					break;
				}
			}
		}

		newZombie->addComponent<TransformComponent>(x, y);
		newZombie->addComponent<SpriteComponent>("assets/Zambie_Test-Sheet.png", true);
		newZombie->addComponent<ColliderComponent>("zombie");
		newZombie->addComponent<TransformStatusComponent>(); // Add transformation status
		zombies.push_back(newZombie);
	}

	// Find the closest zombie to the player at game start
	float closestDistance = std::numeric_limits<float>::max();
	size_t closestZombieIndex = 0;

	for (size_t i = 0; i < zombies.size(); ++i) {
		if (!zombies[i]->getComponent<TransformStatusComponent>().getTransformed()) {
			auto& zombieTransform = zombies[i]->getComponent<TransformComponent>();
			float dx = player->getComponent<TransformComponent>().position.x - zombieTransform.position.x;
			float dy = player->getComponent<TransformComponent>().position.y - zombieTransform.position.y;
			float distance = sqrt(dx * dx + dy * dy);

			if (distance < closestDistance) {
				closestDistance = distance;
				closestZombieIndex = i;
			}
		}
	}

	// Set starting target
	currentZombieIndex = closestZombieIndex;
	targetText = words[currentZombieIndex];

	// Reset zombies remaining / zombies defeated
	zombieCount = zombies.size();
	zombiesDefeated = 0;

	// Reset HP / barrier damage
	barrierHP = maxHP;
	updateBarrierDamage(barrierHP);

	// Reset the typing target
	targetText = words[currentZombieIndex];

	// Clear user input from last game
	userInput.clear();

	// Reset letters typed incorrectly
	typedWrong.clear();

	// Reset accuracy
	levelCorrectLetters = 0;
	levelTotalLetters = 0;
	finalCorrectLetters = 0;
	finalTotalLetters = 0;

	// Reset zambie speed!!
	speed = 0.5f;

	// Reset level
	level = 1;

	// Reset combo variables
	brokenCombo = false;
	laserReady = false;
	comboLevel = 0;
	checkCombo("", targetText);

	// Reset bonus stage variables
	bonusLevel = 0;
	inBonusStage = false;
	bonusSpeed = 2.0f;

	std::cout << "Arcade mode reset!" << std::endl;
}

// To set up next level of arcade mode
void ArcadeMode::nextLevel()
{
	// Go to bonus stage every ten rounds
	if (level % 10 == 0 && !inBonusStage) {
		game->changeState(GameState::BONUS_TITLE); // Transition to bonus title screen
		inBonusStage = true;
		return;
	}

	// Ensuring barrierHP doesn't surpass 100
	if (barrierHP >= 100) {
		barrierHP = 100;
	}

	// Update barrier sprite damage
	updateBarrierDamage(barrierHP);

	// Clear the previous round's zombies and reset zombie index and transformation status
	zombies.clear();
	currentZombieIndex = 0;
	allZombiesTransformed = false;

	// Clear active basic lasers
	activeLasers.clear();

	// Clear laser power-up if still active
	if (laserActive) {
		laserPowerUp->destroy();
		laserPowerUp = nullptr;
		laserActive = false;
	}

	// Setting number of zombies to spawn, with a new one appearing every 5 levels
	int numZombies = 3 + (level / 5);

	// Randomizing words and updating difficulty every 10 levels 
	int cycleLevel = (level % 30) + 1; // Ensures difficulty cycles every 30 rounds

	if (cycleLevel <= 10) {
		difficulty = WordListManager::EASY;
		map->setDifficulty(MapLevel::EASY);
	}
	else if (cycleLevel <= 20) {
		difficulty = WordListManager::MEDIUM;
		map->setDifficulty(MapLevel::MEDIUM);
	}
	else {
		difficulty = WordListManager::HARD;
		map->setDifficulty(MapLevel::HARD);
	}

	// Get random words for the next level, based on the current difficulty and number of zombies spawning
	words = wordManager.getRandomWords(difficulty, numZombies);

	// Spawn zombies at random off-screen positions but not too close to player
	int spawnBuffer = 150; // Distance beyond game window for spawning
	for (size_t i = 0; i < numZombies; ++i)
	{
		Entity* newZombie = &manager.addEntity();

		int spawnEdge = rand() % 3; // 0: top, 1: left, 2: right
		int x, y;
		bool validSpawn = false;

		while (!validSpawn) {
			validSpawn = true;
			switch (spawnEdge)
			{
			case 0: // Top
				x = rand() % 1600; // Full width range
				y = -spawnBuffer;
				break;
			case 1: // Left
				x = -spawnBuffer;
				y = rand() % 650; // Ensures zombies spawn above the barrier orb
				break;
			case 2: // Right
				x = 1600 + spawnBuffer; // Force outside screen bounds
				y = rand() % 650; // Ensures zombies spawn above the barrier orb
				break;
			}

			// Ensure zombie spawn is not too close to player
			auto& playerTransform = player->getComponent<TransformComponent>();
			float dx = playerTransform.position.x - x;
			float dy = playerTransform.position.y - y;
			if (sqrt(dx * dx + dy * dy) < 400.0f) {
				validSpawn = false;
				continue;
			}

			// Check distance to other zombies
			for (Entity* otherZombie : zombies) {
				auto& otherTransform = otherZombie->getComponent<TransformComponent>();
				float odx = otherTransform.position.x - x;
				float ody = otherTransform.position.y - y;
				if (sqrt(odx * odx + ody * ody) < 70.0f) { // Radius for how far zombies spawn from each other
					validSpawn = false;
					break;
				}
			}
		}

		newZombie->addComponent<TransformComponent>(x, y);
		newZombie->addComponent<SpriteComponent>("assets/Zambie_Test-Sheet.png", true);
		newZombie->addComponent<ColliderComponent>("zombie");
		newZombie->addComponent<TransformStatusComponent>(); // Add transformation status
		zombies.push_back(newZombie);
	}

	// Find the closest zombie to the player at game start
	float closestDistance = std::numeric_limits<float>::max();
	size_t closestZombieIndex = 0;

	for (size_t i = 0; i < zombies.size(); ++i) {
		if (!zombies[i]->getComponent<TransformStatusComponent>().getTransformed()) {
			auto& zombieTransform = zombies[i]->getComponent<TransformComponent>();
			float dx = player->getComponent<TransformComponent>().position.x - zombieTransform.position.x;
			float dy = player->getComponent<TransformComponent>().position.y - zombieTransform.position.y;
			float distance = sqrt(dx * dx + dy * dy);

			if (distance < closestDistance) {
				closestDistance = distance;
				closestZombieIndex = i;
			}
		}
	}

	// Set starting target
	currentZombieIndex = closestZombieIndex;
	targetText = words[currentZombieIndex];

	// Intilalize zombies remaining
	zombieCount = zombies.size();

	// Clear user input
	userInput.clear();

	// Reset letters typed incorrectly
	typedWrong.clear();

	// Reset accuracy
	levelCorrectLetters = 0;
	levelTotalLetters = 0;

	// Increase zambie speed!!
	speed += 0.1f;

	// Reset speed every 10 levels
	if (level % 10 == 0) {
		speed = 0.5f;
	}

	// Increment level (round)
	level++;

	// Reset bonus total zombies
	totalBonusZombies = 0;

	inBonusStage = false; // Reset the flag when exiting the bonus stage

	std::cout << "Zombies reset for new round!" << std::endl;
}

// To set up bonus stage
void ArcadeMode::bonusStage()
{
	// Clear the previous round's zombies
	leftToRight.clear();
	rightToLeft.clear();
	currentZombieIndex = 0;
	allZombiesTransformed = false;

	// Clear active basic lasers
	activeLasers.clear();

	// Clear laser power-up if still active
	if (laserActive) {
		laserPowerUp->destroy();
		laserPowerUp = nullptr;
		laserActive = false;
	}

	// Increasing bonus level
	bonusLevel++;

	// Keeping track of zombie count and increases each bonus round
	int numZombiesLeft = 3 + (bonusLevel - 1);
	int numZombiesRight = 3 + (bonusLevel - 1);

	// Randomize letters every new bonus round
	bonusLeft = wordManager.getRandomWords(WordListManager::BONUSLEFT, numZombiesLeft);
	bonusRight = wordManager.getRandomWords(WordListManager::BONUSRIGHT, numZombiesRight);

	int spacing = 120; // Space between zombies

	// Generate random y-coordinate for left-to-right zombie row
	int yLeft = 150 + (rand() % 360); // 150–559

	// Left-to-Right group
	for (int i = 0; i < numZombiesLeft; ++i)
	{
		Entity* newZombie = &manager.addEntity();
		int x = -150 - (i * spacing); // Start just outside the left edge
		int y = yLeft;

		newZombie->addComponent<TransformComponent>(x, y);
		newZombie->addComponent<SpriteComponent>("assets/Zambie_Test-Sheet.png", true);
		newZombie->addComponent<ColliderComponent>("zombie");
		newZombie->addComponent<TransformStatusComponent>(); // Add transformation status
		leftToRight.push_back(newZombie);
		totalBonusZombies++;
	}

	// Generate random y-coordinate for right-to-left zombie row
	int yRight = 150 + (rand() % 360); // 150-559

	// Right-to-Left group
	for (int i = 0; i < numZombiesRight; ++i)
	{
		Entity* newZombie = &manager.addEntity();
		int x = 1600 + (i * spacing); // Start just outside the right edge
		int y = yRight;

		newZombie->addComponent<TransformComponent>(x, y);
		newZombie->addComponent<SpriteComponent>("assets/Zambie_Test-Sheet.png", true);
		newZombie->addComponent<ColliderComponent>("zombie");
		newZombie->addComponent<TransformStatusComponent>(); // Add transformation status
		rightToLeft.push_back(newZombie);
		totalBonusZombies++;
	}

	// Make sure the leftmost zombie is first in the list
	std::reverse(rightToLeft.begin(), rightToLeft.end());

	// Intilalize zombies remaining
	zombieCount += leftToRight.size();
	zombieCount += rightToLeft.size();

	// Set starting target
	targetText = bonusLeft[currentZombieIndex];

	// Reset letters typed incorrectly
	typedWrong.clear();

	// Reset accuracy
	levelCorrectLetters = 0;
	levelTotalLetters = 0;

	// Reset bonus HP
	bonusHP = 0;

	// Reset bonus stage zombies defeated
	bonusZombiesDefeated = 0;

	// Increase zambie speed!!
	bonusSpeed += 1.0;

	std::cout << "Zombies reset for bonus round!" << std::endl;
}

// Key-to-finger sprite mapping
void ArcadeMode::updateHandSprites()
{
	// Get next letter to be typed and update sprite fingers accordingly
	for (size_t i = 0; i < targetText.size(); i++) {
		if (i <= userInput.size()) {
			// Left pinky
			if (targetText[i] == 'q') {
				leftHand->getComponent<SpriteComponent>().setTex("assets/Left_Pinky.png");
				rightHand->getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			else if (targetText[i] == 'a') {
				leftHand->getComponent<SpriteComponent>().setTex("assets/Left_Pinky.png");
				rightHand->getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			else if (targetText[i] == 'z') {
				leftHand->getComponent<SpriteComponent>().setTex("assets/Left_Pinky.png");
				rightHand->getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			// Left ring
			else if (targetText[i] == 'w') {
				leftHand->getComponent<SpriteComponent>().setTex("assets/Left_Ring.png");
				rightHand->getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			else if (targetText[i] == 's') {
				leftHand->getComponent<SpriteComponent>().setTex("assets/Left_Ring.png");
				rightHand->getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			else if (targetText[i] == 'x') {
				leftHand->getComponent<SpriteComponent>().setTex("assets/Left_Ring.png");
				rightHand->getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			// Left middle
			else if (targetText[i] == 'e') {
				leftHand->getComponent<SpriteComponent>().setTex("assets/Left_Middle.png");
				rightHand->getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			else if (targetText[i] == 'd') {
				leftHand->getComponent<SpriteComponent>().setTex("assets/Left_Middle.png");
				rightHand->getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			else if (targetText[i] == 'c') {
				leftHand->getComponent<SpriteComponent>().setTex("assets/Left_Middle.png");
				rightHand->getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			// Left index
			else if (targetText[i] == 'r') {
				leftHand->getComponent<SpriteComponent>().setTex("assets/Left_Index.png");
				rightHand->getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			else if (targetText[i] == 'f') {
				leftHand->getComponent<SpriteComponent>().setTex("assets/Left_Index.png");
				rightHand->getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			else if (targetText[i] == 'v') {
				leftHand->getComponent<SpriteComponent>().setTex("assets/Left_Index.png");
				rightHand->getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			else if (targetText[i] == 't') {
				leftHand->getComponent<SpriteComponent>().setTex("assets/Left_Index.png");
				rightHand->getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			else if (targetText[i] == 'g') {
				leftHand->getComponent<SpriteComponent>().setTex("assets/Left_Index.png");
				rightHand->getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}
			else if (targetText[i] == 'b') {
				leftHand->getComponent<SpriteComponent>().setTex("assets/Left_Index.png");
				rightHand->getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
			}

			// Right index
			else if (targetText[i] == 'y') {
				rightHand->getComponent<SpriteComponent>().setTex("assets/Right_Index.png");
				leftHand->getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			else if (targetText[i] == 'h') {
				rightHand->getComponent<SpriteComponent>().setTex("assets/Right_Index.png");
				leftHand->getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			else if (targetText[i] == 'n') {
				rightHand->getComponent<SpriteComponent>().setTex("assets/Right_Index.png");
				leftHand->getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			else if (targetText[i] == 'u') {
				rightHand->getComponent<SpriteComponent>().setTex("assets/Right_Index.png");
				leftHand->getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			else if (targetText[i] == 'j') {
				rightHand->getComponent<SpriteComponent>().setTex("assets/Right_Index.png");
				leftHand->getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			else if (targetText[i] == 'm') {
				rightHand->getComponent<SpriteComponent>().setTex("assets/Right_Index.png");
				leftHand->getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			// Right middle
			else if (targetText[i] == 'i') {
				rightHand->getComponent<SpriteComponent>().setTex("assets/Right_Middle.png");
				leftHand->getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			else if (targetText[i] == 'k') {
				rightHand->getComponent<SpriteComponent>().setTex("assets/Right_Middle.png");
				leftHand->getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			else if (targetText[i] == ',') {
				rightHand->getComponent<SpriteComponent>().setTex("assets/Right_Middle.png");
				leftHand->getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			// Right ring
			else if (targetText[i] == 'o') {
				rightHand->getComponent<SpriteComponent>().setTex("assets/Right_Ring.png");
				leftHand->getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			else if (targetText[i] == 'l') {
				rightHand->getComponent<SpriteComponent>().setTex("assets/Right_Ring.png");
				leftHand->getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			else if (targetText[i] == '.') {
				rightHand->getComponent<SpriteComponent>().setTex("assets/Right_Ring.png");
				leftHand->getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			// Right pinky
			else if (targetText[i] == 'p') {
				rightHand->getComponent<SpriteComponent>().setTex("assets/Right_Pinky.png");
				leftHand->getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
			}
			// Left thumb / Right thumb
			else if (targetText[i] == ' ') {
				leftHand->getComponent<SpriteComponent>().setTex("assets/Left_Thumb.png");
				rightHand->getComponent<SpriteComponent>().setTex("assets/Right_Thumb.png");
			}
		}
	}
}

// To reset hand sprites back to their default state (no fingers highlighted)
void ArcadeMode::resetHandSprites() {
	leftHand->getComponent<SpriteComponent>().setTex("assets/Left_Hand.png");
	rightHand->getComponent<SpriteComponent>().setTex("assets/Right_Hand.png");
}

// To update the barrier orb sprite based on how much damage has been taken
void ArcadeMode::updateBarrierDamage(int barrierHP) {
	// Convert HP (0–100) to damage level (0–10)
	int damageLevel = 10 - (barrierHP / 10);

	// Clamp damage level at 10
	damageLevel = std::max(0, std::min(damageLevel, 10));

	// Build file path
	std::string texturePath = "assets/Barrier_Orb_" + std::to_string(damageLevel) + ".png";

	// Set texture using c_str()
	barrier->getComponent<SpriteComponent>().setTex(texturePath.c_str());
}

// To check the current combo and update the on screen UI accordingly, and to activate the laser power-up when combo is at max
void ArcadeMode::checkCombo(const std::string& input, const std::string& target) {
	if (brokenCombo || input != target) {
		comboLevel = 0;
		laserReady = false;
	}
	else {
		comboLevel = std::min(comboLevel + 1, 6);
		if (comboLevel == 6) {
			laserReady = true;
		}
	}

	switch (comboLevel) {
	case 0: comboStatus = "";
		comboMeter->getComponent<SpriteComponent>().setTex("assets/Combo_Meter_0.png");
		break;
	case 1: comboStatus = "x1";
		comboMeter->getComponent<SpriteComponent>().setTex("assets/Combo_Meter_1.png");
		break;
	case 2: comboStatus = "x2";
		comboMeter->getComponent<SpriteComponent>().setTex("assets/Combo_Meter_2.png");
		break;
	case 3: comboStatus = "x3";
		comboMeter->getComponent<SpriteComponent>().setTex("assets/Combo_Meter_3.png");
		break;
	case 4: comboStatus = "x4";
		comboMeter->getComponent<SpriteComponent>().setTex("assets/Combo_Meter_4.png");
		break;
	case 5: comboStatus = "x5";
		comboMeter->getComponent<SpriteComponent>().setTex("assets/Combo_Meter_5.png");
		break;
	case 6: comboStatus = "MAX!";
		comboMeter->getComponent<SpriteComponent>().setTex("assets/Combo_Meter_6.png");
		break;
	}
}

// Fires laser power-up
void ArcadeMode::fireLaser() {
	if (laserActive) return; // to prevent multiple lasers...

	laserPowerUp = &manager.addEntity();
	laserPowerUp->addComponent<TransformComponent>(60, 32, 1472, 64, 1);
	laserPowerUp->addComponent<SpriteComponent>("assets/Laser-Sheet.png", true);
	laserPowerUp->addComponent<ColliderComponent>("laser");

	laserActive = true;
}

// Determing wrong letters to pass to results screen
std::string ArcadeMode::getFormattedWrongLetters() {
	// Move unique values from typedWrong to wrongResults
	wrongResults.clear(); // Clear previous values
	for (auto i : typedWrong) {
		if (wrongResults.find(i) == std::string::npos) { // Ensure no duplicates
			wrongResults.push_back(i);
		}
	}

	// Format wrongResults with commas
	formattedResults.str(""); // Clear previous results
	formattedResults.clear(); // Reset state
	for (size_t i = 0; i < wrongResults.size(); ++i) {
		formattedResults << wrongResults[i];
		if (i < wrongResults.size() - 1) { // Add comma for all but the last character
			formattedResults << ", ";
		}
	}

	return "Letters Typed Incorrectly: " + formattedResults.str();
}

// Determining accuracy to pass to results screen
std::string ArcadeMode::getFormattedAccuracy() {
	// Calculate accuracy
	if (levelTotalLetters > 0) {
		levelAccuracy = (static_cast<double>(levelCorrectLetters) / levelTotalLetters) * 100;
	}

	levelAccuracyStream.str(""); // Clear previous accuracy
	levelAccuracyStream.clear(); // Reset state
	levelAccuracyStream << "Level Accuracy: " << std::fixed << std::setprecision(2) << levelAccuracy << "%";
	overallAccuracy = levelAccuracyStream.str(); // Assign the formatted string

	return overallAccuracy;
}

// Determining accuracy to pass to game over screen
std::string ArcadeMode::getOverallAccuracy() {
	// Calculate overall accuracy
	if (finalTotalLetters > 0) {
		levelAccuracy = (static_cast<double>(finalCorrectLetters) / finalTotalLetters) * 100;
	}

	levelAccuracyStream.str(""); // Clear previous accuracy
	levelAccuracyStream.clear(); // Reset state
	levelAccuracyStream << "Overall Accuracy: " << std::fixed << std::setprecision(2) << levelAccuracy << "%";
	overallAccuracy = levelAccuracyStream.str(); // Assign the formatted string

	return overallAccuracy;
}

void ArcadeMode::setPhase(Phase newPhase) {
	currentPhase = newPhase;
}