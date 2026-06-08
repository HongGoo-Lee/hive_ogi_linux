#pragma once
#include <opencv2/opencv.hpp>
#include <thread>
#include <atomic>
#include <string>
#include "../Common/HiveConfig.h"

class RecordManager {
private:
    std::atomic<cv::Mat*> sharedFramePtr{ nullptr }; 
    
    std::atomic<bool> isRunning{ false };
    std::atomic<bool> isRecording{ false };
    std::thread recordThread;
    
    void recordingLoop(); 

public:
    RecordManager() = default;
    ~RecordManager();

    void startThread();
    void stopThread();

    void startRecording();
    void stopRecording();

    void updateFrame(const cv::Mat& frame);

    bool getIsRecording() const { return isRecording; }
};