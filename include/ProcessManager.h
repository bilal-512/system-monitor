#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace sysmon {

struct ProcessInfo {
    uint32_t           pid         = 0;
    std::string        name;
    unsigned long long memBytes    = 0;   // Working Set
    double             cpuPercent  = 0.0; // CPU usage since last sample
    uint64_t           cpuTimePrev = 0;   // for delta computation
    uint32_t           threadCount = 0;
    std::string        status;            // "Running" / "Unknown"
};

enum class ProcSortBy { NAME, PID, MEMORY, CPU };

/**
 * @brief Process manager – list, search, sort, terminate.
 * Windows: CreateToolhelp32Snapshot + OpenProcess + PSAPI.
 * Linux: /proc filesystem.
 */
class ProcessManager {
public:
    ProcessManager();

    // Refresh process list (takes ~100ms on Windows to compute CPU%)
    void refresh();

    const std::vector<ProcessInfo>& getProcesses() const;

    // Search by name (case-insensitive substring)
    std::vector<ProcessInfo> search(const std::string& query) const;

    // Sort the internal list
    void sortBy(ProcSortBy field, bool ascending = false);

    // Terminate a process by PID (may require elevated privileges)
    bool terminate(uint32_t pid);

    // Find a process by PID
    const ProcessInfo* findByPid(uint32_t pid) const;

private:
    std::vector<ProcessInfo> processes_;

#ifdef _WIN32
    // Helper: get total CPU time for a process handle (100-ns units)
    static uint64_t getProcessCpuTime(void* hProcess);
#endif
};

} // namespace sysmon
