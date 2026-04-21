#ifndef PREPROCESS_SERVICE_HPP
#define PREPROCESS_SERVICE_HPP

#include <opencv2/opencv.hpp>

class PreprocessService {
public:
    static cv::Mat resizeFrame(const cv::Mat& frame, int width, int height);
};

#endif
