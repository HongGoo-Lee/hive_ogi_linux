#include "Logger.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <thread>

namespace {
std::tm localTime(std::time_t value) {
    std::tm result{};
    localtime_r(&value, &result);
    return result;
}

std::string timestampForFile() {
    const auto now = std::chrono::system_clock::now();
    const auto nowTime = std::chrono::system_clock::to_time_t(now);
    const auto tm = localTime(nowTime);

    std::ostringstream out;
    out << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return out.str();
}

std::string timestampForLine() {
    const auto now = std::chrono::system_clock::now();
    const auto nowTime = std::chrono::system_clock::to_time_t(now);
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        now.time_since_epoch()) %
                    1000;
    const auto tm = localTime(nowTime);

    std::ostringstream out;
    out << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
        << "." << std::setw(3) << std::setfill('0') << ms.count();
    return out.str();
}
}

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

Logger::Logger() {
    std::filesystem::create_directories("log");
    path_ = (std::filesystem::path("log") / ("hive_ogi_" + timestampForFile() + ".txt")).string();
    file_.open(path_, std::ios::out | std::ios::app);

    if (!file_) {
        std::cerr << "[Logger] Failed to open log file: " << path_ << std::endl;
    }
    else {
        write("INFO", "Logger initialized: " + path_);
    }
}

void Logger::info(const std::string& message) {
    write("INFO", message);
}

void Logger::warn(const std::string& message) {
    write("WARN", message);
}

void Logger::error(const std::string& message) {
    write("ERROR", message);
}

void Logger::debug(const std::string& message) {
    write("DEBUG", message);
}

std::string Logger::path() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return path_;
}

void Logger::write(const char* level, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::ostringstream line;
    line << timestampForLine()
         << " [" << level << "]"
         << " [tid=" << std::this_thread::get_id() << "] "
         << message;

    if (file_) {
        file_ << line.str() << '\n';
        file_.flush();
    }

    std::cout << line.str() << std::endl;
}