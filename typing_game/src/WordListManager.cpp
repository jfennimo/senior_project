#include "WordListManager.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <random>

WordListManager::WordListManager() {
	loadFromFile("wordlists/wpm.txt", WPM);
	loadFromFile("wordlists/easy.txt", EASY);
	loadFromFile("wordlists/medium.txt", MEDIUM);
	loadFromFile("wordlists/hard.txt", HARD);
	loadFromFile("wordlists/bonusLeft.txt", BONUSLEFT);
	loadFromFile("wordlists/bonusRight.txt", BONUSRIGHT);
}

// Load words from text file and shuffle them
void WordListManager::loadFromFile(const std::string& filename, Difficulty difficulty) {
	std::ifstream file(filename);
	if (!file.is_open()) {
		std::cerr << "Error! Could not open file: " << filename << std::endl;
		return;
	}

	std::string word;
	while (std::getline(file, word)) {
		if (!word.empty()) {
			wordLists[difficulty].push_back(word);
		}
	}

	file.close();

	// Shuffle words
	std::random_device rd;
	std::mt19937 rng(rd());
	std::shuffle(wordLists[difficulty].begin(), wordLists[difficulty].end(), rng);
}

// Get multiple random words from the list
std::vector<std::string> WordListManager::getRandomWords(Difficulty difficulty, size_t numWords) const {
	std::vector<std::string> selectedWords;

	if (wordLists.at(difficulty).empty()) {
		return selectedWords; // Return empty if no words are available
	}

	std::random_device rd;
	std::mt19937 rng(rd());
	std::uniform_int_distribution<size_t> dist(0, wordLists.at(difficulty).size() - 1);

	for (size_t i = 0; i < numWords; ++i) {
		selectedWords.push_back(wordLists.at(difficulty)[dist(rng)]);
	}

	return selectedWords;
}