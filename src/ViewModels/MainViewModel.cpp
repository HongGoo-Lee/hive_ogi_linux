#include "MainViewModel.h"
#include "../Common/HiveConfig.h"
#include "../Common/Logger.h"

#include <chrono>
#include <ctime>
#include <filesystem>

namespace {
std::tm localTime(std::time_t value) {
    std::tm result{};
#ifdef _WIN32
    localtime_s(&result, &value);
#else
    localtime_r(&value, &result);
#endif
    return result;
}
}

MainViewModel::MainViewModel(CameraManager& cm, ImageProcessor& ip, RecordManager& rm, DiskManager& dm)
    : camMgr(cm), imgProc(ip), recMgr(rm), diskMgr(dm), isLeakBtn(false), currentStatus("System Ready") {
}

void MainViewModel::updateStatus(const std::string& status) {
    currentStatus = status;
}

std::string MainViewModel::GetStatus() const {
    return currentStatus;
}

bool MainViewModel::IsStreaming() const {
    return camMgr.isStreaming();
}

bool MainViewModel::AutoConnect() {
    LOG_INFO("UI AutoConnect requested");
    updateStatus("Connecting...");
    if (camMgr.autoConnectAndStart()) {
        LOG_INFO("UI AutoConnect success");
        updateStatus("Camera Connected");
        return true;
    }

    LOG_ERROR("UI AutoConnect failed");
    updateStatus("Connection Failed");
    return false;
}

void MainViewModel::TakeSnapshot() {
    if (lastProcessedMonoFrame.empty()) {
        LOG_WARN("Snapshot ignored because lastProcessedMonoFrame is empty");
        return;
    }

    diskMgr.checkAndCleanupNow();

    std::filesystem::path dirPath = HiveConfig::BASE_DIR_SnapShot;
    std::filesystem::create_directories(dirPath);

    const std::time_t now = std::time(nullptr);
    std::tm tstruct = localTime(now);

    char buf[80];
    std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &tstruct);

    const std::filesystem::path filename = dirPath / ("snap_" + std::string(buf) + ".jpg");
    cv::imwrite(filename.string(), lastProcessedMonoFrame);
    LOG_INFO("Snapshot saved: " + filename.string());

    updateStatus("Snapshot Saved");
}

void MainViewModel::StartRecording() {
    LOG_INFO("Recording start requested");
    recMgr.startRecording();
    updateStatus("Recording...");
}

void MainViewModel::StopRecording() {
    LOG_INFO("Recording stop requested");
    recMgr.stopRecording();
    updateStatus("Recording Stopped");
}

void MainViewModel::GetTimeStrings(std::string& dateStr, std::string& timeStr) {
    const auto now = std::chrono::system_clock::now();
    const std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = localTime(now_c);

    char dBuf[20];
    char tBuf[20];
    std::strftime(dBuf, sizeof(dBuf), "%Y-%m-%d ", &now_tm);
    std::strftime(tBuf, sizeof(tBuf), "%H:%M:%S", &now_tm);

    dateStr = dBuf;
    timeStr = tBuf;
}

cv::Mat MainViewModel::ProcessAndGetFrame(int targetWidth, int targetHeight) {
    if (!camMgr.isStreaming()) {
        return cv::Mat();
    }

    cv::Mat rawFrame = camMgr.grabFrame(HiveConfig::TIMEOUT_MS);
    if (rawFrame.empty()) {
        static uint64_t emptyRawCount = 0;
        if (++emptyRawCount <= 10 || emptyRawCount % 100 == 0) {
            LOG_WARN("ProcessAndGetFrame got empty raw frame count=" + std::to_string(emptyRawCount));
        }
        return cv::Mat();
    }

    cv::Mat img8;
    if (rawFrame.type() == CV_16UC1) {
        img8 = imgProc.preprocess16to8(rawFrame);
    }
    else if (rawFrame.type() == CV_8UC1) {
        rawFrame.copyTo(img8);
    }
    else if (rawFrame.channels() == 3) {
        cv::cvtColor(rawFrame, img8, cv::COLOR_BGR2GRAY);
    }
    else {
        LOG_WARN("Unsupported raw frame type=" + std::to_string(rawFrame.type()) +
                 " channels=" + std::to_string(rawFrame.channels()));
        return cv::Mat();
    }

    if (img8.empty()) {
        LOG_WARN("Processed img8 is empty");
        return cv::Mat();
    }

    static uint64_t processedCount = 0;
    if (++processedCount <= 3 || processedCount % 100 == 0) {
        LOG_INFO("Processed frame count=" + std::to_string(processedCount) +
                 " raw_type=" + std::to_string(rawFrame.type()) +
                 " img8=" + std::to_string(img8.cols) + "x" + std::to_string(img8.rows) +
                 " target=" + std::to_string(targetWidth) + "x" + std::to_string(targetHeight));
    }

    lastProcessedMonoFrame = img8;
    recMgr.updateFrame(img8);

    std::string dStr;
    std::string tStr;
    GetTimeStrings(dStr, tStr);

    cv::Mat finalDisplay = imgProc.processGasDetection(img8, isLeakBtn, recMgr.getIsRecording(), dStr, tStr);

    cv::Mat resizedDisplay;
    cv::resize(finalDisplay, resizedDisplay, cv::Size(targetWidth, targetHeight));
    return resizedDisplay;
}
