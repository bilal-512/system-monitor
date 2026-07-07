#include "Config.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace sysmon {

static inline std::string trim(const std::string &s) {
	size_t a = s.find_first_not_of(" \t\r\n");
	if (a == std::string::npos) return "";
	size_t b = s.find_last_not_of(" \t\r\n");
	return s.substr(a, b - a + 1);
}

bool Config::loadFromFile(const std::string &path) {
	std::ifstream ifs(path);
	if (!ifs.is_open()) return false;
	std::string line;
	while (std::getline(ifs, line)) {
		auto pos = line.find('=');
		if (pos == std::string::npos) continue;
		std::string k = trim(line.substr(0, pos));
		std::string v = trim(line.substr(pos + 1));
		if (k.empty()) continue;
		data_[k] = v;
	}
	return true;
}

std::string Config::get(const std::string &key, const std::string &def) const {
	auto it = data_.find(key);
	if (it == data_.end()) return def;
	return it->second;
}

int Config::getInt(const std::string &key, int def) const {
	auto s = get(key);
	if (s.empty()) return def;
	try {
		return std::stoi(s);
	} catch (...) {
		return def;
	}
}

} // namespace sysmon
