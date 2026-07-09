#pragma once

#include <QWidget>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

class ChartWidget : public QWidget {
    Q_OBJECT
public:
    explicit ChartWidget(const QString& title, QColor color, QWidget* parent = nullptr);

public slots:
    void addDataPoint(double value);

private:
    QChart* m_chart;
    QLineSeries* m_series;
    QValueAxis* m_axisX;
    QValueAxis* m_axisY;
    int m_maxPoints;
    int m_pointCount;
};
