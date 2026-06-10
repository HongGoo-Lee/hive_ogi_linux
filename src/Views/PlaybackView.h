#ifndef PLAYBACK_VIEW_H
#define PLAYBACK_VIEW_H

#include "ViewState.h"
#include <vector>

class MainView;

class PlaybackView {
private:
    MainView* parentView;
    std::vector<VideoItem> videoList;

public:
    PlaybackView(MainView* parent);
    void Render();
    const std::vector<VideoItem>& GetVideoList() const { return videoList; }
};

#endif // PLAYBACK_VIEW_H