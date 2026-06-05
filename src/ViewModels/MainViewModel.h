#ifndef MAIN_VIEW_MODEL_H
#define MAIN_VIEW_MODEL_H
#include "../Models/RecordManager.h"
#include "../Models/ImageProcessor.h"
#include "../Models/DiskManager.h"
#include "../Models/CameraManager.h"
#include <string>
#include <opencv2/opencv.hpp>

class MainViewModel {
private:
    CameraManager& camMgr;
    ImageProcessor& imgProc;
    RecordManager& recMgr;
    DiskManager& diskMgr;

    cv::Mat lastProcessedMonoFrame;
    bool isLeakBtn;
    std::string currentStatus;

    void updateStatus(const std::string& status);
    void GetTimeStrings(std::string& dateStr, std::string& timeStr);

public:
    MainViewModel(CameraManager& cm, ImageProcessor& ip, RecordManager& rm, DiskManager& dm);

    // View에서 바인딩할 커맨드
    bool AutoConnect();
    void TakeSnapshot();
    void StartRecording();
    void StopRecording();

    // View의 타이머에서 호출하여 갱신된 프레임을 반환
    cv::Mat ProcessAndGetFrame(int targetWidth, int targetHeight);

    // View에 바인딩할 상태 값
    std::string GetStatus() const;
    bool IsStreaming() const;
};

#endif // MAIN_VIEW_MODEL_H