#include "WordListManager.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <random>

WordListManager::WordListManager() {
	loadFromFile("wordlists/lesson_0.txt", LESSON_0);
	loadFromFile("wordlists/lesson_1.txt", LESSON_1);
	loadFromFile("wordlists/lesson_2.txt", LESSON_2);
	loadFromFile("wordlists/lesson_3.txt", LESSON_3);
	loadFromFile("wordlists/lesson_4.txt", LESSON_4);
	loadFromFile("wordlists/lesson_5.txt", LESSON_5);
	loadFromFile("wordlists/lesson_6.txt", LESSON_6);
	loadFromFile("wordlists/lesson_7.txt", LESSON_7);
	loadFromFile("wordlists/lesson_8.txt", LESSON_8);
	loadFromFile("wordlists/lesson_9.txt", LESSON_9);

	loadFromFile("wordlists/wpm.txt", WPM);
	loadFromFile("wordlists/easy.txt", EASY);
	loadFromFile("wordlists/medium.txt", MEDIUM);
	loadFromFile("wordlists/hard.txt", HARD);
	loadFromFile("wordlists/bonusLeft.txt", BONUSLEFT);
	loadFromFile("wordlists/bonusRight.txt", BONUSRIGHT);
}

// Load words from text file
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
}

// Get multiple words from the list, in order
std::vector<std::string> WordListManager::getWords(Difficulty difficulty, size_t numWords) const {
	std::vector<std::string> selectedWords;

	const auto& words = wordLists.at(difficulty);

	if (words.empty()) return selectedWords; // return empty if no words are available

	// Clamp to max available
	numWords = std::min(numWords, words.size());

	selectedWords.insert(selectedWords.end(), words.begin(), words.begin() + numWords);
	return selectedWords;
}

// Get multiple words from the list, randomized
std::vector<std::string> WordListManager::getRandomWords(Difficulty difficulty, size_t numWords) const {
	std::vector<std::string> selectedWords;

	const auto& words = wordLists.at(difficulty);

	if (words.empty()) return selectedWords; // return empty if no words are available

	std::random_device rd;
	std::mt19937 rng(rd());
	std::uniform_int_distribution<size_t> dist(0, wordLists.at(difficulty).size() - 1);

	for (size_t i = 0; i < numWords; ++i) {
		selectedWords.push_back(wordLists.at(difficulty)[dist(rng)]);
	}

	return selectedWords;
}