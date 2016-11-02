#ifndef AUDIOINPUTDEVICE_H
#define AUDIOINPUTDEVICE_H

#include <QIODevice>
#include <QAudioInput>
#include <QThread>
#include <QTime>
#include <QMap>
#include "common_types.h"

class AudioInputDevice : public QIODevice
{
    Q_OBJECT
public:
    explicit AudioInputDevice(QObject *parent = 0);
    ~AudioInputDevice();
    
public slots:
    void setChannels(AudioChannels channels);
    void setVoltageScaleFactor(qreal maxVoltage);
    void setOffsets(qreal leftChannelOffset, qreal rightChannelOffset);
    
protected:
    qint64 readData(char * /*data*/, qint64 /*maxSize*/);
    qint64 writeData(const char * /*data*/, qint64 /*maxSize*/);
    
private:
    QTime timer;
    qreal samplesReaded;    // use for determinate time of current frame from start
    AudioChannels m_channels;
    // audio samples have to be scaled for reflect signal's amplitude which applied to testpoint on PCB
    qreal m_scaleFactor;
    // some sound cards have channels with incorrect offsets in data:
    // if signal isn't applied to input port, data displays some voltage on the port.
    qreal m_leftChannelOffset;
    qreal m_rightChannelOffset;
    
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
    AudioInputThread();
    void run();
    void startCapturing(bool start);
    QList<QPair<QString, QString> > enumerateDevices();
    void setCapturedChannels(AudioChannels channels);
    QString getDeviceName();
    QStringList getPortsList();
    void setMaxInputVoltage(qreal maxInputVoltage);
    void setAmplifyFactor(qreal amplifyFactor);
    qreal getAmplifyFactor();
    void setChannelOffset(AudioChannels channel, qreal offset);
    qreal getChannelOffset(AudioChannels channel);
    void getVolume(int &baseVolume, int &curVolume, int &maxVolume);
    void setVolume(int volume);
    
public slots:
    void switchInputDevice(QString name);
    void switchPort(QString alsaPort);
    
signals:
    void prepared();
    void initiated(int);
    void dataForOscilloscope(AudioChannels channel, SamplesList data);
    void dataForOscilloscope(SamplesList leftChannelData, SamplesList rightChannelData);
    
private slots:
    void stateChanged(QAudio::State newState);
    
private:
    qint32 m_sampleRate;
    QAudioInput *m_audioInput;
    AudioInputDevice *m_inputBuffer;
    QAudioFormat m_audioFormat;
    bool m_captureEnabled;
    QAudioDeviceInfo m_curAudioDeviceInfo;
    QList<QAudioDeviceInfo> m_audioDeviceInfos;
    AudioChannels m_capturedChannels;
    qreal m_maxInputVoltage;
    qreal m_amplifyFactor;
    qreal m_leftChannelOffset;
    qreal m_rightChannelOffset;
    QMap<QString, QString> m_portsMap;
    int m_systemVolume;
};

#endif // AUDIOINPUTDEVICE_H
