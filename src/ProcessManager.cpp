#include "ProcessManager.h"
#include <algorithm>
#include <cctype>
#include <thread>
#include <chrono>

#ifdef _WIN32
#   ifndef NOMINMAX
#       define NOMINMAX
#   endif
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#   endif
#   include <windows.h>
#   include <tlhelp32.h>
#   include <psapi.h>
#   pragma comment(lib, "psapi.lib")
#else
#   include <dirent.h>
#   include <fstream>
#   include <sstream>
#   include <cstring>
#   include <unistd.h>
#   include <sys/types.h>
#   include <signal.h>
#endif

namespace sysmon {

ProcessManager::ProcessManager() {}

// ──────────────────────────────────────────────────────────────────────────────
// Windows implementation
// ──────────────────────────────────────────────────────────────────────────────
#ifdef _WIN32

uint64_t ProcessManager::getProcessCpuTime(void* hProcess) {
    FILETIME creation, exit, kernel, user;
    if (GetProcessTimes((HANDLE)hProcess, &creation, &exit, &kernel, &user)) {
        ULARGE_INTEGER k, u;
        k.LowPart  = kernel.dwLowDateTime; k.HighPart = kernel.dwHighDateTime;
        u.LowPart  = user.dwLowDateTime;   u.HighPart = user.dwHighDateTime;
        return k.QuadPart + u.QuadPart; // 100-ns units
    }
    return 0;
}

void ProcessManager::refresh() {
    // ── First snapshot of CPU times ──────────────────────────────────────────
    std::vector<ProcessInfo> firstSnap;
    {
        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnap == INVALID_HANDLE_VALUE) return;

        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(pe);
        if (Process32First(hSnap, &pe)) {
            do {
                ProcessInfo pi;
                pi.pid         = pe.th32ProcessID;
#ifdef UNICODE
                std::wstring ws(pe.szExeFile);
                pi.name = std::string(ws.begin(), ws.end());
#else
                pi.name        = pe.szExeFile;
#endif
                pi.threadCount = pe.cntThreads;
                pi.status      = "Running";

                HANDLE hProc = OpenProcess(
                    PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pi.pid);
                if (hProc) {
                    // Memory
                    PROCESS_MEMORY_COUNTERS pmc{};
                    pmc.cb = sizeof(pmc);
                    if (GetProcessMemoryInfo(hProc, &pmc, sizeof(pmc)))
                        pi.memBytes = pmc.WorkingSetSize;
                    // CPU time snapshot 1
                    pi.cpuTimePrev = getProcessCpuTime(hProc);
                    CloseHandle(hProc);
                }
                firstSnap.push_back(pi);
            } while (Process32Next(hSnap, &pe));
        }
        CloseHandle(hSnap);
    }

    // ── Wait a short interval for delta ──────────────────────────────────────
    ULONGLONG wallBefore = GetTickCount64();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    ULONGLONG wallAfter  = GetTickCount64();
    double elapsedMs = (double)(wallAfter - wallBefore);

    // ── Second snapshot ───────────────────────────────────────────────────────
    processes_.clear();
    for (auto& pi : firstSnap) {
        HANDLE hProc = OpenProcess(
            PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pi.pid);
        if (hProc) {
            uint64_t cpuNow = getProcessCpuTime(hProc);
            uint64_t delta  = cpuNow - pi.cpuTimePrev; // 100-ns units
            // Convert to ms: delta * 100 / 1e6 = delta / 10000
            double cpuMs = (double)delta / 10000.0;
            // Normalize by elapsed wall time and number of logical CPUs
            SYSTEM_INFO si{};
            GetSystemInfo(&si);
            int numCores = (int)si.dwNumberOfProcessors;
            pi.cpuPercent = (elapsedMs > 0)
                            ? (cpuMs / ((double)numCores * elapsedMs)) * 100.0
                            : 0.0;
            if (pi.cpuPercent > 100.0) pi.cpuPercent = 100.0;
            CloseHandle(hProc);
        }
        processes_.push_back(pi);
    }
}

bool ProcessManager::terminate(uint32_t pid) {
    HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!hProc) return false;
    bool ok = (TerminateProcess(hProc, 1) != 0);
    CloseHandle(hProc);
    return ok;
}

// ──────────────────────────────────────────────────────────────────────────────
// Linux implementation
// ──────────────────────────────────────────────────────────────────────────────
#else

