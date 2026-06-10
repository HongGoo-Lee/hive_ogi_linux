#include "CameraManager.h"
#include "../Common/Logger.h"

// rc_genicam_api 헤더 포함
#include <rc_genicam_api/system.h>
#include <rc_genicam_api/interface.h>
#include <rc_genicam_api/device.h>
#include <rc_genicam_api/stream.h>
#include <rc_genicam_api/buffer.h>
#include <rc_genicam_api/config.h>
#include <rc_genicam_api/exception.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {
// 필요한 경우 16진수 문자열 변환
std::string hexValue(uint64_t value) {
    std::ostringstream out;
    out << std::hex << value;
    return out.str();
}

// 픽셀 포맷 강제 설정
bool setPixelFormat(std::shared_ptr<GenApi::CNodeMapRef> nodemap) {
    LOG_INFO("[TRACE] setPixelFormat - Start");
    for (const char* format : {"Mono16", "Mono14", "Mono12", "Mono8"}) {
        try {
            rcg::setEnum(nodemap, "PixelFormat", format, true);
            LOG_INFO(std::string("[TRACE] setPixelFormat - Success: ") + format);
            return true;
        } catch (...) {
            // 지원하지 않는 포맷일 경우 예외가 발생하므로 무시하고 다음 시도
        }
    }
    LOG_WARN("[TRACE] setPixelFormat - Could not force Mono formats.");
    return false;
}
} // namespace

struct CameraManager::Impl {
    std::shared_ptr<rcg::Device> device;
    std::shared_ptr<rcg::Stream> stream;
    std::shared_ptr<rcg::System> currentSystem;
    std::shared_ptr<rcg::Interface> currentInterface;
    bool isStreaming = false;
    std::string deviceId;
    std::string modelName = "Unknown";
    std::string serialNumber = "Unknown";
    uint64_t frameCount = 0;
    uint64_t timeoutCount = 0;
    uint64_t badStatusCount = 0;
    bool loggedFirstFrame = false;
};

CameraManager::CameraManager() : impl_(std::make_unique<Impl>()) {}

CameraManager::~CameraManager() {
    stopStreaming();
    rcg::System::clearSystems(); // 프로그램 종료 시 할당된 시스템 강제 회수
    LOG_INFO("rc_genicam_api shutdown complete.");
}

int CameraManager::searchCameras() {
    LOG_INFO("[TRACE] searchCameras - Start");
    int count = 0;
    try {
        auto systems = rcg::System::getSystems();
        for (auto& sys : systems) {
            sys->open();
            for (auto& interf : sys->getInterfaces()) {
                interf->open();
                auto devices = interf->getDevices();
                count += static_cast<int>(devices.size()); // 다시 모든 장치를 카운트하도록 복구
                interf->close();
            }
            sys->close();
        }
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("[TRACE] searchCameras error: ") + e.what());
    }
    LOG_INFO("[TRACE] searchCameras - Found " + std::to_string(count) + " device(s)");
    return count;
}

