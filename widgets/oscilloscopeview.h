#ifndef OSCILLOSCOPEVIEW_H
#define OSCILLOSCOPEVIEW_H

#include <QObject>
#include <qcustomplot/qcustomplot.h>

typedef enum {
    GRAPH_CHANNEL_LEFT   = 30,
    GRAPH_CHANNEL_RIGHT  = 31
} DisplayedGraphId;

class OscilloscopeView : public QCustomPlot
{
    Q_OBJECT
public:
    OscilloscopeView(QWidget *parent = 0);
    ~OscilloscopeView();
    void draw(DisplayedGraphId id, const QVector<double> &keys, const QVector<double> &values);
    void saveView();
    void setXaxisRange(double minValue, double maxValue);
    void setYaxisRange(double minValue, double maxValue);
    void setTriggerLevel(double voltage);
    void showTriggerLine(bool visible);
    void showGraph(DisplayedGraphId id, bool visible);
    void showTimeMeasureGuides(bool visible);
    void showVoltageMeasureGuides(bool visible, bool freezed);
    void showSampleValueUnderMouse(bool visible);
    
signals:
    void triggerLevelChanged(double voltage);
    
protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseDoubleClickEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void wheelEvent(QWheelEvent *event);

private slots:
    void showPointToolTip(QMouseEvent *event);
    
private:
    QCPItemTracer *m_pointUnderMouse;
    QCPItemText *m_pointUnderMouseText;
    
    QCPItemStraightLine *m_timeMeasurementLine1;
    QCPItemStraightLine *m_timeMeasurementLine2;
    QCPItemBracket *m_timeMeasurementBracket;
    QCPItemText *m_timeMeasurementText;

    QCPItemStraightLine *m_voltageMeasurementLine1;
    QCPItemStraightLine *m_voltageMeasurementLine2;
    QCPItemText *m_voltageMeasurementText1;
    QCPItemText *m_voltageMeasurementText2;

    QCPItemStraightLine *m_triggerLevelLine;
    QCPItemText *m_triggerLevelText;
    
    QCPItemStraightLine *m_movedStraightLine;
    QCPGraph *m_graphChannelLeft;
    QCPGraph *m_graphChannelRight;
    
    double m_axisXminValue;
    double m_axisXmaxValue;
    double m_axisYminValue;
    double m_axisYmaxValue;
};

#endif // OSCILLOSCOPEVIEW_H
