# hive_ogi_linux

Linux Ubuntu용 HIVE OGI 카메라/녹화 애플리케이션입니다.

카메라 백엔드는 `rc_genicam_api`를 사용하지 않고 Aravis `aravis-0.8`을 사용합니다.

## Prerequisites

Ubuntu 24.04:

```bash
sudo apt update
sudo apt install build-essential cmake ninja-build git pkg-config \
  libopencv-dev libaravis-dev aravis-tools-cli \
  libgl1-mesa-dev libxinerama-dev libxcursor-dev libxi-dev libxrandr-dev \
  libwayland-dev wayland-protocols libxkbcommon-dev extra-cmake-modules
```

카메라 인식 확인:

```bash
arv-tool-0.8 list
```

WSL처럼 broadcast discovery가 안 되는 환경에서는 카메라 IP를 직접 지정할 수 있습니다:

```bash
export HIVE_CAMERA_IP=169.254.193.5
./build/linux/hive_ogi_linux
```

앱은 기본적으로 GigE Vision discovery를 먼저 수행하고, 발견된 카메라 IP가
호스트 NIC의 subnet과 다르면 GigE Vision FORCEIP 명령으로 접근 가능한 IP를
임시 할당한 뒤 접속합니다. 특정 IP로 강제 할당하려면 다음 환경변수를 사용합니다:

```bash
export HIVE_FORCE_CAMERA_IP=169.254.193.5
./build/linux/hive_ogi_linux
```

## Build

```bash
cmake --preset linux
cmake --build --preset linux
```

실행 파일은 `build/linux/hive_ogi_linux`에 생성됩니다.

## Logs

앱은 실행 디렉터리 기준 `log/` 폴더에 `hive_ogi_YYYYMMDD_HHMMSS.txt`
형식의 로그 파일을 생성합니다. 카메라 discovery, ForceIP, acquisition,
stream buffer status, frame timeout, pixel format, 녹화 상태가 기록됩니다.