bool CameraManager::connectCamera() {
    LOG_INFO("[TRACE] connectCamera - Start");

    auto systems = rcg::System::getSystems();
    for (auto& sys : systems) {
        sys->open();
        auto interfaces = sys->getInterfaces();

        for (auto& inter : interfaces) {
            inter->open();
            auto devices = inter->getDevices();

            if (!devices.empty()) {
                // 원래대로 가장 먼저 발견된 장치(실제 카메라 모듈)를 사용합니다.
                impl_->device = devices[0]; 
                impl_->deviceId = impl_->device->getID();
                impl_->currentSystem = sys;
                impl_->currentInterface = inter;

                LOG_INFO("[TRACE] connectCamera - Found device ID: " + impl_->deviceId);

                try {
                    impl_->device->open(rcg::Device::CONTROL);
                    LOG_INFO("[TRACE] connectCamera - Connected!");

                    auto nodemap = impl_->device->getRemoteNodeMap();
                    impl_->modelName = rcg::getString(nodemap, "DeviceModelName", true, "Unknown");
                    impl_->serialNumber = rcg::getString(nodemap, "DeviceSerialNumber", true, "Unknown");

                    // --- [핵심 수정 부분] 라즈베리파이 MTU 한계(1500)에 맞춰 카메라 패킷 사이즈 강제 조정 ---
                    try {
                        rcg::setInteger(nodemap, "GevSCPSPacketSize", 1400, true);
                        LOG_INFO("[TRACE] Forced Network Packet Size (GevSCPSPacketSize) to 1400 for Raspberry Pi stability.");
                    } catch (...) {
                        LOG_WARN("[TRACE] Could not force packet size. If empty frames persist, check Jumbo Frame settings.");
                    }

                    // --- 라즈베리파이 기가비트 이더넷 과부하 방지를 위한 패킷 딜레이 설정 ---
                    try {
                        rcg::setInteger(nodemap, "GevSCPD", 2000, true);
                    } catch (...) {}

                    // --- [해결 부분] 연결 후 카메라의 영상 스트리밍을 곧바로 시작하게 만듭니다. ---
                    try { rcg::setEnum(nodemap, "TestPattern", "Off", true); } catch(...) {}
                    try { rcg::setEnum(nodemap, "TestImageSelector", "Off", true); } catch(...) {}

                    for (const char* pixelFormat : {"Mono16", "Mono14", "Mono12", "Mono8"}) {
                        try {
                            rcg::setEnum(nodemap, "PixelFormat", pixelFormat, true);
                            LOG_INFO(std::string("[TRACE] PixelFormat set to: ") + pixelFormat);
                            break;
                        } catch (...) {}
                    }

                    // 이제 연결이 끝나면 멈추는게 아니라 스트리밍을 바로 시작(Return) 합니다!
                    return startStreaming();
                }
                catch (const rcg::GenTLException& e) {
                    LOG_ERROR(std::string("[Camera Error] Failed to open device (GenTL): ") + e.what());
                    return false;
                }
                catch (const std::exception& e) {
                    LOG_ERROR(std::string("[Camera Error] Unexpected error: ") + e.what());
                    return false;
                }
            }
            inter->close();
        }
        sys->close();
    }

    LOG_ERROR("[TRACE] connectCamera - No device found across any interface");
    return false;
}

bool CameraManager::startStreaming() {
    LOG_INFO("[TRACE] startStreaming - Start");
    if (!impl_->device) {
        LOG_ERROR("[TRACE] startStreaming - Camera is null");
        return false;
    }

    try {
        LOG_INFO("[TRACE] startStreaming - Getting streams...");
        auto streams = impl_->device->getStreams();
        if (streams.empty()) {
            LOG_ERROR("[TRACE] startStreaming - No streams found");
            stopStreaming();
            return false;
        }

        impl_->stream = streams[0];
        if (!impl_->stream) {
            LOG_ERROR("[TRACE] startStreaming - Failed to get stream");
            return false;
        }
        
        LOG_INFO("[TRACE] startStreaming - stream->startStreaming() calling...");
        impl_->stream->open();
        impl_->stream->startStreaming();

        LOG_INFO("[TRACE] startStreaming - AcquisitionStart calling...");
        auto nodemap = impl_->device->getRemoteNodeMap();
        rcg::callCommand(nodemap, "AcquisitionStart", true);

        impl_->isStreaming = true;
        impl_->frameCount = 0;
        impl_->timeoutCount = 0;
        impl_->badStatusCount = 0;
        impl_->loggedFirstFrame = false;

        LOG_INFO("[TRACE] startStreaming - End (Success)");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("[TRACE] startStreaming - Exception: ") + e.what());
        stopStreaming();
        return false;
    }
}

