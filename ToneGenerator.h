#ifndef TONEGENERATOR_H
#define TONEGENERATOR_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QAudioOutput>

class AudioOutputDevice : public QIODevice
{
    Q_OBJECT
public:
    explicit AudioOutputDevice (QObject *parent = 0);
    ~AudioOutputDevice();
    void configure(const QAudioFormat &format, qint32 frequency);

public slots:

protected:
    qint64 readData (char *data, qint64 maxSize);
    qint64 writeData (const char *data, qint64 maxSize);

private slots:

private:
    qint64 m_sampleIndex;
    QAudioFormat m_audioFormat;
    qint32 m_frequency;

signals:
};

class ToneGenerator : public QThread
{
    Q_OBJECT
public:
    explicit ToneGenerator(QObject *parent = 0);
    ~ToneGenerator();
    QStringList enumerateDevices();

public slots:
    void runGenerator(bool start);
    void changeFrequency(int freq);
    void switchOutputDevice(QString name);

signals:
    void deviceReady(bool ready);

protected:
    void run() Q_DECL_OVERRIDE;

private slots:
    void stateChanged(QAudio::State state);

private:
    QAudioFormat m_audioFormat;
    QAudioOutput *m_audioOutput;
    AudioOutputDevice *m_outputBuffer;
    qint32 m_sampleRate;
    qint32 m_toneFrequency;
    bool m_generationEnabled;
    QAudioDeviceInfo m_curAudioDeviceInfo;
    QList<QAudioDeviceInfo> m_audioDeviceInfos;
};

#endif // TONEGENERATOR_H
