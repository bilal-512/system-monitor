#include "CpuMonitor.h"
#include "SystemInfo.h"
#include <numeric>
#include <cmath>

#ifdef _WIN32
#   define NOMINMAX
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#else
#   include <fstream>
#   include <sstream>
#endif

namespace sysmon {

// ──────────────────────────────────────────────────────────────────────────────
// Platform helpers
// ──────────────────────────────────────────────────────────────────────────────
#ifdef _WIN32
static uint64_t filetimeToU64(const FILETIME& ft) {
    ULARGE_INTEGER ui;
    ui.LowPart  = ft.dwLowDateTime;
    ui.HighPart = ft.dwHighDateTime;
    return ui.QuadPart;
}

CpuMonitor::WinCpuTimes CpuMonitor::getWinTimes() {
    FILETIME idle, kernel, user;
    WinCpuTimes t{};
    if (GetSystemTimes(&idle, &kernel, &user)) {
        t.idle   = filetimeToU64(idle);
        t.kernel = filetimeToU64(kernel); // includes idle
        t.user   = filetimeToU64(user);
    }
    return t;
}
#else
bool CpuMonitor::readProcStat(unsigned long long& idle, unsigned long long& total) {
    std::ifstream ifs("/proc/stat");
    if (!ifs.is_open()) return false;
    std::string line;
    std::getline(ifs, line);
    std::istringstream ss(line);
    std::string cpu;
    ss >> cpu;
    unsigned long long user, nice, sys, idle_v, iowait, irq, softirq, steal;
    user = nice = sys = idle_v = iowait = irq = softirq = steal = 0ULL;
    ss >> user >> nice >> sys >> idle_v >> iowait >> irq >> softirq >> steal;
    idle  = idle_v + iowait;
    total = user + nice + sys + idle_v + iowait + irq + softirq + steal;
    return true;
}
#endif

// ──────────────────────────────────────────────────────────────────────────────
// Constructor / Destructor
// ──────────────────────────────────────────────────────────────────────────────
CpuMonitor::CpuMonitor(int intervalMs, size_t historySize)
    : intervalMs_(intervalMs), historySize_(historySize), running_(false)
{
    coreCount_ = SystemInfo::getCoreCount();
}

CpuMonitor::~CpuMonitor() { stop(); }

// ──────────────────────────────────────────────────────────────────────────────
// Start / Stop
// ──────────────────────────────────────────────────────────────────────────────
void CpuMonitor::start(Callback cb) {
    if (running_) return;
    cb_      = cb;
    running_ = true;
    worker_  = std::thread(&CpuMonitor::runLoop, this);
}

void CpuMonitor::stop() {
    if (!running_) return;
    running_ = false;
    if (worker_.joinable()) worker_.join();
}

// ──────────────────────────────────────────────────────────────────────────────
// Background monitoring loop
// ──────────────────────────────────────────────────────────────────────────────
void CpuMonitor::runLoop() {
    // Prime the previous sample
#ifdef _WIN32
    prevTimes_ = getWinTimes();
#else
    readProcStat(prevIdle_, prevTotal_);
#endif

    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs_));

        double usage = 0.0;

#ifdef _WIN32
        WinCpuTimes cur = getWinTimes();
        uint64_t idleDelta   = cur.idle   - prevTimes_.idle;
        uint64_t kernelDelta = cur.kernel - prevTimes_.kernel;
        uint64_t userDelta   = cur.user   - prevTimes_.user;
        // kernel includes idle, so total active = (kernel - idle) + user
        uint64_t totalDelta  = kernelDelta + userDelta;
        if (totalDelta > 0)
            usage = 100.0 * (1.0 - (double)idleDelta / (double)totalDelta);
        prevTimes_ = cur;
#else
        unsigned long long idle = 0, total = 0;
        if (readProcStat(idle, total)) {
            uint64_t idleDelta  = idle  - prevIdle_;
            uint64_t totalDelta = total - prevTotal_;
            if (totalDelta > 0)
                usage = 100.0 * (1.0 - (double)idleDelta / (double)totalDelta);
            prevIdle_  = idle;
            prevTotal_ = total;
        }
#endif

        // Clamp to [0, 100]
        if (usage < 0.0)   usage = 0.0;
        if (usage > 100.0) usage = 100.0;

        {
            std::lock_guard<std::mutex> lk(dataMtx_);
            currentUsage_ = usage;
            history_.push_back(usage);
            if (history_.size() > historySize_) history_.pop_front();
        }

        if (cb_) cb_(usage);
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Public accessors
// ──────────────────────────────────────────────────────────────────────────────
double CpuMonitor::getCurrentUsage() const {
    std::lock_guard<std::mutex> lk(dataMtx_);
    return currentUsage_;
}

std::vector<double> CpuMonitor::getHistory() const {
    std::lock_guard<std::mutex> lk(dataMtx_);
    return {history_.begin(), history_.end()};
}

double CpuMonitor::getAverageUsage() const {
    std::lock_guard<std::mutex> lk(dataMtx_);
    if (history_.empty()) return 0.0;
    double sum = 0.0;
    for (double v : history_) sum += v;
    return sum / (double)history_.size();
}

double CpuMonitor::getPeakUsage() const {
    std::lock_guard<std::mutex> lk(dataMtx_);
    if (history_.empty()) return 0.0;
    return *std::max_element(history_.begin(), history_.end());
}

int CpuMonitor::getCoreCount() const { return coreCount_; }

} // namespace sysmon
