#include "Display.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

#ifdef _WIN32
#   define NOMINMAX          // prevent windows.h from defining min/max macros
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#endif

namespace sysmon {

// ──────────────────────────────────────────────────────────────────────────────
// Init – enable ANSI VT processing on Windows 10+
// ──────────────────────────────────────────────────────────────────────────────
void Display::init() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD mode = 0;
        if (GetConsoleMode(hOut, &mode)) {
            mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, mode);
        }
    }
    // Set console title
    SetConsoleTitleA("System Resource Monitor & Performance Analyzer");
    // Set UTF-8 code page
    SetConsoleOutputCP(CP_UTF8);
#endif
}

// ──────────────────────────────────────────────────────────────────────────────
// Console dimensions
// ──────────────────────────────────────────────────────────────────────────────
int Display::getWidth() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleScreenBufferInfo(hOut, &csbi))
        return csbi.srWindow.Right - csbi.srWindow.Left + 1;
#endif
    return 80; // default
}

void Display::clearScreen() {
#ifdef _WIN32
    // Use ANSI escape (works on Win10+)
    std::cout << "\033[2J\033[H";
#else
    std::cout << "\033[2J\033[H";
#endif
    std::cout.flush();
}

// ──────────────────────────────────────────────────────────────────────────────
// Helpers
// ──────────────────────────────────────────────────────────────────────────────
std::string Display::colorStr(const std::string& text, const char* color) {
    if (!color) return text;
    return std::string(color) + text + Color::RESET;
}

std::string Display::centerStr(const std::string& text, int width) {
    int len = (int)text.size();
    if (len >= width) return text;
    int pad = (width - len) / 2;
    return std::string(pad, ' ') + text + std::string(width - len - pad, ' ');
}

std::string Display::padRight(const std::string& s, int width) {
    if ((int)s.size() >= width) return s.substr(0, width);
    return s + std::string(width - (int)s.size(), ' ');
}

std::string Display::padLeft(const std::string& s, int width) {
    if ((int)s.size() >= width) return s.substr(0, width);
    return std::string(width - (int)s.size(), ' ') + s;
}

const char* Display::percentColor(double pct) {
    if (pct >= 80.0) return Color::RED;
    if (pct >= 50.0) return Color::YELLOW;
    return Color::GREEN;
}

std::string Display::formatBytes(unsigned long long bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int idx = 0;
    double val = (double)bytes;
    while (val >= 1024.0 && idx < 4) { val /= 1024.0; ++idx; }
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << val << " " << units[idx];
    return ss.str();
}

std::string Display::formatUptime(unsigned long seconds) {
    unsigned long d = seconds / 86400;
    unsigned long h = (seconds % 86400) / 3600;
    unsigned long m = (seconds % 3600) / 60;
    unsigned long s = seconds % 60;
    std::ostringstream ss;
    if (d > 0) ss << d << "d ";
    ss << h << "h " << m << "m " << s << "s";
    return ss.str();
}

// ──────────────────────────────────────────────────────────────────────────────
// Structural elements
// ──────────────────────────────────────────────────────────────────────────────
void Display::printSeparator(int len, char ch) {
    std::cout << Color::DIM
              << std::string(len, ch)
              << Color::RESET << "\n";
}

void Display::printBanner(const std::string& title) {
    int w = std::max(70, (int)title.size() + 6);
    std::string top    = "+" + std::string(w - 2, '=') + "+";
    std::string middle = "| " + centerStr(title, w - 4) + " |";
    std::string bot    = "+" + std::string(w - 2, '=') + "+";

    std::cout << Color::CYAN << Color::BOLD
              << top    << "\n"
              << middle << "\n"
              << bot    << "\n"
              << Color::RESET;
}

void Display::printSectionHeader(const std::string& title) {
    std::cout << "\n" << Color::CYAN << Color::BOLD
              << "  >> " << title << "\n"
              << Color::RESET;
    printSeparator(70);
}

