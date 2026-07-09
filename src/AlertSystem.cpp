#include "AlertSystem.h"
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace sysmon {

// ──────────────────────────────────────────────────────────────────────────────
// Constructor
// ──────────────────────────────────────────────────────────────────────────────
AlertSystem::AlertSystem()
    : cpuThreshold_(85.0), memThreshold_(85.0), diskThreshold_(90.0),
      cooldownSecs_(60)
{
    // Initialise last-fire times to epoch (so first check can fire immediately)
    auto epoch = Clock::time_point{};
    lastCpuFire_  = epoch;
    lastMemFire_  = epoch;
    lastDiskFire_ = epoch;
}

// ──────────────────────────────────────────────────────────────────────────────
// Configuration
// ──────────────────────────────────────────────────────────────────────────────
void AlertSystem::setCpuThreshold(double pct)    { cpuThreshold_  = pct; }
void AlertSystem::setMemoryThreshold(double pct) { memThreshold_  = pct; }
void AlertSystem::setDiskThreshold(double pct)   { diskThreshold_ = pct; }
void AlertSystem::setCooldownSeconds(int secs)   { cooldownSecs_  = secs; }
void AlertSystem::setCallback(AlertCallback cb)  { callback_      = cb;  }

// ──────────────────────────────────────────────────────────────────────────────
// Timestamp helper
// ──────────────────────────────────────────────────────────────────────────────
std::string AlertSystem::now() {
    using namespace std::chrono;
    auto t = system_clock::to_time_t(system_clock::now());
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

// ──────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ──────────────────────────────────────────────────────────────────────────────
bool AlertSystem::shouldFire(AlertType type) const {
    auto& last = (type == AlertType::CPU)    ? lastCpuFire_
               : (type == AlertType::MEMORY) ? lastMemFire_
                                             : lastDiskFire_;
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                       Clock::now() - last).count();
    return elapsed >= cooldownSecs_;
}

void AlertSystem::fire(AlertType type, double value, double threshold,
                        const std::string& msg) {
    Alert a;
    a.type      = type;
    a.value     = value;
    a.threshold = threshold;
    a.message   = msg;
    a.timestamp = now();
    history_.push_back(a);

    // Keep history capped
    if (history_.size() > 500) history_.erase(history_.begin());

    // Update last fire time
    if      (type == AlertType::CPU)    lastCpuFire_  = Clock::now();
    else if (type == AlertType::MEMORY) lastMemFire_  = Clock::now();
    else                                lastDiskFire_ = Clock::now();

    if (callback_) callback_(a);
}

// ──────────────────────────────────────────────────────────────────────────────
// Check metrics
// ──────────────────────────────────────────────────────────────────────────────
void AlertSystem::check(double cpuPct, double memPct, double diskPct) {
    if (cpuPct >= cpuThreshold_ && shouldFire(AlertType::CPU)) {
        std::ostringstream ss;
        ss << "CPU usage " << std::fixed << std::setprecision(1)
           << cpuPct << "% exceeded threshold "
           << cpuThreshold_ << "%";
        fire(AlertType::CPU, cpuPct, cpuThreshold_, ss.str());
    }
    if (memPct >= memThreshold_ && shouldFire(AlertType::MEMORY)) {
        std::ostringstream ss;
        ss << "Memory usage " << std::fixed << std::setprecision(1)
           << memPct << "% exceeded threshold "
           << memThreshold_ << "%";
        fire(AlertType::MEMORY, memPct, memThreshold_, ss.str());
    }
    if (diskPct >= diskThreshold_ && shouldFire(AlertType::DISK)) {
        std::ostringstream ss;
        ss << "Disk usage " << std::fixed << std::setprecision(1)
           << diskPct << "% exceeded threshold "
           << diskThreshold_ << "%";
        fire(AlertType::DISK, diskPct, diskThreshold_, ss.str());
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// History access
// ──────────────────────────────────────────────────────────────────────────────
const std::vector<Alert>& AlertSystem::getAlerts() const { return history_; }

void AlertSystem::acknowledgeAll() {
    for (auto& a : history_) a.acknowledged = true;
}

void AlertSystem::clearHistory() { history_.clear(); }

int AlertSystem::getPendingCount() const {
    int count = 0;
    for (auto& a : history_) if (!a.acknowledged) ++count;
    return count;
}

} // namespace sysmon
