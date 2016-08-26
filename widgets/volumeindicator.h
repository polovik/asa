#ifndef VOLUMEINDICATOR_H
#define VOLUMEINDICATOR_H

#include <QGraphicsView>
#include "common_types.h"

class SmoothFilter;

class VolumeIndicator : public QGraphicsView
{
    Q_OBJECT
public:
    VolumeIndicator (QWidget *parent = 0);
    ~VolumeIndicator ();
    void setSamplingRate(int samplingRate);
    static const int updateTemp = 24; // how much refresh indicator per second

private:
    SmoothFilter* smoothFilter;
    QGraphicsRectItem* volumeItem;
    QGraphicsLineItem* peakItem;
    int updateVolumeInterval; // in samples
    int updatePeakInterval; // in samples
    int global_counter;
    double maxVolume;

    void drawVolumeLevel (double level);

signals:

public slots:
    void processSamples (SamplesList samples);
};

#endif // VOLUMEINDICATOR_H
