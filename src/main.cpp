#include "./Views/MainView.h"
#include "./Common/Logger.h"

#include <GLFW/glfw3.h>
#include <chrono>
#include <cstdlib> // std::exit 사용
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include <thread>

int main() {
    LOG_INFO("Application starting");
    LOG_INFO("Log file: " + Logger::instance().path());

    if (!glfwInit()) {
        LOG_ERROR("glfwInit failed");
        return -1;
    }

    // 라즈베리파이(Linux)용 OpenGL 버전 설정
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);

    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
    if (!mode) {
        LOG_ERROR("glfwGetVideoMode returned null");
        glfwTerminate();
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "HIVE OGI", primaryMonitor, nullptr);
    if (!window) {
        LOG_ERROR("glfwCreateWindow failed");
        glfwTerminate();
        return -1;
    }
    LOG_INFO("GLFW window created " + std::to_string(mode->width) + "x" + std::to_string(mode->height));

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    io.Fonts->AddFontDefault();
    // 라즈베리파이 기본 폰트 적용
    io.Fonts->AddFontFromFileTTF("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 26.0f);

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 140");

    CameraManager camMgr;
    RecordManager recMgr;
    LOG_INFO("RecordManager size: " + std::to_string(sizeof(RecordManager)) + " Byte");
    ImageProcessor imgProc;
    DiskManager diskMgr;

    MainViewModel viewModel(camMgr, imgProc, recMgr, diskMgr);
    MainView view(&viewModel, mode->width, mode->height, "HIVE OGI");

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    recMgr.startThread();
    diskMgr.start();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        view.Render();

        ImGui::Render();
        int display_w = 0;
        int display_h = 0;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    diskMgr.stop();
    recMgr.stopThread();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    LOG_INFO("Application exiting");
    
    // 유령 프로세스(포트 꼬임) 방지용 완벽 종료
    std::exit(0); 
    return 0;
}