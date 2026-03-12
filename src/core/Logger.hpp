#pragma once

#include <chrono>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>

namespace logging {

enum class LogLevel {
    DEBUG = 0,
    INFO,
    WARN,
    ERROR
};

class Logger {
public:
    static Logger& instance();

    void setLevel(LogLevel level);
    void setConsoleEnabled(bool enabled);
    void setFileEnabled(bool enabled);
    void setLogFile(const std::string& path);

    void toggleVerbose();
    bool isVerbose() const;

    void log(LogLevel level, const std::string& module, const std::string& message);

    class ScopedTimer {
    public:
        ScopedTimer(const std::string& module,
                    const std::string& label,
                    LogLevel level = LogLevel::DEBUG);
        ~ScopedTimer();

        double elapsedMs() const;

    private:
        std::string module_;
        std::string label_;
        LogLevel level_;
        std::chrono::steady_clock::time_point start_;
    };

private:
    Logger();
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string timestampString() const;
    std::string levelToString(LogLevel level) const;

    mutable std::mutex mutex_;
    std::ofstream file_;
    bool consoleEnabled_;
    bool fileEnabled_;
    LogLevel level_;
    LogLevel baseLevel_;
    bool verboseOverride_;
};

#define LOG_DEBUG(module, msg) \
    logging::Logger::instance().log(logging::LogLevel::DEBUG, module, msg)

#define LOG_INFO(module, msg) \
    logging::Logger::instance().log(logging::LogLevel::INFO, module, msg)

#define LOG_WARN(module, msg) \
    logging::Logger::instance().log(logging::LogLevel::WARN, module, msg)

#define LOG_ERROR(module, msg) \
    logging::Logger::instance().log(logging::LogLevel::ERROR, module, msg)

}  // namespace logging
