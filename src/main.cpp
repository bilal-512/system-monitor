#include <iostream>
#include <chrono>
#include <thread>
#include "SystemInfo.h"
#include "Config.h"
#include "Logger.h"
#include "CpuMonitor.h"
#include <sstream>

int main(int argc, char **argv) {
	using namespace sysmon;

	Config cfg;
	std::string cfgPath = "config/config.ini";
	cfg.loadFromFile(cfgPath);

	std::string logPath = cfg.get("log_file", "logs/sample.log");
	Logger logger(logPath);

	logger.info("Starting System Resource Monitor");

	std::cout << "OS: " << SystemInfo::getOS() << std::endl;
	std::cout << "Host: " << SystemInfo::getHostName() << std::endl;
	std::cout << "Uptime (s): " << SystemInfo::getUptimeSeconds() << std::endl;
	std::cout << "Total RAM (KB): " << SystemInfo::getTotalRAMKB() << std::endl;
	std::cout << "Free RAM (KB): " << SystemInfo::getFreeRAMKB() << std::endl;

	int interval = cfg.getInt("monitor_interval_ms", 1000);

	CpuMonitor cpu(interval);
	cpu.start([&](double pct){
		std::ostringstream ss;
		ss << "CPU Usage: " << pct << "%";
		logger.info(ss.str());
		std::cout << ss.str() << std::endl;
	});

	// Run for a short time (demo). Replace with proper signal handling.
	std::this_thread::sleep_for(std::chrono::seconds(5));

	cpu.stop();
	logger.info("Stopping System Resource Monitor");

	return 0;
}
