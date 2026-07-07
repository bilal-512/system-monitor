#include "SystemInfo.h"
#include <sys/utsname.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sstream>

namespace sysmon {

std::string SystemInfo::getOS() {
	struct utsname buf;
	if (uname(&buf) == 0) {
		std::ostringstream ss;
		ss << buf.sysname << " " << buf.release << " " << buf.machine;
		return ss.str();
	}
	return std::string("unknown");
}

std::string SystemInfo::getHostName() {
	char name[256] = {0};
	if (gethostname(name, sizeof(name)) == 0) {
		return std::string(name);
	}
	return std::string("unknown");
}

unsigned long SystemInfo::getUptimeSeconds() {
	struct sysinfo info;
	if (sysinfo(&info) == 0) {
		return static_cast<unsigned long>(info.uptime);
	}
	return 0;
}

unsigned long long SystemInfo::getTotalRAMKB() {
	struct sysinfo info;
	if (sysinfo(&info) == 0) {
		return static_cast<unsigned long long>(info.totalram) * info.mem_unit / 1024ULL;
	}
	return 0ULL;
}

unsigned long long SystemInfo::getFreeRAMKB() {
	struct sysinfo info;
	if (sysinfo(&info) == 0) {
		return static_cast<unsigned long long>(info.freeram) * info.mem_unit / 1024ULL;
	}
	return 0ULL;
}

} // namespace sysmon
