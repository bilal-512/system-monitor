#pragma once

#include <string>
#include <unordered_map>

namespace sysmon {

class Config {
public:
	bool loadFromFile(const std::string &path);
	std::string get(const std::string &key, const std::string &def = "") const;
	int getInt(const std::string &key, int def = 0) const;

private:
	std::unordered_map<std::string, std::string> data_;
};

} // namespace sysmon
