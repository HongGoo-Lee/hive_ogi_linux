#include "RecordManager.h"
#include <filesystem>
#include <iostream>
#include <sstream>
#include <iomanip>

namespace fs = std::filesystem;

RecordManager::RecordManager() {
    // 디렉토리가 없으면 자동 생성
    if (!fs::exists(recordDir)) {
        fs::create_directories(recordDir);
    }
}

RecordManager::~RecordManager() {
    stopThread();
}

void RecordManager::startThread() {
    isRunning = true;
    writerThread = std::thread(&RecordManager::WriterLoop, this);
}

void RecordManager::stopThread() {
    isRunning = false;
    if (writerThread.joinable()) {
        writerThread.join();
    }
    if (isRecording) {
        StopRecording();
    }
}

void RecordManager::StartRecording() {
    std::lock_guard<std::mutex> lock(queueMutex);
    if (isRecording) return;

    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&t);
    
    std::ostringstream oss;
    oss << recordDir << "REC_" << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".avi";
    
    // 리눅스/라즈베리파이 환경에서 안정적인 기본 코덱 MJPG
    int codec = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
    writer.open(oss.str(), codec, 30.0, cv::Size(640, 512), true);
    
    if (writer.isOpened()) {
        isRecording = true;
        currentChunkStartTime = now; // 녹화 시작 시점 기록
        std::cout << "[RecordManager] Started recording: " << oss.str() << std::endl;
    } else {
        std::cerr << "[RecordManager] Failed to open VideoWriter." << std::endl;
    }
}

void RecordManager::StopRecording() {
    std::lock_guard<std::mutex> lock(queueMutex);
    if (!isRecording) return;
    
    isRecording = false;
    if (writer.isOpened()) {
        writer.release(); // 정상적으로 파일 저장 및 닫기
        std::cout << "[RecordManager] Stopped recording. Video Saved." << std::endl;
    }
    
    // 남은 큐 비우기
    std::queue<cv::Mat> empty;
    std::swap(frameQueue, empty);
}

// 5분이 경과했는지 검사하고 맞다면 새로운 파일로 이어 그립니다.
void RecordManager::CheckAndRestartChunk() {
    auto now = std::chrono::system_clock::now();
    auto elapsedMinutes = std::chrono::duration_cast<std::chrono::minutes>(now - currentChunkStartTime).count();
    
    if (elapsedMinutes >= CHUNK_DURATION_MINUTES) {
        std::cout << "[RecordManager] 5 minutes elapsed. Restarting chunk." << std::endl;
        
        // 큐 뮤텍스를 재잠금하지 않고 스레드 내부에서 직접 재시작 수행
        if (writer.isOpened()) writer.release();
        
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm = *std::localtime(&t);
        std::ostringstream oss;
        oss << recordDir << "REC_" << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".avi";
        
        int codec = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
        writer.open(oss.str(), codec, 30.0, cv::Size(640, 512), true);
        currentChunkStartTime = now;
    }
}

void RecordManager::ProcessFrame(const cv::Mat& frame) {
    if (!isRecording || frame.empty()) return;
    
    std::lock_guard<std::mutex> lock(queueMutex);
    frameQueue.push(frame.clone());
}

void RecordManager::WriterLoop() {
    while (isRunning) {
        cv::Mat frame;
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (isRecording && !frameQueue.empty()) {
                frame = frameQueue.front();
                frameQueue.pop();
                
                // 프레임을 쓰기 전에 5분 초과 검사 진행
                CheckAndRestartChunk();
            }
        }
        
        if (!frame.empty() && writer.isOpened()) {
            writer.write(frame);
        } else {
            // CPU 점유율을 낮추기 위한 휴식
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}