/**
 * ============================================================
 *  System Resource Monitor & Performance Analyzer
 *  Main Application Entry Point
 *
 *  Features:
 *   1. System Information Display
 *   2. Real-time CPU Monitoring (multithreaded)
 *   3. Memory Monitoring
 *   4. Disk Monitoring
 *   5. Process Manager (list/search/sort/terminate)
 *   6. Performance Logging (timestamped)
 *   7. Report Generation (CSV + TXT)
 *   8. Alert System (configurable thresholds)
 *   9. Search & Statistics
 *  10. Configuration Management
 * ============================================================
 */

#include <iostream>
#include <sstream>
#include <iomanip>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <limits>
#include <algorithm>

// Project modules
#include "Config.h"
#include "Logger.h"
#include "SystemInfo.h"
#include "CpuMonitor.h"
#include "MemoryMonitor.h"
#include "DiskMonitor.h"
#include "ProcessManager.h"
#include "AlertSystem.h"
#include "ReportGenerator.h"
#include "PerformanceStats.h"
#include "Display.h"

#ifdef _WIN32
#   define NOMINMAX          // prevent windows.h from defining min/max macros
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#   define MKDIR_CMD(p) _mkdir(p)
#   include <direct.h>
#else
#   include <sys/stat.h>
#   define MKDIR_CMD(p) mkdir((p), 0755)
#endif

using namespace sysmon;

// ──────────────────────────────────────────────────────────────────────────────
// Global shared state (thread-safe)
// ──────────────────────────────────────────────────────────────────────────────
struct AppState {
    std::mutex         mtx;
    double             cpuPct    = 0.0;
    double             memPct    = 0.0;
    double             diskPct   = 0.0;
    int                procCount = 0;
    MemoryStats        memStats;
    std::vector<DriveInfo> driveStats;
    std::atomic<bool>  running{true};
};

static AppState g_state;
static PerformanceStats g_perfStats;
static AlertSystem      g_alerts;
static Logger*          g_logger = nullptr;

// ──────────────────────────────────────────────────────────────────────────────
// Utility: timestamp for DataPoint
// ──────────────────────────────────────────────────────────────────────────────
static DataPoint makeDataPoint(double cpu, double mem, double disk, int procs) {
    using namespace std::chrono;
    auto t  = system_clock::to_time_t(system_clock::now());
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    DataPoint pt;
    std::ostringstream tss, dss;
    tss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    dss << std::put_time(&tm, "%Y-%m-%d");
    pt.timestamp = tss.str();
    pt.date      = dss.str();
    pt.cpuPct    = cpu;
    pt.memPct    = mem;
    pt.diskPct   = disk;
    pt.procCount = procs;
    return pt;
}

// ──────────────────────────────────────────────────────────────────────────────
// Input helpers
// ──────────────────────────────────────────────────────────────────────────────
static int readInt(const std::string& prompt, int def = -1) {
    std::cout << Color::YELLOW << "  " << prompt << Color::RESET;
    std::string line;
    std::getline(std::cin, line);
    if (line.empty()) return def;
    try { return std::stoi(line); } catch (...) { return def; }
}

static double readDouble(const std::string& prompt, double def = 0.0) {
    std::cout << Color::YELLOW << "  " << prompt << Color::RESET;
    std::string line;
    std::getline(std::cin, line);
    if (line.empty()) return def;
    try { return std::stod(line); } catch (...) { return def; }
}

static std::string readString(const std::string& prompt) {
    std::cout << Color::YELLOW << "  " << prompt << Color::RESET;
    std::string line;
    std::getline(std::cin, line);
    return line;
}

static void waitEnter() {
    std::cout << Color::DIM << "\n  Press ENTER to continue..." << Color::RESET;
    std::string dummy;
    std::getline(std::cin, dummy);
}

