#ifndef AUDIOCAPTURE_H
#define AUDIOCAPTURE_H

#include <QObject>
#include <QThread>
#include <QAudioInput>

class QBuffer;

class AudioCapture : public QThread
{
    Q_OBJECT
public:
    AudioCapture(QObject *parent = 0);
    ~AudioCapture();

public slots:
    void stopCapture();

signals:

protected:
    void run() Q_DECL_OVERRIDE;

private slots:
    void stateChanged(QAudio::State newState);

private:
    QAudioInput *m_audioInput;
    QBuffer *m_inputBuffer;
};

#endif // AUDIOCAPTURE_H
