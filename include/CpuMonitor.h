#pragma once

#include <atomic>
#include <thread>
#include <functional>
#include <string>

namespace sysmon {

class CpuMonitor {
public:
	using Callback = std::function<void(double)>; // percent

	explicit CpuMonitor(int intervalMs = 1000);
	~CpuMonitor();

	void start(Callback cb);
	void stop();

private:
	void runLoop();

	int intervalMs_;
	std::atomic<bool> running_;
	std::thread worker_;
	Callback cb_;
	// previous counters
	unsigned long long prevIdle_ = 0, prevTotal_ = 0;
};

} // namespace sysmon
