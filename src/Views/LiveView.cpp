#include "LiveView.h"
#include "MainView.h"
#include "Theme.h"
#include <imgui.h>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

LiveView::LiveView(MainView* parent, MainViewModel* vm)
    : parentView(parent), viewModel(vm) {}

void LiveView::Render() {
    float winW = ImGui::GetWindowWidth();
    float winH = ImGui::GetWindowHeight();
    float pad = 20.0f; 

    // 1. Header Bar
    float headerH = 70.0f; 
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(12.f/255.f, 15.f/255.f, 20.f/255.f, 1.f));
    ImGui::SetCursorPos(ImVec2(0, 0));
    ImGui::BeginChild("Header", ImVec2(winW, headerH), false, ImGuiWindowFlags_NoScrollbar);
    float textY = (headerH - ImGui::GetTextLineHeight()) * 0.5f;
    ImGui::SetCursorPos(ImVec2(pad, textY));
    ImGui::TextColored(COL_ACCENT, "%s", parentView->GetWindowTitle().c_str());

    // 녹화 중일 때 우측 상단에 깜빡이는 빨간색 텍스트(점) 표시
    if (isRecording) {
        ImVec2 recTextSize = ImGui::CalcTextSize("● REC");
        ImGui::SetCursorPos(ImVec2(winW - pad - recTextSize.x, textY));
        // ImGui::GetTime()을 이용해 1초 주기로 깜빡이는 애니메이션 효과
        if (static_cast<int>(ImGui::GetTime() * 2) % 2 == 0) {
            ImGui::TextColored(COL_RED, "● REC");
        } else {
            ImGui::TextColored(ImVec4(0.5f, 0.0f, 0.0f, 1.0f), "● REC"); // 어두운 빨간색
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    // 공간 분배 계산 (2x3 그리드에 맞게 버튼 영역 높이 수정)
    float statusH = ImGui::GetTextLineHeight() + pad;
    float btnRowH = 75.0f;                            
    float btnAreaH = (btnRowH * 3) + (pad * 2); // 버튼 3줄 기준의 높이 계산
    
    float vbh = winH - headerH - btnAreaH - statusH - (pad * 4); 
    float vbw = winW - (pad * 2); 

    // 2. 영상 화면
    ImGui::SetCursorPos(ImVec2(pad, headerH + pad)); 
    if (viewModel->IsStreaming()) {
        cv::Mat frame = viewModel->ProcessAndGetFrame((int)vbw, (int)vbh);
        videoTexture.Update(frame);
    }

    if (videoTexture.GetID() != 0) {
        ImGui::Image((ImTextureID)(uintptr_t)videoTexture.GetID(), ImVec2(vbw, vbh));
        
        // 영상 뷰어 테두리에 녹화중임을 알리는 붉은 외곽선 오버레이
        if (isRecording) {
            ImVec2 p0 = ImGui::GetItemRectMin();
            ImVec2 p1 = ImGui::GetItemRectMax();
            ImGui::GetWindowDrawList()->AddRect(p0, p1, IM_COL32(255, 71, 87, 150), 0.0f, 0, 3.0f);
        }
    } else {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.05f, 0.05f, 1.0f)); 
        ImGui::BeginChild("VideoPlaceholder", ImVec2(vbw, vbh), true);
        const char* placeholderText = "NO VIDEO SIGNAL";
        ImVec2 textSize = ImGui::CalcTextSize(placeholderText);
        ImGui::SetCursorPos(ImVec2((vbw - textSize.x) * 0.5f, (vbh - textSize.y) * 0.5f));
        ImGui::TextDisabled("%s", placeholderText); 
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    // 3. 버튼 배치 (2열 3행 - 2x3 배열로 변경)
    float btnW = (winW - (pad * 3)) * 0.5f; // 좌, 우, 가운데 여백(3개)을 뺀 너비 절반
    float btnStartY = headerH + pad + vbh + pad; 

    ImGui::PushStyleColor(ImGuiCol_Button, COL_SURFACE);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, COL_PANEL);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, COL_CARD);

    // [첫 번째 줄 버튼 (Row 1)]
    ImGui::SetCursorPos(ImVec2(pad, btnStartY));
    ImGui::PushStyleColor(ImGuiCol_Text, COL_TEXT);
    if (ImGui::Button("Snapshot", ImVec2(btnW, btnRowH))) {
        viewModel->TakeSnapshot();
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        snapshotMessage = oss.str() + " 캡처되었습니다.";
        showSnapshotToast = true;
        toastTimer = 2.5f;
    }
    
    ImGui::SetCursorPos(ImVec2(pad + btnW + pad, btnStartY)); 
    if (ImGui::Button("Playback", ImVec2(btnW, btnRowH))) {
        parentView->ChangeState(AppState::PlaybackList);
    }
    ImGui::PopStyleColor(); 

    // [두 번째 줄 버튼 (Row 2)]
    float row2Y = btnStartY + btnRowH + pad; 
    ImGui::SetCursorPos(ImVec2(pad, row2Y));
    if (ImGui::Button("Menu", ImVec2(btnW, btnRowH))) {
    }

    ImGui::SetCursorPos(ImVec2(pad + btnW + pad, row2Y));
    
    // 녹화(Record) 버튼 토글 색상 상태 관리
    if (isRecording) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.0f, 0.0f, 1.0f)); // 켜져있을 땐 어두운 빨간 배경
        ImGui::PushStyleColor(ImGuiCol_Text, COL_RED); // 글씨는 빨간색
    }
    if (ImGui::Button(isRecording ? "RECORD [ON]" : "RECORD [OFF]", ImVec2(btnW, btnRowH))) {
        isRecording = !isRecording; // 상태 반전
        
        // 상태에 맞춰 ViewModel의 함수 호출
        if (isRecording) {
            viewModel->StartRecording();
        } else {
            viewModel->StopRecording();
        }
    }
    if (isRecording) ImGui::PopStyleColor(2);

    // [세 번째 줄 버튼 (Row 3)]
    float row3Y = row2Y + btnRowH + pad; 
    ImGui::SetCursorPos(ImVec2(pad, row3Y));
    if (ImGui::Button("Btn 1", ImVec2(btnW, btnRowH))) {
    }

    ImGui::SetCursorPos(ImVec2(pad + btnW + pad, row3Y));
    if (ImGui::Button("Btn 2", ImVec2(btnW, btnRowH))) {
         if (!viewModel->AutoConnect()) showCamAlert = true;
    }

    ImGui::PopStyleColor(3); 

    // 4. Alert Popup
    if (showCamAlert) {
        ImGui::OpenPopup("Alert");
        showCamAlert = false;
    }
    
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(30.0f, 30.0f));
    if (ImGui::BeginPopupModal("Alert", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar)) {
        ImGui::TextColored(COL_RED, "WARNING");
        ImGui::Separator(); 
        ImGui::Dummy(ImVec2(0.0f, 10.0f)); 
        ImGui::Text("Camera Not Found!\nPlease check the connection.");
        ImGui::Dummy(ImVec2(0.0f, 20.0f));
        
        float okBtnW = 150.0f;
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - okBtnW) * 0.5f);
        if (ImGui::Button("OK", ImVec2(okBtnW, 50.0f))) { 
            ImGui::CloseCurrentPopup(); 
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(2);

    // 5. Status Bar
    float statusY = winH - statusH; 
    ImGui::SetCursorPos(ImVec2(pad, statusY));
    ImGui::TextColored(COL_TEXT, "Status: %s", viewModel->GetStatus().c_str()); 

    // 스냅샷 알림 오버레이 렌더링
    ShowToastNotification();
}

void LiveView::ShowToastNotification() {
    if (showSnapshotToast) {
        toastTimer -= ImGui::GetIO().DeltaTime;
        if (toastTimer <= 0.0f) {
            showSnapshotToast = false;
        } else {
            ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, -2.0f));
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing;
            
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 20.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(30.0f, 15.0f));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 0.9f));
            
            ImGui::Begin("Toast", nullptr, flags);
            ImGui::TextColored(COL_ACCENT, "%s", snapshotMessage.c_str());
            ImGui::End();
            
            ImGui::PopStyleColor();
            ImGui::PopStyleVar(2);
        }
    }
}