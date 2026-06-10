#ifndef MAIN_VIEW_MODEL_H
#define MAIN_VIEW_MODEL_H

#include "../Models/CameraManager.h"
#include "../Models/ImageProcessor.h"
#include "../Models/RecordManager.h"
#include "../Models/DiskManager.h"
#include <opencv2/opencv.hpp>
#include <string>

class MainViewModel {
private:
    // main.cpp에서 넘겨준 인스턴스를 참조(&)로 받아서 사용하도록 수정
    CameraManager& camMgr;
    ImageProcessor& imgProc;
    RecordManager& recMgr;
    DiskManager& diskMgr;

    std::string currentStatus = "Disconnected";
    bool isConnected = false;
    bool isLeakBtn = false;

    std::string GetCurrentTimeString();

public:
    // 생성자 인자로 4개의 매니저 객체 참조를 받도록 수정
    MainViewModel(CameraManager& cm, ImageProcessor& ip, RecordManager& rm, DiskManager& dm);
    ~MainViewModel();

    bool AutoConnect();
    void TakeSnapshot();
    void StartRecording();
    void StopRecording();

    // 뷰(LiveView 등)에서 렌더링에 사용할 프레임 획득
    cv::Mat ProcessAndGetFrame(int targetW, int targetH);

    bool IsStreaming() const;
    std::string GetStatus() const;
    
    // UI 쪽에서 현재 녹화 상태를 읽어올 때 사용 (버튼 색상 등 제어용)
    bool IsRecording() const;
};

#endif // MAIN_VIEW_MODEL_H