// ──────────────────────────────────────────────────────────────────────────────
// MENU 1 – Real-time Dashboard
// ──────────────────────────────────────────────────────────────────────────────
static void showDashboard(CpuMonitor& cpu, MemoryMonitor& mem, DiskMonitor& disk) {
    std::cout << "\n" << Color::DIM
              << "  Live dashboard — refreshes every 2 seconds. Enter 'q' + ENTER to return.\n"
              << Color::RESET;

#ifdef _WIN32
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    DWORD  prevMode;
    GetConsoleMode(hIn, &prevMode);
    SetConsoleMode(hIn, ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);
#endif

    std::atomic<bool> quit{false};
    std::thread inputThread([&]() {
        std::string s;
        std::getline(std::cin, s);
        if (s == "q" || s == "Q") quit = true;
    });

    while (!quit) {
        // Refresh data
        auto ms   = mem.refresh();
        auto ds   = disk.refresh();
        double dp = disk.getOverallUsedPercent();
        double cp = cpu.getCurrentUsage();
        double mp = ms.usedPercent;

        // Update global state
        {
            std::lock_guard<std::mutex> lk(g_state.mtx);
            g_state.cpuPct    = cp;
            g_state.memPct    = mp;
            g_state.diskPct   = dp;
            g_state.memStats  = ms;
            g_state.driveStats = ds;
        }

        Display::clearScreen();
        Display::printBanner("SYSTEM RESOURCE MONITOR  |  Real-time Dashboard");

        // System info line
        std::cout << "\n  " << Color::DIM
                  << "Host: " << Color::WHITE << SystemInfo::getHostName()
                  << Color::DIM << "   OS: " << Color::WHITE << SystemInfo::getOS()
                  << Color::DIM << "   Uptime: " << Color::WHITE
                  << SystemInfo::getUptimeString()
                  << Color::RESET << "\n\n";

        Display::printSectionHeader("Resource Overview");
        Display::printProgressBar("CPU",  cp, std::to_string(cpu.getCoreCount()) + " cores  |  "
                                            + "avg: " + [&](){
                                                std::ostringstream s;
                                                s << std::fixed << std::setprecision(1)
                                                  << cpu.getAverageUsage() << "%";
                                                return s.str(); }());
        Display::printProgressBar("MEM",  mp,
            Display::formatBytes(ms.usedPhys) + " / " + Display::formatBytes(ms.totalPhys));
        Display::printProgressBar("DISK", dp,
            Display::formatBytes(disk.getUsedBytesAll()) + " / " +
            Display::formatBytes(disk.getTotalBytesAll()));

        // Per-drive breakdown
        if (ds.size() > 1) {
            std::cout << "\n";
            for (auto& d : ds) {
                std::string lbl = d.drivePath;
                if (lbl.size() > 4) lbl = lbl.substr(0, 4);
                Display::printProgressBar(lbl, d.usedPercent,
                    Display::formatBytes(d.usedBytes) + " / " +
                    Display::formatBytes(d.totalBytes));
            }
        }

        // CPU history sparkline
        {
            auto hist = cpu.getHistory();
            if (!hist.empty()) {
                std::cout << "\n  " << Color::DIM << "CPU History (last " << hist.size() << " samples):  ";
                for (double v : hist) {
                    const char* c = Display::percentColor(v);
                    // Simple sparkline: use block chars based on value
                    char sym = (v < 25) ? '_' : (v < 50) ? '.' : (v < 75) ? '+' : '#';
                    std::cout << c << sym;
                }
                std::cout << Color::RESET << "\n";
            }
        }

        // Pending alerts
        int pending = g_alerts.getPendingCount();
        if (pending > 0) {
            std::cout << "\n";
            Display::printAlert(std::to_string(pending) + " unacknowledged alert(s) — "
                                "use menu [8] to view", pending > 2);
        }

        Display::printFooter("Type 'q' + ENTER to return to menu");

        // Sleep 2 seconds in 200ms chunks (check quit flag)
        for (int i = 0; i < 10 && !quit; ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    if (inputThread.joinable()) inputThread.join();
}

// ──────────────────────────────────────────────────────────────────────────────
// MENU 2 – CPU Details
// ──────────────────────────────────────────────────────────────────────────────
static void showCpuDetails(CpuMonitor& cpu) {
    Display::clearScreen();
    Display::printBanner("CPU MONITOR — Details");
    Display::printSectionHeader("CPU Information");

    Display::printKeyValue("CPU Model",    SystemInfo::getCpuModel());
    Display::printKeyValue("Physical Cores", std::to_string(SystemInfo::getCoreCount()));
    Display::printKeyValue("Logical Cores",  std::to_string(SystemInfo::getLogicalCoreCount()));

    Display::printSectionHeader("Live Usage");
    double cur = cpu.getCurrentUsage();
    Display::printProgressBar("CPU", cur, [&](){
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(1) << cur << "%";
        return ss.str(); }());

    auto hist = cpu.getHistory();
    Display::printKeyValue("Average (history)", [&](){
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2) << cpu.getAverageUsage() << "%";
        return ss.str(); }());
    Display::printKeyValue("Peak (history)", [&](){
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2) << cpu.getPeakUsage() << "%";
        return ss.str(); }());
    Display::printKeyValue("History samples", std::to_string(hist.size()));

    if (!hist.empty()) {
        Display::printSectionHeader("Usage History Sparkline");
        std::cout << "  ";
        for (double v : hist) {
            const char* c = Display::percentColor(v);
            int bars = (int)(v / 10.0);
            std::cout << c;
            if      (bars == 0) std::cout << "_";
            else if (bars <= 2) std::cout << ".";
            else if (bars <= 4) std::cout << ":";
            else if (bars <= 6) std::cout << "|";
            else if (bars <= 8) std::cout << "I";
            else                std::cout << "#";
        }
        std::cout << Color::RESET << "\n";
    }

    Display::printFooter();
    waitEnter();
}

// ──────────────────────────────────────────────────────────────────────────────
// MENU 3 – Memory Details
// ──────────────────────────────────────────────────────────────────────────────
static void showMemoryDetails(MemoryMonitor& mem) {
    Display::clearScreen();
    Display::printBanner("MEMORY MONITOR — Details");

    MemoryStats ms = mem.refresh();
    Display::printSectionHeader("Physical Memory");

    Display::printProgressBar("RAM", ms.usedPercent,
        Display::formatBytes(ms.usedPhys) + " / " + Display::formatBytes(ms.totalPhys));

    Display::printKeyValue("Total",     Display::formatBytes(ms.totalPhys));
    Display::printKeyValue("Used",      Display::formatBytes(ms.usedPhys));
    Display::printKeyValue("Available", Display::formatBytes(ms.availPhys));
    Display::printKeyValue("Used %",    [&](){
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2) << ms.usedPercent << "%";
        return ss.str(); }());

    if (ms.totalSwap > 0) {
        Display::printSectionHeader("Swap / Page File");
        Display::printProgressBar("SWAP", ms.swapPercent,
            Display::formatBytes(ms.usedSwap) + " / " + Display::formatBytes(ms.totalSwap));
        Display::printKeyValue("Total Swap",     Display::formatBytes(ms.totalSwap));
        Display::printKeyValue("Used Swap",      Display::formatBytes(ms.usedSwap));
        Display::printKeyValue("Available Swap", Display::formatBytes(ms.availSwap));
    }

    Display::printFooter();
    waitEnter();
}

