#pragma once
#include <atomic>
#include <thread>
#include "../Common/HiveConfig.h"

class DiskManager {
private:
    std::atomic<bool> isRunning{ false };
    std::thread mgrThread;

    void performCleanup();
    void threadLoop();

public:
    ~DiskManager();

    void start();
    void stop();
    void checkAndCleanupNow();
};