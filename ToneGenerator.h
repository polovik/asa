#ifndef TONEGENERATOR_H
#define TONEGENERATOR_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QAudioOutput>

class ToneGenerator : public QThread
{
    Q_OBJECT
public:
    explicit ToneGenerator(QObject *parent = 0);
    ~ToneGenerator();

public slots:

signals:

protected:
    void run() Q_DECL_OVERRIDE;

private slots:
    void stateChanged(QAudio::State state);

private:
    QAudioOutput *m_audioOutput;
};

#endif // TONEGENERATOR_H
