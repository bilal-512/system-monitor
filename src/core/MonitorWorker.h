#pragma once

#include <QObject>
#include <QTimer>
#include <vector>
#include <QString>

#include "CpuMonitor.h"
#include "MemoryMonitor.h"
#include "DiskMonitor.h"
#include "ProcessManager.h"
#include "Logger.h"
#include "Config.h"
#include "AlertSystem.h"
#include "PerformanceStats.h"
#include "ReportGenerator.h"

class MonitorWorker : public QObject {
    Q_OBJECT
public:
    explicit MonitorWorker(QObject* parent = nullptr);
    ~MonitorWorker();

    sysmon::Config& getConfig() { return m_config; }
    sysmon::AlertSystem& getAlertSystem() { return m_alerts; }
    sysmon::PerformanceStats& getStats() { return m_stats; }
    sysmon::ReportGenerator& getReportGen() { return m_reportGen; }

public slots:
    void startMonitoring(int intervalMs = 1000);
    void stopMonitoring();
    void refreshNow();
    void killProcess(uint32_t pid);
    void generateReport(int reportType); // 0=Daily, 1=Health, 2=CPU, 3=Mem, 4=Resource

signals:
    void cpuUpdated(double currentUsage);
    void memoryUpdated(const sysmon::MemoryStats& stats);
    void diskUpdated(const std::vector<sysmon::DriveInfo>& drives, double totalPercent);
    void processesUpdated(const std::vector<sysmon::ProcessInfo>& processes);
    
    void alertTriggered(const QString& message, bool isCritical);
    void performanceStatsUpdated(const sysmon::PerformanceStats& stats);
    void reportGenerated(bool success, const QString& path);

private slots:
    void updateData();

private:
    QTimer m_timer;
    sysmon::CpuMonitor m_cpu;
    sysmon::MemoryMonitor m_mem;
    sysmon::DiskMonitor m_disk;
    sysmon::ProcessManager m_proc;
    
    sysmon::Logger m_logger;
    sysmon::Config m_config;
    sysmon::AlertSystem m_alerts;
    sysmon::PerformanceStats m_stats;
    sysmon::ReportGenerator m_reportGen;
};
