#pragma once

#include <string>
#include <vector>

namespace sysmon {

// ANSI color escape codes
namespace Color {
    static const char* RESET    = "\033[0m";
    static const char* BOLD     = "\033[1m";
    static const char* DIM      = "\033[2m";
    static const char* RED      = "\033[91m";
    static const char* GREEN    = "\033[92m";
    static const char* YELLOW   = "\033[93m";
    static const char* BLUE     = "\033[94m";
    static const char* MAGENTA  = "\033[95m";
    static const char* CYAN     = "\033[96m";
    static const char* WHITE    = "\033[97m";
    static const char* BG_RED   = "\033[41m";
    static const char* BG_GREEN = "\033[42m";
    static const char* BG_BLUE  = "\033[44m";
}

/**
 * @brief Console UI renderer using ANSI escape codes.
 * Enables VT processing on Windows 10+.
 */
class Display {
public:
    static void init();         // Enable VT processing on Windows
    static void clearScreen();
    static int  getWidth();     // Console width in characters

    // Structural elements
    static void printBanner(const std::string& title);
    static void printSectionHeader(const std::string& title);
    static void printSeparator(int len = 70, char ch = '-');
    static void printFooter(const std::string& hint = "");

    // Metrics & progress
    static void printProgressBar(const std::string& label,
                                  double percent,
                                  const std::string& detail,
                                  int barWidth = 40);
    static void printKeyValue(const std::string& key,
                               const std::string& value,
                               const char* valueColor = nullptr);
    static void printAlert(const std::string& msg, bool critical = false);

    // Table
    static void printTable(const std::vector<std::string>& headers,
                           const std::vector<std::vector<std::string>>& rows,
                           const std::vector<int>& colWidths);

    // Menu
    static void printMenu(const std::string& title,
                          const std::vector<std::string>& options);

    // Helpers
    static const char* percentColor(double pct);
    static std::string colorStr(const std::string& text, const char* color);
    static std::string centerStr(const std::string& text, int width);
    static std::string formatBytes(unsigned long long bytes);
    static std::string formatUptime(unsigned long seconds);
    static std::string padRight(const std::string& s, int width);
    static std::string padLeft(const std::string& s, int width);
};

} // namespace sysmon
