#pragma once

#include <string>

namespace sysmon {

class SystemInfo {
public:
	static std::string getOS();
	static std::string getHostName();
	static unsigned long getUptimeSeconds();
	static unsigned long long getTotalRAMKB();
	static unsigned long long getFreeRAMKB();
};

} // namespace sysmon
