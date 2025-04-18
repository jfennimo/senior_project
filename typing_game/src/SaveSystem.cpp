#include "SaveSystem.h"
#include <fstream>
#include <iostream>

bool SaveSystem::saveToFile(const std::string& filename, const SaveData& data) {
	std::ofstream outFile(filename);
	if (!outFile.is_open()) {
		std::cerr << "Failed to save game data!" << std::endl;
		return false;
	}

	outFile << data.highestWpm << "\n";
	outFile << data.finalCorrectLetters << "\n";
	outFile << data.finalTotalLetters << "\n";

	// Save lifetime wrong letters
	outFile << data.lifetimeWrongLetters.size() << "\n";
	for (const auto& [ch, count] : data.lifetimeWrongLetters) {
		outFile << ch << " " << count << "\n";
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
		std::cerr << "No save file found. Starting fresh!" << std::endl;
		return false;
	}

	inFile >> data.highestWpm;
	inFile >> data.finalCorrectLetters;
	inFile >> data.finalTotalLetters;

	// Load lifetime wrong letters
	size_t wrongCount;
	inFile >> wrongCount;
	data.lifetimeWrongLetters.clear();
	for (size_t i = 0; i < wrongCount; ++i) {
		char ch;
		int count;
		inFile >> ch >> count;
		data.lifetimeWrongLetters[ch] = count;
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
