# System Resource Monitor & Performance Analyzer

A production-ready **C++ console application** that monitors system resources in real-time,
manages processes, generates performance reports, and alerts on threshold breaches.

## Features Implemented

| # | Feature | Status |
|---|---------|--------|
| 1 | System Information (OS, CPU, RAM, Disk, Hostname, Uptime) | ✅ |
| 2 | CPU Monitoring (usage %, cores, history, avg, peak) | ✅ |
| 3 | Memory Monitoring (total/used/free/%, swap) | ✅ |
| 4 | Disk Monitoring (per-drive + aggregate, %) | ✅ |
| 5 | Process Manager (list/search/sort/terminate) | ✅ |
| 6 | Performance Logging (timestamped, filterable by date) | ✅ |
| 7 | Report Generation (Daily, Health, CPU, Memory, Resource — CSV + TXT) | ✅ |
| 8 | Alert System (configurable thresholds + cooldown) | ✅ |
| 9 | Search & Statistics (trend analysis, date filter, stddev) | ✅ |
| 10 | Configuration Management (ini file, runtime changes, save) | ✅ |

## Technology Stack

- **Language**: C++17
- **Build**: CMake 3.10+
- **OS APIs**: WinAPI (`GetSystemTimes`, `GlobalMemoryStatusEx`, `GetDiskFreeSpaceEx`, `ToolHelp32`, `PSAPI`, `RegOpenKeyEx`)
- **Threading**: `std::thread`, `std::atomic`, `std::mutex`
- **UI**: ANSI console TUI (VT100 escape codes, enabled via `ENABLE_VIRTUAL_TERMINAL_PROCESSING`)

## Build Instructions

### Option A – CMake (Recommended)

```cmd
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
cd Release
sysmon.exe
```

### Option B – Visual Studio (CMake integration)

1. Open Visual Studio 2019/2022
2. `File > Open > CMake...` → select `CMakeLists.txt`
3. Visual Studio auto-configures from CMakeLists.txt
4. `Build > Build All` (Ctrl+Shift+B)
5. Run `sysmon.exe`

> **Note:** Requires Windows 10+ for full ANSI color support.  
> Run in Windows Terminal or a modern console for best display.

## Project Structure

```
system-monitor/
├── include/                 # Header files
│   ├── Config.h
│   ├── Logger.h
│   ├── SystemInfo.h
│   ├── CpuMonitor.h
│   ├── MemoryMonitor.h
│   ├── DiskMonitor.h
│   ├── ProcessManager.h
│   ├── AlertSystem.h
│   ├── ReportGenerator.h
│   ├── PerformanceStats.h
│   └── Display.h
├── src/                     # Source implementations
│   ├── main.cpp             # Application entry + interactive menu
│   ├── Config.cpp
│   ├── Logger.cpp
│   ├── SystemInfo.cpp
│   ├── CpuMonitor.cpp
│   ├── MemoryMonitor.cpp
│   ├── DiskMonitor.cpp
│   ├── ProcessManager.cpp
│   ├── AlertSystem.cpp
│   ├── ReportGenerator.cpp
│   ├── PerformanceStats.cpp
│   └── Display.cpp
├── config/
│   └── config.ini           # Runtime configuration
├── logs/                    # Auto-created: timestamped log files
├── reports/                 # Auto-created: CSV + TXT reports
├── CMakeLists.txt
└── README.md
```

## Configuration (`config/config.ini`)

```ini
monitor_interval_ms=1000        # Polling interval in ms
log_file=logs/sysmon.log        # Log output path
report_dir=reports              # Report output directory
alert_cpu_threshold=85.0        # CPU alert threshold (%)
alert_mem_threshold=85.0        # Memory alert threshold (%)
alert_disk_threshold=90.0       # Disk alert threshold (%)
alert_cooldown_secs=60          # Seconds between repeated alerts
```

## Menu Overview

```
[1]  System Information
[2]  Real-time Dashboard     ← live refresh, CPU sparkline, all resources
[3]  CPU Monitor Details     ← model, cores, history, avg, peak
[4]  Memory Monitor Details  ← RAM + swap breakdown
[5]  Disk Monitor Details    ← per-drive usage
[6]  Process Manager         ← list / search / sort / terminate
[7]  Performance Logs        ← view entries, filter by date, alert history
[8]  Generate Report         ← 5 report types in CSV + TXT
[9]  Alert Configuration     ← set thresholds, cooldown, test alerts
[10] Search & Statistics     ← trends, date range, stddev analysis
[11] Configuration Settings  ← runtime config + save to file
[0]  Exit
```
