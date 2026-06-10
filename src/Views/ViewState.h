#ifndef VIEW_STATE_H
#define VIEW_STATE_H

#include <string>

// 애플리케이션의 현재 화면 상태를 나타내는 열거형
enum class AppState {
    Main,           // 메인 라이브 뷰 화면
    PlaybackList,   // 저장된 영상 리스트 화면
    VideoPlayer     // 영상 재생 화면
};

// Playback 리스트용 데이터 구조체
struct VideoItem {
    std::string title;
    std::string date;
    std::string duration;
};

#endif // VIEW_STATE_H