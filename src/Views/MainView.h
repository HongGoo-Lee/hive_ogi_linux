#ifndef MAIN_VIEW_H
#define MAIN_VIEW_H

#include "ViewModels/MainViewModel.h"
#include "ImageTexture.h"
#include <string>

class MainView {
private:
    MainViewModel* viewModel;
    ImageTexture videoTexture;
    
    int windowW;
    int windowH;
    std::string windowTitle;

    bool showCamAlert = false;

public:
    MainView(MainViewModel* vm, int W, int H, const char* t);
    void Render();
};

#endif