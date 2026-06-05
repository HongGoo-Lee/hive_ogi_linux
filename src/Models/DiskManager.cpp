#include "DiskManager.h"
#include "../Common/Logger.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <vector>

DiskManager::~DiskManager() {
    stop();
}

void DiskManager::start() {
    if (!isRunning) {
        isRunning = true;
        mgrThread = std::thread(&DiskManager::threadLoop, this);
        LOG_INFO("DiskManager thread started");
    }
}

void DiskManager::stop() {
    if (isRunning) {
        isRunning = false;
        if (mgrThread.joinable()) {
            mgrThread.join();
        }
        LOG_INFO("DiskManager thread stopped");
    }
}

void DiskManager::checkAndCleanupNow() {
    performCleanup();
}

void DiskManager::threadLoop() {
    while (isRunning) {
        performCleanup();
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

void DiskManager::performCleanup() {
    try {
        const auto spaceInfo = std::filesystem::space(HiveConfig::ROOT_DRIVE);
        if (spaceInfo.capacity == 0 || spaceInfo.capacity == static_cast<std::uintmax_t>(-1)) {
            return;
        }

        const double usedRatio =
            1.0 - (static_cast<double>(spaceInfo.available) / static_cast<double>(spaceInfo.capacity));

        if (usedRatio <= HiveConfig::DISK_USAGE_THRESHOLD) {
            return;
        }

        LOG_WARN("Disk usage " + std::to_string(usedRatio * 100.0) +
                 "% exceeds threshold. Cleaning up...");

        struct FileInfo {
            std::filesystem::path path;
            std::filesystem::file_time_type time;
        };

        std::vector<FileInfo> files;
        if (std::filesystem::exists(HiveConfig::BASE_DIR)) {
            for (const auto& p : std::filesystem::recursive_directory_iterator(HiveConfig::BASE_DIR)) {
                if (p.is_regular_file() && p.path().extension() == ".avi") {
                    files.push_back({p.path(), p.last_write_time()});
                }
            }
        }

        std::sort(files.begin(), files.end(), [](const FileInfo& a, const FileInfo& b) {
            return a.time < b.time;
        });

        int deletedCount = 0;
        for (const auto& file : files) {
            try {
                std::filesystem::remove(file.path);
                LOG_INFO("Disk cleanup deleted: " + file.path.string());
                if (++deletedCount >= 5) {
                    break;
                }
            }
            catch (const std::exception& e) {
                LOG_WARN("Disk cleanup delete failed: " + file.path.string() + " - " + e.what());
            }
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR(std::string("Disk cleanup exception: ") + e.what());
    }
}