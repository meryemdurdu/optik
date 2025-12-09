#include "ROIDetector.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>

#include "core/Logger.hpp"

namespace {

cv::Rect rectPct(const cv::Mat& img, float x, float y, float w, float h) {
    int X = static_cast<int>(x * img.cols);
    int Y = static_cast<int>(y * img.rows);
    int W = static_cast<int>(w * img.cols);
    int H = static_cast<int>(h * img.rows);
    return cv::Rect(X, Y, W, H);
}

cv::Rect shrinkRect(const cv::Rect& r, int marginX, int marginY) {
    cv::Rect shrunk = r;
    shrunk.x += marginX;
    shrunk.y += marginY;
    shrunk.width -= 2 * marginX;
    shrunk.height -= 2 * marginY;
    if (shrunk.width <= 0 || shrunk.height <= 0) {
        return r;
    }
    return shrunk;
}

double computeCircularFill(const cv::Mat& binary, double maskRatio) {
    if (binary.empty())
        return 0.0;

    cv::Mat mask = cv::Mat::zeros(binary.size(), CV_8U);
    int diameter = std::min(binary.cols, binary.rows);
    int radius = std::max(1, static_cast<int>(diameter * maskRatio));
    cv::Point center(binary.cols / 2, binary.rows / 2);
    cv::circle(mask, center, radius, 255, -1);

    cv::Mat masked;
    cv::bitwise_and(binary, mask, masked);

    double denom = static_cast<double>(cv::countNonZero(mask));
    if (denom < 1.0)
        return 0.0;

    double numer = static_cast<double>(cv::countNonZero(masked));
    return numer / denom;
}

std::string detectOMRGrid(const cv::Mat& roiGray,
                          int rows,
                          int cols,
                          double fillThreshold,
                          double cropRatio,
                          double confidenceGap,
                          double maskRatio,
                          double minAbsoluteFill,
                          std::vector<double>* bestVals = nullptr) {
    cv::Mat blurImg, thr;
    cv::Mat normGray;
    cv::equalizeHist(roiGray, normGray);
    cv::GaussianBlur(normGray, blurImg, cv::Size(3, 3), 0);
    cv::adaptiveThreshold(blurImg, thr, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV,
                          25, 5);
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
    cv::morphologyEx(thr, thr, cv::MORPH_OPEN, kernel);
    cv::dilate(thr, thr, kernel, cv::Point(-1, -1), 1);

    int cellH = roiGray.rows / rows;
    int cellW = roiGray.cols / cols;

    std::string result;
    result.reserve(rows * 2);

    if (bestVals)
        bestVals->assign(rows, 0.0);

    for (int r = 0; r < rows; ++r) {
        int bestCol = -1;
        double bestVal = 0.0;
        double secondVal = 0.0;

        for (int c = 0; c < cols; ++c) {
            int x = c * cellW;
            int y = r * cellH;
            cv::Rect cell(x, y, cellW, cellH);
            int marginX = std::max(1, static_cast<int>(cellW * cropRatio));
            int marginY = std::max(1, static_cast<int>(cellH * cropRatio));
            cv::Rect core = shrinkRect(cell, marginX, marginY);
            core &= cv::Rect(0, 0, thr.cols, thr.rows);
            if (core.width <= 0 || core.height <= 0)
                core = cell;

            cv::Mat sub = thr(core).clone();
            double filled = computeCircularFill(sub, maskRatio);

            if (filled > bestVal) {
                secondVal = bestVal;
                bestVal = filled;
                bestCol = c;
            } else if (filled > secondVal) {
                secondVal = filled;
            }
        }

        if (bestVals)
            (*bestVals)[r] = bestVal;

        bool highFill = bestVal >= fillThreshold;
        bool dominant = ((bestVal - secondVal) >= confidenceGap) &&
                        (bestVal >= std::max(minAbsoluteFill, fillThreshold * 0.4));
        bool confident = (bestCol >= 0) && (highFill || dominant);

        if (!confident || bestCol < 0) {
            result += "-";
        } else {
            char mark = static_cast<char>('A' + bestCol);
            result += mark;
        }

        if (r != rows - 1)
            result += ",";
    }

    return result;
}

std::string detectSingleColumn(const cv::Mat& roiGray,
                               int rows,
                               double fillThreshold,
                               double cropRatio,
                               double maskRatio,
                               double minAbsoluteFill) {
    cv::Mat blurImg, thr;
    cv::Mat normGray;
    cv::equalizeHist(roiGray, normGray);
    cv::GaussianBlur(normGray, blurImg, cv::Size(3, 3), 0);
    cv::adaptiveThreshold(blurImg, thr, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV,
                          25, 5);
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
    cv::morphologyEx(thr, thr, cv::MORPH_OPEN, kernel);
    cv::dilate(thr, thr, kernel, cv::Point(-1, -1), 1);

    int cellH = roiGray.rows / rows;
    int cellW = roiGray.cols;

    int bestIdx = -1;
    double bestVal = 0.0;

    for (int r = 0; r < rows; ++r) {
        cv::Rect cell(0, r * cellH, cellW, cellH);
        int marginX = std::max(1, static_cast<int>(cellW * cropRatio));
        int marginY = std::max(1, static_cast<int>(cellH * cropRatio));
        cv::Rect core = shrinkRect(cell, marginX, marginY);
        core &= cv::Rect(0, 0, thr.cols, thr.rows);
        if (core.width <= 0 || core.height <= 0)
            core = cell;

        cv::Mat sub = thr(core).clone();
        double filled = computeCircularFill(sub, maskRatio);
        if (filled > bestVal) {
            bestVal = filled;
            bestIdx = r;
        }
    }

    if (bestIdx < 0)
        return "-";
    bool highFill = bestVal >= fillThreshold;
    bool dominant = bestVal >= std::max(minAbsoluteFill, fillThreshold * 0.4);
    return (highFill || dominant) ? std::to_string(bestIdx) : "-";
}

}  // namespace

