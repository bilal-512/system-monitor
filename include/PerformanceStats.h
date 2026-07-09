#pragma once

#include <string>
#include <vector>
#include <deque>
#include <chrono>

namespace sysmon {

struct DataPoint {
    std::string timestamp;  // "YYYY-MM-DD HH:MM:SS"
    std::string date;       // "YYYY-MM-DD"
    double      cpuPct     = 0.0;
    double      memPct     = 0.0;
    double      diskPct    = 0.0;
    int         procCount  = 0;
};

struct StatSummary {
    double minVal = 0.0;
    double maxVal = 0.0;
    double avgVal = 0.0;
    double stdDev = 0.0;
    // Trend: +1 = increasing, -1 = decreasing, 0 = stable
    int    trend  = 0;
};

/**
 * @brief Performance history store with statistics and search.
 */
class PerformanceStats {
public:
    explicit PerformanceStats(size_t maxPoints = 86400); // 1 day at 1s interval

    void addPoint(const DataPoint& pt);

    // All stored points
    const std::deque<DataPoint>& getAll() const;

    // Filter by date string "YYYY-MM-DD"
    std::vector<DataPoint> filterByDate(const std::string& date) const;

    // Filter by date range
    std::vector<DataPoint> filterByRange(const std::string& from, const std::string& to) const;

    // Search by process count range
    std::vector<DataPoint> filterByProcCount(int minProcs, int maxProcs) const;

    // Compute statistics for a metric over points
    StatSummary computeCpuStats()  const;
    StatSummary computeMemStats()  const;
    StatSummary computeDiskStats() const;

    // Summary of distinct dates in history
    std::vector<std::string> getAvailableDates() const;

    void clear();

private:
    static StatSummary compute(const std::vector<double>& vals);
    static int         detectTrend(const std::vector<double>& vals);

    std::deque<DataPoint> points_;
    size_t                maxPoints_;
};

} // namespace sysmon
