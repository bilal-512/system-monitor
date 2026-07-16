#include "SystemInfo.h"
#include <sstream>
#include <cstring>

#ifdef _WIN32
#   ifndef NOMINMAX
#       define NOMINMAX
#   endif
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#   endif
#   include <windows.h>
#   include <sysinfoapi.h>
#   pragma comment(lib, "advapi32.lib")
#else
#   include <sys/utsname.h>
#   include <unistd.h>
#   include <sys/sysinfo.h>
#   include <fstream>
#endif

namespace sysmon {

// ──────────────────────────────────────────────────────────────────────────────
// OS Information
// ──────────────────────────────────────────────────────────────────────────────
std::string SystemInfo::getOS() {
#ifdef _WIN32
    // Use RtlGetVersion (always works, ignores compat shims)
    typedef LONG(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (ntdll) {
        auto fn = (RtlGetVersionPtr)GetProcAddress(ntdll, "RtlGetVersion");
        if (fn) {
            RTL_OSVERSIONINFOW info{};
            info.dwOSVersionInfoSize = sizeof(info);
            fn(&info);
            std::ostringstream ss;
            ss << "Windows " << info.dwMajorVersion << "."
               << info.dwMinorVersion << " (Build " << info.dwBuildNumber << ")";
            return ss.str();
        }
    }
    return "Windows (unknown version)";
#else
    struct utsname buf;
    if (uname(&buf) == 0) {
        std::ostringstream ss;
        ss << buf.sysname << " " << buf.release << " " << buf.machine;
        return ss.str();
    }
    return "Linux (unknown)";
#endif
}

// ──────────────────────────────────────────────────────────────────────────────
// Host Name
// ──────────────────────────────────────────────────────────────────────────────
std::string SystemInfo::getHostName() {
#ifdef _WIN32
    char name[MAX_COMPUTERNAME_LENGTH + 1] = {0};
    DWORD size = sizeof(name);
    if (GetComputerNameA(name, &size)) return std::string(name);
    return "unknown";
#else
    char name[256] = {0};
    if (gethostname(name, sizeof(name)) == 0) return std::string(name);
    return "unknown";
#endif
}

// ──────────────────────────────────────────────────────────────────────────────
// Uptime
// ──────────────────────────────────────────────────────────────────────────────
unsigned long SystemInfo::getUptimeSeconds() {
#ifdef _WIN32
    return static_cast<unsigned long>(GetTickCount64() / 1000ULL);
#else
    struct sysinfo info;
    if (sysinfo(&info) == 0) return static_cast<unsigned long>(info.uptime);
    return 0;
#endif
}

std::string SystemInfo::getUptimeString() {
    unsigned long s = getUptimeSeconds();
    unsigned long d = s / 86400;
    unsigned long h = (s % 86400) / 3600;
    unsigned long m = (s % 3600) / 60;
    unsigned long sec = s % 60;
    std::ostringstream ss;
    if (d > 0) ss << d << "d ";
    ss << h << "h " << m << "m " << sec << "s";
    return ss.str();
}

// ──────────────────────────────────────────────────────────────────────────────
// CPU Model
// ──────────────────────────────────────────────────────────────────────────────
std::string SystemInfo::getCpuModel() {
#ifdef _WIN32
    // Read from registry
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                      "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char buf[256] = {0};
        DWORD size = sizeof(buf);
        RegQueryValueExA(hKey, "ProcessorNameString", nullptr, nullptr,
                         (LPBYTE)buf, &size);
        RegCloseKey(hKey);
        // Trim leading/trailing spaces
        std::string s(buf);
        size_t a = s.find_first_not_of(' ');
        size_t b = s.find_last_not_of(' ');
        if (a != std::string::npos) return s.substr(a, b - a + 1);
    }
    return "Unknown CPU";
#else
    // Read from /proc/cpuinfo
    std::ifstream ifs("/proc/cpuinfo");
    std::string line;
    while (std::getline(ifs, line)) {
        if (line.find("model name") != std::string::npos) {
            auto pos = line.find(':');
            if (pos != std::string::npos) {
                std::string s = line.substr(pos + 1);
                size_t a = s.find_first_not_of(' ');
                if (a != std::string::npos) return s.substr(a);
            }
        }
    }
    return "Unknown CPU";
#endif
}

// ──────────────────────────────────────────────────────────────────────────────
// Core Count
// ──────────────────────────────────────────────────────────────────────────────
int SystemInfo::getCoreCount() {
#ifdef _WIN32
    SYSTEM_INFO si{};
    GetSystemInfo(&si);
    return static_cast<int>(si.dwNumberOfProcessors);
#else
    return static_cast<int>(sysconf(_SC_NPROCESSORS_ONLN));
#endif
}

int SystemInfo::getLogicalCoreCount() {
    return getCoreCount(); // On Windows, dwNumberOfProcessors is logical cores
}

// ──────────────────────────────────────────────────────────────────────────────
// RAM
// ──────────────────────────────────────────────────────────────────────────────
unsigned long long SystemInfo::getTotalRAM() {
#ifdef _WIN32
    MEMORYSTATUSEX m{};
    m.dwLength = sizeof(m);
    if (GlobalMemoryStatusEx(&m)) return m.ullTotalPhys;
    return 0;
#else
    struct sysinfo info;
    if (sysinfo(&info) == 0)
        return (unsigned long long)info.totalram * info.mem_unit;
    return 0;
#endif
}

unsigned long long SystemInfo::getFreeRAM() {
#ifdef _WIN32
    MEMORYSTATUSEX m{};
    m.dwLength = sizeof(m);
    if (GlobalMemoryStatusEx(&m)) return m.ullAvailPhys;
    return 0;
#else
    struct sysinfo info;
    if (sysinfo(&info) == 0)
        return (unsigned long long)info.freeram * info.mem_unit;
    return 0;
#endif
}

unsigned long long SystemInfo::getUsedRAM() {
    return getTotalRAM() - getFreeRAM();
}

double SystemInfo::getRAMUsagePercent() {
    unsigned long long total = getTotalRAM();
    if (total == 0) return 0.0;
    return 100.0 * (double)getUsedRAM() / (double)total;
}

// Legacy KB wrappers
unsigned long long SystemInfo::getTotalRAMKB() { return getTotalRAM() / 1024ULL; }
unsigned long long SystemInfo::getFreeRAMKB()  { return getFreeRAM()  / 1024ULL; }

} // namespace sysmon
