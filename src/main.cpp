#include <chrono>
#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <opencv2/opencv.hpp>

#include "AnswerKey.hpp"
#include "ROIDetector.hpp"
#include "core/Logger.hpp"
#include "core/PerspectiveCorrector.hpp"

using namespace std::chrono;

void drawScoreOverlay(cv::Mat& frame, const AnswerKey::ScoreResult& score) {
    if (score.totalQuestions == 0)
        return;

    int startY = 30;
    int lineHeight = 35;

    cv::Rect bgRect(10, 10, 360, 250);
    cv::Mat roi = frame(bgRect);
    cv::Mat color(roi.size(), CV_8UC3, cv::Scalar(0, 0, 0));
    cv::addWeighted(color, 0.6, roi, 0.4, 0, roi);

    cv::putText(frame, "CANLI PUAN", cv::Point(20, startY), cv::FONT_HERSHEY_SIMPLEX, 0.7,
                cv::Scalar(0, 255, 255), 2);
    startY += lineHeight;

    std::stringstream ss;
    ss << "SKOR: " << std::fixed << std::setprecision(1) << score.score << " / 100";
    cv::putText(frame, ss.str(), cv::Point(20, startY), cv::FONT_HERSHEY_SIMPLEX, 0.8,
                cv::Scalar(0, 255, 0), 2);
    startY += lineHeight;

    ss.str("");
    ss << "D:" << score.correctAnswers << " Y:" << score.wrongAnswers << " B:" << score.emptyAnswers;
    cv::putText(frame, ss.str(), cv::Point(20, startY), cv::FONT_HERSHEY_SIMPLEX, 0.6,
                cv::Scalar(255, 255, 255), 1);
    startY += lineHeight;

    cv::putText(frame, "DERSLER", cv::Point(20, startY), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                cv::Scalar(200, 200, 200), 1);
    startY += 25;

    for (const auto& subject : score.subjectScores) {
        ss.str("");
        ss << subject.first << ": " << subject.second;
        cv::putText(frame, ss.str(), cv::Point(30, startY), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                    cv::Scalar(255, 255, 255), 1);
        startY += 22;
        if (startY > frame.rows - 20)
            break;
    }
}

void drawQuestionAnalysis(cv::Mat& frame,
                          const std::map<std::string, std::vector<ROIDetector::QuestionDetail>>& details) {
    if (details.empty())
        return;

    int startX = frame.cols - 420;
    int startY = 30;

    cv::Rect bgRect(startX - 10, 10, 410, std::min(frame.rows - 20, 520));
    cv::Mat roi = frame(bgRect);
    cv::Mat color(roi.size(), CV_8UC3, cv::Scalar(0, 0, 0));
    cv::addWeighted(color, 0.6, roi, 0.4, 0, roi);

    cv::putText(frame, "SORU ANALIZI", cv::Point(startX, startY), cv::FONT_HERSHEY_SIMPLEX, 0.6,
                cv::Scalar(255, 255, 0), 2);
    startY += 35;

    for (const auto& subjectPair : details) {
        cv::putText(frame, subjectPair.first, cv::Point(startX, startY), cv::FONT_HERSHEY_SIMPLEX,
                    0.5, cv::Scalar(100, 200, 255), 1);
        startY += 20;

        int shown = 0;
        for (const auto& q : subjectPair.second) {
            if (shown >= 5)
                break;

            if (q.markedAnswer == '-')
                continue;

            std::stringstream ss;
            ss << "S" << (q.questionNumber + 1) << ": " << q.markedAnswer << " -> "
               << q.correctAnswer;

            cv::Scalar color = q.isCorrect ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255);
            cv::putText(frame, ss.str(), cv::Point(startX + 10, startY), cv::FONT_HERSHEY_SIMPLEX,
                        0.45, color, 1);
            startY += 18;
            ++shown;
        }

        startY += 12;
        if (startY > frame.rows - 30)
            break;
    }
}

