#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <deque>

namespace sysmon {

enum class LogLevel { INFO, WARN, ERROR_LVL };

struct LogEntry {
    std::string timestamp;
    LogLevel    level;
    std::string message;
};

/**
 * @brief Thread-safe logger with file output and in-memory history.
 * Windows-compatible (uses localtime_s).
 */
class Logger {
public:
    explicit Logger(const std::string& path, size_t maxEntries = 10000);
    ~Logger();

    void info(const std::string& msg);
    void warn(const std::string& msg);
    void error(const std::string& msg);

    // Retrieve recent log entries for display/search
    std::deque<LogEntry> getEntries() const;
    std::deque<LogEntry> getEntriesByDate(const std::string& date) const;
    void clear();

private:
    void log(LogLevel level, const std::string& msg);
    static std::string currentTimestamp();
    static std::string levelToStr(LogLevel l);

    std::ofstream          ofs_;
    mutable std::mutex     mtx_;
    std::deque<LogEntry>   history_;
    size_t                 maxEntries_;
};

} // namespace sysmon