ROIDetector::ROIDetector() {
    regions_.push_back({"tc_kimlik", {0.000f, 0.245f, 0.273f, 0.213f}, 11, 10, RegionType::GRID});
    regions_.push_back({"ogrenci_no", {0.279f, 0.245f, 0.136f, 0.210f}, 10, 10, RegionType::GRID});
    regions_.push_back({"adi_soyadi", {0.000f, 0.459f, 0.507f, 0.541f}, 30, 12, RegionType::GRID});

    regions_.push_back({"turkce", {0.53f, 0.23f, 0.12f, 0.37f}, 20, 5, RegionType::GRID});
    regions_.push_back({"sosyal", {0.65f, 0.23f, 0.12f, 0.37f}, 20, 5, RegionType::GRID});
    regions_.push_back({"din", {0.77f, 0.23f, 0.12f, 0.37f}, 20, 5, RegionType::GRID});
    regions_.push_back({"ingilizce", {0.89f, 0.23f, 0.12f, 0.37f}, 20, 5, RegionType::GRID});
    regions_.push_back({"matematik", {0.64f, 0.62f, 0.12f, 0.36f}, 20, 5, RegionType::GRID});
    regions_.push_back({"fen", {0.76f, 0.62f, 0.12f, 0.36f}, 20, 5, RegionType::GRID});
}

void ROIDetector::setFillThreshold(double threshold) {
    fillThreshold_ = std::clamp(threshold, 0.05, 0.95);
    LOG_INFO("ROI", "Doluluk esigi -> " + std::to_string(fillThreshold_));
}

void ROIDetector::setDebugMode(bool enabled) {
    debugMode_ = enabled;
    LOG_INFO("ROI", std::string("Debug modu ") + (enabled ? "ACIK" : "KAPALI"));
}

cv::Mat ROIDetector::getLastDebugVisualization() const {
    return lastDebug_.clone();
}

std::map<std::string, std::string> ROIDetector::process(const cv::Mat& warped, cv::Mat& debugOut) {
    logging::Logger::ScopedTimer timer("ROI", "ROIDetector::process");
    CV_Assert(!warped.empty());

    cv::Mat gray;
    if (warped.channels() == 3) {
        cv::cvtColor(warped, gray, cv::COLOR_BGR2GRAY);
        debugOut = warped.clone();
    } else {
        gray = warped.clone();
        cv::cvtColor(warped, debugOut, cv::COLOR_GRAY2BGR);
    }

    std::map<std::string, std::string> out;
    LOG_INFO("ROI", "ROI sayisi: " + std::to_string(regions_.size()));

    for (const auto& reg : regions_) {
        cv::Rect roi = rectPct(gray, reg.rectPct[0], reg.rectPct[1], reg.rectPct[2], reg.rectPct[3]);
        roi &= cv::Rect(0, 0, gray.cols, gray.rows);
        if (roi.width <= 0 || roi.height <= 0) {
            LOG_ERROR("ROI", "Region hatali boyut: " + reg.name);
            continue;
        }

        cv::Mat sub = gray(roi).clone();
        std::string val;

        if (reg.type == RegionType::GRID) {
            val = detectOMRGrid(sub, reg.rows, reg.cols, fillThreshold_, bubbleCropRatio_,
                                confidenceGap_, bubbleMaskRadiusRatio_, minAbsoluteFill_);
        } else {
            val = detectSingleColumn(sub, reg.rows, fillThreshold_, bubbleCropRatio_,
                                     bubbleMaskRadiusRatio_, minAbsoluteFill_);
        }

        out[reg.name] = val;

        cv::rectangle(debugOut, roi, cv::Scalar(0, 255, 0), 2);
        cv::putText(debugOut, reg.name, roi.tl() + cv::Point(3, 15), cv::FONT_HERSHEY_SIMPLEX, 0.45,
                    cv::Scalar(0, 0, 0), 1);

        LOG_DEBUG("ROI", reg.name + " ROI isleme tamamlandi");
    }

    lastDebug_ = debugOut.clone();
    return out;
}

