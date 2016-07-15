#ifndef AUDIOINPUTDEVICE_H
#define AUDIOINPUTDEVICE_H

#include <QIODevice>
#include <QAudioInput>
#include <QThread>
#include <QTime>

typedef QList<qreal> SamplesList;

class AudioInputDevice : public QIODevice
{
    Q_OBJECT
public:
    explicit AudioInputDevice (QObject *parent = 0);
    ~AudioInputDevice();
    int samplingRate;

protected:
    qint64 readData (char * /*data*/, qint64 /*maxSize*/);
    qint64 writeData (const char * /*data*/, qint64 /*maxSize*/);

private:
    QTime timer;
    QAudioInput* audio;
    qreal samplesReaded;    // use for determinate time of current frame from start

    void setFormat();

signals:
    void audioValume (qreal left, qreal right);
    void samplesReceived (SamplesList);

public slots:
    void stopRecording ();
    void startReading (bool start);

private slots:
    void stateChanged(QAudio::State newState);
};

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

    AudioInputDevice* audioDevice;
    QAudioFormat m_audioFormat;
    bool m_captureEnabled;
    QAudioDeviceInfo m_curAudioDeviceInfo;
    QList<QAudioDeviceInfo> m_audioDeviceInfos;

public slots:
    void updateBuffers (SamplesList samples);
    void switchInputDevice(QString name);

signals:
    void initiated (int);
    // This is signal emit same data but with different rate
    void dataForCompressor (SamplesList);
    void dataForVolumeIndicator (SamplesList);
    void dataForTuner (SamplesList);
    void dataForOscilloscope (SamplesList);
};

#endif // AUDIOINPUTDEVICE_H
