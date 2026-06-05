#pragma once

#include <fstream>
#include <mutex>
#include <sstream>
#include <string>

class Logger {
public:
    static Logger& instance();

    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);
    void debug(const std::string& message);

    std::string path() const;

private:
    Logger();

    void write(const char* level, const std::string& message);

    mutable std::mutex mutex_;
    std::ofstream file_;
    std::string path_;
};

#define LOG_INFO(message) Logger::instance().info(message)
#define LOG_WARN(message) Logger::instance().warn(message)
#define LOG_ERROR(message) Logger::instance().error(message)
#define LOG_DEBUG(message) Logger::instance().debug(message)

template <typename T>
std::string logString(const T& value) {
    std::ostringstream out;
    out << value;
    return out.str();
}
