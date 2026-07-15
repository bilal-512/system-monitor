#include "MainWindow.h"
#include <QHeaderView>
#include <QPushButton>
#include <QMessageBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QThread>
#include <QFile>
#include <QTextStream>
#include <QSplitter>
#include <QFrame>

#include "SystemInfo.h"

class NumericTableItem : public QTableWidgetItem {
public:
    NumericTableItem(const QString& text, double val) : QTableWidgetItem(text), numericVal(val) {}
    bool operator<(const QTableWidgetItem& other) const override {
        return numericVal < static_cast<const NumericTableItem&>(other).numericVal;
    }
    double numericVal;
};

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("System Resource Monitor");
    resize(1200, 800);
    setMinimumSize(1000, 700);

    setupUi();

    QFile styleFile(":/theme.qss");
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream in(&styleFile);
        setStyleSheet(in.readAll());
        styleFile.close();
    }

    m_worker = new MonitorWorker(this);

    connect(m_worker, &MonitorWorker::cpuUpdated, this, &MainWindow::onCpuUpdated);
    connect(m_worker, &MonitorWorker::memoryUpdated, this, &MainWindow::onMemoryUpdated);
    connect(m_worker, &MonitorWorker::diskUpdated, this, &MainWindow::onDiskUpdated);
    connect(m_worker, &MonitorWorker::processesUpdated, this, &MainWindow::onProcessesUpdated);

    connect(m_worker, &MonitorWorker::alertTriggered, this, &MainWindow::onAlertTriggered);
    connect(m_worker, &MonitorWorker::performanceStatsUpdated, this, &MainWindow::onPerformanceStatsUpdated);
    connect(m_worker, &MonitorWorker::reportGenerated, this, &MainWindow::onReportGenerated);

    int interval = m_worker->getConfig().getInt("UpdateIntervalMs", 2000);
    m_worker->startMonitoring(interval);
}

MainWindow::~MainWindow() {
    m_worker->stopMonitoring();
}

void MainWindow::setupUi() {
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    m_tabWidget = new QTabWidget(this);
    mainLayout->addWidget(m_tabWidget);

    setupDashboardTab();
    setupProcessTab();
    setupSystemInfoTab();
    setupReportsTab();
    setupSettingsTab();

    setCentralWidget(centralWidget);
}

void MainWindow::setupDashboardTab() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QFrame* summaryFrame = new QFrame(tab);
    summaryFrame->setObjectName("summaryCard");
    summaryFrame->setFrameShape(QFrame::StyledPanel);
    QVBoxLayout* summaryLayout = new QVBoxLayout(summaryFrame);

    QLabel* overviewTitle = new QLabel("Live System Overview");
    overviewTitle->setStyleSheet("font-size: 16px; font-weight: bold;");
    summaryLayout->addWidget(overviewTitle);

    m_dashboardSummaryLabel = new QLabel("Collecting live system metrics...");
    m_dashboardSummaryLabel->setWordWrap(true);
    summaryLayout->addWidget(m_dashboardSummaryLabel);
    layout->addWidget(summaryFrame);

    QHBoxLayout* chartsLayout = new QHBoxLayout();

    QVBoxLayout* cpuLayout = new QVBoxLayout();
    m_cpuChart = new ChartWidget("CPU Usage (%)", QColor("#e74c3c"), this);
    m_cpuDetailsLabel = new QLabel("Load: 0% | Active Cores: 0");
    m_cpuDetailsLabel->setAlignment(Qt::AlignCenter);
    cpuLayout->addWidget(m_cpuChart);
    cpuLayout->addWidget(m_cpuDetailsLabel);

    QVBoxLayout* memLayout = new QVBoxLayout();
    m_memChart = new ChartWidget("Memory Usage (%)", QColor("#3498db"), this);
    m_memDetailsLabel = new QLabel("Total: 0 GB | Used: 0 GB | Available: 0 GB");
    m_memDetailsLabel->setAlignment(Qt::AlignCenter);
    memLayout->addWidget(m_memChart);
    memLayout->addWidget(m_memDetailsLabel);

    chartsLayout->addLayout(cpuLayout);
    chartsLayout->addLayout(memLayout);

    layout->addLayout(chartsLayout, 2);

    QGroupBox* diskGroup = new QGroupBox("Storage Overview");
    m_diskListLayout = new QVBoxLayout(diskGroup);
    m_diskListLayout->addWidget(new QLabel("Loading disk info..."));

    layout->addWidget(diskGroup, 1);

    m_tabWidget->addTab(tab, "Dashboard");
}

