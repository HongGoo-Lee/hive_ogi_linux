# hive_ogi_linux

Linux (Raspberry Pi CM5, ARM64) 환경을 위한 HIVE OGI 카메라/녹화 애플리케이션입니다.

기존 Aravis 기반에서 **`rc_genicam_api`** 기반으로 카메라 백엔드가 변경되었으며, 라즈베리파이의 하드웨어 가속을 위해 데스크톱 OpenGL 대신 **OpenGL ES (GLESv2)**를 사용하도록 최적화되었습니다.

## Prerequisites (사전 요구 사항)

Ubuntu (Raspberry Pi OS 64-bit 등) 환경에서 다음 시스템 패키지를 설치해야 합니다:

sudo apt update  
sudo apt install build-essential cmake ninja-build git pkg-config libopencv-dev libgl1-mesa-dev libgles2-mesa-dev libxinerama-dev libxcursor-dev libxi-dev libxrandr-dev libwayland-dev wayland-protocols libxkbcommon-dev extra-cmake-modules

## External Libraries (외부 라이브러리 세팅)

이 프로젝트는 빌드 및 실행을 위해 다음과 같은 외부 라이브러리들을 사용합니다. 아래의 안내에 따라 세팅을 진행해 주세요.

1. **rc_genicam_api (수동 설치 필수)**
   * 카메라 제어 백엔드를 위해 시스템에 미리 설치되어 있어야 합니다.
   * CMake 과정에서 자동으로 찾을 수 있도록 `/usr/local/lib/cmake/rc_genicam_api` 또는 `/usr/local/lib/aarch64-linux-gnu/cmake/rc_genicam_api` 경로에 설치되도록 세팅해야 합니다.

2. **OpenCV & OpenGL ES (시스템 패키지로 설치됨)**
   * 영상 처리 및 하드웨어 가속 렌더링에 사용됩니다.
   * 위의 `Prerequisites` 단계에서 `apt install`로 `libopencv-dev`와 `libgles2-mesa-dev` 등을 설치하면 자동으로 세팅이 완료됩니다.

3. **GLFW (3.4) & Dear ImGui (v1.91.5) (자동 다운로드 및 빌드)**
   * GUI 화면 구성을 위해 사용됩니다.
   * **수동으로 설치할 필요가 없습니다.** 프로젝트 빌드(CMake) 시 `FetchContent` 모듈을 통해 Github에서 소스코드를 자동으로 다운로드하고 프로젝트와 함께 빌드(정적 링크)됩니다.

## Build (빌드 방법)

위의 의존성 라이브러리 및 패키지 세팅이 모두 완료되었다면, 다음 명령어로 프로젝트를 빌드합니다:

cmake --preset linux
cmake --build --preset linux

실행 파일은 `build/linux/hive_ogi_linux`에 생성됩니다.

## Features & Optimizations

* **Raspberry Pi CM5 최적화:** `cortex-a76` 하드웨어 타겟팅 및 컴파일러 최적화(-O3, -ffast-math) 적용
* **OpenGL ES 적용:** 임베디드 환경에 맞춘 GLESv2 렌더링 파이프라인(GLSL #version 300 es) 구성
* **수동 FPS 제한:** 리소스 관리를 위해 렌더링 루프를 약 30FPS 타겟으로 동작하도록 제한
* **안전한 종료 처리:** 프로그램 종료 시 `rc_genicam_api` 시스템 자원의 안전한 반환 보장

## Logs

앱은 실행 디렉터리 기준 `log/` 폴더에 `hive_ogi_YYYYMMDD_HHMMSS.txt` 형식의 로그 파일을 생성합니다.