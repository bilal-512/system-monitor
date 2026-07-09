#include "Config.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdexcept>

namespace sysmon {

// ──────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ──────────────────────────────────────────────────────────────────────────────
static std::string trim(const std::string& s) {
    const char* ws = " \t\r\n";
    size_t a = s.find_first_not_of(ws);
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(ws);
    return s.substr(a, b - a + 1);
}

// ──────────────────────────────────────────────────────────────────────────────
// Load / Save
// ──────────────────────────────────────────────────────────────────────────────
bool Config::loadFromFile(const std::string& path) {
    filePath_ = path;
    std::ifstream ifs(path);
    if (!ifs.is_open()) return false;
    std::string line;
    while (std::getline(ifs, line)) {
        // Skip comments and blank lines
        std::string t = trim(line);
        if (t.empty() || t[0] == '#' || t[0] == ';') continue;
        auto pos = t.find('=');
        if (pos == std::string::npos) continue;
        std::string k = trim(t.substr(0, pos));
        std::string v = trim(t.substr(pos + 1));
        // Strip inline comments
        auto cpos = v.find('#');
        if (cpos != std::string::npos) v = trim(v.substr(0, cpos));
        if (k.empty()) continue;
        data_[k] = v;
    }
    return true;
}

bool Config::saveToFile(const std::string& path) const {
    const std::string& target = path.empty() ? filePath_ : path;
    if (target.empty()) return false;
    std::ofstream ofs(target);
    if (!ofs.is_open()) return false;
    ofs << "# System Resource Monitor Configuration\n";
    for (auto& [k, v] : data_) {
        ofs << k << "=" << v << "\n";
    }
    return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// Getters
// ──────────────────────────────────────────────────────────────────────────────
std::string Config::get(const std::string& key, const std::string& def) const {
    auto it = data_.find(key);
    return (it != data_.end()) ? it->second : def;
}

int Config::getInt(const std::string& key, int def) const {
    std::string s = get(key);
    if (s.empty()) return def;
    try { return std::stoi(s); } catch (...) { return def; }
}

double Config::getDouble(const std::string& key, double def) const {
    std::string s = get(key);
    if (s.empty()) return def;
    try { return std::stod(s); } catch (...) { return def; }
}

// ──────────────────────────────────────────────────────────────────────────────
// Setters
// ──────────────────────────────────────────────────────────────────────────────
void Config::set(const std::string& key, const std::string& value) {
    data_[key] = value;
}

void Config::setInt(const std::string& key, int value) {
    data_[key] = std::to_string(value);
}

void Config::setDouble(const std::string& key, double value) {
    std::ostringstream ss;
    ss << value;
    data_[key] = ss.str();
}

} // namespace sysmon
