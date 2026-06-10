#ifndef DISK_MANAGER_H
#define DISK_MANAGER_H

#include <string>
#include <thread>
#include <atomic>

class DiskManager {
private:
    std::string recordDirectory = "./Records/";
    const double MAX_STORAGE_USAGE_PERCENT = 50.0; // 사용량 제한 50%
    
    std::thread monitorThread;
    std::atomic<bool> isRunning{false};

    void MonitorLoop();
    void CheckStorageSpace();
    void DeleteOldestVideo();

public:
    DiskManager();
    ~DiskManager();

    void start();
    void stop();
};

#endif // DISK_MANAGER_H