int main(int argc, char** argv) {
    logging::Logger& logger = logging::Logger::instance();
    logger.setLogFile("omr.log");
    logger.setFileEnabled(true);
    logger.log(logging::LogLevel::INFO, "APP", "Optik form sistemi baslatiliyor");

    int camIndex = 0;
    if (argc > 1)
        camIndex = std::atoi(argv[1]);

    cv::VideoCapture cap(camIndex);
    if (!cap.isOpened()) {
        logger.log(logging::LogLevel::ERROR, "CAMERA", "Kamera acilamadi index=" +
                                                  std::to_string(camIndex));
        return 1;
    }

    cap.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 720);
    logger.log(logging::LogLevel::INFO, "CAMERA", "Kamera acildi 1280x720 hedeflendi");

    core::PerspectiveCorrector corrector(1600, 2200);
    ROIDetector detector;
    detector.setDebugMode(true);

    AnswerKey answerKey;
    std::vector<AnswerKey::QuestionAnswer> answers;

    auto appendAnswers = [&answers](const std::string& subject, const std::string& key) {
        for (int i = 0; i < static_cast<int>(key.size()); ++i) {
            answers.push_back({subject, i, key[i]});
        }
    };

    appendAnswers("turkce", "BACCDABCDDABCCBADCBA");
    appendAnswers("sosyal", "DCBAEDCBAEABCDEDCBAE");
    appendAnswers("din", "AEBCDAEBCDAEBCDAEBCD");
    appendAnswers("ingilizce", "EABCDCBADEEDCBAACBDE");
    appendAnswers("matematik", "CDABECDABECDABECDBAE");
    appendAnswers("fen", "BCDEABCDEABCDEABCDEA");

    answerKey.loadAnswerKey(answers);
    const auto& answerKeyMap = answerKey.answers();

    bool showDebug = true;
    bool showAnalysis = true;
    bool showBubbleDebug = true;
    bool verboseLogs = false;

    cv::namedWindow("Kamera", cv::WINDOW_NORMAL);
    cv::namedWindow("Form", cv::WINDOW_NORMAL);
    cv::namedWindow("Bubble Debug", cv::WINDOW_NORMAL);
    cv::resizeWindow("Form", 800, 1100);
    cv::resizeWindow("Bubble Debug", 400, 800);

    cv::Mat lastWarped;
    AnswerKey::ScoreResult lastScore;
    std::map<std::string, std::vector<ROIDetector::QuestionDetail>> lastDetails;

    auto lastFrameTime = steady_clock::now();

    while (true) {
        cv::Mat frame;
        auto frameStart = steady_clock::now();
        if (!cap.read(frame) || frame.empty()) {
            logger.log(logging::LogLevel::ERROR, "CAMERA", "Frame okunamadi, dongu sonlandi");
            break;
        }

        logger.log(logging::LogLevel::DEBUG, "CAMERA",
                   "Frame alindi " + std::to_string(frame.cols) + "x" +
                       std::to_string(frame.rows));

        double perspectiveMs = 0.0;
        double roiMs = 0.0;
        double bubbleMs = 0.0;

        auto perspectiveStart = steady_clock::now();
        auto warpResult = corrector.findAndWarp(frame, showDebug);
        perspectiveMs = duration<double, std::milli>(steady_clock::now() - perspectiveStart).count();

        cv::Mat roiDebug;

        if (warpResult.ok && !warpResult.warped.empty()) {
            lastWarped = warpResult.warped.clone();

            auto roiStart = steady_clock::now();
            auto studentAnswers = detector.process(warpResult.warped, roiDebug);
            roiMs = duration<double, std::milli>(steady_clock::now() - roiStart).count();

            auto bubbleStart = steady_clock::now();
            lastDetails = detector.processWithDetails(warpResult.warped, roiDebug, answerKeyMap);
            bubbleMs = duration<double, std::milli>(steady_clock::now() - bubbleStart).count();

            lastScore = answerKey.calculateScore(studentAnswers);

            if (!roiDebug.empty()) {
                cv::Mat scoreOverlay = roiDebug.clone();
                drawScoreOverlay(scoreOverlay, lastScore);
                if (showAnalysis) {
                    drawQuestionAnalysis(scoreOverlay, lastDetails);
                }
                cv::imshow("Form", scoreOverlay);
            }

            if (showBubbleDebug) {
                cv::Mat bubbleDbg = detector.getLastDebugVisualization();
                if (!bubbleDbg.empty()) {
                    cv::imshow("Bubble Debug", bubbleDbg);
                }
            }
        } else {
            logger.log(logging::LogLevel::WARN, "PERSPECTIVE", "Form tespit edilemedi bu frame'de");
        }

        cv::Mat displayFrame = frame.clone();
        if (lastScore.totalQuestions > 0) {
            drawScoreOverlay(displayFrame, lastScore);
            if (showAnalysis) {
                drawQuestionAnalysis(displayFrame, lastDetails);
            }
        }

        if (showDebug && !warpResult.debug.empty()) {
            cv::imshow("Kamera", warpResult.debug);
        } else {
            cv::imshow("Kamera", displayFrame);
        }

        double frameMs = duration<double, std::milli>(steady_clock::now() - frameStart).count();
        double fps = frameMs > 0.0 ? 1000.0 / frameMs : 0.0;

        std::stringstream perf;
        perf << std::fixed << std::setprecision(2) << "Frame=" << frameMs << "ms | FPS=" << fps
             << " | warp=" << perspectiveMs << "ms | roi=" << roiMs << "ms | bubble="
             << bubbleMs << "ms";
        logger.log(logging::LogLevel::DEBUG, "PERF", perf.str());

        int key = cv::waitKey(1) & 0xFF;
        if (key == 27)
            break;

        if (key == 'd' || key == 'D') {
            showDebug = !showDebug;
            logger.log(logging::LogLevel::INFO, "INPUT", std::string("Debug goruntu ") +
                                                              (showDebug ? "ACIK" : "KAPALI"));
        }

        if (key == 'a' || key == 'A') {
            showAnalysis = !showAnalysis;
            logger.log(logging::LogLevel::INFO, "INPUT", std::string("Analiz paneli ") +
                                                              (showAnalysis ? "ACIK" : "KAPALI"));
        }

        if (key == 'b' || key == 'B') {
            showBubbleDebug = !showBubbleDebug;
            detector.setDebugMode(showBubbleDebug);
            if (!showBubbleDebug) {
                cv::destroyWindow("Bubble Debug");
            } else {
                cv::namedWindow("Bubble Debug", cv::WINDOW_NORMAL);
            }
        }

        if (key == '+' || key == '=') {
            double current = detector.getFillThreshold();
            detector.setFillThreshold(current + 0.05);
        }

        if (key == '-' || key == '_') {
            double current = detector.getFillThreshold();
            detector.setFillThreshold(current - 0.05);
        }

        if ((key == 's' || key == 'S') && !lastWarped.empty()) {
            static int saved = 0;
            std::string name = "warped_" + std::to_string(saved++) + ".png";
            cv::imwrite(name, lastWarped);
            logger.log(logging::LogLevel::INFO, "APP", "Warped kaydedildi -> " + name);
        }

        if (key == 'l' || key == 'L') {
            logger.toggleVerbose();
            verboseLogs = !verboseLogs;
            logger.log(logging::LogLevel::INFO, "INPUT",
                       std::string("Log seviyesi ") + (verboseLogs ? "DEBUG" : "INFO"));
        }
    }

    cap.release();
    cv::destroyAllWindows();
    logger.log(logging::LogLevel::INFO, "APP", "Program sonlandi");
    return 0;
}
