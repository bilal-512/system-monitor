#include "ChartWidget.h"
#include <QVBoxLayout>
#include <QPen>

ChartWidget::ChartWidget(const QString& title, QColor color, QWidget* parent)
    : QWidget(parent), m_maxPoints(60), m_pointCount(0)
{
    m_series = new QLineSeries();
    QPen pen(color);
    pen.setWidth(3);
    m_series->setPen(pen);

    m_chart = new QChart();
    m_chart->addSeries(m_series);
    m_chart->setTitle(title);
    m_chart->legend()->hide();
    m_chart->setAnimationOptions(QChart::NoAnimation);

    // Make chart look modern
    m_chart->setBackgroundBrush(QColor("#2b2b2b"));
    m_chart->setTitleBrush(QBrush(Qt::white));
    
    QFont titleFont = m_chart->titleFont();
    titleFont.setBold(true);
    titleFont.setPointSize(12);
    m_chart->setTitleFont(titleFont);

    m_axisX = new QValueAxis();
    m_axisX->setRange(0, m_maxPoints);
    m_axisX->setLabelFormat("%d");
    m_axisX->setReverse(true); // 0 on the right, 60 on the left
    m_axisX->setLabelsColor(QColor("#aaaaaa"));
    m_axisX->setGridLineColor(QColor("#444444"));
    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_series->attachAxis(m_axisX);

    m_axisY = new QValueAxis();
    m_axisY->setRange(0, 100);
    m_axisY->setLabelFormat("%d");
    m_axisY->setLabelsColor(QColor("#aaaaaa"));
    m_axisY->setGridLineColor(QColor("#444444"));
    m_chart->addAxis(m_axisY, Qt::AlignLeft);
    m_series->attachAxis(m_axisY);

    QChartView* chartView = new QChartView(m_chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setBackgroundBrush(QColor("#2b2b2b"));

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(chartView);
}

void ChartWidget::addDataPoint(double value) {
    // Shift data left (since x=0 is right, x=60 is left)
    QList<QPointF> points = m_series->points();
    m_series->clear();

    for (int i = 0; i < points.size(); ++i) {
        m_series->append(points.at(i).x() + 1, points.at(i).y());
    }
    m_series->append(0, value);

    if (m_series->count() > m_maxPoints) {
        m_series->remove(0);
    }
}
