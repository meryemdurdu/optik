#pragma once

#include <array>
#include <vector>

#include <opencv2/opencv.hpp>

namespace core {

struct CornerResult {
    bool paper_ok = false;
    cv::Mat warped_gray;
    cv::Mat debug_bgr;
    std::array<cv::Point2f, 4> markers_orig{{
        cv::Point2f(-1.f, -1.f),
        cv::Point2f(-1.f, -1.f),
        cv::Point2f(-1.f, -1.f),
        cv::Point2f(-1.f, -1.f)}};
};

class CornerFinder {
public:
    CornerFinder(int outW, int outH);

    CornerResult processFrame(const cv::Mat& bgr, bool debug_on) const;

private:
    bool findCornerSquares(const cv::Mat& gray,
                           std::vector<cv::Point2f>& corners,
                           cv::Mat* dbg) const;

    std::vector<cv::Point2f> orderTLTRBRBL(const std::vector<cv::Point2f>& pts,
                                           cv::Point2f C) const;

    int outW_;
    int outH_;
};

}  // namespace core
