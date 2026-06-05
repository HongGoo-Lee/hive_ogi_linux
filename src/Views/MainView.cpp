#include "MainView.h"
#include "Theme.h"
#include <imgui.h>

// MainView 생성자: 뷰모델(ViewModel) 포인터와 창의 너비/높이, 윈도우 타이틀을 초기화합니다.
MainView::MainView(MainViewModel* vm, int W, int H, const char* t)
    : viewModel(vm), windowW(W), windowH(H), windowTitle(t) {}

void MainView::Render() {
    // --- 글로벌 스타일 설정 ---
    // UI 요소들의 모서리 둥글기와 아이템 간의 기본 여백을 설정합니다.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);   // 메인 윈도우는 각지게 설정
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);  // 내부 자식 패널은 둥글게 설정
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);   // 버튼 등 프레임 요소도 둥글게 설정
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f)); // 아이템 간의 기본 자동 간격 제거 (수동으로 좌표 제어)

    // Bold 폰트 적용 (로드된 폰트가 2개 이상일 경우 두 번째 폰트를 사용)
    ImFont* boldFont = (ImGui::GetIO().Fonts->Fonts.Size > 1) ? ImGui::GetIO().Fonts->Fonts[1] : nullptr;
    if (boldFont) ImGui::PushFont(boldFont);

    // 뷰포트를 가져와서 메인 윈도우의 위치와 크기를 전체 작업 영역에 꽉 차게 맞춥니다.
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    // 배경색 적용 (Theme.h에 정의된 COL_BG 사용)
    ImGui::PushStyleColor(ImGuiCol_WindowBg, COL_BG);

    // 타이틀 바, 크기 조절, 이동, 포커스 시 최상단 이동 기능을 모두 끈 상태로 고정 윈도우 생성
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::Begin("Main UI", nullptr, windowFlags);

    float winW = ImGui::GetWindowWidth();
    float winH = ImGui::GetWindowHeight();
    float pad = 20.0f; // UI 요소 간의 기본 여백 수치

    // --- 1. Header Bar (상단 헤더 영역) ---
    float headerH = 70.0f; 
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(12.f/255.f, 15.f/255.f, 20.f/255.f, 1.f)); // 헤더 전용 다크 블루 계열 배경색
    ImGui::SetCursorPos(ImVec2(0, 0)); // 최상단에 배치
    ImGui::BeginChild("Header", ImVec2(winW, headerH), false, ImGuiWindowFlags_NoScrollbar);
    
    // 타이틀 텍스트를 헤더 영역의 수직 중앙에 정렬하기 위한 Y 좌표 계산
    float textY = (headerH - ImGui::GetTextLineHeight()) * 0.5f;
    ImGui::SetCursorPos(ImVec2(pad, textY));
    ImGui::TextColored(COL_ACCENT, "%s", windowTitle.c_str()); // 강조색으로 타이틀 출력

    ImGui::EndChild();
    ImGui::PopStyleColor();

    // --- 공간 분배 계산 ---
    // 비디오 출력 영역의 동적 크기 할당을 위해 나머지 UI 요소들이 차지할 공간을 미리 계산합니다.
    float statusH = ImGui::GetTextLineHeight() + pad; // 하단 상태표시줄 영역의 높이
    float btnRowH = 85.0f;                            // 버튼 1줄의 높이
    float btnAreaH = (btnRowH * 2) + pad;             // 버튼 2줄이 차지하는 총 높이 (버튼 간 간격 포함)

    // 전체 높이에서 헤더, 버튼 영역, 상태표시줄, 그리고 각종 위/아래 여백을 뺀 나머지 영역을 비디오 높이로 지정
    float vbh = winH - headerH - btnAreaH - statusH - (pad * 4); 
    float vbw = winW - (pad * 2); // 좌우 여백을 제외한 비디오 너비

    // --- 2. 영상 화면 ---
    ImGui::SetCursorPos(ImVec2(pad, headerH + pad)); // 헤더 바로 아래에 비디오 영역 시작 위치 지정

    // ViewModel에서 스트리밍 상태라고 판단하면, 프레임을 처리하고 텍스처를 업데이트합니다.
    if (viewModel->IsStreaming()) {
        cv::Mat frame = viewModel->ProcessAndGetFrame((int)vbw, (int)vbh);
        videoTexture.Update(frame); // OpenCV Mat 이미지를 OpenGL/DirectX 텍스처로 변환
    }

    // 텍스처 ID가 유효하면 영상을 출력하고, 그렇지 않으면 Placeholder 화면을 출력합니다.
    if (videoTexture.GetID() != 0) {
        ImGui::Image((ImTextureID)(uintptr_t)videoTexture.GetID(), ImVec2(vbw, vbh));
    } else {
        // 영상 신호가 없을 때 표시할 빈 패널 설정
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.05f, 0.05f, 1.0f)); // 어두운 회색
        ImGui::BeginChild("VideoPlaceholder", ImVec2(vbw, vbh), true);
        
        const char* placeholderText = "NO VIDEO SIGNAL";
        ImVec2 textSize = ImGui::CalcTextSize(placeholderText);
        // 텍스트를 패널의 정중앙에 배치
        ImGui::SetCursorPos(ImVec2((vbw - textSize.x) * 0.5f, (vbh - textSize.y) * 0.5f));
        ImGui::TextDisabled("%s", placeholderText); // 비활성화된 색상으로 텍스트 출력
        
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    // --- 3. 버튼 배치 ---
    // 버튼 2개를 가로로 나란히 꽉 차게 배치하기 위한 너비 계산 (좌, 중앙, 우측 여백 고려)
    float btnW = (winW - (pad * 3)) * 0.5f; 
    float btnStartY = headerH + pad + vbh + pad; // 버튼 영역이 시작되는 Y 좌표

    // 버튼의 기본 상태, 마우스 오버 상태, 클릭 상태의 색상 지정
    ImGui::PushStyleColor(ImGuiCol_Button, COL_SURFACE);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, COL_PANEL);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, COL_CARD);

    // [첫 번째 줄 버튼]
    ImGui::SetCursorPos(ImVec2(pad, btnStartY));
    ImGui::PushStyleColor(ImGuiCol_Text, COL_TEXT);
    if (ImGui::Button("> AUTO CONNECT", ImVec2(btnW, btnRowH))) {
        // 연결을 시도하고 실패하면 경고 팝업 플래그를 true로 설정
        if (!viewModel->AutoConnect()) showCamAlert = true;
    }
    
    ImGui::SetCursorPos(ImVec2(pad + btnW + pad, btnStartY)); // 첫 번째 버튼의 우측에 배치
    if (ImGui::Button("+ SNAPSHOT", ImVec2(btnW, btnRowH))) {
        viewModel->TakeSnapshot();
    }
    ImGui::PopStyleColor(); // 텍스트 컬러 팝

    // [두 번째 줄 버튼]
    float btnRow2Y = btnStartY + btnRowH + pad; // 첫 번째 줄 밑으로 이동
    ImGui::SetCursorPos(ImVec2(pad, btnRow2Y));
    ImGui::PushStyleColor(ImGuiCol_Text, COL_ACCENT); // 녹화 시작 버튼 텍스트는 강조색 사용
    if (ImGui::Button("O REC START", ImVec2(btnW, btnRowH))) {
        viewModel->StartRecording();
    }
    ImGui::PopStyleColor();

    ImGui::SetCursorPos(ImVec2(pad + btnW + pad, btnRow2Y));
    ImGui::PushStyleColor(ImGuiCol_Text, COL_RED); // 녹화 중지 버튼 텍스트는 빨간색 사용
    if (ImGui::Button("[] REC STOP", ImVec2(btnW, btnRowH))) {
        viewModel->StopRecording();
    }
    ImGui::PopStyleColor();

    ImGui::PopStyleColor(3); // 버튼 상태 3종 색상 팝

    // --- 4. Alert Popup (카메라 연결 실패 경고창) ---
    // 플래그가 켜졌을 때 모달 팝업을 띄움
    if (showCamAlert) {
        ImGui::OpenPopup("Alert");
        showCamAlert = false;
    }
    
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(30.0f, 30.0f));
    // 내용물 크기에 맞춰 자동 리사이징 되는 모달 창
    if (ImGui::BeginPopupModal("Alert", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar)) {
        ImGui::TextColored(COL_RED, "WARNING");
        ImGui::Separator(); // 가로선 그리기
        ImGui::Dummy(ImVec2(0.0f, 10.0f)); // 수직 빈 공간 추가
        ImGui::Text("Camera Not Found!\nPlease check the connection.");
        ImGui::Dummy(ImVec2(0.0f, 20.0f));
        
        float okBtnW = 150.0f;
        // 확인 버튼을 모달 창 정중앙에 배치
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - okBtnW) * 0.5f);
        if (ImGui::Button("OK", ImVec2(okBtnW, 50.0f))) { 
            ImGui::CloseCurrentPopup(); // 버튼 클릭 시 모달 닫기
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(2); // 팝업용 스타일 팝

    // --- 5. Status Bar (하단 상태 표시줄) ---
    float statusY = winH - statusH; // 윈도우 맨 아래쪽 위치 계산
    ImGui::SetCursorPos(ImVec2(pad, statusY));
    ImGui::TextColored(COL_TEXT, "Status: %s", viewModel->GetStatus().c_str()); // 뷰모델의 현재 상태 문자열 출력

    ImGui::End(); // 메인 윈도우 끝

    // 적용했던 폰트 및 글로벌 스타일 설정들을 원상태로 복구 (Push 횟수와 동일하게 Pop 수행)
    if (boldFont) ImGui::PopFont();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(4);
}