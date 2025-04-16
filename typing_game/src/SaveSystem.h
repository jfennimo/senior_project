#pragma once
#include <unordered_map>
#include <string>

class SaveSystem {
public:
	struct SaveData {
		int highestWpm = 0;
		int finalCorrectLetters = 0;
		int finalTotalLetters = 0;
		std::unordered_map<char, int> lifetimeWrongLetters;
	};

	static bool saveToFile(const std::string& filename, const SaveData& data);
	static bool loadFromFile(const std::string& filename, SaveData& data);
};