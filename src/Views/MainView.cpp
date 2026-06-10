#include "MainView.h"
#include "Theme.h"
#include "LiveView.h"
#include "PlaybackView.h"
#include "PlayerView.h"
#include <imgui.h>

MainView::MainView(MainViewModel* vm, int W, int H, const char* t)
    : viewModel(vm), windowW(W), windowH(H), windowTitle(t) 
{
    // 각 화면별 뷰 클래스 초기화 (this를 넘겨서 상태 전환 함수를 쓸 수 있게 함)
    liveView = std::make_unique<LiveView>(this, viewModel);
    playbackView = std::make_unique<PlaybackView>(this);
    playerView = std::make_unique<PlayerView>(this);
}

MainView::~MainView() = default;

void MainView::ChangeState(AppState newState) {
    currentState = newState;
}

void MainView::SetSelectedVideoIndex(int index) {
    selectedVideoIndex = index;
}

int MainView::GetSelectedVideoIndex() const {
    return selectedVideoIndex;
}

void MainView::Render() {
    // --- 글로벌 스타일 설정 ---
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);   
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);  
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);   
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f)); 

    ImFont* boldFont = (ImGui::GetIO().Fonts->Fonts.Size > 1) ? ImGui::GetIO().Fonts->Fonts[1] : nullptr;
    if (boldFont) ImGui::PushFont(boldFont);

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, COL_BG);

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::Begin("Main UI", nullptr, windowFlags);

    // --- 상태에 따라 해당 파일에 구현된 렌더링 로직 호출 ---
    if (currentState == AppState::Main) {
        liveView->Render();
    } else if (currentState == AppState::PlaybackList) {
        playbackView->Render();
    } else if (currentState == AppState::VideoPlayer) {
        playerView->Render();
    }

    ImGui::End(); 

    if (boldFont) ImGui::PopFont();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(4);
}