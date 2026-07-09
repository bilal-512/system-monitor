#pragma once

#include <string>
#include <vector>
#include <functional>
#include <chrono>

namespace sysmon {

enum class AlertType { CPU, MEMORY, DISK };

struct Alert {
    AlertType   type;
    std::string message;
    double      value;
    double      threshold;
    std::string timestamp;
    bool        acknowledged = false;
};

/**
 * @brief Alert system with configurable thresholds and cooldown.
 * Fires callbacks when CPU/Memory/Disk exceed configured limits.
 */
class AlertSystem {
public:
    using AlertCallback = std::function<void(const Alert&)>;

    AlertSystem();

    // Configure thresholds (percentage 0-100)
    void setCpuThreshold(double pct);
    void setMemoryThreshold(double pct);
    void setDiskThreshold(double pct);
    void setCooldownSeconds(int secs);

    double getCpuThreshold()    const { return cpuThreshold_; }
    double getMemoryThreshold() const { return memThreshold_; }
    double getDiskThreshold()   const { return diskThreshold_; }

    // Register a callback for when any alert fires
    void setCallback(AlertCallback cb);

    // Check current values and fire alerts if needed
    void check(double cpuPct, double memPct, double diskPct);

    // Alert history
    const std::vector<Alert>& getAlerts() const;
    void acknowledgeAll();
    void clearHistory();

    int getPendingCount() const;

private:
    bool shouldFire(AlertType type) const;
    void fire(AlertType type, double value, double threshold, const std::string& msg);
    static std::string now();

    double cpuThreshold_  = 85.0;
    double memThreshold_  = 85.0;
    double diskThreshold_ = 90.0;
    int    cooldownSecs_  = 60;

    AlertCallback callback_;
    std::vector<Alert> history_;

    // Last fire time per alert type
    using Clock = std::chrono::steady_clock;
    std::chrono::time_point<Clock> lastCpuFire_;
    std::chrono::time_point<Clock> lastMemFire_;
    std::chrono::time_point<Clock> lastDiskFire_;
};

} // namespace sysmon
