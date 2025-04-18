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
		int highestWpm = 0;
		int finalCorrectLetters = 0;
		int finalTotalLetters = 0;
		std::unordered_map<char, int> lifetimeWrongLetters;
		std::unordered_map<WordListManager::Difficulty, LessonProgress> lessonProgressMap;
	};

	static bool saveToFile(const std::string& filename, const SaveData& data);
	static bool loadFromFile(const std::string& filename, SaveData& data);
};