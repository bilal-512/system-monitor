#include "ReportGenerator.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <iostream>
#include <algorithm>

#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#   include <direct.h>
#   define MKDIR(p) _mkdir(p)
#else
#   include <sys/stat.h>
#   define MKDIR(p) mkdir((p), 0755)
#endif

namespace sysmon {

// ──────────────────────────────────────────────────────────────────────────────
// Helpers
// ──────────────────────────────────────────────────────────────────────────────
ReportGenerator::ReportGenerator(const std::string& reportDir)
    : reportDir_(reportDir) { ensureDir(reportDir_); }

void ReportGenerator::setReportDir(const std::string& dir) {
    reportDir_ = dir;
    ensureDir(dir);
}

bool ReportGenerator::ensureDir(const std::string& dir) {
    MKDIR(dir.c_str());
    return true;
}

std::string ReportGenerator::buildPath(const std::string& filename) {
    return reportDir_ + "/" + filename;
}

std::string ReportGenerator::today() {
    auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d");
    return ss.str();
}

std::string ReportGenerator::nowTimestamp() {
    auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
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

static std::string formatPct(double v) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << v << "%";
    return ss.str();
}

static std::string trendStr(int t) {
    if (t > 0) return "Increasing ↑";
    if (t < 0) return "Decreasing ↓";
    return "Stable →";
}

// ──────────────────────────────────────────────────────────────────────────────
// Daily Performance Report (CSV + TXT)
// ──────────────────────────────────────────────────────────────────────────────
bool ReportGenerator::generateDailyReport(const PerformanceStats& stats,
                                           const std::string& date) {
    std::string d = date.empty() ? today() : date;
    auto points   = stats.filterByDate(d);

    // ── CSV ──────────────────────────────────────────────────────────────────
    {
        std::string path = buildPath("daily_" + d + ".csv");
        std::ofstream f(path);
        if (!f.is_open()) return false;
        f << "Timestamp,CPU%,Memory%,Disk%,Processes\n";
        for (auto& p : points) {
            f << p.timestamp << ","
              << std::fixed << std::setprecision(2)
              << p.cpuPct  << ","
              << p.memPct  << ","
              << p.diskPct << ","
              << p.procCount << "\n";
        }
        std::cout << "[Report] Daily CSV saved: " << path << "\n";
    }

    // ── TXT ──────────────────────────────────────────────────────────────────
    {
        auto cpuStat  = stats.computeCpuStats();
        auto memStat  = stats.computeMemStats();
        auto diskStat = stats.computeDiskStats();

        std::string path = buildPath("daily_" + d + ".txt");
        std::ofstream f(path);
        if (!f.is_open()) return false;

        f << "======================================================\n"
          << "       DAILY PERFORMANCE REPORT — " << d << "\n"
          << "       Generated: " << nowTimestamp() << "\n"
          << "======================================================\n\n"
          << "Total data points : " << points.size() << "\n\n"
          << "CPU Usage\n"
          << "  Min     : " << formatPct(cpuStat.minVal)  << "\n"
          << "  Max     : " << formatPct(cpuStat.maxVal)  << "\n"
          << "  Average : " << formatPct(cpuStat.avgVal)  << "\n"
          << "  Std Dev : " << formatPct(cpuStat.stdDev)  << "\n"
          << "  Trend   : " << trendStr(cpuStat.trend)    << "\n\n"
          << "Memory Usage\n"
          << "  Min     : " << formatPct(memStat.minVal)  << "\n"
          << "  Max     : " << formatPct(memStat.maxVal)  << "\n"
          << "  Average : " << formatPct(memStat.avgVal)  << "\n"
          << "  Std Dev : " << formatPct(memStat.stdDev)  << "\n"
          << "  Trend   : " << trendStr(memStat.trend)    << "\n\n"
          << "Disk Usage\n"
          << "  Min     : " << formatPct(diskStat.minVal) << "\n"
          << "  Max     : " << formatPct(diskStat.maxVal) << "\n"
          << "  Average : " << formatPct(diskStat.avgVal) << "\n"
          << "  Std Dev : " << formatPct(diskStat.stdDev) << "\n"
          << "  Trend   : " << trendStr(diskStat.trend)   << "\n\n"
          << "======================================================\n";
        std::cout << "[Report] Daily TXT saved: " << path << "\n";
    }
    return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// System Health Report (TXT)
// ──────────────────────────────────────────────────────────────────────────────
bool ReportGenerator::generateHealthReport(const SystemSnapshot& snap,
                                            const PerformanceStats& stats)
{
    std::string path = buildPath("health_" + today() + ".txt");
    std::ofstream f(path);
    if (!f.is_open()) return false;

    auto cpuStat  = stats.computeCpuStats();
    auto memStat  = stats.computeMemStats();
    auto diskStat = stats.computeDiskStats();

    // Overall health score (simple heuristic)
    double score = 100.0;
    if (cpuStat.avgVal  > 80) score -= 20;
    else if (cpuStat.avgVal > 60) score -= 10;
    if (memStat.avgVal  > 80) score -= 20;
    else if (memStat.avgVal > 60) score -= 10;
    if (diskStat.avgVal > 80) score -= 20;
    else if (diskStat.avgVal > 60) score -= 10;
    if (cpuStat.trend  > 0) score -= 5;
    if (memStat.trend  > 0) score -= 5;
    if (diskStat.trend > 0) score -= 5;
    if (score < 0) score = 0;

    std::string health = (score >= 80) ? "EXCELLENT" :
                         (score >= 60) ? "GOOD"      :
                         (score >= 40) ? "FAIR"      : "POOR";

    f << "======================================================\n"
      << "           SYSTEM HEALTH REPORT\n"
      << "       Generated: " << nowTimestamp() << "\n"
      << "======================================================\n\n"
      << "System Information\n"
      << "  Host      : " << snap.hostName  << "\n"
      << "  OS        : " << snap.osName    << "\n"
      << "  CPU       : " << snap.cpuModel  << "\n"
      << "  Cores     : " << snap.coreCount << "\n"
      << "  Total RAM : " << (snap.totalRamBytes / (1024*1024)) << " MB\n\n"
      << "Current Metrics (at report time)\n"
      << "  CPU Usage  : " << formatPct(snap.cpuPct)  << "\n"
      << "  Memory     : " << formatPct(snap.memPct)  << "\n"
      << "  Disk       : " << formatPct(snap.diskPct) << "\n"
      << "  Processes  : " << snap.procCount           << "\n\n"
      << "Historical Analysis\n"
      << "  CPU  Avg/Peak : " << formatPct(cpuStat.avgVal)
                              << " / " << formatPct(cpuStat.maxVal)
                              << "  [" << trendStr(cpuStat.trend) << "]\n"
      << "  Mem  Avg/Peak : " << formatPct(memStat.avgVal)
                              << " / " << formatPct(memStat.maxVal)
                              << "  [" << trendStr(memStat.trend) << "]\n"
      << "  Disk Avg/Peak : " << formatPct(diskStat.avgVal)
                              << " / " << formatPct(diskStat.maxVal)
                              << "  [" << trendStr(diskStat.trend) << "]\n\n"
      << "Overall Health Score : " << std::fixed << std::setprecision(0)
                                   << score << "/100 — " << health << "\n\n"
      << "======================================================\n";

    std::cout << "[Report] Health report saved: " << path << "\n";
    return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// CPU Summary (CSV + TXT)
// ──────────────────────────────────────────────────────────────────────────────
bool ReportGenerator::generateCpuSummary(const PerformanceStats& stats,
                                          const std::string& date)
{
    std::string d      = date.empty() ? today() : date;
    auto points        = stats.filterByDate(d);
    auto s             = stats.computeCpuStats();
    std::string csvPath = buildPath("cpu_summary_" + d + ".csv");
    std::string txtPath = buildPath("cpu_summary_" + d + ".txt");

    {
        std::ofstream f(csvPath);
        if (!f.is_open()) return false;
        f << "Timestamp,CPU%\n";
        for (auto& p : points)
            f << p.timestamp << "," << std::fixed << std::setprecision(2) << p.cpuPct << "\n";
    }
    {
        std::ofstream f(txtPath);
        if (!f.is_open()) return false;
        f << "=== CPU USAGE SUMMARY — " << d << " ===\n"
          << "Samples : " << points.size() << "\n"
          << "Min     : " << formatPct(s.minVal) << "\n"
          << "Max     : " << formatPct(s.maxVal) << "\n"
          << "Average : " << formatPct(s.avgVal) << "\n"
          << "Std Dev : " << formatPct(s.stdDev) << "\n"
          << "Trend   : " << trendStr(s.trend)   << "\n";
    }
    std::cout << "[Report] CPU summary saved: " << txtPath << "\n";
    return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// Memory Summary (CSV + TXT)
// ──────────────────────────────────────────────────────────────────────────────
bool ReportGenerator::generateMemorySummary(const PerformanceStats& stats,
                                             const std::string& date)
{
    std::string d      = date.empty() ? today() : date;
    auto points        = stats.filterByDate(d);
    auto s             = stats.computeMemStats();
    std::string csvPath = buildPath("mem_summary_" + d + ".csv");
    std::string txtPath = buildPath("mem_summary_" + d + ".txt");

    {
        std::ofstream f(csvPath);
        if (!f.is_open()) return false;
        f << "Timestamp,Memory%\n";
        for (auto& p : points)
            f << p.timestamp << "," << std::fixed << std::setprecision(2) << p.memPct << "\n";
    }
    {
        std::ofstream f(txtPath);
        if (!f.is_open()) return false;
        f << "=== MEMORY USAGE SUMMARY — " << d << " ===\n"
          << "Samples : " << points.size() << "\n"
          << "Min     : " << formatPct(s.minVal) << "\n"
          << "Max     : " << formatPct(s.maxVal) << "\n"
          << "Average : " << formatPct(s.avgVal) << "\n"
          << "Std Dev : " << formatPct(s.stdDev) << "\n"
          << "Trend   : " << trendStr(s.trend)   << "\n";
    }
    std::cout << "[Report] Memory summary saved: " << txtPath << "\n";
    return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// Resource Consumption Report (TXT)
// ──────────────────────────────────────────────────────────────────────────────
bool ReportGenerator::generateResourceReport(const SystemSnapshot& snap,
                                              const PerformanceStats& stats)
{
    std::string path = buildPath("resource_consumption_" + today() + ".txt");
    std::ofstream f(path);
    if (!f.is_open()) return false;

    auto cpuStat  = stats.computeCpuStats();
    auto memStat  = stats.computeMemStats();
    auto diskStat = stats.computeDiskStats();

    f << "======================================================\n"
      << "        RESOURCE CONSUMPTION REPORT\n"
      << "       Generated: " << nowTimestamp() << "\n"
      << "======================================================\n\n"
      << "Host    : " << snap.hostName << "\n"
      << "OS      : " << snap.osName   << "\n"
      << "CPU     : " << snap.cpuModel << " (" << snap.coreCount << " cores)\n"
      << "RAM     : " << (snap.totalRamBytes / 1024 / 1024 / 1024) << " GB total\n\n"
      << "--- Resource Usage Summary ---\n\n"
      << "  CPU Usage\n"
      << "    Current : " << formatPct(snap.cpuPct) << "\n"
      << "    Average : " << formatPct(cpuStat.avgVal) << "\n"
      << "    Peak    : " << formatPct(cpuStat.maxVal) << "\n"
      << "    Trend   : " << trendStr(cpuStat.trend) << "\n\n"
      << "  Memory Usage\n"
      << "    Current : " << formatPct(snap.memPct) << "\n"
      << "    Average : " << formatPct(memStat.avgVal) << "\n"
      << "    Peak    : " << formatPct(memStat.maxVal) << "\n"
      << "    Trend   : " << trendStr(memStat.trend) << "\n\n"
      << "  Disk Usage\n"
      << "    Current : " << formatPct(snap.diskPct) << "\n"
      << "    Average : " << formatPct(diskStat.avgVal) << "\n"
      << "    Peak    : " << formatPct(diskStat.maxVal) << "\n"
      << "    Trend   : " << trendStr(diskStat.trend) << "\n\n"
      << "  Processes: " << snap.procCount << " running\n\n"
      << "======================================================\n";

    std::cout << "[Report] Resource report saved: " << path << "\n";
    return true;
}

} // namespace sysmon
