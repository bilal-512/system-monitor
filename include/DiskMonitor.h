#pragma once

#include <string>
#include <vector>

namespace sysmon {

struct DriveInfo {
    std::string        drivePath;   // e.g. "C:\\" or "/mnt/data"
    std::string        driveLabel;  // optional label
    unsigned long long totalBytes   = 0;
    unsigned long long usedBytes    = 0;
    unsigned long long freeBytes    = 0;
    double             usedPercent  = 0.0;
    bool               available    = false;
};

/**
 * @brief Disk monitor that enumerates all mounted drives.
 * Windows: GetLogicalDriveStrings + GetDiskFreeSpaceEx.
 * Linux: /proc/mounts + statvfs.
 */
class DiskMonitor {
public:
    DiskMonitor() = default;

    std::vector<DriveInfo> refresh();
    std::vector<DriveInfo> getLastStats() const;

    // Aggregate totals across all drives
    unsigned long long getTotalBytesAll() const;
    unsigned long long getUsedBytesAll() const;
    unsigned long long getFreeBytesAll() const;
    double             getOverallUsedPercent() const;

    static std::string formatBytes(unsigned long long bytes);

private:
    std::vector<DriveInfo> drives_;
};

} // namespace sysmon