void MainWindow::setupProcessTab() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QHBoxLayout* controlsLayout = new QHBoxLayout();
    m_processSearch = new QLineEdit();
    m_processSearch->setPlaceholderText("Search processes by name...");
    m_processSearch->setClearButtonEnabled(true);
    controlsLayout->addWidget(m_processSearch, 1);

    m_refreshProcessesButton = new QPushButton("Refresh Now");
    connect(m_refreshProcessesButton, &QPushButton::clicked, this, &MainWindow::onRefreshProcessesClicked);
    controlsLayout->addWidget(m_refreshProcessesButton);

    layout->addLayout(controlsLayout);

    m_processTable = new QTableWidget();
    m_processTable->setColumnCount(6);
    m_processTable->setHorizontalHeaderLabels({"PID", "Name", "Status", "CPU %", "Memory", "Threads"});
    m_processTable->horizontalHeader()->setStretchLastSection(false);
    m_processTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_processTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_processTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_processTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_processTable->setAlternatingRowColors(true);
    m_processTable->setSortingEnabled(true);
    m_processTable->sortByColumn(3, Qt::DescendingOrder);

    connect(m_processSearch, &QLineEdit::textChanged, this, [this](const QString&) {
        if (!m_lastProcs.empty()) {
            onProcessesUpdated(m_lastProcs);
        }
    });

    connect(m_processTable->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::onProcessSelectionChanged);

    QSplitter* splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(m_processTable);

    m_processDetailsView = new QPlainTextEdit();
    m_processDetailsView->setReadOnly(true);
    m_processDetailsView->setPlaceholderText("Select a process to view its details...");
    splitter->addWidget(m_processDetailsView);
    splitter->setSizes({3, 1});

    layout->addWidget(splitter, 1);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    QPushButton* btnSortCpu = new QPushButton("Sort by CPU");
    connect(btnSortCpu, &QPushButton::clicked, this, &MainWindow::onSortByCpuClicked);

    QPushButton* btnSortMem = new QPushButton("Sort by Memory");
    connect(btnSortMem, &QPushButton::clicked, this, &MainWindow::onSortByMemoryClicked);

    QPushButton* btnDetails = new QPushButton("View Details");
    connect(btnDetails, &QPushButton::clicked, this, &MainWindow::onViewDetailsClicked);

    QPushButton* btnKill = new QPushButton("End Task");
    connect(btnKill, &QPushButton::clicked, this, &MainWindow::onKillProcessClicked);

    btnLayout->addWidget(btnSortCpu);
    btnLayout->addWidget(btnSortMem);
    btnLayout->addWidget(btnDetails);
    btnLayout->addWidget(btnKill);
    layout->addLayout(btnLayout);

    m_tabWidget->addTab(tab, "Process Manager");
}

void MainWindow::setupSystemInfoTab() {
    QWidget* tab = new QWidget();
    QFormLayout* layout = new QFormLayout(tab);

    m_sysOsLabel = new QLabel(QString::fromStdString(sysmon::SystemInfo::getOS()));
    m_sysCpuLabel = new QLabel(QString::fromStdString(sysmon::SystemInfo::getCpuModel()));

    unsigned long long memTotal = sysmon::SystemInfo::getTotalRAM();
    QString memStr = QString::number(memTotal / (1024 * 1024)) + " MB";
    m_sysMemLabel = new QLabel(memStr);
    m_sysAvailRamLabel = new QLabel("Loading...");
    m_sysDiskCapLabel = new QLabel("Loading...");
    m_sysUptimeLabel = new QLabel(QString::fromStdString(sysmon::SystemInfo::getUptimeString()));

    layout->addRow("<b>Operating System:</b>", m_sysOsLabel);
    layout->addRow("<b>System Uptime:</b>", m_sysUptimeLabel);
    layout->addRow("<b>Processor:</b>", m_sysCpuLabel);
    layout->addRow("<b>Logical Cores:</b>", new QLabel(QString::number(sysmon::SystemInfo::getLogicalCoreCount())));
    layout->addRow("<b>Total RAM:</b>", m_sysMemLabel);
    layout->addRow("<b>Available RAM:</b>", m_sysAvailRamLabel);
    layout->addRow("<b>Disk Capacity:</b>", m_sysDiskCapLabel);
    layout->addRow("<b>Host Name:</b>", new QLabel(QString::fromStdString(sysmon::SystemInfo::getHostName())));

    m_tabWidget->addTab(tab, "System Info");
}

