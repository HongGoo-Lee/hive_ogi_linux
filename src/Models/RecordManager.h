#ifndef RECORD_MANAGER_H
#define RECORD_MANAGER_H

#include <opencv2/opencv.hpp>
#include <string>
#include <chrono>
#include <mutex>
#include <thread>
#include <atomic>
#include <queue>

class RecordManager {
private:
    cv::VideoWriter writer;
    std::string recordDir = "./Records/";
    std::atomic<bool> isRecording{false};

    // 5분 분할 저장을 위한 시간 추적 변수
    std::chrono::system_clock::time_point currentChunkStartTime;
    const int CHUNK_DURATION_MINUTES = 5;

    // 비동기 처리를 위한 스레드 및 큐
    std::queue<cv::Mat> frameQueue;
    std::mutex queueMutex;
    std::thread writerThread;
    std::atomic<bool> isRunning{false};

    void CheckAndRestartChunk();
    void WriterLoop();

public:
    RecordManager();
    ~RecordManager();

    void startThread();
    void stopThread();
    void StartRecording();
    void StopRecording();
    void ProcessFrame(const cv::Mat& frame);
};

#endif // RECORD_MANAGER_H