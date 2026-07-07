#include "Logger.h"
#include <chrono>
#include <ctime>
#include <iomanip>

namespace sysmon {

static std::string timestamp() {
	using namespace std::chrono;
	auto now = system_clock::now();
	std::time_t t = system_clock::to_time_t(now);
	std::tm tm{};
	localtime_r(&t, &tm);
	std::ostringstream ss;
	ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
	return ss.str();
}

Logger::Logger(const std::string &path) {
	ofs_.open(path, std::ios::app);
}

Logger::~Logger() {
	if (ofs_.is_open()) ofs_.close();
}

void Logger::info(const std::string &msg) {
	std::lock_guard<std::mutex> lk(mtx_);
	if (ofs_.is_open()) ofs_ << timestamp() << " [INFO] " << msg << std::endl;
}

void Logger::warn(const std::string &msg) {
	std::lock_guard<std::mutex> lk(mtx_);
	if (ofs_.is_open()) ofs_ << timestamp() << " [WARN] " << msg << std::endl;
}

void Logger::error(const std::string &msg) {
	std::lock_guard<std::mutex> lk(mtx_);
	if (ofs_.is_open()) ofs_ << timestamp() << " [ERROR] " << msg << std::endl;
}

} // namespace sysmon
