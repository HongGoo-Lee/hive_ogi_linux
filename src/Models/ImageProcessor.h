#pragma once
#include <opencv2/opencv.hpp>
#include "../Common/HiveConfig.h"
#include <string>

class IRDestriper {
private:
    cv::Mat columnNoiseMask;
    cv::Mat accumMean;
    int frameCount = 0;
    const int CALIBRATION_FRAMES = 30;
    bool isReady = false;

public:
    void processFrame(cv::Mat& frame);
    void calibrate(cv::Mat& frame);
    void applyFilter(cv::Mat& frame);
    bool calibrated() const { return isReady; }
};

class ImageProcessor {
private:
    cv::Mat img_background, img_binary2, img_binary3;
    cv::Mat filtered1, filtered2, filtered3, filtered5;
    cv::Mat img_sub, img_binary;
    cv::Mat prevdeflicker;
    int interval_time = 0;

    IRDestriper destriper;

public:
    ImageProcessor() = default;

    cv::Mat preprocess16to8(const cv::Mat& img16);

    cv::Mat processGasDetection(cv::Mat& img8, bool isLeakBtnActive, bool isRecording, const std::string& dateStr, const std::string& timeStr);

    cv::Mat deflicker(cv::Mat Mat1, int strengthcutoff = 9);
    cv::Mat deflicker_fast(cv::Mat currentFrame, int strengthcutoff = 9);
};