#include "PlayerView.h"
#include "MainView.h"
#include "Theme.h"
#include <imgui.h>

PlayerView::PlayerView(MainView* parent) : parentView(parent) {}

void PlayerView::Render() {
    float winW = ImGui::GetWindowWidth();
    float winH = ImGui::GetWindowHeight();
    float pad = 20.0f;

    // Header Area
    float headerH = 70.0f;
    ImGui::BeginChild("VP_Header", ImVec2(winW, headerH), false, ImGuiWindowFlags_NoScrollbar);
    ImGui::SetCursorPos(ImVec2(pad, (headerH - 40) * 0.5f));
    if (ImGui::Button("< Back", ImVec2(100, 40))) {
        parentView->ChangeState(AppState::PlaybackList); // 리스트로 돌아가기
    }
    
    // 현재 선택된 비디오 인덱스를 부모 뷰에서 가져오기 (실제 적용시 PlaybackView 데이터를 참조해야 함)
    std::string titleStr = "Video Player";
    
    ImVec2 titleSize = ImGui::CalcTextSize(titleStr.c_str());
    ImGui::SetCursorPos(ImVec2((winW - titleSize.x) * 0.5f, (headerH - titleSize.y) * 0.5f));
    ImGui::TextColored(COL_TEXT, "%s", titleStr.c_str());
    ImGui::EndChild();

    // Video Screen Area
    float videoH = winH * 0.5f;
    ImGui::SetCursorPos(ImVec2(pad, headerH + pad));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.05f, 0.08f, 1.0f));
    ImGui::BeginChild("VP_Screen", ImVec2(winW - pad*2, videoH), true);
    
    const char* vText = "Video Play Area (640x512)";
    ImVec2 vSize = ImGui::CalcTextSize(vText);
    ImGui::SetCursorPos(ImVec2((winW - pad*2 - vSize.x) * 0.5f, (videoH - vSize.y) * 0.5f));
    ImGui::TextDisabled("%s", vText);
    
    ImGui::EndChild();
    ImGui::PopStyleColor();

    // Progress Bar Area
    float progressY = headerH + pad + videoH + pad;
    ImGui::SetCursorPos(ImVec2(pad, progressY));
    ImGui::BeginChild("VP_Progress", ImVec2(winW - pad*2, 80.0f), false);
    
    ImGui::Text("Progress Bar (SeekBar)");
    static float progress = 0.25f; 
    ImGui::SliderFloat("##seek", &progress, 0.0f, 1.0f, "");
    
    ImGui::Text("01:23"); 
    ImGui::SameLine(winW - pad*2 - 50.0f); 
    ImGui::Text("05:00");
    
    ImGui::EndChild();

    // Control Buttons Area
    float controlY = progressY + 80.0f + pad;
    float btnSize = 70.0f;
    float spacing = 20.0f;
    float totalBtnWidth = (btnSize * 5) + (spacing * 4);
    
    ImGui::SetCursorPos(ImVec2((winW - totalBtnWidth) * 0.5f, controlY));
    
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, btnSize * 0.5f); 
    ImGui::Button("Stop", ImVec2(btnSize, btnSize)); ImGui::SameLine(0, spacing);
    ImGui::Button("Prev", ImVec2(btnSize, btnSize)); ImGui::SameLine(0, spacing);
    
    ImGui::PushStyleColor(ImGuiCol_Button, COL_ACCENT); 
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0,0,0,1));
    ImGui::Button("Play", ImVec2(btnSize, btnSize)); 
    ImGui::PopStyleColor(2);
    ImGui::SameLine(0, spacing);
    
    ImGui::Button("Next", ImVec2(btnSize, btnSize)); ImGui::SameLine(0, spacing);
    ImGui::Button("Vol", ImVec2(btnSize, btnSize));
    ImGui::PopStyleVar();
}