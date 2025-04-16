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

	for (const auto& [ch, count] : data.lifetimeWrongLetters) {
		outFile << ch << " " << count << "\n";
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

	data.lifetimeWrongLetters.clear();
	char ch;
	int count;
	while (inFile >> ch >> count) {
		data.lifetimeWrongLetters[ch] = count;
	}

	inFile.close();
	return true;
}
