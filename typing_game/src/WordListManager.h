#pragma once

#include <vector>
#include <string>
#include <unordered_map>

class WordListManager {
public:
	enum Difficulty { LESSON_0, LESSON_1, LESSON_2, LESSON_3, LESSON_4, LESSON_5, LESSON_6, LESSON_7, LESSON_8, LESSON_9, WPM, EASY, MEDIUM, HARD, BONUSLEFT, BONUSRIGHT };

	WordListManager();

	// Get words from text file based on difficulty
	std::vector<std::string> getWords(Difficulty difficulty, size_t numWords) const;
	std::vector<std::string> getRandomWords(Difficulty difficulty, size_t numWords) const;

private:
	std::unordered_map<Difficulty, std::vector<std::string>> wordLists;

	void loadFromFile(const std::string& filename, Difficulty difficulty);

};