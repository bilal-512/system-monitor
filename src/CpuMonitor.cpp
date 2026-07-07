#include "CpuMonitor.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>

namespace sysmon {

static bool readProcStat(unsigned long long &idle, unsigned long long &total) {
	std::ifstream ifs("/proc/stat");
	if (!ifs.is_open()) return false;
	std::string line;
	std::getline(ifs, line);
	std::istringstream ss(line);
	std::string cpu;
	ss >> cpu;
	unsigned long long user, nice, system, idle_v, iowait, irq, softirq, steal;
	user = nice = system = idle_v = iowait = irq = softirq = steal = 0ULL;
	ss >> user >> nice >> system >> idle_v >> iowait >> irq >> softirq >> steal;
	idle = idle_v + iowait;
	total = user + nice + system + idle_v + iowait + irq + softirq + steal;
	return true;
}

CpuMonitor::CpuMonitor(int intervalMs) : intervalMs_(intervalMs), running_(false) {}

CpuMonitor::~CpuMonitor() { stop(); }

void CpuMonitor::start(Callback cb) {
	if (running_) return;
	cb_ = cb;
	running_ = true;
	worker_ = std::thread(&CpuMonitor::runLoop, this);
}

void CpuMonitor::stop() {
	if (!running_) return;
	running_ = false;
	if (worker_.joinable()) worker_.join();
}

void CpuMonitor::runLoop() {
	using namespace std::chrono_literals;
	// initialize previous counters
	readProcStat(prevIdle_, prevTotal_);
	while (running_) {
		std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs_));
		unsigned long long idle = 0, total = 0;
		if (!readProcStat(idle, total)) continue;
		unsigned long long idleDelta = idle - prevIdle_;
		unsigned long long totalDelta = total - prevTotal_;
		double usage = 0.0;
		if (totalDelta != 0) usage = (1.0 - (double)idleDelta / (double)totalDelta) * 100.0;
		prevIdle_ = idle;
		prevTotal_ = total;
		if (cb_) cb_(usage);
	}
}

} // namespace sysmon
