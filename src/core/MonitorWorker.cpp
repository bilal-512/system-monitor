#include "MonitorWorker.h"
#include <QCoreApplication>
#include <QDir>
#include <chrono>
#include <sstream>
#include <iomanip>
#include "SystemInfo.h"

static sysmon::DataPoint makeDataPoint(double cpu, double mem, double disk, int procs) {
    using namespace std::chrono;
    auto t  = system_clock::to_time_t(system_clock::now());
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    sysmon::DataPoint pt;
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

MonitorWorker::MonitorWorker(QObject* parent) 
    : QObject(parent), m_cpu(1000, 60), m_logger("logs/sysmon.log")
{
    connect(&m_timer, &QTimer::timeout, this, &MonitorWorker::updateData);
    
    QDir().mkpath("config");
    QDir().mkpath("logs");
    QDir().mkpath("reports");
    
    // Setup config
    m_config.loadFromFile("config/sysmon.ini");
    m_logger.setLogFile(m_config.get("LogFile", "logs/sysmon.log"));
    m_reportGen.setReportDir(m_config.get("ReportDir", "reports"));
    
    // Setup alerts
    m_alerts.setCpuThreshold(m_config.getDouble("CpuAlertThreshold", 85.0));
    m_alerts.setMemoryThreshold(m_config.getDouble("MemAlertThreshold", 85.0));
    m_alerts.setDiskThreshold(m_config.getDouble("DiskAlertThreshold", 90.0));
    m_alerts.setCooldownSeconds(m_config.getInt("AlertCooldown", 60));
    
    m_alerts.setCallback([this](const sysmon::Alert& a) {
        emit alertTriggered(QString::fromStdString(a.message), true);
    });
}

MonitorWorker::~MonitorWorker() {
    stopMonitoring();
}

void MonitorWorker::startMonitoring(int intervalMs) {
    m_cpu.start(); // starts the internal cpu worker thread
    m_timer.start(intervalMs);
    updateData(); // immediate fetch
}

void MonitorWorker::stopMonitoring() {
    m_timer.stop();
    m_cpu.stop();
}

void MonitorWorker::killProcess(uint32_t pid) {
    m_proc.terminate(pid);
    updateData();
}

void MonitorWorker::generateReport(int reportType) {
    sysmon::SystemSnapshot snap;
    snap.osName = sysmon::SystemInfo::getOS();
    snap.hostName = sysmon::SystemInfo::getHostName();
    snap.cpuModel = sysmon::SystemInfo::getCpuModel();
    snap.coreCount = sysmon::SystemInfo::getCoreCount();
    snap.totalRamBytes = sysmon::SystemInfo::getTotalRAM();
    
    bool ok = false;
    switch(reportType) {
        case 0: ok = m_reportGen.generateDailyReport(m_stats); break;
        case 1: ok = m_reportGen.generateHealthReport(snap, m_stats); break;
        case 2: ok = m_reportGen.generateCpuSummary(m_stats); break;
        case 3: ok = m_reportGen.generateMemorySummary(m_stats); break;
        case 4: ok = m_reportGen.generateResourceReport(snap, m_stats); break;
    }
    emit reportGenerated(ok, QString::fromStdString(m_reportGen.getReportDir()));
}

void MonitorWorker::updateData() {
    sysmon::MemoryStats memStats = m_mem.refresh();
    std::vector<sysmon::DriveInfo> drives = m_disk.refresh();
    double diskOverall = m_disk.getOverallUsedPercent();
    
    m_proc.refresh();
    const std::vector<sysmon::ProcessInfo>& procs = m_proc.getProcesses();
    
    double cpuUsage = m_cpu.getCurrentUsage();
    
    sysmon::DataPoint pt = makeDataPoint(cpuUsage, memStats.usedPercent, diskOverall, procs.size());
    m_stats.addPoint(pt);
    
    m_alerts.check(cpuUsage, memStats.usedPercent, diskOverall);
    
    QString logMsg = QString("CPU: %1% | Mem: %2% | Disk: %3% | Procs: %4")
                     .arg(cpuUsage, 0, 'f', 1).arg(memStats.usedPercent, 0, 'f', 1)
                     .arg(diskOverall, 0, 'f', 1).arg(procs.size());
    m_logger.info(logMsg.toStdString());
    
    emit cpuUpdated(cpuUsage);
    emit memoryUpdated(memStats);
    emit diskUpdated(drives, diskOverall);
    emit processesUpdated(procs);
    emit performanceStatsUpdated(m_stats);
}
