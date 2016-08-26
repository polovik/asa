#include <QDebug>
#include <QGraphicsRectItem>
#include "volumeindicator.h"
#include "smoothfilter.h"

VolumeIndicator::VolumeIndicator(QWidget *parent) :
    QGraphicsView(parent), smoothFilter(0), volumeItem(0), global_counter(-1),
    maxVolume(0.0)
{
    // Do not show scroll bars for correct calculate and display of volume level
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    smoothFilter = new SmoothFilter(1000, 1. / 1000);
    
    QGraphicsScene *scene = new QGraphicsScene(0, 0, 1000, 1000);
    setScene(scene);
    volumeItem = new QGraphicsRectItem(scene->sceneRect());
    scene->addItem(volumeItem);
}

VolumeIndicator::~VolumeIndicator()
{
    if (smoothFilter)
        delete smoothFilter;
}

void VolumeIndicator::setSamplingRate(int samplingRate)
{
    updateVolumeInterval = samplingRate / updateTemp;
    //double smoothFactor = 1. / updateVolumeInterval;
}

/*
 Periodic signal have even positive and negative half-periods.
 For calc mean value of volume need smooth only absolute values
 */
void VolumeIndicator::processSamples(SamplesList samples)
{
    if (!smoothFilter)
        return;
    int length = samples.size();
    //double minVolume = 100;
    for (int i = 0; i < length; i++) {
        double smoothedLevel = smoothFilter->processSample(qAbs(samples.at(i)));
        maxVolume = qMax(maxVolume, qAbs(samples.at(i)));
        global_counter++;
        if (global_counter % updateVolumeInterval == 0) {
            drawVolumeLevel(smoothedLevel); //maxVolume
            maxVolume = 0.0;
        }
        //minVolume = qMin (minVolume, samples.at (i));
    }
    //qDebug() << "VolumeIndicator::processSamples" << minVolume << maxVolume;
}

/*
 Draw volume level, where level bounded in [0 ... 1]
 */
void VolumeIndicator::drawVolumeLevel(double level)
{
    if (!volumeItem)
        return;
    double heightVolume = viewport()->geometry().height();
    double widthVolume = viewport()->geometry().width() - 1;
    QPointF downRigth(mapToScene(widthVolume, heightVolume - 1));
    heightVolume *= level;
    heightVolume = viewport()->geometry().height() - heightVolume;  // draw from down to up jumps
    QPointF upLeft(mapToScene(0, heightVolume));
    QRectF rectVolume(upLeft, downRigth);
    volumeItem->setRect(rectVolume);
}
