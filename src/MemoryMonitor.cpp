#include "MemoryMonitor.h"
#include <sstream>
#include <iomanip>

#ifdef _WIN32
#   define NOMINMAX
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#else
#   include <fstream>
#   include <string>
#   include <unordered_map>
#endif

namespace sysmon {

// ──────────────────────────────────────────────────────────────────────────────
// Refresh
// ──────────────────────────────────────────────────────────────────────────────
MemoryStats MemoryMonitor::refresh() {
    MemoryStats s{};

#ifdef _WIN32
    MEMORYSTATUSEX mem{};
    mem.dwLength = sizeof(mem);
    if (GlobalMemoryStatusEx(&mem)) {
        s.totalPhys   = mem.ullTotalPhys;
        s.availPhys   = mem.ullAvailPhys;
        s.usedPhys    = mem.ullTotalPhys - mem.ullAvailPhys;
        s.usedPercent = (s.totalPhys > 0)
                        ? 100.0 * (double)s.usedPhys / (double)s.totalPhys
                        : 0.0;

        // Page file (swap equivalent on Windows)
        s.totalSwap  = mem.ullTotalPageFile;
        s.availSwap  = mem.ullAvailPageFile;
        s.usedSwap   = mem.ullTotalPageFile - mem.ullAvailPageFile;
        s.swapPercent = (s.totalSwap > 0)
                        ? 100.0 * (double)s.usedSwap / (double)s.totalSwap
                        : 0.0;
    }
#else
    // Parse /proc/meminfo
    std::ifstream ifs("/proc/meminfo");
    std::unordered_map<std::string, unsigned long long> vals;
    std::string key;
    unsigned long long val;
    std::string unit;
    while (ifs >> key >> val) {
        if (!key.empty() && key.back() == ':') key.pop_back();
        vals[key] = val * 1024ULL; // kB -> bytes
        std::getline(ifs, unit);   // consume rest of line
    }
    s.totalPhys   = vals["MemTotal"];
    s.availPhys   = vals["MemAvailable"];
    s.usedPhys    = s.totalPhys - s.availPhys;
    s.usedPercent = (s.totalPhys > 0)
                    ? 100.0 * (double)s.usedPhys / (double)s.totalPhys
                    : 0.0;
    s.totalSwap   = vals["SwapTotal"];
    s.availSwap   = vals["SwapFree"];
    s.usedSwap    = s.totalSwap - s.availSwap;
    s.swapPercent = (s.totalSwap > 0)
                    ? 100.0 * (double)s.usedSwap / (double)s.totalSwap
                    : 0.0;
#endif

    last_ = s;
    return s;
}

MemoryStats MemoryMonitor::getLastStats() const { return last_; }

// ──────────────────────────────────────────────────────────────────────────────
// Format helper
// ──────────────────────────────────────────────────────────────────────────────
std::string MemoryMonitor::formatBytes(unsigned long long bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int idx = 0;
    double val = (double)bytes;
    while (val >= 1024.0 && idx < 4) { val /= 1024.0; ++idx; }
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << val << " " << units[idx];
    return ss.str();
}

} // namespace sysmon
