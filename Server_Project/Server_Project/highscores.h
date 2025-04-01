#pragma once
#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <algorithm>
struct PlayerScore
{
    std::string playerName;
    uint32_t score;

    // Constructor for easy creation
    PlayerScore(const std::string &name = "", uint32_t playerScore = 0)
        : playerName(name), score(playerScore)
    {
    }

    // Operator for sorting scores (highest first)
    bool operator<(const PlayerScore &other) const
    {
        return score > other.score; // Note: reversed for descending order
    }
};

// Vector to store the top 5 scores
std::vector<PlayerScore> topScores;
const std::string HIGH_SCORE_FILENAME = "highscores.txt";
const int MAX_HIGH_SCORES = 5;

// Load high scores from file
void LoadHighScores()
{
    topScores.clear();

    std::filesystem::path currentPath = std::filesystem::current_path();
    std::filesystem::path scoreFilePath = currentPath / HIGH_SCORE_FILENAME;

    // Create file if it doesn't exist
    std::ifstream scoreFile(scoreFilePath);
    if (!scoreFile.is_open())
    {
        std::cout << "High score file not found, will create when scores are available." << std::endl;
        return;
    }

    std::string line;
    while (std::getline(scoreFile, line) && topScores.size() < MAX_HIGH_SCORES)
    {
        std::istringstream iss(line);
        std::string name;
        uint32_t score;

        if (iss >> name >> score)
        {
            topScores.push_back(PlayerScore(name, score));
        }
    }

    scoreFile.close();

    // Sort scores (highest first)
    std::sort(topScores.begin(), topScores.end());

    std::cout << "Loaded " << topScores.size() << " high scores." << std::endl;
}

// Save high scores to file
void SaveHighScores()
{
    std::filesystem::path currentPath = std::filesystem::current_path();
    std::filesystem::path scoreFilePath = currentPath / HIGH_SCORE_FILENAME;

    std::ofstream scoreFile(scoreFilePath);
    if (!scoreFile.is_open())
    {
        std::cerr << "Error: Unable to open high score file for writing." << std::endl;
        return;
    }

    for (const auto &score : topScores)
    {
        scoreFile << score.playerName << " " << score.score << std::endl;
    }

    scoreFile.close();
    std::cout << "High scores saved to " << HIGH_SCORE_FILENAME << std::endl;

}
// Update high scores with a new score
bool UpdateHighScores(const std::string &playerName, uint32_t score)
{
    bool scoreAdded = false;

    // If we have fewer than MAX_HIGH_SCORES, add the score
    if (topScores.size() < MAX_HIGH_SCORES)
    {
        topScores.push_back(PlayerScore(playerName, score));
        scoreAdded = true;
    }
    // Otherwise, check if this score is higher than the lowest current score
    else if (score > topScores.back().score)
    {
        topScores.back() = PlayerScore(playerName, score);
        scoreAdded = true;
    }

    // If the score was added or replaced, sort the scores and save to file
    if (scoreAdded)
    {
        std::sort(topScores.begin(), topScores.end());
        SaveHighScores();
        std::cout << "Player " << playerName << " added to high scores with " << score << " points!" << std::endl;
    }

    return scoreAdded;
}

// Get formatted high scores as a string
std::string GetHighScoresMessage()
{
    std::ostringstream oss;
    oss << "TOP " << MAX_HIGH_SCORES << " HIGH SCORES:\n";

    for (size_t i = 0; i < topScores.size(); ++i)
    {
        oss << (i + 1) << ". " << topScores[i].playerName << ": " << topScores[i].score << "\n";
    }

    return oss.str();
}