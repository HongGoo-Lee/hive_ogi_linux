#ifndef MAIN_VIEW_H
#define MAIN_VIEW_H

#include "ViewModels/MainViewModel.h"
#include "ViewState.h"
#include <string>
#include <memory>
#include <vector>

// 전방 선언 (Forward Declarations)
class LiveView;
class PlaybackView;
class PlayerView;

class MainView {
private:
    MainViewModel* viewModel;
    
    int windowW;
    int windowH;
    std::string windowTitle;

    // --- 화면 상태 관리 ---
    AppState currentState = AppState::Main;
    int selectedVideoIndex = -1;

    // --- 분리된 하위 뷰(Sub-Views) 포인터 ---
    std::unique_ptr<LiveView> liveView;
    std::unique_ptr<PlaybackView> playbackView;
    std::unique_ptr<PlayerView> playerView;

public:
    MainView(MainViewModel* vm, int W, int H, const char* t);
    ~MainView(); // 소멸자 선언 필요 (unique_ptr 전방 선언 문제 해결)

    void Render();

    // --- 하위 뷰들이 상태를 변경할 때 사용하는 인터페이스 ---
    void ChangeState(AppState newState);
    void SetSelectedVideoIndex(int index);
    int GetSelectedVideoIndex() const;
    const std::string& GetWindowTitle() const { return windowTitle; }
};

#endif // MAIN_VIEW_H