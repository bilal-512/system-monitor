#pragma once

#include "PerformanceStats.h"
#include <string>
#include <vector>

namespace sysmon {

struct SystemSnapshot {
    std::string osName;
    std::string hostName;
    std::string cpuModel;
    int         coreCount   = 0;
    unsigned long long totalRamBytes = 0;
    std::string timestamp;
    double      cpuPct      = 0.0;
    double      memPct      = 0.0;
    double      diskPct     = 0.0;
    int         procCount   = 0;
};

/**
 * @brief Report generator – produces CSV and TXT performance reports.
 */
class ReportGenerator {
public:
    explicit ReportGenerator(const std::string& reportDir = "reports");

    // Generate a daily performance report (CSV + TXT)
    bool generateDailyReport(const PerformanceStats& stats,
                             const std::string& date = "");

    // Generate a comprehensive system health report (TXT)
    bool generateHealthReport(const SystemSnapshot& snap,
                              const PerformanceStats& stats);

    // Generate CPU usage summary (CSV + TXT)
    bool generateCpuSummary(const PerformanceStats& stats,
                            const std::string& date = "");

    // Generate memory usage summary (CSV + TXT)
    bool generateMemorySummary(const PerformanceStats& stats,
                               const std::string& date = "");

    // Generate resource consumption report (TXT)
    bool generateResourceReport(const SystemSnapshot& snap,
                                const PerformanceStats& stats);

    const std::string& getReportDir() const { return reportDir_; }
    void setReportDir(const std::string& dir);

private:
    bool ensureDir(const std::string& dir);
    std::string buildPath(const std::string& filename);
    static std::string today();
    static std::string nowTimestamp();

    std::string reportDir_;
};

} // namespace sysmon
