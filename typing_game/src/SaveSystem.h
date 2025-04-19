#pragma once
#include <unordered_map>
#include <string>
#include <fstream>
#include <iostream>
#include "WordListManager.h"

class SaveSystem {
public:
	struct LessonProgress {
		bool passed = false;
		bool fullyCompleted = false;
		int bestAccuracy = 0;
		int bestTime = 0;
	};

	struct SaveData {
		// Lessons
		int lessonGamesPlayed = 0;
		float lessonAccuracyTotal = 0.0f;
		std::unordered_map<WordListManager::Difficulty, LessonProgress> lessonProgressMap;

		// Arcade
		int arcadeGamesPlayed = 0;
		float arcadeAccuracyTotal = 0.0f;
		int arcadeHighestLevel = 0;

		// WPM
		int wpmGamesPlayed = 0;
		float wpmAccuracyTotal = 0.0f;
		int highestWpm = 0;

		// Overall
		std::unordered_map<char, int> lifetimeWrongCharacters;
	};

	static bool saveToFile(const std::string& filename, const SaveData& data);
	static bool loadFromFile(const std::string& filename, SaveData& data);
};