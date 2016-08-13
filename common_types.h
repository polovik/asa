#ifndef COMMON_TYPES
#define COMMON_TYPES

#include <QPoint>
#include <QImage>

typedef enum {
    WAVE_UNKNOWN    = 0,
    WAVE_SINE       = 1,
    WAVE_SQUARE     = 2,
    WAVE_SAWTOOTH   = 3,
    WAVE_TRIANGLE   = 4
} ToneWaveForm;

typedef struct {
    int id;
    QPoint pos;
    ToneWaveForm signalType;
    int signalFrequency;
    int signalVoltage;
    bool isCurrent;
    QImage signature;
    QList<QPointF> data;
} TestpointMeasure;

#endif // COMMON_TYPES

