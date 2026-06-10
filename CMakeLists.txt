cmake_minimum_required(VERSION 3.20)
project(hive_ogi_linux CXX)
include_directories(/home/hivesystem/workspace/rc_genicam_api/genicam/library/CPP/include)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# [수정됨] 라즈베리파이 CM5 (Cortex-A76) 하드웨어 최적화 플래그 추가
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -mcpu=cortex-a76 -mtune=cortex-a76 -ffast-math")

include(FetchContent)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE)

FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.5
    GIT_SHALLOW TRUE)

# rc_genicam_api는 이미 시스템에 설치했으므로 FetchContent에서 뺐습니다.
FetchContent_MakeAvailable(glfw imgui)

add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
target_include_directories(imgui_lib PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui_lib PUBLIC glfw)

find_package(OpenCV REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

# 라즈베리파이(ARM64) 빌드 시 rc_genicam_api가 설치되는 전용 경로들을 탐색 목록에 강제 추가
list(APPEND CMAKE_PREFIX_PATH 
    "/usr/local/lib/cmake/rc_genicam_api"
    "/usr/local/lib/aarch64-linux-gnu/cmake/rc_genicam_api"
    "/usr/local/share/rc_genicam_api"
)

# 설치된 시스템 라이브러리를 찾습니다.
find_package(RC_GENICAM_API REQUIRED)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "src/*.cpp"
    "src/*.h")

add_executable(hive_ogi_linux ${SOURCES}
    src/Views/ViewState.h
    src/Views/LiveView.cpp)

target_include_directories(hive_ogi_linux PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OpenCV_INCLUDE_DIRS})

target_link_libraries(hive_ogi_linux PRIVATE
    imgui_lib
    # [수정됨] 데스크톱 OpenGL::GL 대신 라즈베리파이 하드웨어 가속을 위한 GLESv2 링크
    GLESv2
    ${OpenCV_LIBS}
    rc_genicam_api)

target_link_libraries(hive_ogi_linux PRIVATE rc_genicam_api)