#include "AnswerKey.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

#include "core/Logger.hpp"

namespace {

std::vector<char> splitAnswers(const std::string& raw) {
    std::vector<char> tokens;
    std::stringstream ss(raw);
    std::string token;
    while (std::getline(ss, token, ',')) {
        if (token.empty()) {
            tokens.push_back('-');
            continue;
        }
        char c = static_cast<char>(std::toupper(static_cast<unsigned char>(token[0])));
        tokens.push_back(c);
    }
    return tokens;
}

}  // namespace

void AnswerKey::loadAnswerKey(const std::vector<QuestionAnswer>& data) {
    answers_.clear();
    totalQuestions_ = 0;
    for (const auto& item : data) {
        answers_[item.subject][item.questionNumber] = item.correctAnswer;
        ++totalQuestions_;
    }
    LOG_INFO("ANSWER_KEY", "Cevap anahtari yüklendi. Toplam soru: " +
                                  std::to_string(totalQuestions_));
}

AnswerKey::ScoreResult AnswerKey::calculateScore(
    const std::map<std::string, std::string>& studentAnswers) const {
    ScoreResult result;
    result.totalQuestions = totalQuestions_;
    if (totalQuestions_ == 0) {
        LOG_WARN("SCORE", "Cevap anahtari bos, skor hesaplanamiyor");
        return result;
    }

    for (const auto& subjectEntry : answers_) {
        result.subjectScores[subjectEntry.first] = 0;
    }

    for (const auto& sa : studentAnswers) {
        auto answerIt = answers_.find(sa.first);
        if (answerIt == answers_.end()) {
            LOG_DEBUG("SCORE", "Cevap anahtarinda olmayan ders: " + sa.first);
            continue;
        }

        auto tokens = splitAnswers(sa.second);
        for (size_t idx = 0; idx < tokens.size(); ++idx) {
            char studentChoice = tokens[idx];
            auto correctIt = answerIt->second.find(static_cast<int>(idx));
            char correctChoice = correctIt != answerIt->second.end() ? correctIt->second : '-';

            if (studentChoice == '-' || studentChoice == ' ') {
                result.emptyAnswers++;
                continue;
            }

            if (studentChoice == correctChoice) {
                result.correctAnswers++;
                result.subjectScores[sa.first]++;
            } else {
                result.wrongAnswers++;
            }
        }
    }

    if (result.correctAnswers > 0 || result.wrongAnswers > 0 || result.emptyAnswers > 0) {
        result.score = (static_cast<double>(result.correctAnswers) /
                        static_cast<double>(std::max(1, totalQuestions_))) *
                       100.0;
    }

    std::stringstream summary;
    summary << "Skor: " << result.score << " | D:" << result.correctAnswers
            << " Y:" << result.wrongAnswers << " B:" << result.emptyAnswers;
    LOG_INFO("SCORE", summary.str());

    return result;
}
