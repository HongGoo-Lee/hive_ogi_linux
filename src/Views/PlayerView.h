#ifndef PLAYER_VIEW_H
#define PLAYER_VIEW_H

class MainView;

class PlayerView {
private:
    MainView* parentView;

public:
    PlayerView(MainView* parent);
    void Render();
};

#endif // PLAYER_VIEW_H