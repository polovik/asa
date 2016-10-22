#ifndef VOLUMEINDICATOR_H
#define VOLUMEINDICATOR_H

#include <QGraphicsView>
#include "common_types.h"

class SmoothFilter;

class VolumeIndicator : public QGraphicsView
{
    Q_OBJECT
public:
    VolumeIndicator(QWidget *parent = 0);
    ~VolumeIndicator();
    void setSamplingRate(int samplingRate);
    void setMaximumAmplitude(qreal voltage);
    static const int updateTemp = 24; // how much refresh indicator per second

protected:
    void resizeEvent(QResizeEvent *event);

private:
    SmoothFilter *smoothFilter;
    QGraphicsRectItem *volumeItem;
    QGraphicsLineItem *peakItem;
    int updateVolumeInterval; // in samples
    int updatePeakInterval; // in samples
    int global_counter;
    double maxVolume;
    qreal m_maxInputVoltage;
    double m_curVolumeLevel;
    
    void drawVolumeLevel(double level);
    
signals:

public slots:
    void processSamples(SamplesList samples);
};

#endif // VOLUMEINDICATOR_H
