#include "PlaybackView.h"
#include "MainView.h"
#include "Theme.h"
#include <imgui.h>

PlaybackView::PlaybackView(MainView* parent) : parentView(parent) {
    videoList = {
        {"Event 1", "2021-12-13 17:07:51", "00:00"},
        {"Recording 2", "2021-12-13 17:07:51", "00:02"},
        {"Recording 1", "2021-12-13 17:27:51", "00:00"},
        {"Recording 2", "2021-12-13 17:27:51", "00:00"},
        {"Recording 3", "2021-12-13 17:27:51", "00:00"}
    };
}

void PlaybackView::Render() {
    float winW = ImGui::GetWindowWidth();
    float winH = ImGui::GetWindowHeight();
    float pad = 20.0f;

    // Header Area
    float headerH = 70.0f;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(20.f/255.f, 24.f/255.f, 30.f/255.f, 1.f));
    ImGui::BeginChild("PB_Header", ImVec2(winW, headerH), false, ImGuiWindowFlags_NoScrollbar);
    
    ImGui::SetCursorPos(ImVec2(pad, (headerH - 40) * 0.5f));
    if (ImGui::Button("< Back", ImVec2(100, 40))) {
        parentView->ChangeState(AppState::Main); // 메인으로 복귀 요청
    }
    
    ImVec2 titleSize = ImGui::CalcTextSize("Playlist (영상 목록)");
    ImGui::SetCursorPos(ImVec2((winW - titleSize.x) * 0.5f, (headerH - titleSize.y) * 0.5f));
    ImGui::TextColored(COL_TEXT, "Playlist (영상 목록)");
    ImGui::EndChild();
    ImGui::PopStyleColor();

    // Filters and Options Toolbar
    float toolbarH = 50.0f;
    ImGui::SetCursorPos(ImVec2(pad, headerH + pad));
    ImGui::BeginChild("PB_Toolbar", ImVec2(winW - pad*2, toolbarH), false);
    
    ImGui::Button("정렬"); ImGui::SameLine(0, 10);
    ImGui::Button("역순정렬"); ImGui::SameLine(0, 10);
    ImGui::Button("전체영상"); ImGui::SameLine(0, 10);
    ImGui::Button("Event만"); ImGui::SameLine(0, 10);
    ImGui::Button("Normal만"); ImGui::SameLine(0, 10);
    ImGui::Button("날짜선택"); ImGui::SameLine(0, 10);
    ImGui::Button("기간선택");

    ImGui::EndChild();

    // Video List Area
    float listY = headerH + pad + toolbarH + pad;
    float listH = winH - listY - pad;
    
    ImGui::SetCursorPos(ImVec2(pad, listY));
    ImGui::BeginChild("PB_List", ImVec2(winW - pad*2, listH), true);
    
    for (int i = 0; i < videoList.size(); ++i) {
        ImGui::PushID(i);
        
        float itemH = 90.0f;
        ImGui::BeginChild("Item", ImVec2(0, itemH), true);
        
        ImGui::Columns(3, "ItemCols", false);
        ImGui::SetColumnWidth(0, 130.0f);
        
        // Thumbnail Area
        ImGui::PushStyleColor(ImGuiCol_Button, COL_CARD);
        ImGui::Button("PLAY\nICON", ImVec2(110, 70));
        ImGui::PopStyleColor();
        ImGui::NextColumn();
        
        // Info Area
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 15.0f);
        ImGui::Text("%s", videoList[i].title.c_str());
        ImGui::TextDisabled("%s", videoList[i].date.c_str());
        ImGui::NextColumn();
        
        // Duration Area
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 35.0f);
        ImGui::Text("%s", videoList[i].duration.c_str());
        
        ImGui::Columns(1);
        ImGui::EndChild();
        
        // 클릭 영역 처리
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - itemH - ImGui::GetStyle().ItemSpacing.y);
        if (ImGui::InvisibleButton("ClickArea", ImVec2(ImGui::GetWindowWidth(), itemH))) {
            parentView->SetSelectedVideoIndex(i);          // 부모에게 선택된 영상 인덱스 알림
            parentView->ChangeState(AppState::VideoPlayer); // 재생 화면으로 전환 요청
        }
        ImGui::PopID();
    }
    ImGui::EndChild();
}