void Display::printFooter(const std::string& hint) {
    std::cout << "\n";
    printSeparator(70);
    if (!hint.empty())
        std::cout << Color::DIM << "  " << hint << Color::RESET << "\n";
}

// ──────────────────────────────────────────────────────────────────────────────
// Progress bar
// ──────────────────────────────────────────────────────────────────────────────
void Display::printProgressBar(const std::string& label,
                                double percent,
                                const std::string& detail,
                                int barWidth)
{
    // Clamp
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    int filled = (int)std::round(percent / 100.0 * barWidth);
    const char* col = percentColor(percent);

    std::string bar(filled, '#');
    std::string empty(barWidth - filled, '.');

    std::cout << "  " << Color::BOLD << padRight(label, 6) << Color::RESET
              << " [" << col << bar << Color::DIM << empty << Color::RESET << "] "
              << col << Color::BOLD
              << std::fixed << std::setprecision(1) << percent << "%"
              << Color::RESET
              << "  " << Color::DIM << detail << Color::RESET << "\n";
}

// ──────────────────────────────────────────────────────────────────────────────
// Key-value pair
// ──────────────────────────────────────────────────────────────────────────────
void Display::printKeyValue(const std::string& key,
                             const std::string& value,
                             const char* valueColor)
{
    std::cout << "  " << Color::DIM << padRight(key, 22) << Color::RESET
              << ": ";
    if (valueColor) std::cout << valueColor;
    std::cout << value;
    if (valueColor) std::cout << Color::RESET;
    std::cout << "\n";
}

// ──────────────────────────────────────────────────────────────────────────────
// Alert
// ──────────────────────────────────────────────────────────────────────────────
void Display::printAlert(const std::string& msg, bool critical) {
    const char* col = critical ? Color::RED : Color::YELLOW;
    const char* sym = critical ? "[!!]" : "[! ]";
    std::cout << col << Color::BOLD << sym << " ALERT: " << msg << Color::RESET << "\n";
}

// ──────────────────────────────────────────────────────────────────────────────
// Table
// ──────────────────────────────────────────────────────────────────────────────
void Display::printTable(const std::vector<std::string>& headers,
                          const std::vector<std::vector<std::string>>& rows,
                          const std::vector<int>& colWidths)
{
    // Header row
    std::cout << "  " << Color::CYAN << Color::BOLD;
    for (size_t i = 0; i < headers.size() && i < colWidths.size(); ++i) {
        std::cout << padRight(headers[i], colWidths[i]) << " ";
    }
    std::cout << Color::RESET << "\n";

    // Separator
    std::cout << "  " << Color::DIM;
    for (size_t i = 0; i < colWidths.size(); ++i) {
        std::cout << std::string(colWidths[i], '-') << " ";
    }
    std::cout << Color::RESET << "\n";

    // Data rows (alternating dim)
    int rowIdx = 0;
    for (auto& row : rows) {
        std::cout << (rowIdx++ % 2 == 0 ? "" : Color::DIM);
        std::cout << "  ";
        for (size_t i = 0; i < row.size() && i < colWidths.size(); ++i) {
            std::cout << padRight(row[i], colWidths[i]) << " ";
        }
        std::cout << Color::RESET << "\n";
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Menu
// ──────────────────────────────────────────────────────────────────────────────
void Display::printMenu(const std::string& title,
                         const std::vector<std::string>& options)
{
    if (!title.empty()) {
        std::cout << "\n" << Color::CYAN << Color::BOLD
                  << "  " << title << Color::RESET << "\n";
        printSeparator(50, '-');
    }
    for (size_t i = 0; i < options.size(); ++i) {
        std::cout << "  " << Color::YELLOW << Color::BOLD
                  << "[" << i + 1 << "] "
                  << Color::RESET << options[i] << "\n";
    }
    std::cout << "  " << Color::YELLOW << Color::BOLD
              << "[0] " << Color::RESET << "Back / Exit\n";
}

} // namespace sysmon
