#pragma once
#include <string>

struct HiveConfig {
    static inline const std::string BASE_DIR = "Recordings/VIDEO";
    static inline const std::string BASE_DIR_SnapShot = "Recordings/SNAPSHOT";
    static inline const std::string ROOT_DRIVE = ".";

    static inline const int REC_SEGMENT_DURATION_MS = 5 * 60 * 1000;
    static inline const double DISK_USAGE_THRESHOLD = 0.70;

    // 추가된 UI 및 카메라 해상도 / 히스토그램 설정
    static inline const int RES_WIDTH = 600;
    static inline const int RES_HEIGHT = 1024;
    static inline const int DISPLAY_WIDTH = 640;
    static inline const int DISPLAY_HEIGHT = 480;
    static inline const int HIST_SIZE = 4095;

    static inline const int TIMEOUT_MS = 20; // grabFrame 타임아웃 (밀리초)
    static inline const int GevSCPD = 2000; // GevSCPD 기본값 (밀리초) - 카메라에 따라 2000~5000 사이로 조정 필요
};