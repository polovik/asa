#ifndef OSCILLOSCOPEENGINE_H
#define OSCILLOSCOPEENGINE_H

#include <QObject>
#include "../common_types.h"
#include "../devices/audioinputdevice.h"

class OscilloscopeView;

class OscilloscopeEngine : public QObject
{
    Q_OBJECT
public:
    explicit OscilloscopeEngine(OscilloscopeView *view, QObject *parent = 0);
    ~OscilloscopeEngine();
    void prepareToStart(int samplingRate);
    void stop();
    void setDisplayedChannels(AudioChannels channels);
    void setTriggerChannel(AudioChannels channel);
    void setTriggerMode(OscTriggerMode mode);
    void setTriggerSlope(OscTriggerSlope slope);

public slots:
    void processOscilloscopeData(SamplesList leftChannelData, SamplesList rightChannelData);

signals:

private slots:
    void updateTriggerLevel(double voltage);

private:
    void draw(AudioChannels channel, const QVector<double> &values);
    void displayOscilloscopeChannelData(int dislayFrom, int displayedLength, int removeFrom);

    static const int OSCILLOSCOPE_PLOT_FREQUENCY_HZ = 5;
    OscilloscopeView *m_view;
    double m_triggerLevel;
    int m_samplingRate;
    int m_frameLength;
    AudioChannels m_capturedChannels;
    AudioChannels m_triggerChannel;
    bool m_dataForSingleCaptureAcqured;
    OscTriggerMode m_triggerMode;
    OscTriggerSlope m_triggerSlope;
    SamplesList m_samplesInputBufferLeft;
    SamplesList m_samplesInputBufferRight;
    bool m_isRunning;
};

#endif // OSCILLOSCOPEENGINE_H
