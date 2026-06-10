#ifndef LIVE_VIEW_H
#define LIVE_VIEW_H

#include "ViewModels/MainViewModel.h"
#include "ImageTexture.h"
#include <string>

// 전방 선언
class MainView;

class LiveView {
private:
    MainView* parentView;      // 화면 전환을 위해 부모 참조
    MainViewModel* viewModel;  // 카메라 제어 등을 위해 참조
    
    ImageTexture videoTexture;
    
    bool showCamAlert = false;
    bool showSnapshotToast = false;
    std::string snapshotMessage;
    float toastTimer = 0.0f;

    bool isRecording = true; // 추가: 녹화 상태 관리용 변수

    void ShowToastNotification();

public:
    LiveView(MainView* parent, MainViewModel* vm);
    void Render();
};

#endif // LIVE_VIEW_H