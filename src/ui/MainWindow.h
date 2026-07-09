#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <QTableWidget>
#include <QProgressBar>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>

#include "ChartWidget.h"
#include "../core/MonitorWorker.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onCpuUpdated(double usage);
    void onMemoryUpdated(const sysmon::MemoryStats& stats);
    void onDiskUpdated(const std::vector<sysmon::DriveInfo>& drives, double totalPercent);
    void onProcessesUpdated(const std::vector<sysmon::ProcessInfo>& procs);
    void onKillProcessClicked();
    void onViewDetailsClicked();
    void onSortByCpuClicked();
    void onSortByMemoryClicked();
    void onAlertTriggered(const QString& message, bool isCritical);
    void onPerformanceStatsUpdated(const sysmon::PerformanceStats& stats);
    void onReportGenerated(bool success, const QString& path);
    void onApplySettings();

private:
    void setupUi();
    void setupDashboardTab();
    void setupProcessTab();
    void setupSystemInfoTab();
    void setupReportsTab();
    void setupSettingsTab();

    QTabWidget* m_tabWidget;
    
    // Dashboard widgets
    ChartWidget* m_cpuChart;
    QLabel* m_cpuDetailsLabel;
    
    ChartWidget* m_memChart;
    QLabel* m_memDetailsLabel;
    
    QVBoxLayout* m_diskListLayout;
    
    // Process widgets
    QTableWidget* m_processTable;
    QLineEdit* m_processSearch;
    std::vector<sysmon::ProcessInfo> m_lastProcs;
    
    // System info widgets
    QLabel* m_sysOsLabel;
    QLabel* m_sysCpuLabel;
    QLabel* m_sysMemLabel;
    QLabel* m_sysAvailRamLabel;
    QLabel* m_sysUptimeLabel;
    QLabel* m_sysDiskCapLabel;
    
    // Stats
    QLabel* m_statsLabel;
    
    // Settings widgets
    QSpinBox* m_intervalSpinBox;
    QDoubleSpinBox* m_cpuAlertSpin;
    QDoubleSpinBox* m_memAlertSpin;
    QDoubleSpinBox* m_diskAlertSpin;
    
    // Worker thread connection
    MonitorWorker* m_worker;
};
