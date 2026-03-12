#include "core/Logger.hpp"

#include <ctime>
#include <iomanip>
#include <iostream>

namespace logging {

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

Logger::Logger()
    : consoleEnabled_(true),
      fileEnabled_(false),
      level_(LogLevel::INFO),
      baseLevel_(LogLevel::INFO),
      verboseOverride_(false) {}

Logger::~Logger() {
    if (file_.is_open()) {
        file_.close();
    }
}

void Logger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    baseLevel_ = level;
    if (!verboseOverride_) {
        level_ = level;
    }
}

void Logger::setConsoleEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    consoleEnabled_ = enabled;
}

void Logger::setFileEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    fileEnabled_ = enabled && file_.is_open();
}

void Logger::setLogFile(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_.is_open()) {
        file_.close();
    }
    file_.open(path, std::ios::out | std::ios::app);
    fileEnabled_ = file_.is_open();
    if (!fileEnabled_) {
        std::cerr << "Logger: Dosya acilamadi: " << path << std::endl;
    }
}

void Logger::toggleVerbose() {
    std::lock_guard<std::mutex> lock(mutex_);
    verboseOverride_ = !verboseOverride_;
    level_ = verboseOverride_ ? LogLevel::DEBUG : baseLevel_;
}

bool Logger::isVerbose() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return verboseOverride_;
}

void Logger::log(LogLevel level, const std::string& module, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (static_cast<int>(level) < static_cast<int>(level_)) {
        return;
    }

    std::ostringstream oss;
    oss << "[" << timestampString() << "]"
        << "[" << levelToString(level) << "]"
        << "[" << module << "] "
        << message;

    const std::string line = oss.str();

    if (consoleEnabled_) {
        std::cout << line << std::endl;
    }
    if (fileEnabled_ && file_.is_open()) {
        file_ << line << std::endl;
    }
}

Logger::ScopedTimer::ScopedTimer(const std::string& module,
                                 const std::string& label,
                                 LogLevel level)
    : module_(module),
      label_(label),
      level_(level),
      start_(std::chrono::steady_clock::now()) {}

Logger::ScopedTimer::~ScopedTimer() {
    double ms = elapsedMs();
    std::ostringstream oss;
    oss << label_ << " took " << std::fixed << std::setprecision(3) << ms << " ms";
    Logger::instance().log(level_, module_, oss.str());
}

double Logger::ScopedTimer::elapsedMs() const {
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> diff = end - start_;
    return diff.count();
}

std::string Logger::timestampString() const {
    using std::chrono::system_clock;
    auto now = system_clock::now();
    std::time_t tt = system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&tt);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");
    return oss.str();
}

std::string Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::WARN:
            return "WARN";
        case LogLevel::ERROR:
        default:
            return "ERROR";
    }
}

}  // namespace logging
