#include "DiskManager.h"
#include <filesystem>
#include <iostream>
#include <chrono>

namespace fs = std::filesystem;

DiskManager::DiskManager() {
    if (!fs::exists(recordDirectory)) {
        fs::create_directories(recordDirectory);
    }
}

DiskManager::~DiskManager() {
    stop();
}

void DiskManager::start() {
    isRunning = true;
    monitorThread = std::thread(&DiskManager::MonitorLoop, this);
}

void DiskManager::stop() {
    isRunning = false;
    if (monitorThread.joinable()) {
        monitorThread.join();
    }
}

void DiskManager::MonitorLoop() {
    while (isRunning) {
        CheckStorageSpace();
        
        // 1분(60초)마다 한 번씩 디스크 용량을 검사합니다.
        for (int i = 0; i < 60 && isRunning; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

void DiskManager::CheckStorageSpace() {
    try {
        fs::space_info si = fs::space(recordDirectory);
        double usedPercentage = static_cast<double>(si.capacity - si.available) / si.capacity * 100.0;
        
        // 사용량이 50%를 초과하면 반복적으로 가장 오래된 파일을 지웁니다.
        while (usedPercentage > MAX_STORAGE_USAGE_PERCENT) {
            std::cout << "[DiskManager] Storage usage " << usedPercentage 
                      << "% exceeds limit " << MAX_STORAGE_USAGE_PERCENT << "%." << std::endl;
            
            DeleteOldestVideo();
            
            // 삭제 후 용량 다시 체크
            si = fs::space(recordDirectory);
            usedPercentage = static_cast<double>(si.capacity - si.available) / si.capacity * 100.0;
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "[DiskManager] Filesystem error: " << e.what() << std::endl;
    }
}

void DiskManager::DeleteOldestVideo() {
    fs::path oldestFile;
    auto oldestTime = fs::file_time_type::max();

    if (!fs::exists(recordDirectory)) return;

    bool fileFound = false;
    for (const auto& entry : fs::directory_iterator(recordDirectory)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            // 저장된 영상 포맷 검사
            if (ext == ".avi" || ext == ".mp4" || ext == ".mkv") { 
                auto ftime = fs::last_write_time(entry);
                if (ftime < oldestTime) {
                    oldestTime = ftime;
                    oldestFile = entry.path();
                    fileFound = true;
                }
            }
        }
    }

    if (fileFound && !oldestFile.empty()) {
        std::cout << "[DiskManager] Deleting oldest video to free up space: " << oldestFile.string() << std::endl;
        fs::remove(oldestFile); // 삭제 처리
    } else {
        std::cerr << "[DiskManager] Space needed, but no old video files found!" << std::endl;
        // 빈 디렉토리 오류로 인한 무한루프를 방지하기 위해 잠시 대기
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}