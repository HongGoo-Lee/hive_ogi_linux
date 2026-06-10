#include "CameraManager.h"
#include "../Common/Logger.h"

// rc_genicam_api 헤더 포함
#include <rc_genicam_api/system.h>
#include <rc_genicam_api/interface.h>
#include <rc_genicam_api/device.h>
#include <rc_genicam_api/stream.h>
#include <rc_genicam_api/buffer.h>
#include <rc_genicam_api/config.h>

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
                count += devices.size();
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

    const char* cameraIp = std::getenv("HIVE_CAMERA_IP");
    try {
        if (cameraIp && cameraIp[0] != '\0') {
            impl_->deviceId = cameraIp;
            LOG_INFO("[TRACE] connectCamera - Using HIVE_CAMERA_IP=" + impl_->deviceId);
            impl_->device = rcg::getDevice(impl_->deviceId.c_str());
        } else {
            LOG_INFO("[TRACE] connectCamera - Searching for device ID...");
            auto systems = rcg::System::getSystems();
            for (auto& sys : systems) {
                sys->open();
                for (auto& interf : sys->getInterfaces()) {
                    interf->open();
                    auto devices = interf->getDevices();
                    if (!devices.empty()) {
                        impl_->device = devices[0];
                        impl_->deviceId = impl_->device->getID();
                        LOG_INFO("[TRACE] connectCamera - Found device ID=" + impl_->deviceId);
                        break;
                    }
                }
                if (impl_->device) break;
            }
        }

        if (!impl_->device) {
            LOG_ERROR("[TRACE] connectCamera - No device found via rc_genicam_api");
            return false;
        }

        LOG_INFO("[TRACE] connectCamera - device->open() calling...");
        impl_->device->open(rcg::Device::CONTROL);

        LOG_INFO("[TRACE] connectCamera - Getting model and serial...");
        auto nodemap = impl_->device->getRemoteNodeMap();
        impl_->modelName = rcg::getString(nodemap, "DeviceModelName", true, "Unknown");
        impl_->serialNumber = rcg::getString(nodemap, "DeviceSerialNumber", true, "Unknown");

        LOG_INFO("[TRACE] connectCamera - End (Success)");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("[TRACE] connectCamera - Failed to open camera: ") + e.what());
        return false;
    }
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
        impl_->stream->open();

        LOG_INFO("[TRACE] startStreaming - stream->startStreaming() calling...");
        // rc_genicam_api는 버퍼 할당 및 관리를 내부에서 자동으로 처리합니다.
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
    if (!connectCamera()) {
        LOG_ERROR("[TRACE] autoConnectAndStart - connectCamera failed");
        return false;
    }

    try {
        auto nodemap = impl_->device->getRemoteNodeMap();
        setPixelFormat(nodemap);

        LOG_INFO("[TRACE] autoConnectAndStart - set AcquisitionMode to Continuous...");
        rcg::setEnum(nodemap, "AcquisitionMode", "Continuous", true);
    } catch (const std::exception& e) {
        LOG_WARN(std::string("[TRACE] autoConnectAndStart - set mode warning: ") + e.what());
    }

    LOG_INFO("[TRACE] autoConnectAndStart - Calling startStreaming()...");
    const bool started = startStreaming();
    if (!started) {
        LOG_INFO("[TRACE] autoConnectAndStart - startStreaming failed, ensuring teardown");
        stopStreaming();
    }

    LOG_INFO(std::string("[TRACE] autoConnectAndStart - End result=") + (started ? "success" : "failed"));
    return started;
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

        if (buffer->getIsIncomplete()) {
            impl_->badStatusCount++;
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
            // GenICam PFNC 0x01100007은 Mono16을 의미합니다.
            if (pixelFormat == 0x01100007 && size >= static_cast<size_t>(width) * height * 2) {
                // rc_genicam_api의 buffer 메모리는 copyTo를 통해 복사되므로 별도의 메모리 관리가 필요하지 않습니다.
                cv::Mat(height, width, CV_16UC1, const_cast<void*>(data)).copyTo(frame);
            }
            else if (size >= static_cast<size_t>(width) * height) {
                cv::Mat(height, width, CV_8UC1, const_cast<void*>(data)).copyTo(frame);
            }
        }
        return frame;
    } catch (const std::exception& e) {
        impl_->timeoutCount++;
        return cv::Mat();
    }
}

bool CameraManager::isStreaming() const { return impl_->isStreaming; }
std::string CameraManager::getModelName() { return impl_->modelName; }
std::string CameraManager::getSerialNumber() { return impl_->serialNumber; }