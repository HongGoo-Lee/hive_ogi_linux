#include "MainViewModel.h"
#include "../Common/Logger.h"
#include <chrono>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <filesystem>

// 프레임 로그 주기를 제어하기 위한 카운터 (약 30FPS 기준, 90프레임 = 3초)
static int frameLogCounter = 0;

// 생성자 인자를 받아서 멤버 변수(참조)를 초기화 리스트에서 할당하도록 수정
MainViewModel::MainViewModel(CameraManager& cm, ImageProcessor& ip, RecordManager& rm, DiskManager& dm)
    : camMgr(cm), imgProc(ip), recMgr(rm), diskMgr(dm) 
{
    LOG_INFO("MainViewModel initialized");
    // main.cpp에서 이미 관리하고 있는 스레드 시작 로직을 제거했습니다.
    // 여기서 중복으로 스레드를 생성하면 std::terminate 에러가 발생합니다.
}

MainViewModel::~MainViewModel() {
    // main.cpp에서 스레드 종료를 관리하므로 중복 종료 코드를 제거했습니다.
    LOG_INFO("MainViewModel destroyed");
}

std::string MainViewModel::GetCurrentTimeString() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&t);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return oss.str();
}

bool MainViewModel::AutoConnect() {
    LOG_INFO("AutoConnect requested");
    // 원본 메서드명(소문자 시작)으로 호출
    if (camMgr.connectCamera()) {
        isConnected = true;
        currentStatus = "Connected (Auto)";
        LOG_INFO("Camera connected via AutoConnect");
        return true;
    } else {
        isConnected = false;
        currentStatus = "Camera Not Found";
        Logger::instance().warn("Camera connection failed");
        return false;
    }
}

void MainViewModel::TakeSnapshot() {
    if (!isConnected) {
        Logger::instance().warn("TakeSnapshot ignored: no camera connected");
        return;
    }
    
    cv::Mat frame = camMgr.grabFrame();
    if (frame.empty()) {
        Logger::instance().warn("TakeSnapshot failed: frame empty");
        return;
    }

    std::string filename = "SNAP_" + GetCurrentTimeString() + ".jpg";
    if (cv::imwrite(filename, frame)) {
        LOG_INFO("Snapshot saved: " + filename);
    } else {
        Logger::instance().error("Snapshot save failed: " + filename);
    }
}

void MainViewModel::StartRecording() {
    if (!isConnected) {
        Logger::instance().warn("StartRecording ignored: no camera connected");
        return;
    }
    LOG_INFO("StartRecording requested");
    currentStatus = "Recording Started";
    recMgr.StartRecording();
}

void MainViewModel::StopRecording() {
    LOG_INFO("StopRecording requested");
    currentStatus = "Recording Stopped";
    recMgr.StopRecording();
}

bool MainViewModel::IsRecording() const {
    // RecordManager를 직접 참조하지 않고 현재 뷰모델의 상태로 녹화 여부를 판단합니다.
    return currentStatus == "Recording Started";
}

cv::Mat MainViewModel::ProcessAndGetFrame(int targetW, int targetH) {
    if (!isConnected) return cv::Mat();

    cv::Mat rawFrame = camMgr.grabFrame();
    
    // --- [추가] 프레임 유입 확인 로그 (약 3초에 한 번씩 출력) ---
    frameLogCounter++;
    if (frameLogCounter >= 90) {
        frameLogCounter = 0;
        if (rawFrame.empty()) {
            Logger::instance().warn("[Frame Monitor] Camera connected but grabbing EMPTY frames!");
        } else {
            std::ostringstream msg;
            msg << "[Frame Monitor] Receiving frames successfully. Format: " 
                << rawFrame.cols << "x" << rawFrame.rows << ", Channels: " << rawFrame.channels() 
                << ", Depth: " << rawFrame.depth();
            Logger::instance().info(msg.str());
        }
    }
    // --------------------------------------------------------

    if (rawFrame.empty()) {
        currentStatus = "No Frame Data";
        return cv::Mat();
    }
    
    // 녹화 중이 아닐 때만 Streaming 상태로 표기
    if (currentStatus != "Recording Started") {
        currentStatus = "Streaming";
    }

    // 안전한 8비트 변환
    cv::Mat img8;
    // 16비트 센서 데이터인 경우에만 Normalize 수행 (이미 8비트인 경우 원본 유지)
    if (rawFrame.depth() == CV_16U || rawFrame.depth() == CV_16S) {
        cv::normalize(rawFrame, img8, 0, 255, cv::NORM_MINMAX, CV_8U);
    } else {
        img8 = rawFrame.clone();
    }

    // BGR 채널 생성 (단일 채널이나 RGBA 채널인 경우 변환)
    if (img8.channels() == 1) {
        cv::cvtColor(img8, img8, cv::COLOR_GRAY2BGR);
    } else if (img8.channels() == 4) {
        cv::cvtColor(img8, img8, cv::COLOR_BGRA2BGR);
    }

    // 시간 및 온도 문자열 생성
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&t);
    std::ostringstream dateOss, timeOss;
    dateOss << std::put_time(&tm, "%Y-%m-%d");
    timeOss << std::put_time(&tm, "%H:%M:%S");

    std::string dStr = dateOss.str();
    std::string tStr = timeOss.str();

    // 레코딩 큐에 프레임 전달
    recMgr.ProcessFrame(img8);

    // OSD 및 가스 탐지 화면 오버레이 적용
    cv::Mat finalDisplay = imgProc.processGasDetection(img8, isLeakBtn, IsRecording(), dStr, tStr);

    // 가스 탐지 처리가 실패/빈 프레임을 리턴했을 경우 안전장치
    if (finalDisplay.empty()) {
        finalDisplay = img8.clone();
    }

    // UI 크기에 맞게 리사이징
    if (targetW > 0 && targetH > 0 && !finalDisplay.empty()) {
        cv::resize(finalDisplay, finalDisplay, cv::Size(targetW, targetH));
    }

    return finalDisplay;
}

bool MainViewModel::IsStreaming() const {
    return isConnected;
}

std::string MainViewModel::GetStatus() const {
    return currentStatus;
}