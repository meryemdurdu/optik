#pragma once

#include <map>
#include <string>
#include <vector>

#include <opencv2/opencv.hpp>

class ROIDetector {
public:
    enum class RegionType { GRID, COLUMN };

    struct RegionDef {
        std::string name;
        float rectPct[4];  // x, y, w, h
        int rows;
        int cols;
        RegionType type;
    };

    struct QuestionDetail {
        int questionNumber = 0;
        char markedAnswer = '-';
        char correctAnswer = '-';
        bool isCorrect = false;
        double fillRatio = 0.0;
    };

    ROIDetector();

    std::map<std::string, std::string> process(const cv::Mat& warped, cv::Mat& debugOut);

    std::map<std::string, std::vector<QuestionDetail>> processWithDetails(
        const cv::Mat& warped,
        cv::Mat& debugOut,
        const std::map<std::string, std::map<int, char>>& answerKey);

    void setFillThreshold(double threshold);
    double getFillThreshold() const { return fillThreshold_; }

    void setDebugMode(bool enabled);
    bool debugMode() const { return debugMode_; }

    cv::Mat getLastDebugVisualization() const;

private:
    std::vector<QuestionDetail> analyzeGridWithDetails(const cv::Mat& roiGray,
                                                       int rows,
                                                       int cols,
                                                       const std::map<int, char>& correctAnswers,
                                                       char firstLabel = 'A');

    std::vector<RegionDef> regions_;
    double fillThreshold_ = 0.20;
    bool debugMode_ = false;
    cv::Mat lastDebug_;
};
