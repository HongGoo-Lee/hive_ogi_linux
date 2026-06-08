#include "./Views/MainView.h"
#include "./Common/Logger.h"

#include <GLFW/glfw3.h>
#include <chrono>
#include <cstdlib> // std::exit 사용 (하지만 안전한 종료를 위해 return으로 변경)
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

    // [수정됨] 라즈베리파이(Linux)용 OpenGL ES(임베디드 시스템용 API) 설정으로 변경
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API); // 추가됨: GLES 사용
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    
    // [수정됨] ES 환경에서는 Profile 설정이 필요 없으므로 제거/주석처리
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);

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
    
    // [수정됨] OpenGL ES 3.0 이상에 맞는 GLSL 쉐이더 버전 선언으로 변경
    ImGui_ImplOpenGL3_Init("#version 300 es");

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

    // while 루프 진입 전, 이전 프레임 시간 기록용 변수 초기화
    auto lastTime = std::chrono::high_resolution_clock::now();

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
        
        // ==========================================
        // [추가] 수동 FPS 제한 (약 30FPS 타겟)
        // ==========================================
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastTime).count();

        // 1프레임당 33ms (약 30FPS 기준) 미만으로 걸렸다면 남은 시간만큼 강제 휴식
        if (elapsedTime < 33) {
            std::this_thread::sleep_for(std::chrono::milliseconds(33 - elapsedTime));
        }
        lastTime = std::chrono::high_resolution_clock::now();
    }

    diskMgr.stop();
    recMgr.stopThread();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    LOG_INFO("Application exiting");
    
    // [수정됨] 유령 프로세스(포트 꼬임) 방지를 위해 exit(0) 대신 return 0; 를 사용합니다.
    // 이렇게 해야 CameraManager 등의 C++ 소멸자(Destructor)가 정상적으로 호출되어
    // rc_genicam_api의 clearSystems() 자원 반환이 안전하게 이루어집니다.
    return 0;
}