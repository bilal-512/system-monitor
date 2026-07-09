#pragma once

#include <atomic>
#include <thread>
#include <functional>
#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include <cstdint>

namespace sysmon {

/**
 * @brief Background CPU monitor.
 * Uses GetSystemTimes() on Windows, /proc/stat on Linux.
 * Maintains a rolling history of CPU usage readings.
 */
class CpuMonitor {
public:
    using Callback = std::function<void(double)>; // percent [0..100]

    explicit CpuMonitor(int intervalMs = 1000, size_t historySize = 60);
    ~CpuMonitor();

    void start(Callback cb = nullptr);
    void stop();

    double getCurrentUsage() const;
    std::vector<double> getHistory() const;
    double getAverageUsage() const;
    double getPeakUsage() const;

    int getCoreCount() const;

private:
    void runLoop();

#ifdef _WIN32
    struct WinCpuTimes {
        uint64_t idle  = 0;
        uint64_t kernel = 0;
        uint64_t user  = 0;
    };
    static WinCpuTimes getWinTimes();
    WinCpuTimes prevTimes_;
#else
    unsigned long long prevIdle_  = 0;
    unsigned long long prevTotal_ = 0;
    static bool readProcStat(unsigned long long& idle, unsigned long long& total);
#endif

    int                 intervalMs_;
    size_t              historySize_;
    std::atomic<bool>   running_;
    std::thread         worker_;
    Callback            cb_;

    mutable std::mutex  dataMtx_;
    double              currentUsage_{0.0};
    std::deque<double>  history_;
    int                 coreCount_{1};
};

} // namespace sysmon