void MainWindow::refreshDashboardSummary() {
    m_dashboardSummaryLabel->setText(QString(
        "CPU: %1% | Memory: %2% | Disk: %3% | Processes: %4")
        .arg(m_currentCpu, 0, 'f', 1)
        .arg(m_currentMemPercent, 0, 'f', 1)
        .arg(m_currentDiskPercent, 0, 'f', 1)
        .arg(m_processCount));
}

void MainWindow::onCpuUpdated(double usage) {
    m_currentCpu = usage;
    m_cpuChart->addDataPoint(usage);
    m_cpuDetailsLabel->setText(QString("Load: %1% | Active Cores: %2")
                               .arg(usage, 0, 'f', 1)
                               .arg(sysmon::SystemInfo::getLogicalCoreCount()));

    m_sysUptimeLabel->setText(QString::fromStdString(sysmon::SystemInfo::getUptimeString()));
    refreshDashboardSummary();
}

void MainWindow::onMemoryUpdated(const sysmon::MemoryStats& stats) {
    m_currentMemPercent = stats.usedPercent;
    m_memChart->addDataPoint(stats.usedPercent);

    double totalGB = stats.totalPhys / (1024.0 * 1024.0 * 1024.0);
    double usedGB = stats.usedPhys / (1024.0 * 1024.0 * 1024.0);
    double availGB = stats.availPhys / (1024.0 * 1024.0 * 1024.0);
    double swapGB = stats.usedSwap / (1024.0 * 1024.0 * 1024.0);

    m_memDetailsLabel->setText(QString("Total: %1 GB | Used: %2 GB | Avail: %3 GB | Swap: %4 GB")
                               .arg(totalGB, 0, 'f', 1)
                               .arg(usedGB, 0, 'f', 1)
                               .arg(availGB, 0, 'f', 1)
                               .arg(swapGB, 0, 'f', 1));

    m_sysAvailRamLabel->setText(QString::number(stats.availPhys / (1024 * 1024)) + " MB");
    refreshDashboardSummary();
}