void CameraManager::stopStreaming() {
    LOG_INFO("[TRACE] stopStreaming - Start");
    try {
        if (impl_->device && impl_->isStreaming) {
            LOG_INFO("[TRACE] stopStreaming - AcquisitionStop calling...");
            auto nodemap = impl_->device->getRemoteNodeMap();
            rcg::callCommand(nodemap, "AcquisitionStop", true);
        }
    } catch (const std::exception& e) {
        LOG_WARN(std::string("[TRACE] stopStreaming - AcquisitionStop warning: ") + e.what());
    }

    impl_->isStreaming = false;

    if (impl_->stream) {
        LOG_INFO("[TRACE] stopStreaming - closing stream");
        try {
            impl_->stream->stopStreaming();
            impl_->stream->close();
        } catch(...) {}
        impl_->stream.reset();
    }

    if (impl_->device) {
        LOG_INFO("[TRACE] stopStreaming - closing device");
        try {
            impl_->device->close();
        } catch(...) {}
        impl_->device.reset();
    }
    LOG_INFO("[TRACE] stopStreaming - End");
}

bool CameraManager::autoConnectAndStart() {
    LOG_INFO("[TRACE] autoConnectAndStart - Start");
    // 이제 connectCamera() 내부에서 startStreaming()까지 모두 호출해주므로 단순화됩니다.
    return connectCamera();
}

cv::Mat CameraManager::grabFrame(int timeout_ms) {
    if (!impl_->isStreaming || !impl_->stream) {
        return cv::Mat();
    }

    try {
        const rcg::Buffer* buffer = impl_->stream->grab(timeout_ms);

        if (!buffer) {
            impl_->timeoutCount++;
            return cv::Mat();
        }

        // --- [빈 프레임 원인] 데이터 패킷 드롭으로 인한 불완전 버퍼 로그 ---
        if (buffer->getIsIncomplete()) {
            impl_->badStatusCount++;
            if (impl_->badStatusCount % 30 == 1) { // 로그 폭주 방지
                LOG_WARN("[CameraManager] Incomplete Buffer detected! Packet lost due to network MTU issues.");
            }
            return cv::Mat();
        }

        cv::Mat frame;
        size_t size = buffer->getSize(0);
        const void* data = buffer->getBase(0);
        const int width = static_cast<int>(buffer->getWidth(0));
        const int height = static_cast<int>(buffer->getHeight(0));
        const uint64_t pixelFormat = buffer->getPixelFormat(0); 

        if (!impl_->loggedFirstFrame) {
            LOG_INFO("[TRACE] grabFrame - FIRST FRAME SUCCESS width=" + std::to_string(width) + " height=" + std::to_string(height));
            impl_->loggedFirstFrame = true;
        }

        if (data && width > 0 && height > 0) {
            // GenICam PFNC 포맷 파싱 로직 및 안전한 복사
            // Mono16 (0x01100007) 또는 16비트 Mono 데이터를 처리
            if (pixelFormat == 0x01100007) {
                cv::Mat temp(height, width, CV_16UC1, const_cast<void*>(data));
                temp.copyTo(frame);
            }
            // 8비트 데이터 처리
            else if (pixelFormat == 0x01080008 || pixelFormat == 0x01080009) {
                cv::Mat temp(height, width, CV_8UC1, const_cast<void*>(data));
                temp.copyTo(frame);
            }
            // 알 수 없는 포맷인 경우라도 안전하게 복사 시도
            else {
                cv::Mat temp(height, width, CV_8UC1, const_cast<void*>(data));
                temp.copyTo(frame);
            }
        }
        return frame;
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("[Camera Error] grabFrame Exception: ") + e.what());
        impl_->timeoutCount++;
        return cv::Mat();
    }
}

bool CameraManager::isStreaming() const { return impl_->isStreaming; }
std::string CameraManager::getModelName() { return impl_->modelName; }
std::string CameraManager::getSerialNumber() { return impl_->serialNumber; }