std::vector<ROIDetector::QuestionDetail> ROIDetector::analyzeGridWithDetails(
    const cv::Mat& roiGray,
    int rows,
    int cols,
    const std::map<int, char>& correctAnswers,
    char firstLabel) {
    std::vector<ROIDetector::QuestionDetail> details;
    cv::Mat blurImg, thr;
    cv::Mat normGray;
    cv::equalizeHist(roiGray, normGray);
    cv::GaussianBlur(normGray, blurImg, cv::Size(3, 3), 0);
    cv::adaptiveThreshold(blurImg, thr, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV,
                          25, 5);
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
    cv::morphologyEx(thr, thr, cv::MORPH_OPEN, kernel);
    cv::dilate(thr, thr, kernel, cv::Point(-1, -1), 1);

    int cellH = roiGray.rows / rows;
    int cellW = roiGray.cols / cols;

    for (int r = 0; r < rows; ++r) {
        int bestCol = -1;
        double bestVal = 0.0;
        double secondVal = 0.0;

        for (int c = 0; c < cols; ++c) {
            int x = c * cellW;
            int y = r * cellH;
            cv::Rect cell(x, y, cellW, cellH);
            int marginX = std::max(1, static_cast<int>(cellW * bubbleCropRatio_));
            int marginY = std::max(1, static_cast<int>(cellH * bubbleCropRatio_));
            cv::Rect core = shrinkRect(cell, marginX, marginY);
            core &= cv::Rect(0, 0, thr.cols, thr.rows);
            if (core.width <= 0 || core.height <= 0)
                core = cell;

            cv::Mat sub = thr(core).clone();

            double filled = computeCircularFill(sub, bubbleMaskRadiusRatio_);
            if (filled > bestVal) {
                secondVal = bestVal;
                bestVal = filled;
                bestCol = c;
            } else if (filled > secondVal) {
                secondVal = filled;
            }
        }

        QuestionDetail qd;
        qd.questionNumber = r;
        qd.fillRatio = bestVal;

        bool highFill = bestVal >= fillThreshold_;
        bool dominant = ((bestVal - secondVal) >= confidenceGap_) &&
                        (bestVal >= std::max(minAbsoluteFill_, fillThreshold_ * 0.5));
        bool confident = (bestCol >= 0) && (highFill || dominant);
        if (!confident) {
            qd.markedAnswer = '-';
        } else {
            qd.markedAnswer = static_cast<char>(firstLabel + bestCol);
        }

        auto it = correctAnswers.find(r);
        if (it != correctAnswers.end()) {
            qd.correctAnswer = it->second;
            qd.isCorrect = (qd.markedAnswer == qd.correctAnswer);
        } else {
            qd.correctAnswer = '-';
            qd.isCorrect = false;
        }

        if (debugMode_) {
            std::ostringstream dbg;
            dbg << "Soru " << (r + 1) << " -> best=" << std::fixed << std::setprecision(3)
                << bestVal << " second=" << secondVal << " mark=" << qd.markedAnswer;
            LOG_DEBUG("BUBBLE", dbg.str());
        }

        details.push_back(qd);
    }

    return details;
}

std::map<std::string, std::vector<ROIDetector::QuestionDetail>>
ROIDetector::processWithDetails(
    const cv::Mat& warped,
    cv::Mat& debugOut,
    const std::map<std::string, std::map<int, char>>& answerKey) {
    logging::Logger::ScopedTimer timer("BUBBLE", "ROIDetector::processWithDetails");
    CV_Assert(!warped.empty());

    cv::Mat gray;
    if (warped.channels() == 3) {
        cv::cvtColor(warped, gray, cv::COLOR_BGR2GRAY);
        debugOut = warped.clone();
    } else {
        gray = warped.clone();
        cv::cvtColor(warped, debugOut, cv::COLOR_GRAY2BGR);
    }

    std::map<std::string, std::vector<QuestionDetail>> allDetails;

    for (const auto& reg : regions_) {
        if (reg.type != RegionType::GRID)
            continue;

        auto akIt = answerKey.find(reg.name);
        if (akIt == answerKey.end())
            continue;

        cv::Rect roi = rectPct(gray, reg.rectPct[0], reg.rectPct[1], reg.rectPct[2], reg.rectPct[3]);
        roi &= cv::Rect(0, 0, gray.cols, gray.rows);
        if (roi.width <= 0 || roi.height <= 0)
            continue;

        cv::Mat sub = gray(roi).clone();
        auto details = analyzeGridWithDetails(sub, reg.rows, reg.cols, akIt->second);
        allDetails[reg.name] = details;

        int detected = 0;
        for (const auto& q : details) {
            if (q.markedAnswer != '-')
                ++detected;
        }

        LOG_INFO("BUBBLE", reg.name + " dersinde " + std::to_string(detected) +
                                 " soru isaretlendi");

        if (debugMode_) {
            cv::rectangle(debugOut, roi, cv::Scalar(255, 0, 0), 2);
            std::stringstream ss;
            ss << detected << "/" << reg.rows;
            cv::putText(debugOut, ss.str(), roi.tl() + cv::Point(5, 20),
                        cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 0, 255), 2);
        }
    }

    lastDebug_ = debugOut.clone();
    return allDetails;
}
