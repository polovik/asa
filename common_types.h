#ifndef COMMON_TYPES
#define COMMON_TYPES

#include <QPoint>
#include <QImage>

typedef QList<qreal> SamplesList;

class ToneWaveForm
{
public:
    typedef enum {
        WAVE_UNKNOWN    = 0,
        WAVE_SINE       = 1,
        WAVE_SQUARE     = 2,
        WAVE_SAWTOOTH   = 3,
        WAVE_TRIANGLE   = 4,
        WAVE_LAST       = 5   // mark of enum's end
    } Id;
    
    ToneWaveForm();
    ToneWaveForm(Id id);
    ToneWaveForm(QString name);
    
    Id id();
    void setId(Id id);
    
    QString getName() const;
    static QString getName(Id id);
    
private:
    Id m_id;
};

QDebug operator<<(QDebug d, const ToneWaveForm &tone);

typedef enum {
    CHANNEL_NONE   = 0,
    CHANNEL_LEFT   = 1,
    CHANNEL_RIGHT  = 2,
    CHANNEL_BOTH   = 3
} AudioChannels;

typedef struct {
    int id;
    QPoint pos;
    ToneWaveForm signalType;
    int signalFrequency;
    double signalVoltage;
    bool isCurrent;
    QImage signature;
    QList<QPointF> data;
} TestpointMeasure;

#endif // COMMON_TYPES

