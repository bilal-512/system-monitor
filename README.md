# System Resource Monitor & Performance Analyzer (C++)

Skeleton project containing initial folder structure and minimal implementation for a System Resource Monitor.

Features included in this starter commit:
- Basic SystemInfo utilities (OS, hostname, uptime, memory)
- Simple Logger
- Config file reader (key=value)
- CPU monitor that logs instantaneous CPU usage periodically (Linux /proc/stat based)
- CMake build

Build (Linux/macOS with CMake):
mkdir build && cd build
cmake ..
cmake --build .

Run:
./sysmon

You can complete and extend the modules (disk, processes, reports, alerts, GUI) as needed.
