#pragma once

#include "../Common/HiveConfig.h"
#include <memory>
#include <opencv2/opencv.hpp>
#include <string>

class CameraManager {
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

public:
    CameraManager();
    ~CameraManager();

    int searchCameras();
    bool connectCamera();
    bool startStreaming();
    void stopStreaming();
    bool autoConnectAndStart();

    cv::Mat grabFrame(int timeout_ms = HiveConfig::TIMEOUT_MS);

    bool isStreaming() const;
    std::string getModelName();
    std::string getSerialNumber();
};