void MainWindow::onDiskUpdated(const std::vector<sysmon::DriveInfo>& drives, double totalPercent) {
    m_currentDiskPercent = totalPercent;

    QLayoutItem* item;
    while ((item = m_diskListLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    unsigned long long totalSpace = 0;

    for (const auto& d : drives) {
        if (!d.available || d.totalBytes == 0) continue;
        totalSpace += d.totalBytes;

        double dTotalGB = d.totalBytes / (1024.0 * 1024.0 * 1024.0);
        double dUsedGB = d.usedBytes / (1024.0 * 1024.0 * 1024.0);

        QHBoxLayout* h = new QHBoxLayout();
        h->addWidget(new QLabel(QString::fromStdString(d.drivePath)));

        QProgressBar* pb = new QProgressBar();
        pb->setRange(0, 100);
        pb->setValue(static_cast<int>(d.usedPercent));
        h->addWidget(pb, 1);

        h->addWidget(new QLabel(QString("%1 GB / %2 GB").arg(dUsedGB, 0, 'f', 1).arg(dTotalGB, 0, 'f', 1)));

        QWidget* w = new QWidget();
        w->setLayout(h);
        m_diskListLayout->addWidget(w);
    }

    m_sysDiskCapLabel->setText(QString::number(totalSpace / (1024 * 1024 * 1024)) + " GB");
    refreshDashboardSummary();
}

void MainWindow::onProcessesUpdated(const std::vector<sysmon::ProcessInfo>& procs) {
    m_lastProcs = procs;
    m_processCount = static_cast<int>(procs.size());
    refreshDashboardSummary();

    int selectedPid = -1;
    if (m_processTable->currentRow() >= 0) {
        selectedPid = m_processTable->item(m_processTable->currentRow(), 0)->text().toInt();
    }

    int sortCol = m_processTable->horizontalHeader()->sortIndicatorSection();
    Qt::SortOrder sortOrder = m_processTable->horizontalHeader()->sortIndicatorOrder();
    m_processTable->setSortingEnabled(false);

    QString filter = m_processSearch->text().toLower();

    int row = 0;
    m_processTable->setRowCount(static_cast<int>(procs.size()));

    int newSelectedRow = -1;

    for (const auto& p : procs) {
        if (!filter.isEmpty() && QString::fromStdString(p.name).toLower().indexOf(filter) == -1) {
            continue;
        }

        NumericTableItem* pidItem = new NumericTableItem(QString::number(p.pid), p.pid);
        QTableWidgetItem* nameItem = new QTableWidgetItem(QString::fromStdString(p.name));
        QTableWidgetItem* statusItem = new QTableWidgetItem(QString::fromStdString(p.status));

        QString memStr = QString::number(p.memBytes / (1024 * 1024)) + " MB";
        NumericTableItem* memItem = new NumericTableItem(memStr, p.memBytes);

        NumericTableItem* cpuItem = new NumericTableItem(QString::number(p.cpuPercent, 'f', 1) + " %", p.cpuPercent);
        NumericTableItem* threadItem = new NumericTableItem(QString::number(p.threadCount), p.threadCount);

        pidItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        memItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        cpuItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        threadItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        m_processTable->setItem(row, 0, pidItem);
        m_processTable->setItem(row, 1, nameItem);
        m_processTable->setItem(row, 2, statusItem);
        m_processTable->setItem(row, 3, cpuItem);
        m_processTable->setItem(row, 4, memItem);
        m_processTable->setItem(row, 5, threadItem);

        if (p.pid == static_cast<uint32_t>(selectedPid)) {
            newSelectedRow = row;
        }
        row++;
    }

    m_processTable->setRowCount(row);
    m_processTable->setSortingEnabled(true);

    if (sortCol >= 0) {
        m_processTable->sortByColumn(sortCol, sortOrder);
    } else {
        m_processTable->sortByColumn(3, Qt::DescendingOrder);
    }

    if (newSelectedRow >= 0) {
        m_processTable->selectRow(newSelectedRow);
    } else if (row > 0) {
        m_processTable->selectRow(0);
    }

    updateProcessDetailsFromSelection();
}

void MainWindow::updateProcessDetailsFromSelection() {
    m_processDetailsView->clear();
    int row = m_processTable->currentRow();
    if (row < 0) return;

    QTableWidgetItem* pidItem = m_processTable->item(row, 0);
    if (!pidItem) return;

    int pid = pidItem->text().toInt();
    for (const auto& p : m_lastProcs) {
        if (p.pid == static_cast<uint32_t>(pid)) {
            QString details = QString(
                "Process Name: %1\nPID: %2\nStatus: %3\nThreads: %4\nMemory: %5 MB\nCPU Usage: %6%")
                .arg(QString::fromStdString(p.name))
                .arg(p.pid)
                .arg(QString::fromStdString(p.status))
                .arg(p.threadCount)
                .arg(p.memBytes / (1024 * 1024))
                .arg(p.cpuPercent, 0, 'f', 1);

            m_processDetailsView->setPlainText(details);
            return;
        }
    }

    m_processDetailsView->setPlainText("No process selected.");
}

void MainWindow::onProcessSelectionChanged() {
    updateProcessDetailsFromSelection();
}

void MainWindow::onRefreshProcessesClicked() {
    if (m_worker) {
        m_worker->refreshNow();
    }
}

void MainWindow::onViewDetailsClicked() {
    int row = m_processTable->currentRow();
    if (row < 0) return;

    int pid = m_processTable->item(row, 0)->text().toInt();

    for (const auto& p : m_lastProcs) {
        if (p.pid == static_cast<uint32_t>(pid)) {
            QString details = QString("Process Name: %1\nPID: %2\nStatus: %3\nThreads: %4\nMemory: %5 MB\nCPU Usage: %6%")
                .arg(QString::fromStdString(p.name))
                .arg(p.pid)
                .arg(QString::fromStdString(p.status))
                .arg(p.threadCount)
                .arg(p.memBytes / (1024 * 1024))
                .arg(p.cpuPercent, 0, 'f', 1);

            QMessageBox::information(this, "Process Details", details);
            break;
        }
    }
}

void MainWindow::onKillProcessClicked() {
    int row = m_processTable->currentRow();
    if (row < 0) return;

    int pid = m_processTable->item(row, 0)->text().toInt();
    QString name = m_processTable->item(row, 1)->text();

    QMessageBox::StandardButton reply = QMessageBox::question(this, "End Process",
        "Are you sure you want to terminate " + name + " (PID " + QString::number(pid) + ")?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        m_worker->killProcess(pid);
    }
}

void MainWindow::onSortByCpuClicked() {
    m_processTable->sortByColumn(3, Qt::DescendingOrder);
}

void MainWindow::onSortByMemoryClicked() {
    m_processTable->sortByColumn(4, Qt::DescendingOrder);
}

void MainWindow::setupReportsTab() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    m_statsLabel = new QLabel("Loading performance statistics...");
    m_statsLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    layout->addWidget(m_statsLabel, 1);

    QGroupBox* gb = new QGroupBox("Generate Reports");
    QHBoxLayout* h = new QHBoxLayout(gb);

    QStringList reports = {"Daily Report", "Health Report", "CPU Summary", "Memory Summary", "Resource Report"};
    for (int i = 0; i < reports.size(); ++i) {
        QPushButton* btn = new QPushButton(reports[i]);
        connect(btn, &QPushButton::clicked, this, [this, i]() {
            m_worker->generateReport(i);
        });
        h->addWidget(btn);
    }

    layout->addWidget(gb);
    m_tabWidget->addTab(tab, "Reports & Stats");
}

void MainWindow::setupSettingsTab() {
    QWidget* tab = new QWidget();
    QFormLayout* layout = new QFormLayout(tab);

    m_intervalSpinBox = new QSpinBox();
    m_intervalSpinBox->setRange(100, 60000);
    m_intervalSpinBox->setSuffix(" ms");

    m_cpuAlertSpin = new QDoubleSpinBox();
    m_cpuAlertSpin->setRange(0, 100);
    m_cpuAlertSpin->setSuffix(" %");

    m_memAlertSpin = new QDoubleSpinBox();
    m_memAlertSpin->setRange(0, 100);
    m_memAlertSpin->setSuffix(" %");

    m_diskAlertSpin = new QDoubleSpinBox();
    m_diskAlertSpin->setRange(0, 100);
    m_diskAlertSpin->setSuffix(" %");

    layout->addRow("Update Interval:", m_intervalSpinBox);
    layout->addRow("CPU Alert Threshold:", m_cpuAlertSpin);
    layout->addRow("Memory Alert Threshold:", m_memAlertSpin);
    layout->addRow("Disk Alert Threshold:", m_diskAlertSpin);

    QPushButton* applyBtn = new QPushButton("Apply Settings");
    connect(applyBtn, &QPushButton::clicked, this, &MainWindow::onApplySettings);
    layout->addRow("", applyBtn);

    m_tabWidget->addTab(tab, "Settings");
}

void MainWindow::onAlertTriggered(const QString& message, bool isCritical) {
    QMessageBox::warning(this, "System Alert", message);
}

void MainWindow::onPerformanceStatsUpdated(const sysmon::PerformanceStats& stats) {
    sysmon::StatSummary cpu = stats.computeCpuStats();
    sysmon::StatSummary mem = stats.computeMemStats();
    sysmon::StatSummary disk = stats.computeDiskStats();

    QString text = QString("<b>CPU Stats</b><br>Avg: %1% | Min: %2% | Max: %3%<br><br>"
                           "<b>Memory Stats</b><br>Avg: %4% | Min: %5% | Max: %6%<br><br>"
                           "<b>Disk Stats</b><br>Avg: %7% | Min: %8% | Max: %9%<br>")
                   .arg(cpu.avgVal, 0, 'f', 1).arg(cpu.minVal, 0, 'f', 1).arg(cpu.maxVal, 0, 'f', 1)
                   .arg(mem.avgVal, 0, 'f', 1).arg(mem.minVal, 0, 'f', 1).arg(mem.maxVal, 0, 'f', 1)
                   .arg(disk.avgVal, 0, 'f', 1).arg(disk.minVal, 0, 'f', 1).arg(disk.maxVal, 0, 'f', 1);

    m_statsLabel->setText(text);

    if (m_intervalSpinBox->value() == 100) {
        m_intervalSpinBox->setValue(m_worker->getConfig().getInt("UpdateIntervalMs", 2000));
        m_cpuAlertSpin->setValue(m_worker->getAlertSystem().getCpuThreshold());
        m_memAlertSpin->setValue(m_worker->getAlertSystem().getMemoryThreshold());
        m_diskAlertSpin->setValue(m_worker->getAlertSystem().getDiskThreshold());
    }
}

void MainWindow::onReportGenerated(bool success, const QString& path) {
    if (success) {
        QMessageBox::information(this, "Report Generated", "Successfully exported report to: " + path);
    } else {
        QMessageBox::critical(this, "Report Error", "Failed to generate report.");
    }
}

void MainWindow::onApplySettings() {
    m_worker->stopMonitoring();

    int ms = m_intervalSpinBox->value();
    m_worker->getConfig().setInt("UpdateIntervalMs", ms);
    m_worker->getConfig().setDouble("CpuAlertThreshold", m_cpuAlertSpin->value());
    m_worker->getConfig().setDouble("MemAlertThreshold", m_memAlertSpin->value());
    m_worker->getConfig().setDouble("DiskAlertThreshold", m_diskAlertSpin->value());
    m_worker->getConfig().saveToFile("config/sysmon.ini");

