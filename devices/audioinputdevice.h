#ifndef AUDIOINPUTDEVICE_H
#define AUDIOINPUTDEVICE_H

#include <QIODevice>
#include <QAudioInput>
#include <QThread>
#include <QTime>
#include "common_types.h"

class AudioInputDevice : public QIODevice
{
    Q_OBJECT
public:
    explicit AudioInputDevice (QObject *parent = 0);
    ~AudioInputDevice();

public slots:
    void setChannels(AudioChannels channels);

protected:
    qint64 readData (char * /*data*/, qint64 /*maxSize*/);
    qint64 writeData (const char * /*data*/, qint64 /*maxSize*/);

private:
    QTime timer;
    qreal samplesReaded;    // use for determinate time of current frame from start
    AudioChannels m_channels;

signals:
    void samplesReceived(AudioChannels channel, SamplesList data);
    void samplesReceived(SamplesList leftChannelData, SamplesList rightChannelData);

private slots:
};

typedef enum {
    TRIG_AUTO   = 0,
    TRIG_NORMAL = 1,
    TRIG_SINGLE = 2
} OscTriggerMode;
typedef enum {
    TRIG_RISING   = 1,
    TRIG_FALLING  = 2
} OscTriggerSlope;

class AudioInputThread : public QThread
{
    Q_OBJECT

public:
    enum ThreadPurpose
    {
        COMPRESSOR,
        VOLUME_INDICATOR,
        TUNER,
        OSCILLOSCOPE
    };

    AudioInputThread ();
    void run ();
    void startCapturing (bool start);
    void changeFrameSize (ThreadPurpose purpose, int size);
    QStringList enumerateDevices();
    void setCapturedChannels(AudioChannels channels);
    QString getDeviceName();

public slots:
    void updateBuffers(AudioChannels channel, SamplesList samples);
    void switchInputDevice(QString name);

signals:
    void initiated (int);
    // This is signal emit same data but with different rate
    void dataForCompressor (SamplesList);
    void dataForVolumeIndicator (SamplesList);
    void dataForTuner (SamplesList);
    void dataForOscilloscope(AudioChannels channel, SamplesList data);
    void dataForOscilloscope(SamplesList leftChannelData, SamplesList rightChannelData);

private slots:
    void stateChanged(QAudio::State newState);

private:
    int compressorFrameSize;
    int volumeIndicatorFrameSize;
    int tunerFrameSize;
    int oscilloscopeFrameSize;
    // FIFO buffers with same data but with different size
    SamplesList bufferCompressor;
    SamplesList bufferIndicator;
    SamplesList bufferTuner;
    SamplesList bufferOscilloscope;

    qint32 m_sampleRate;
    QAudioInput* m_audioInput;
    AudioInputDevice* m_inputBuffer;
    QAudioFormat m_audioFormat;
    bool m_captureEnabled;
    QAudioDeviceInfo m_curAudioDeviceInfo;
    QList<QAudioDeviceInfo> m_audioDeviceInfos;
    AudioChannels m_capturedChannels;
};

#endif // AUDIOINPUTDEVICE_H
