#pragma once

#include <map>
#include <string>
#include <vector>

struct AnswerKey {
    struct QuestionAnswer {
        std::string subject;
        int questionNumber;
        char correctAnswer;
    };

    struct ScoreResult {
        double score = 0.0;
        int correctAnswers = 0;
        int wrongAnswers = 0;
        int emptyAnswers = 0;
        int totalQuestions = 0;
        std::map<std::string, int> subjectScores;
    };

    void loadAnswerKey(const std::vector<QuestionAnswer>& data);

    ScoreResult calculateScore(const std::map<std::string, std::string>& studentAnswers) const;

    const std::map<std::string, std::map<int, char>>& answers() const { return answers_; }

private:
    std::map<std::string, std::map<int, char>> answers_;
    int totalQuestions_ = 0;
};
