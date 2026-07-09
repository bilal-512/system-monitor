#pragma once

#include <string>

namespace sysmon {

/**
 * @brief Static system information utilities.
 * Windows-compatible implementation via WinAPI.
 */
class SystemInfo {
public:
    // OS & host
    static std::string getOS();
    static std::string getHostName();

    // Uptime
    static unsigned long getUptimeSeconds();
    static std::string   getUptimeString();

    // CPU
    static std::string getCpuModel();
    static int         getCoreCount();
    static int         getLogicalCoreCount();

    // RAM (bytes)
    static unsigned long long getTotalRAM();
    static unsigned long long getFreeRAM();
    static unsigned long long getUsedRAM();
    static double             getRAMUsagePercent();
    // Legacy KB methods kept for compatibility
    static unsigned long long getTotalRAMKB();
    static unsigned long long getFreeRAMKB();
};

} // namespace sysmon