// ──────────────────────────────────────────────────────────────────────────────
// MENU 4 – Disk Details
// ──────────────────────────────────────────────────────────────────────────────
static void showDiskDetails(DiskMonitor& disk) {
    Display::clearScreen();
    Display::printBanner("DISK MONITOR — Details");

    auto drives = disk.refresh();

    Display::printSectionHeader("Aggregate (All Drives)");
    Display::printProgressBar("TOTAL", disk.getOverallUsedPercent(),
        Display::formatBytes(disk.getUsedBytesAll()) + " / " +
        Display::formatBytes(disk.getTotalBytesAll()));

    Display::printSectionHeader("Per Drive");
    for (auto& d : drives) {
        std::cout << "\n";
        std::string hdr = "  Drive: " + d.drivePath;
        if (!d.driveLabel.empty() && d.driveLabel != d.drivePath)
            hdr += " [" + d.driveLabel + "]";
        std::cout << Color::CYAN << hdr << Color::RESET << "\n";
        Display::printProgressBar("Used", d.usedPercent,
            Display::formatBytes(d.usedBytes) + " / " + Display::formatBytes(d.totalBytes));
        Display::printKeyValue("  Total",  Display::formatBytes(d.totalBytes));
        Display::printKeyValue("  Used",   Display::formatBytes(d.usedBytes));
        Display::printKeyValue("  Free",   Display::formatBytes(d.freeBytes));
    }

    Display::printFooter();
    waitEnter();
}

