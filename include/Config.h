#pragma once

#include <string>
#include <unordered_map>

namespace sysmon {

/**
 * @brief Configuration manager.
 * Loads/saves key=value .ini-style config files.
 */
class Config {
public:
    bool loadFromFile(const std::string& path);
    bool saveToFile(const std::string& path) const;

    std::string get(const std::string& key, const std::string& def = "") const;
    int         getInt(const std::string& key, int def = 0) const;
    double      getDouble(const std::string& key, double def = 0.0) const;

    void set(const std::string& key, const std::string& value);
    void setInt(const std::string& key, int value);
    void setDouble(const std::string& key, double value);

private:
    std::unordered_map<std::string, std::string> data_;
    std::string filePath_;
};

} // namespace sysmon
