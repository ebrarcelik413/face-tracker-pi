#include "preprocess_service.hpp"

cv::Mat PreprocessService::resizeFrame(const cv::Mat& frame, int width, int height) {
    cv::Mat resized;
    cv::resize(frame, resized, cv::Size(width, height));
    return resized;
}
