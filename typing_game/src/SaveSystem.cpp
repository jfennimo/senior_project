#include "SaveSystem.h"
#include <fstream>
#include <iostream>

bool SaveSystem::saveToFile(const std::string& filename, const SaveData& data) {
	std::ofstream outFile(filename);
	if (!outFile.is_open()) {
		std::cerr << "Failed to save game data!" << std::endl;
		return false;
	}

	outFile << data.lessonGamesPlayed << "\n";
	outFile << data.lessonAccuracyTotal << "\n";
	outFile << data.arcadeGamesPlayed << "\n";
	outFile << data.arcadeAccuracyTotal << "\n";
	outFile << data.arcadeHighestLevel << "\n";
	outFile << data.wpmGamesPlayed << "\n";
	outFile << data.wpmAccuracyTotal << "\n";
	outFile << data.highestWpm << "\n";

	// Save lifetime wrong characters
	outFile << data.lifetimeWrongCharacters.size() << "\n";
	for (const auto& [ch, count] : data.lifetimeWrongCharacters) {
		std::string encodedChar = (ch == ' ') ? "<space>" : std::string(1, ch);
		outFile << encodedChar << " " << count << "\n";
	}

	// Save lesson progress
	outFile << data.lessonProgressMap.size() << "\n";
	for (const auto& [difficulty, progress] : data.lessonProgressMap) {
		outFile << static_cast<int>(difficulty) << " "
			<< progress.passed << " "
			<< progress.fullyCompleted << " "
			<< progress.bestAccuracy << " "
			<< progress.bestTime << "\n";
	}

	outFile.close();
	return true;
}

bool SaveSystem::loadFromFile(const std::string& filename, SaveData& data) {
	std::ifstream inFile(filename);
	if (!inFile.is_open()) {
		std::cerr << "No save file found. Starting fresh (or make that... flesh)!" << std::endl;
		return false;
	}

	inFile >> data.lessonGamesPlayed;
	inFile >> data.lessonAccuracyTotal;
	inFile >> data.arcadeGamesPlayed;
	inFile >> data.arcadeAccuracyTotal;
	inFile >> data.arcadeHighestLevel;
	inFile >> data.wpmGamesPlayed;
	inFile >> data.wpmAccuracyTotal;
	inFile >> data.highestWpm;

	// Load lifetime wrong characters
	size_t wrongCount;
	if (!(inFile >> wrongCount)) return false; // Safety measure for corrupt save
	data.lifetimeWrongCharacters.clear();
	for (size_t i = 0; i < wrongCount; ++i) {
		std::string chStr;
		int count;
		if (!(inFile >> chStr >> count)) return false;

		char ch = (chStr == "<space>") ? ' ' : chStr[0];
		data.lifetimeWrongCharacters[ch] = count;
	}

	// Load lesson progress
	size_t lessonCount;
	inFile >> lessonCount;
	data.lessonProgressMap.clear();
	for (size_t i = 0; i < lessonCount; ++i) {
		int difficultyInt;
		bool passed, fullyCompleted;
		int accuracy, time;

		inFile >> difficultyInt >> passed >> fullyCompleted >> accuracy >> time;
		WordListManager::Difficulty difficulty = static_cast<WordListManager::Difficulty>(difficultyInt);
		data.lessonProgressMap[difficulty] = { passed, fullyCompleted, accuracy, time };
	}

	inFile.close();
	return true;
}