static uint64_t readProcCpuTime(uint32_t pid) {
    std::string path = "/proc/" + std::to_string(pid) + "/stat";
    std::ifstream ifs(path);
    if (!ifs.is_open()) return 0;
    std::string line;
    std::getline(ifs, line);
    // Fields: pid comm state ppid pgroup session tty ... utime stime ...
    // utime=14th token, stime=15th token (0-indexed)
    std::istringstream ss(line);
    std::string tok;
    uint64_t utime = 0, stime = 0;
    int idx = 0;
    while (ss >> tok) {
        if (idx == 13) utime = std::stoull(tok);
        if (idx == 14) { stime = std::stoull(tok); break; }
        ++idx;
    }
    return utime + stime; // in clock ticks
}

static unsigned long long readProcMem(uint32_t pid) {
    std::string path = "/proc/" + std::to_string(pid) + "/statm";
    std::ifstream ifs(path);
    if (!ifs.is_open()) return 0;
    unsigned long pages = 0;
    ifs >> pages; // VmRSS in pages
    return (unsigned long long)pages * (unsigned long long)getpagesize();
}

static std::string readProcName(uint32_t pid) {
    std::string path = "/proc/" + std::to_string(pid) + "/comm";
    std::ifstream ifs(path);
    if (!ifs.is_open()) return "?";
    std::string name;
    std::getline(ifs, name);
    return name;
}

void ProcessManager::refresh() {
    std::vector<ProcessInfo> first;
    DIR* dir = opendir("/proc");
    if (!dir) return;
    dirent* ent;
    while ((ent = readdir(dir)) != nullptr) {
        if (ent->d_type != DT_DIR) continue;
        bool allDigit = true;
        for (char c : std::string(ent->d_name))
            if (!std::isdigit(c)) { allDigit = false; break; }
        if (!allDigit) continue;
        uint32_t pid = (uint32_t)std::stoul(ent->d_name);
        ProcessInfo pi;
        pi.pid         = pid;
        pi.name        = readProcName(pid);
        pi.memBytes    = readProcMem(pid);
        pi.cpuTimePrev = readProcCpuTime(pid);
        pi.status      = "Running";
        first.push_back(pi);
    }
    closedir(dir);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    long hz = sysconf(_SC_CLK_TCK);

    processes_.clear();
    for (auto& pi : first) {
        uint64_t cpuNow = readProcCpuTime(pi.pid);
        uint64_t delta  = cpuNow - pi.cpuTimePrev; // ticks in 200ms
        pi.cpuPercent   = (hz > 0)
                          ? ((double)delta / (double)hz) * 100.0 / 0.2
                          : 0.0;
        if (pi.cpuPercent > 100.0) pi.cpuPercent = 100.0;
        processes_.push_back(pi);
    }
}

bool ProcessManager::terminate(uint32_t pid) {
    return (kill((pid_t)pid, SIGTERM) == 0);
}
#endif

// ──────────────────────────────────────────────────────────────────────────────
// Common methods
// ──────────────────────────────────────────────────────────────────────────────
const std::vector<ProcessInfo>& ProcessManager::getProcesses() const {
    return processes_;
}

std::vector<ProcessInfo> ProcessManager::search(const std::string& query) const {
    std::string q = query;
    std::transform(q.begin(), q.end(), q.begin(), ::tolower);
    std::vector<ProcessInfo> res;
    for (auto& p : processes_) {
        std::string name = p.name;
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        if (name.find(q) != std::string::npos) res.push_back(p);
    }
    return res;
}

void ProcessManager::sortBy(ProcSortBy field, bool ascending) {
    auto cmp = [&](const ProcessInfo& a, const ProcessInfo& b) {
        bool less = false;
        switch (field) {
            case ProcSortBy::NAME:   less = a.name < b.name; break;
            case ProcSortBy::PID:    less = a.pid  < b.pid;  break;
            case ProcSortBy::MEMORY: less = a.memBytes   < b.memBytes;   break;
            case ProcSortBy::CPU:    less = a.cpuPercent < b.cpuPercent; break;
        }
        return ascending ? less : !less;
    };
    std::sort(processes_.begin(), processes_.end(), cmp);
}

const ProcessInfo* ProcessManager::findByPid(uint32_t pid) const {
    for (auto& p : processes_)
        if (p.pid == pid) return &p;
    return nullptr;
}

} // namespace sysmon
