# System Monitor

System Monitor is a desktop application built with C++ and Qt 6 for monitoring CPU, memory, disk, and running processes in real time. It provides a graphical dashboard, process management tools, system information views, alerting, and report generation for system performance analysis.

## Features

- Real-time monitoring of CPU, memory, and disk usage
- Live charts and resource trend visualization
- Process listing with filtering and termination support
- System information summary including OS, hardware, and uptime
- Configurable alert thresholds for CPU, memory, and disk usage
- Report generation for daily and resource-specific performance data
- Persistent settings stored in a configuration file

## Project Overview

This project is organized around a modular C++ architecture:

- Core monitoring logic in the include and src folders
- Qt-based UI components in src/ui
- Background worker thread logic in src/core
- Runtime configuration, logs, and reports under config, logs, and reports

## Technology Stack

- Language: C++17
- UI Framework: Qt 6 (Core, Gui, Widgets, Charts)
- Build System: CMake
- Platform: Windows-focused, with Windows API support for system metrics

## Prerequisites

Before building the project, make sure you have:

1. A C++17 compiler
2. CMake 3.16 or newer
3. Qt 6 installed with the following modules:
   - Core
   - Gui
   - Widgets
   - Charts

## Building the Project

### Option 1: Qt Creator

1. Open Qt Creator.
2. Open the CMake project by selecting the project root CMakeLists.txt file.
3. Configure the project with a Qt 6 desktop kit.
4. Build and run the application.

### Option 2: CMake from the Command Line

On Windows, you can build it with:

```bash
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
cmake --build . --config Debug
```

If you are using Visual Studio generators, replace the generator line with the appropriate Visual Studio version.

## Running the Application

After building, run the generated executable from the build output directory. The application will load its configuration from the config folder and generate reports and logs at runtime.

## Project Structure

```text
System-Moniter/
├── config/           # Runtime configuration files
├── docs/             # Design and architecture notes
├── include/          # Header files for system monitoring modules
├── logs/             # Log output directory
├── reports/          # Generated reports
├── scripts/          # Build or utility scripts
├── src/              # Source code
│   ├── core/         # Worker/threading logic
│   ├── ui/           # Qt UI widgets and windows
│   └── *.cpp         # Monitoring and support modules
├── CMakeLists.txt    # Build configuration
└── README.md         # Project documentation
```

## Configuration

Application settings can be adjusted through the Settings tab in the UI or by editing the configuration file in the config directory.

Example:

```ini
UpdateIntervalMs=2000
CpuAlertThreshold=85.0
MemAlertThreshold=85.0
DiskAlertThreshold=90.0
```

## Notes

- Generated build output, logs, and reports are intended to be ignored by Git.
- You may need to adjust Qt paths depending on your local environment.

## License

This project is intended for educational and development purposes.
