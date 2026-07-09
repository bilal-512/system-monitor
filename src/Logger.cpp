#include "Logger.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <iostream>

namespace sysmon {

// ──────────────────────────────────────────────────────────────────────────────
// Internal timestamp helper (Windows-safe)
// ──────────────────────────────────────────────────────────────────────────────
std::string Logger::currentTimestamp() {
    using namespace std::chrono;
    auto now = system_clock::now();
    std::time_t t = system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string Logger::levelToStr(LogLevel l) {
    switch (l) {
        case LogLevel::INFO:      return "INFO ";
        case LogLevel::WARN:      return "WARN ";
        case LogLevel::ERROR_LVL: return "ERROR";
    }
    return "?????";
}

// ──────────────────────────────────────────────────────────────────────────────
// Constructor / Destructor
// ──────────────────────────────────────────────────────────────────────────────
Logger::Logger(const std::string& path, size_t maxEntries)
    : maxEntries_(maxEntries)
{
    // Ensure log directory exists by attempting to open file
    ofs_.open(path, std::ios::app);
    if (!ofs_.is_open()) {
        std::cerr << "[Logger] Warning: could not open log file: " << path << "\n";
    }
}

Logger::~Logger() {
    if (ofs_.is_open()) ofs_.close();
}

// ──────────────────────────────────────────────────────────────────────────────
// Core log method
// ──────────────────────────────────────────────────────────────────────────────
void Logger::log(LogLevel level, const std::string& msg) {
    std::lock_guard<std::mutex> lk(mtx_);
    std::string ts = currentTimestamp();
    // Write to file
    if (ofs_.is_open()) {
        ofs_ << ts << " [" << levelToStr(level) << "] " << msg << "\n";
        ofs_.flush();
    }
    // Keep in-memory history
    history_.push_back({ts, level, msg});
    while (history_.size() > maxEntries_) {
        history_.pop_front();
    }
}

void Logger::info(const std::string& msg)  { log(LogLevel::INFO,      msg); }
void Logger::warn(const std::string& msg)  { log(LogLevel::WARN,      msg); }
void Logger::error(const std::string& msg) { log(LogLevel::ERROR_LVL, msg); }

// ──────────────────────────────────────────────────────────────────────────────
// History access
// ──────────────────────────────────────────────────────────────────────────────
std::deque<LogEntry> Logger::getEntries() const {
    std::lock_guard<std::mutex> lk(mtx_);
    return history_;
}

std::deque<LogEntry> Logger::getEntriesByDate(const std::string& date) const {
    std::lock_guard<std::mutex> lk(mtx_);
    std::deque<LogEntry> result;
    for (auto& e : history_) {
        if (e.timestamp.substr(0, 10) == date) result.push_back(e);
    }
    return result;
}

void Logger::clear() {
    std::lock_guard<std::mutex> lk(mtx_);
    history_.clear();
}

} // namespace sysmon
