#include "PerformanceStats.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace sysmon {

// ──────────────────────────────────────────────────────────────────────────────
// Constructor
// ──────────────────────────────────────────────────────────────────────────────
PerformanceStats::PerformanceStats(size_t maxPoints)
    : maxPoints_(maxPoints) {}

// ──────────────────────────────────────────────────────────────────────────────
// Add point
// ──────────────────────────────────────────────────────────────────────────────
void PerformanceStats::addPoint(const DataPoint& pt) {
    points_.push_back(pt);
    while (points_.size() > maxPoints_) points_.pop_front();
}

const std::deque<DataPoint>& PerformanceStats::getAll() const { return points_; }

void PerformanceStats::clear() { points_.clear(); }

// ──────────────────────────────────────────────────────────────────────────────
// Filter methods
// ──────────────────────────────────────────────────────────────────────────────
std::vector<DataPoint> PerformanceStats::filterByDate(const std::string& date) const {
    std::vector<DataPoint> res;
    for (auto& p : points_)
        if (p.date == date) res.push_back(p);
    return res;
}

std::vector<DataPoint> PerformanceStats::filterByRange(
    const std::string& from, const std::string& to) const
{
    std::vector<DataPoint> res;
    for (auto& p : points_)
        if (p.date >= from && p.date <= to) res.push_back(p);
    return res;
}

std::vector<DataPoint> PerformanceStats::filterByProcCount(
    int minProcs, int maxProcs) const
{
    std::vector<DataPoint> res;
    for (auto& p : points_)
        if (p.procCount >= minProcs && p.procCount <= maxProcs) res.push_back(p);
    return res;
}

// ──────────────────────────────────────────────────────────────────────────────
// Statistics computation
// ──────────────────────────────────────────────────────────────────────────────
int PerformanceStats::detectTrend(const std::vector<double>& vals) {
    if (vals.size() < 2) return 0;
    // Simple linear regression slope sign
    size_t n = vals.size();
    double sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
    for (size_t i = 0; i < n; ++i) {
        sumX  += (double)i;
        sumY  += vals[i];
        sumXY += (double)i * vals[i];
        sumX2 += (double)i * (double)i;
    }
    double denom = (double)n * sumX2 - sumX * sumX;
    if (std::abs(denom) < 1e-9) return 0;
    double slope = ((double)n * sumXY - sumX * sumY) / denom;
    if (slope >  0.05) return  1;
    if (slope < -0.05) return -1;
    return 0;
}

StatSummary PerformanceStats::compute(const std::vector<double>& vals) {
    StatSummary s{};
    if (vals.empty()) return s;
    s.minVal = *std::min_element(vals.begin(), vals.end());
    s.maxVal = *std::max_element(vals.begin(), vals.end());
    s.avgVal = std::accumulate(vals.begin(), vals.end(), 0.0) / (double)vals.size();
    double var = 0.0;
    for (double v : vals) var += (v - s.avgVal) * (v - s.avgVal);
    s.stdDev = std::sqrt(var / (double)vals.size());
    s.trend  = detectTrend(vals);
    return s;
}

StatSummary PerformanceStats::computeCpuStats() const {
    std::vector<double> v;
    for (auto& p : points_) v.push_back(p.cpuPct);
    return compute(v);
}

StatSummary PerformanceStats::computeMemStats() const {
    std::vector<double> v;
    for (auto& p : points_) v.push_back(p.memPct);
    return compute(v);
}

StatSummary PerformanceStats::computeDiskStats() const {
    std::vector<double> v;
    for (auto& p : points_) v.push_back(p.diskPct);
    return compute(v);
}

std::vector<std::string> PerformanceStats::getAvailableDates() const {
    std::vector<std::string> dates;
    for (auto& p : points_) {
        if (dates.empty() || dates.back() != p.date)
            dates.push_back(p.date);
    }
    // Unique
    std::sort(dates.begin(), dates.end());
    dates.erase(std::unique(dates.begin(), dates.end()), dates.end());
    return dates;
}

} // namespace sysmon
