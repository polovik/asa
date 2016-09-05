#include <QDebug>
#include "smoothfilter.h"

SmoothFilter::SmoothFilter() : filter(0)
{
    qDebug() << "SmoothFilter::SmoothFilter Default constructor";
}

SmoothFilter::SmoothFilter(int length, double smoothFactor) :
    filter(0), length(length), smoothFactor(smoothFactor), globalPos(0), outputValue(0)
{
    filter = new double [length];
    //Q_ASSERT (filter);
    for (int i = 0; i < length; i++) {
        filter[i] = 0;
    }
}

SmoothFilter::~SmoothFilter()
{
    if (filter)
        delete[] filter;
}

double SmoothFilter::processSample(double sample)
{
    double inputedSample = sample * smoothFactor;
    int pos = globalPos % length;
    double oldestSample = filter[pos];
    outputValue = outputValue + inputedSample - oldestSample;
    filter[pos] = inputedSample;
    globalPos++;
    return outputValue;
}
