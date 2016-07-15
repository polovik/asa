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
    void configure(qint32 samplingRate, qint32 frequency);

public slots:

protected:
    qint64 readData (char *data, qint64 maxSize);
    qint64 writeData (const char *data, qint64 maxSize);

private slots:

private:
    qint64 m_sampleIndex;
    qint32 m_samplingRate;
    qint32 m_frequency;

signals:
};

class ToneGenerator : public QThread
{
    Q_OBJECT
public:
    explicit ToneGenerator(QObject *parent = 0);
    ~ToneGenerator();

public slots:
    void runGenerator(bool start);

signals:

protected:
    void run() Q_DECL_OVERRIDE;

private slots:
    void stateChanged(QAudio::State state);

private:
    QAudioOutput *m_audioOutput;
    AudioOutputDevice *m_outputBuffer;
    qint32 m_sampleRate;
    qint32 m_toneFrequency;
    bool m_generationEnabled;
};

#endif // TONEGENERATOR_H
