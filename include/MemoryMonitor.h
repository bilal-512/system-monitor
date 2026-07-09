#pragma once

#include <string>

namespace sysmon {

struct MemoryStats {
    unsigned long long totalPhys    = 0;  // bytes
    unsigned long long usedPhys     = 0;  // bytes
    unsigned long long availPhys    = 0;  // bytes
    double             usedPercent  = 0.0;

    // Swap / Page file
    unsigned long long totalSwap    = 0;  // bytes
    unsigned long long usedSwap     = 0;  // bytes
    unsigned long long availSwap    = 0;  // bytes
    double             swapPercent  = 0.0;
};

/**
 * @brief Real-time memory statistics.
 * Windows: GlobalMemoryStatusEx().
 * Linux: /proc/meminfo.
 */
class MemoryMonitor {
public:
    MemoryMonitor() = default;

    // Refresh and return current stats
    MemoryStats refresh();
    MemoryStats getLastStats() const;

    // Convenience helpers
    static std::string formatBytes(unsigned long long bytes);

private:
    MemoryStats last_;
};

} // namespace sysmon
