#include "DiskMonitor.h"
#include <sstream>
#include <iomanip>
#include <numeric>

#ifdef _WIN32
#   ifndef NOMINMAX
#       define NOMINMAX
#   endif
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#   endif
#   include <windows.h>
#else
#   include <sys/statvfs.h>
#   include <fstream>
#   include <sstream>
#   include <set>
#endif

namespace sysmon {

// ──────────────────────────────────────────────────────────────────────────────
// Format helper
// ──────────────────────────────────────────────────────────────────────────────
std::string DiskMonitor::formatBytes(unsigned long long bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int idx = 0;
    double val = (double)bytes;
    while (val >= 1024.0 && idx < 4) { val /= 1024.0; ++idx; }
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << val << " " << units[idx];
    return ss.str();
}

// ──────────────────────────────────────────────────────────────────────────────
// Refresh
// ──────────────────────────────────────────────────────────────────────────────
std::vector<DriveInfo> DiskMonitor::refresh() {
    drives_.clear();

#ifdef _WIN32
    // Get all drive letters
    char buf[512] = {0};
    DWORD len = GetLogicalDriveStringsA((DWORD)sizeof(buf) - 1, buf);
    for (char* p = buf; p < buf + len; ) {
        std::string drive(p);
        p += drive.size() + 1;

        // Skip non-fixed drives (removable, network, CD-ROM)
        UINT type = GetDriveTypeA(drive.c_str());
        if (type != DRIVE_FIXED && type != DRIVE_REMOVABLE) continue;

        ULARGE_INTEGER freeBytesAvail, totalBytes, totalFreeBytes;
        if (!GetDiskFreeSpaceExA(drive.c_str(),
                                 &freeBytesAvail,
                                 &totalBytes,
                                 &totalFreeBytes)) continue;

        // Get volume label
        char label[256] = {0};
        GetVolumeInformationA(drive.c_str(), label, sizeof(label),
                              nullptr, nullptr, nullptr, nullptr, 0);

        DriveInfo di;
        di.drivePath  = drive;
        di.driveLabel = (label[0] != '\0') ? std::string(label) : drive;
        di.totalBytes = totalBytes.QuadPart;
        di.freeBytes  = totalFreeBytes.QuadPart;
        di.usedBytes  = di.totalBytes - di.freeBytes;
        di.usedPercent = (di.totalBytes > 0)
                         ? 100.0 * (double)di.usedBytes / (double)di.totalBytes
                         : 0.0;
        di.available  = true;
        drives_.push_back(di);
    }
#else
    // Parse /proc/mounts and use statvfs
    std::ifstream mf("/proc/mounts");
    std::string line;
    std::set<std::string> seen;
    while (std::getline(mf, line)) {
        std::istringstream ss(line);
        std::string dev, mnt, fs;
        ss >> dev >> mnt >> fs;
        if (fs == "tmpfs" || fs == "devtmpfs" || fs == "sysfs" ||
            fs == "proc"  || fs == "cgroup"   || fs == "cgroup2") continue;
        if (mnt.find("/sys") == 0 || mnt.find("/dev") == 0 ||
            mnt.find("/proc") == 0 || mnt.find("/run") == 0) continue;
        if (seen.count(mnt)) continue;
        seen.insert(mnt);

        struct statvfs st;
        if (statvfs(mnt.c_str(), &st) != 0) continue;

        DriveInfo di;
        di.drivePath  = mnt;
        di.driveLabel = mnt;
        di.totalBytes = (unsigned long long)st.f_blocks * st.f_frsize;
        di.freeBytes  = (unsigned long long)st.f_bavail * st.f_frsize;
        di.usedBytes  = di.totalBytes - di.freeBytes;
        di.usedPercent = (di.totalBytes > 0)
                         ? 100.0 * (double)di.usedBytes / (double)di.totalBytes
                         : 0.0;
        di.available  = true;
        if (di.totalBytes == 0) continue;
        drives_.push_back(di);
    }
#endif

    return drives_;
}

std::vector<DriveInfo> DiskMonitor::getLastStats() const { return drives_; }

// ──────────────────────────────────────────────────────────────────────────────
// Aggregate helpers
// ──────────────────────────────────────────────────────────────────────────────
unsigned long long DiskMonitor::getTotalBytesAll() const {
    unsigned long long t = 0;
    for (auto& d : drives_) t += d.totalBytes;
    return t;
}

unsigned long long DiskMonitor::getUsedBytesAll() const {
    unsigned long long t = 0;
    for (auto& d : drives_) t += d.usedBytes;
    return t;
}

unsigned long long DiskMonitor::getFreeBytesAll() const {
    unsigned long long t = 0;
    for (auto& d : drives_) t += d.freeBytes;
    return t;
}

double DiskMonitor::getOverallUsedPercent() const {
    unsigned long long total = getTotalBytesAll();
    if (total == 0) return 0.0;
    return 100.0 * (double)getUsedBytesAll() / (double)total;
}

} // namespace sysmon