// ──────────────────────────────────────────────────────────────────────────────
// MENU 5 – Process Manager
// ──────────────────────────────────────────────────────────────────────────────
static void showProcessManager() {
    ProcessManager pm;

    while (true) {
        Display::clearScreen();
        Display::printBanner("PROCESS MANAGER");

        Display::printMenu("Options", {
            "View All Processes (sorted by Memory)",
            "View All Processes (sorted by CPU)",
            "Search Processes by Name",
            "View Process Details (by PID)",
            "Terminate a Process (by PID)"
        });

        int choice = readInt("\nEnter choice [0-5]: ", 0);
        if (choice == 0) break;

        if (choice >= 1 && choice <= 2) {
            Display::clearScreen();
            Display::printBanner("PROCESS MANAGER — Loading...");
            std::cout << Color::DIM << "\n  Collecting process CPU data (approx 0.2s)...\n"
                      << Color::RESET;
            pm.refresh();
            pm.sortBy(choice == 1 ? ProcSortBy::MEMORY : ProcSortBy::CPU, false);

            auto& procs = pm.getProcesses();
            Display::clearScreen();
            Display::printBanner("PROCESS MANAGER — " + std::to_string(procs.size()) + " processes");

            std::vector<std::string> headers = {"PID", "Name", "Memory", "CPU%", "Threads"};
            std::vector<int>        widths   = {7, 30, 12, 8, 8};
            std::vector<std::vector<std::string>> rows;

            size_t limit = std::min(procs.size(), (size_t)40);
            for (size_t i = 0; i < limit; ++i) {
                auto& p = procs[i];
                std::ostringstream mem, cpu;
                mem << std::fixed << std::setprecision(1)
                    << (double)p.memBytes / 1024.0 / 1024.0 << " MB";
                cpu << std::fixed << std::setprecision(2) << p.cpuPercent << "%";
                rows.push_back({
                    std::to_string(p.pid),
                    p.name.size() > 28 ? p.name.substr(0,28) + ".." : p.name,
                    mem.str(),
                    cpu.str(),
                    std::to_string(p.threadCount)
                });
            }
            Display::printTable(headers, rows, widths);
            if (procs.size() > limit)
                std::cout << Color::DIM << "\n  ... and " << (procs.size() - limit)
                          << " more processes\n" << Color::RESET;
            Display::printFooter();
            waitEnter();

        } else if (choice == 3) {
            std::string q = readString("Search name: ");
            pm.refresh();
            auto results = pm.search(q);
            Display::clearScreen();
            Display::printBanner("PROCESS SEARCH — \"" + q + "\"");
            if (results.empty()) {
                std::cout << Color::YELLOW << "\n  No processes found matching '" << q << "'.\n"
                          << Color::RESET;
            } else {
                std::vector<std::string> headers = {"PID", "Name", "Memory", "CPU%"};
                std::vector<int>        widths   = {7, 32, 14, 10};
                std::vector<std::vector<std::string>> rows;
                for (auto& p : results) {
                    std::ostringstream mem, cpu;
                    mem << std::fixed << std::setprecision(1)
                        << (double)p.memBytes / 1024.0 / 1024.0 << " MB";
                    cpu << std::fixed << std::setprecision(2) << p.cpuPercent << "%";
                    rows.push_back({std::to_string(p.pid), p.name, mem.str(), cpu.str()});
                }
                Display::printTable(headers, rows, widths);
            }
            Display::printFooter();
            waitEnter();

        } else if (choice == 4) {
            int pid = readInt("Enter PID: ", -1);
            if (pid < 0) continue;
            pm.refresh();
            auto* p = pm.findByPid((uint32_t)pid);
            if (!p) {
                std::cout << Color::RED << "\n  Process PID " << pid << " not found.\n"
                          << Color::RESET;
            } else {
                Display::clearScreen();
                Display::printBanner("PROCESS DETAILS — PID " + std::to_string(pid));
                Display::printKeyValue("PID",     std::to_string(p->pid));
                Display::printKeyValue("Name",    p->name);
                Display::printKeyValue("Memory",  Display::formatBytes(p->memBytes));
                Display::printKeyValue("CPU %",   [&](){
                    std::ostringstream ss;
                    ss << std::fixed << std::setprecision(2) << p->cpuPercent << "%";
                    return ss.str(); }());
                Display::printKeyValue("Threads", std::to_string(p->threadCount));
                Display::printKeyValue("Status",  p->status);
                Display::printFooter();
            }
            waitEnter();

        } else if (choice == 5) {
            int pid = readInt("Enter PID to terminate: ", -1);
            if (pid <= 0) continue;
            std::cout << Color::RED << Color::BOLD
                      << "\n  WARNING: This will forcefully terminate PID " << pid << "\n"
                      << Color::RESET;
            std::string confirm = readString("Type 'YES' to confirm: ");
            if (confirm == "YES") {
                if (pm.terminate((uint32_t)pid)) {
                    std::cout << Color::GREEN << "\n  Process " << pid
                              << " terminated successfully.\n" << Color::RESET;
                    if (g_logger) g_logger->warn("Process terminated by user: PID " + std::to_string(pid));
                } else {
                    std::cout << Color::RED << "\n  Failed to terminate process " << pid
                              << ". Insufficient privileges?\n" << Color::RESET;
                }
            } else {
                std::cout << Color::DIM << "\n  Cancelled.\n" << Color::RESET;
            }
            waitEnter();
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// MENU 6 – Performance Logs
// ──────────────────────────────────────────────────────────────────────────────
static void showPerformanceLogs() {
    while (true) {
        Display::clearScreen();
        Display::printBanner("PERFORMANCE LOGS");
        Display::printMenu("Options", {
            "View Recent Log Entries (last 30)",
            "Filter Logs by Date",
            "View Alert History",
            "Acknowledge All Alerts",
            "Clear Alert History"
        });

        int choice = readInt("\nEnter choice [0-5]: ", 0);
        if (choice == 0) break;

        if (choice == 1) {
            Display::clearScreen();
            Display::printBanner("RECENT LOG ENTRIES");
            if (!g_logger) {
                std::cout << Color::RED << "\n  Logger not initialized.\n" << Color::RESET;
            } else {
                auto entries = g_logger->getEntries();
                size_t start = entries.size() > 30 ? entries.size() - 30 : 0;
                for (size_t i = start; i < entries.size(); ++i) {
                    auto& e = entries[i];
                    const char* col = (e.level == LogLevel::ERROR_LVL) ? Color::RED
                                    : (e.level == LogLevel::WARN)       ? Color::YELLOW
                                    :                                      Color::GREEN;
                    std::cout << Color::DIM << e.timestamp << " "
                              << col
                              << (e.level == LogLevel::INFO      ? "[INFO ] " :
                                  e.level == LogLevel::WARN      ? "[WARN ] " : "[ERROR] ")
                              << Color::RESET << e.message << "\n";
                }
            }
            Display::printFooter();
            waitEnter();

        } else if (choice == 2) {
            std::string date = readString("Enter date (YYYY-MM-DD): ");
            Display::clearScreen();
            Display::printBanner("LOGS FOR " + date);
            if (!g_logger) {
                std::cout << Color::RED << "\n  Logger not initialized.\n" << Color::RESET;
            } else {
                auto entries = g_logger->getEntriesByDate(date);
                if (entries.empty()) {
                    std::cout << Color::YELLOW << "\n  No log entries found for date: "
                              << date << "\n" << Color::RESET;
                } else {
                    for (auto& e : entries) {
                        const char* col = (e.level == LogLevel::ERROR_LVL) ? Color::RED
                                        : (e.level == LogLevel::WARN)       ? Color::YELLOW
                                        :                                      Color::GREEN;
                        std::cout << Color::DIM << e.timestamp << " "
                                  << col << e.message << Color::RESET << "\n";
                    }
                }
            }
            Display::printFooter();
            waitEnter();

        } else if (choice == 3) {
            Display::clearScreen();
            Display::printBanner("ALERT HISTORY");
            auto& alerts = g_alerts.getAlerts();
            if (alerts.empty()) {
                std::cout << Color::GREEN << "\n  No alerts have been generated.\n"
                          << Color::RESET;
            } else {
                for (auto& a : alerts) {
                    const char* col = a.acknowledged ? Color::DIM : Color::YELLOW;
                    std::string typeStr = (a.type == AlertType::CPU)    ? "CPU"
                                       : (a.type == AlertType::MEMORY) ? "MEM"
                                       :                                  "DISK";
                    std::cout << col << "[" << typeStr << "] "
                              << a.timestamp << " — " << a.message
                              << (a.acknowledged ? " [ACK]" : " [NEW]")
                              << Color::RESET << "\n";
                }
            }
            Display::printFooter();
            waitEnter();

        } else if (choice == 4) {
            g_alerts.acknowledgeAll();
            std::cout << Color::GREEN << "\n  All alerts acknowledged.\n" << Color::RESET;
            waitEnter();

        } else if (choice == 5) {
            g_alerts.clearHistory();
            std::cout << Color::GREEN << "\n  Alert history cleared.\n" << Color::RESET;
            waitEnter();
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// MENU 7 – Report Generation
// ──────────────────────────────────────────────────────────────────────────────
static void showReportGeneration(ReportGenerator& rg,
                                  CpuMonitor& cpu,
                                  MemoryMonitor& mem,
                                  DiskMonitor& disk)
{
    while (true) {
        Display::clearScreen();
        Display::printBanner("REPORT GENERATION");
        Display::printKeyValue("Report Directory", rg.getReportDir());
        Display::printMenu("Report Types", {
            "Daily Performance Report  (CSV + TXT)",
            "System Health Report      (TXT)",
            "CPU Usage Summary         (CSV + TXT)",
            "Memory Usage Summary      (CSV + TXT)",
            "Resource Consumption Report (TXT)"
        });

        int choice = readInt("\nEnter choice [0-5]: ", 0);
        if (choice == 0) break;

        // Build a snapshot for reports that need it
        auto ms = mem.refresh();
        disk.refresh();

        SystemSnapshot snap;
        snap.osName       = SystemInfo::getOS();
        snap.hostName     = SystemInfo::getHostName();
        snap.cpuModel     = SystemInfo::getCpuModel();
        snap.coreCount    = SystemInfo::getCoreCount();
        snap.totalRamBytes = SystemInfo::getTotalRAM();
        snap.cpuPct       = cpu.getCurrentUsage();
        snap.memPct       = ms.usedPercent;
        snap.diskPct      = disk.getOverallUsedPercent();
        snap.procCount    = g_state.procCount;

        bool ok = false;
        switch (choice) {
            case 1: ok = rg.generateDailyReport(g_perfStats);       break;
            case 2: ok = rg.generateHealthReport(snap, g_perfStats); break;
            case 3: ok = rg.generateCpuSummary(g_perfStats);        break;
            case 4: ok = rg.generateMemorySummary(g_perfStats);     break;
            case 5: ok = rg.generateResourceReport(snap, g_perfStats); break;
        }

        if (ok) {
            std::cout << Color::GREEN << "\n  Report generated successfully in '"
                      << rg.getReportDir() << "'\n" << Color::RESET;
            if (g_logger) g_logger->info("Report generated (type " + std::to_string(choice) + ")");
        } else {
            std::cout << Color::RED << "\n  Report generation failed. Check directory permissions.\n"
                      << Color::RESET;
        }
        waitEnter();
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// MENU 8 – Alert Configuration
// ──────────────────────────────────────────────────────────────────────────────
static void showAlertConfig(Config& cfg) {
    while (true) {
        Display::clearScreen();
        Display::printBanner("ALERT SYSTEM CONFIGURATION");

        Display::printSectionHeader("Current Thresholds");
        Display::printKeyValue("CPU Threshold",    [&](){
            std::ostringstream ss;
            ss << g_alerts.getCpuThreshold() << "%";
            return ss.str(); }());
        Display::printKeyValue("Memory Threshold", [&](){
            std::ostringstream ss;
            ss << g_alerts.getMemoryThreshold() << "%";
            return ss.str(); }());
        Display::printKeyValue("Disk Threshold",   [&](){
            std::ostringstream ss;
            ss << g_alerts.getDiskThreshold() << "%";
            return ss.str(); }());

        Display::printMenu("Options", {
            "Set CPU Threshold",
            "Set Memory Threshold",
            "Set Disk Threshold",
            "Set Alert Cooldown (seconds)",
            "View Pending Alerts",
            "Trigger Test Alert"
        });

        int choice = readInt("\nEnter choice [0-6]: ", 0);
        if (choice == 0) break;

        if (choice == 1) {
            double v = readDouble("New CPU threshold [0-100]: ", g_alerts.getCpuThreshold());
            if (v >= 0 && v <= 100) {
                g_alerts.setCpuThreshold(v);
                cfg.setDouble("alert_cpu_threshold", v);
                std::cout << Color::GREEN << "\n  CPU threshold set to " << v << "%\n" << Color::RESET;
            }
        } else if (choice == 2) {
            double v = readDouble("New Memory threshold [0-100]: ", g_alerts.getMemoryThreshold());
            if (v >= 0 && v <= 100) {
                g_alerts.setMemoryThreshold(v);
                cfg.setDouble("alert_mem_threshold", v);
                std::cout << Color::GREEN << "\n  Memory threshold set to " << v << "%\n" << Color::RESET;
            }
        } else if (choice == 3) {
            double v = readDouble("New Disk threshold [0-100]: ", g_alerts.getDiskThreshold());
            if (v >= 0 && v <= 100) {
                g_alerts.setDiskThreshold(v);
                cfg.setDouble("alert_disk_threshold", v);
                std::cout << Color::GREEN << "\n  Disk threshold set to " << v << "%\n" << Color::RESET;
            }
        } else if (choice == 4) {
            int secs = readInt("Cooldown seconds: ", 60);
            if (secs > 0) {
                g_alerts.setCooldownSeconds(secs);
                std::cout << Color::GREEN << "\n  Cooldown set to " << secs << "s\n" << Color::RESET;
            }
        } else if (choice == 5) {
            int pending = g_alerts.getPendingCount();
            std::cout << Color::YELLOW << "\n  Pending (unacknowledged) alerts: "
                      << pending << "\n" << Color::RESET;
        } else if (choice == 6) {
            // Force a test alert by temporarily lowering threshold
            double old = g_alerts.getCpuThreshold();
            g_alerts.setCpuThreshold(0.0);
            g_alerts.setCooldownSeconds(0);
            double cpu_now;
            { std::lock_guard<std::mutex> lk(g_state.mtx); cpu_now = g_state.cpuPct; }
            g_alerts.check(cpu_now, 0.0, 0.0);
            g_alerts.setCpuThreshold(old);
            g_alerts.setCooldownSeconds(60);
            std::cout << Color::GREEN << "\n  Test alert triggered. Check menu [6] > Alert History.\n"
                      << Color::RESET;
        }
        waitEnter();
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// MENU 9 – Search & Statistics
// ──────────────────────────────────────────────────────────────────────────────
static void showStatistics() {
    while (true) {
        Display::clearScreen();
        Display::printBanner("SEARCH & STATISTICS");

        auto dates = g_perfStats.getAvailableDates();
        Display::printKeyValue("Data points collected", std::to_string(g_perfStats.getAll().size()));
        Display::printKeyValue("Dates in history",      std::to_string(dates.size()));

        Display::printMenu("Options", {
            "View Overall CPU Statistics",
            "View Overall Memory Statistics",
            "View Overall Disk Statistics",
            "Filter by Date (view data points)",
            "Analyze Resource Trends",
            "List Available Dates"
        });

        int choice = readInt("\nEnter choice [0-6]: ", 0);
        if (choice == 0) break;

        if (choice >= 1 && choice <= 3) {
            StatSummary s;
            std::string metric;
            if      (choice == 1) { s = g_perfStats.computeCpuStats();  metric = "CPU";    }
            else if (choice == 2) { s = g_perfStats.computeMemStats();  metric = "Memory"; }
            else                  { s = g_perfStats.computeDiskStats(); metric = "Disk";   }

            Display::clearScreen();
            Display::printBanner(metric + " STATISTICS");
            Display::printKeyValue("Min",     [&](){
                std::ostringstream ss; ss << std::fixed << std::setprecision(2) << s.minVal << "%";
                return ss.str(); }());
            Display::printKeyValue("Max",     [&](){
                std::ostringstream ss; ss << std::fixed << std::setprecision(2) << s.maxVal << "%";
                return ss.str(); }());
            Display::printKeyValue("Average", [&](){
                std::ostringstream ss; ss << std::fixed << std::setprecision(2) << s.avgVal << "%";
                return ss.str(); }());
            Display::printKeyValue("Std Dev", [&](){
                std::ostringstream ss; ss << std::fixed << std::setprecision(2) << s.stdDev << "%";
                return ss.str(); }());
            Display::printKeyValue("Trend",
                s.trend > 0 ? "Increasing ↑" :
                s.trend < 0 ? "Decreasing ↓" : "Stable →");
            Display::printFooter();
            waitEnter();

        } else if (choice == 4) {
            std::string date = readString("Enter date (YYYY-MM-DD): ");
            auto pts = g_perfStats.filterByDate(date);
            Display::clearScreen();
            Display::printBanner("DATA POINTS FOR " + date + " (" + std::to_string(pts.size()) + " points)");
            if (pts.empty()) {
                std::cout << Color::YELLOW << "\n  No data for this date.\n" << Color::RESET;
            } else {
                std::vector<std::string> headers = {"Time", "CPU%", "Mem%", "Disk%", "Procs"};
                std::vector<int>        widths   = {20, 8, 8, 8, 6};
                std::vector<std::vector<std::string>> rows;
                size_t limit = std::min(pts.size(), (size_t)30);
                for (size_t i = 0; i < limit; ++i) {
                    auto& p = pts[i];
                    auto fmt = [](double v){
                        std::ostringstream ss;
                        ss << std::fixed << std::setprecision(1) << v << "%";
                        return ss.str();
                    };
                    rows.push_back({p.timestamp, fmt(p.cpuPct), fmt(p.memPct),
                                    fmt(p.diskPct), std::to_string(p.procCount)});
                }
                Display::printTable(headers, rows, widths);
                if (pts.size() > limit)
                    std::cout << Color::DIM << "  ... " << (pts.size() - limit)
                              << " more entries\n" << Color::RESET;
            }
            Display::printFooter();
            waitEnter();

        } else if (choice == 5) {
            auto cs = g_perfStats.computeCpuStats();
            auto ms = g_perfStats.computeMemStats();
            auto ds = g_perfStats.computeDiskStats();
            Display::clearScreen();
            Display::printBanner("RESOURCE TREND ANALYSIS");
            auto trendStr = [](int t) -> const char* {
                if (t > 0) return "↑ INCREASING (attention needed)";
                if (t < 0) return "↓ Decreasing (good)";
                return "→ Stable";
            };
            Display::printKeyValue("CPU  trend", trendStr(cs.trend));
            Display::printKeyValue("MEM  trend", trendStr(ms.trend));
            Display::printKeyValue("DISK trend", trendStr(ds.trend));
            Display::printFooter();
            waitEnter();

        } else if (choice == 6) {
            Display::clearScreen();
            Display::printBanner("AVAILABLE DATES IN HISTORY");
            if (dates.empty()) {
                std::cout << Color::YELLOW << "\n  No data collected yet.\n" << Color::RESET;
            } else {
                for (auto& d : dates)
                    std::cout << "  " << Color::CYAN << d << Color::RESET << "\n";
            }
            Display::printFooter();
            waitEnter();
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// MENU 10 – Configuration
// ──────────────────────────────────────────────────────────────────────────────
static void showConfiguration(Config& cfg, const std::string& cfgPath) {
    while (true) {
        Display::clearScreen();
        Display::printBanner("CONFIGURATION MANAGEMENT");
        Display::printKeyValue("Config File",    cfgPath);
        Display::printKeyValue("Monitor Interval", cfg.get("monitor_interval_ms", "1000") + " ms");
        Display::printKeyValue("Log File",       cfg.get("log_file", "logs/sysmon.log"));
        Display::printKeyValue("Report Dir",     cfg.get("report_dir", "reports"));
        Display::printKeyValue("CPU Threshold",  cfg.get("alert_cpu_threshold", "85.0") + "%");
        Display::printKeyValue("Mem Threshold",  cfg.get("alert_mem_threshold", "85.0") + "%");
        Display::printKeyValue("Disk Threshold", cfg.get("alert_disk_threshold", "90.0") + "%");

        Display::printMenu("Options", {
            "Change Monitor Interval (ms)",
            "Change Log File Path",
            "Change Report Directory",
            "Save Configuration to File",
            "Reload Configuration from File"
        });

        int choice = readInt("\nEnter choice [0-5]: ", 0);
        if (choice == 0) break;

        if (choice == 1) {
            int ms = readInt("New interval in ms [100-60000]: ", 1000);
            if (ms >= 100 && ms <= 60000) {
                cfg.setInt("monitor_interval_ms", ms);
                std::cout << Color::GREEN << "\n  Interval updated (takes effect on restart).\n"
                          << Color::RESET;
            }
        } else if (choice == 2) {
            std::string p = readString("New log file path: ");
            if (!p.empty()) cfg.set("log_file", p);
        } else if (choice == 3) {
            std::string p = readString("New report directory: ");
            if (!p.empty()) cfg.set("report_dir", p);
        } else if (choice == 4) {
            if (cfg.saveToFile(cfgPath)) {
                std::cout << Color::GREEN << "\n  Configuration saved to: " << cfgPath << "\n"
                          << Color::RESET;
            } else {
                std::cout << Color::RED << "\n  Failed to save config.\n" << Color::RESET;
            }
        } else if (choice == 5) {
            cfg.loadFromFile(cfgPath);
            std::cout << Color::GREEN << "\n  Configuration reloaded.\n" << Color::RESET;
        }
        waitEnter();
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// MENU 0 – System Information
// ──────────────────────────────────────────────────────────────────────────────
static void showSystemInfo() {
    Display::clearScreen();
    Display::printBanner("SYSTEM INFORMATION");

    Display::printSectionHeader("Host & OS");
    Display::printKeyValue("Hostname",   SystemInfo::getHostName());
    Display::printKeyValue("OS",         SystemInfo::getOS());
    Display::printKeyValue("Uptime",     SystemInfo::getUptimeString());

    Display::printSectionHeader("CPU");
    Display::printKeyValue("Model",           SystemInfo::getCpuModel());
    Display::printKeyValue("Physical Cores",  std::to_string(SystemInfo::getCoreCount()));
    Display::printKeyValue("Logical Cores",   std::to_string(SystemInfo::getLogicalCoreCount()));

    Display::printSectionHeader("Memory");
    Display::printKeyValue("Total RAM",
        Display::formatBytes(SystemInfo::getTotalRAM()));
    Display::printKeyValue("Available RAM",
        Display::formatBytes(SystemInfo::getFreeRAM()));
    Display::printKeyValue("Used RAM",
        Display::formatBytes(SystemInfo::getUsedRAM()));

    Display::printFooter();
    waitEnter();
}

// ──────────────────────────────────────────────────────────────────────────────
// Background Monitoring Thread
// ──────────────────────────────────────────────────────────────────────────────
static void backgroundMonitor(MemoryMonitor* mem, DiskMonitor* disk,
                               int intervalMs)
{
    while (g_state.running) {
        auto ms = mem->refresh();
        auto dp = disk->getOverallUsedPercent();
        double cpu, memPct;

        {
            std::lock_guard<std::mutex> lk(g_state.mtx);
            cpu    = g_state.cpuPct;
            memPct = ms.usedPercent;
            g_state.memPct    = memPct;
            g_state.diskPct   = dp;
            g_state.memStats  = ms;
        }

        // Record data point for statistics
        auto pt = makeDataPoint(cpu, memPct, dp, g_state.procCount);
        g_perfStats.addPoint(pt);

        // Log to file
        if (g_logger) {
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(1)
               << "CPU:" << cpu << "% MEM:" << memPct << "% DISK:" << dp << "%";
            g_logger->info(ss.str());
        }

        // Check alerts
        g_alerts.check(cpu, memPct, dp);

        std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Main
// ──────────────────────────────────────────────────────────────────────────────
int main() {
    // ── Init display (enable VT processing on Windows) ────────────────────────
    Display::init();

    // ── Splash screen ─────────────────────────────────────────────────────────
    Display::clearScreen();
    Display::printBanner("SYSTEM RESOURCE MONITOR & PERFORMANCE ANALYZER");
    std::cout << "\n"
              << Color::DIM << "  Domain   : C/C++ Systems Programming\n"
              << "  Platform : " << SystemInfo::getOS() << "\n"
              << "  Host     : " << SystemInfo::getHostName() << "\n"
              << Color::RESET << "\n";

    // ── Create directories ────────────────────────────────────────────────────
    MKDIR_CMD("logs");
    MKDIR_CMD("reports");

    // ── Load configuration ────────────────────────────────────────────────────
    Config cfg;
    std::string cfgPath = "config/config.ini";
    if (!cfg.loadFromFile(cfgPath)) {
        std::cout << Color::YELLOW
                  << "  [Warning] Could not load config from '" << cfgPath
                  << "'. Using defaults.\n" << Color::RESET;
    }

    std::string logFile   = cfg.get("log_file", "logs/sysmon.log");
    std::string reportDir = cfg.get("report_dir", "reports");
    int         intervalMs = cfg.getInt("monitor_interval_ms", 1000);

    // ── Configure alerts from config ──────────────────────────────────────────
    g_alerts.setCpuThreshold(cfg.getDouble("alert_cpu_threshold", 85.0));
    g_alerts.setMemoryThreshold(cfg.getDouble("alert_mem_threshold", 85.0));
    g_alerts.setDiskThreshold(cfg.getDouble("alert_disk_threshold", 90.0));
    g_alerts.setCallback([](const Alert& a) {
        // Alert fires in background thread — just note it (logger will pick it up)
        (void)a;
    });

    // ── Init logger ───────────────────────────────────────────────────────────
    Logger logger(logFile);
    g_logger = &logger;
    logger.info("=== System Resource Monitor started ===");
    logger.info("Host: " + SystemInfo::getHostName() + "  OS: " + SystemInfo::getOS());

    // ── Init monitors ─────────────────────────────────────────────────────────
    CpuMonitor    cpu(intervalMs);
    MemoryMonitor mem;
    DiskMonitor   disk;
    ReportGenerator rg(reportDir);

    // Start CPU monitor (background thread)
    cpu.start([&](double pct) {
        std::lock_guard<std::mutex> lk(g_state.mtx);
        g_state.cpuPct = pct;
    });

    // Start background monitor thread (mem, disk, logging, stats, alerts)
    std::thread bgThread(backgroundMonitor, &mem, &disk, intervalMs * 5);

    std::cout << Color::GREEN << "  Monitoring started.\n" << Color::RESET;
    std::cout << Color::DIM  << "  Press ENTER to open the main menu...\n" << Color::RESET;
    std::string dummy;
    std::getline(std::cin, dummy);

    // ── Main menu loop ────────────────────────────────────────────────────────
    while (true) {
        Display::clearScreen();
        Display::printBanner("SYSTEM RESOURCE MONITOR & PERFORMANCE ANALYZER");

        // Quick status bar
        double cp, mp, dp;
        {
            std::lock_guard<std::mutex> lk(g_state.mtx);
            cp = g_state.cpuPct; mp = g_state.memPct; dp = g_state.diskPct;
        }
        std::cout << "\n  Status: ";
        std::cout << "CPU " << Display::percentColor(cp) << Color::BOLD
                  << std::fixed << std::setprecision(1) << cp << "%" << Color::RESET << "  ";
        std::cout << "MEM " << Display::percentColor(mp) << Color::BOLD
                  << std::fixed << std::setprecision(1) << mp << "%" << Color::RESET << "  ";
        std::cout << "DISK " << Display::percentColor(dp) << Color::BOLD
                  << std::fixed << std::setprecision(1) << dp << "%" << Color::RESET;

        int pending = g_alerts.getPendingCount();
        if (pending > 0) {
            std::cout << "  " << Color::RED << Color::BOLD
                      << "[!" << pending << " ALERT" << (pending > 1 ? "S" : "") << "]"
                      << Color::RESET;
        }
        std::cout << "\n";

        Display::printSeparator(70);
        std::cout << "\n";

        std::vector<std::string> menuItems = {
            "System Information",
            "Real-time Dashboard  (live refresh)",
            "CPU Monitor Details",
            "Memory Monitor Details",
            "Disk Monitor Details",
            "Process Manager      (list / search / sort / terminate)",
            "Performance Logs     (view / filter by date)",
            "Generate Report      (CSV / TXT)",
            "Alert Configuration  (thresholds / history)",
            "Search & Statistics  (trends / analysis)",
            "Configuration Settings"
        };

        for (size_t i = 0; i < menuItems.size(); ++i) {
            std::cout << "  " << Color::YELLOW << Color::BOLD
                      << "[" << (i + 1) << "] " << Color::RESET
                      << menuItems[i] << "\n";
        }
        std::cout << "  " << Color::YELLOW << Color::BOLD
                  << "[0] " << Color::RESET << "Exit\n";

        int choice = readInt("\n  Enter choice [0-11]: ", -1);

        switch (choice) {
            case 0: goto exit_loop;
            case 1: showSystemInfo();                                         break;
            case 2: showDashboard(cpu, mem, disk);                            break;
            case 3: showCpuDetails(cpu);                                      break;
            case 4: showMemoryDetails(mem);                                   break;
            case 5: showDiskDetails(disk);                                    break;
            case 6: showProcessManager();                                     break;
            case 7: showPerformanceLogs();                                    break;
            case 8: showReportGeneration(rg, cpu, mem, disk);                 break;
            case 9: showAlertConfig(cfg);                                     break;
            case 10: showStatistics();                                        break;
            case 11: showConfiguration(cfg, cfgPath);                        break;
            default:
                std::cout << Color::RED << "\n  Invalid choice. Please try again.\n"
                          << Color::RESET;
                std::this_thread::sleep_for(std::chrono::milliseconds(800));
                break;
        }
    }

exit_loop:
    // ── Shutdown ──────────────────────────────────────────────────────────────
    Display::clearScreen();
    std::cout << Color::CYAN << Color::BOLD
              << "\n  Shutting down System Resource Monitor...\n"
              << Color::RESET;

    g_state.running = false;
    cpu.stop();

    if (bgThread.joinable()) bgThread.join();

    logger.info("=== System Resource Monitor stopped ===");

    // Auto-save config on exit
    cfg.saveToFile(cfgPath);

    std::cout << Color::GREEN << "  Goodbye!\n\n" << Color::RESET;
    return 0;
}
