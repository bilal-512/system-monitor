System Resource Monitor - Architecture (starter)

This document outlines the intended modular architecture for the monitor.

Modules:
- core: system info, monitors (CPU, memory, disk)
- io: logger, config, report generation
- ui: CLI or GUI (optional)
- agents: process manager, alerting

The current skeleton implements core/system info, io/logger, and a CPU monitor.
