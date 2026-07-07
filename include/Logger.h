#pragma once

#include <string>
#include <fstream>
#include <mutex>

namespace sysmon {

class Logger {
public:
	explicit Logger(const std::string &path);
	~Logger();

	void info(const std::string &msg);
	void warn(const std::string &msg);
	void error(const std::string &msg);

private:
	std::ofstream ofs_;
	std::mutex mtx_;
};

} // namespace sysmon
