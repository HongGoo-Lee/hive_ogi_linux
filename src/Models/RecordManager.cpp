#include "RecordManager.h"
#include "../Common/Logger.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iostream>

namespace {
std::tm localTime(std::time_t value) {
    std::tm result{};
    localtime_r(&value, &result);
    return result;
}
}

RecordManager::~RecordManager() {
    LOG_INFO("RecordManager destructor called. Cleaning up...");
    stopThread();

    cv::Mat* leftover = sharedFramePtr.exchange(nullptr);
    delete leftover;
}

void RecordManager::startThread() {
    if (!isRunning) {
        isRunning = true;
        recordThread = std::thread(&RecordManager::recordingLoop, this);
        LOG_INFO("RecordManager thread started");
    }
}

void RecordManager::stopThread() {
    if (isRunning) {
        isRunning = false;
        if (recordThread.joinable()) {
            recordThread.join();
        }
        LOG_INFO("RecordManager thread stopped");
    }
}

void RecordManager::startRecording() {
    isRecording = true;
    LOG_INFO("RecordManager recording enabled");
}

void RecordManager::stopRecording() {
    isRecording = false;
    LOG_INFO("RecordManager recording disabled");
}

void RecordManager::updateFrame(const cv::Mat& frame) {
    if (frame.empty()) {
        return;
    }

    cv::Mat* newFrame = new cv::Mat();
    frame.copyTo(*newFrame);

    cv::Mat* oldFrame = sharedFramePtr.exchange(newFrame);
    delete oldFrame;
}

void RecordManager::recordingLoop() {
    cv::VideoWriter writer;
    auto lastSegmentTime = std::chrono::steady_clock::now();
    cv::Mat frameToWrite;

    while (isRunning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        if (!isRecording) {
            if (writer.isOpened()) {
                writer.release();
                LOG_INFO("Recording writer released");
            }
            continue;
        }

        cv::Mat* grabbedFrame = sharedFramePtr.exchange(nullptr);
        if (grabbedFrame) {
            frameToWrite = std::move(*grabbedFrame);
            delete grabbedFrame;
        }

        if (frameToWrite.empty()) {
            continue;
        }

        const auto now = std::chrono::steady_clock::now();
        bool startNewFile = !writer.isOpened();

        if (writer.isOpened()) {
            const auto duration =
                std::chrono::duration_cast<std::chrono::milliseconds>(now - lastSegmentTime).count();
            if (duration >= HiveConfig::REC_SEGMENT_DURATION_MS) {
                writer.release();
                startNewFile = true;
            }
        }

        if (startNewFile) {
            const auto t_now = std::chrono::system_clock::now();
            const std::time_t now_c = std::chrono::system_clock::to_time_t(t_now);
            std::tm parts = localTime(now_c);

            char dateBuf[20];
            char timeBuf[20];
            std::strftime(dateBuf, sizeof(dateBuf), "%Y%m%d", &parts);
            std::strftime(timeBuf, sizeof(timeBuf), "%H%M%S", &parts);

            const std::filesystem::path dirPath = std::filesystem::path(HiveConfig::BASE_DIR) / dateBuf;
            std::filesystem::create_directories(dirPath);

            const std::filesystem::path filePath = dirPath / (std::string(timeBuf) + ".avi");

            writer.open(filePath.string(), cv::CAP_FFMPEG, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'),
                20.0, frameToWrite.size(), false);

            if (writer.isOpened()) {
                LOG_INFO("Recording new segment: " + filePath.string());
                lastSegmentTime = now;
            }
            else {
                LOG_ERROR("Recording failed to open: " + filePath.string());
            }
        }

        if (writer.isOpened()) {
            writer.write(frameToWrite);
        }
    }

    if (writer.isOpened()) {
        writer.release();
    }
}