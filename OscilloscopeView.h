#ifndef OSCILLOSCOPEVIEW_H
#define OSCILLOSCOPEVIEW_H

#include <QObject>
#include <qcustomplot/qcustomplot.h>

class OscilloscopeView : public QCustomPlot
{
    Q_OBJECT
public:
    OscilloscopeView(QWidget *parent = 0);
    ~OscilloscopeView();
    void draw(const QVector<double> &keys, const QVector<double> &values);
    void saveView();
    void setXaxisRange(double minValue, double maxValue);
    void setYaxisRange(double minValue, double maxValue);
    void setTriggerLevel(double voltage);
    void showTriggerLine(bool visible);

signals:
    void triggerLevelChanged(double voltage);

protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

private slots:
    void showPointToolTip(QMouseEvent* event);

private:
    QCPItemTracer *m_pointUnderMouse;
    QCPItemText *m_pointUnderMouseText;

    QCPItemStraightLine *m_timeMeasurementLine1;
    QCPItemStraightLine *m_timeMeasurementLine2;
    QCPItemBracket *m_timeMeasurementBracket;
    QCPItemText *m_timeMeasurementText;

    QCPItemStraightLine *m_triggerLevelLine;
    QCPItemText *m_triggerLevelText;

    QCPItemStraightLine *m_movedStraightLine;

    double m_axisXminValue;
    double m_axisXmaxValue;
    double m_axisYminValue;
    double m_axisYmaxValue;
};

#endif // OSCILLOSCOPEVIEW_H
