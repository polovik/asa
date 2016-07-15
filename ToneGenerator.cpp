#include <QDebug>
#include <QAudioDeviceInfo>
#include <QAudioOutput>
#include <QMediaPlayer>
#include <QBuffer>
#include <QEventLoop>
#include <QtMath>
#include "ToneGenerator.h"

ToneGenerator::ToneGenerator(QObject *parent) : QThread(parent)
{
//    foreach (const QAudioDeviceInfo &info, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)) {
//        if (info.isNull()) {
//            continue;
//        }
//        QList<int> sampleRates = info.supportedSampleRates();
//        if (sampleRates.empty()) {
//            continue;
//        }
//        qDebug() << "Device output name: " << info.deviceName() << sampleRates
//                 << info.supportedCodecs() << info.supportedSampleTypes()
//                 << info.supportedByteOrders() << info.supportedChannelCounts()
//                 << info.supportedSampleSizes();
//    }

}

ToneGenerator::~ToneGenerator()
{

}

void ToneGenerator::run()
{
    /*  Prepare audio output device */
    QAudioFormat format;
    // Set up the format, eg.
    qint32 sampleRate = 44100;
    format.setSampleRate(sampleRate);
    format.setChannelCount(2);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);
    qDebug() << "ToneGenerator::run" << format.byteOrder() << format.channelCount()
            << format.codec() << format.sampleRate() << format.sampleSize()
            << format.sampleType();

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    qDebug() << info.deviceName();
//    foreach (QAudioDeviceInfo deviceInfo, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
//    {
//        if ("default" == deviceInfo.deviceName())
//            info = deviceInfo;
//    }
    if (!info.isFormatSupported(format))
    {
        qDebug() << "Metronome::Metronome default format not supported try to use nearest";
        qDebug() << info.deviceName();
        Q_ASSERT(false);
        return;
    }
    m_audioOutput = new QAudioOutput(info, format);
    connect(m_audioOutput, SIGNAL(stateChanged(QAudio::State)), SLOT(stateChanged(QAudio::State)));
    qint32 freq = 441;
    qint32 amplitude = 65536 / 2 - 1;
    double periodLength = 1. * sampleRate / freq;
    QByteArray data;
    data.resize(sampleRate * 2 * 4);
    int up = 0;
    for (int i = 0; i < sampleRate * 2; ++i) {
        double angle = 360. * i / periodLength;
        double v = qSin(qDegreesToRadians(angle));
        qint32 val = v * amplitude;
//        val = qrand();
        int sample = i * 4;
        data[sample + 0] = data[sample + 2] = (val & 0x00FF);
        data[sample + 1] = data[sample + 3] = (val >> 8) & 0x00FF;
    }

    QBuffer outputBuffer;
    outputBuffer.setData(data);
    outputBuffer.open(QIODevice::ReadOnly);
    outputBuffer.seek(0);

    m_audioOutput->start(&outputBuffer);
//    outputBuffer.close();

    exec();
}

void ToneGenerator::stateChanged(QAudio::State state)
{
    Q_ASSERT(m_audioOutput);
    QAudio::Error error = m_audioOutput->error();
    qDebug() << "Metronome::stateChanged State=" << state << ", Error=" << error;
    qDebug() << "Metronome::stateChanged" << m_audioOutput->bufferSize() << m_audioOutput->bytesFree() << m_audioOutput->periodSize();
    qDebug() << "elapsedUSecs = " << m_audioOutput->elapsedUSecs()
               << ", " << "processedUSecs = " << m_audioOutput->processedUSecs();
    if (state == QAudio::IdleState)
    {
        qDebug() << "Metronome::stateChanged IdleState Stop output";
        // http://www.bitchx.com/log/qt-f/qt-f-20-Apr-2010/qt-f-20-Apr-2010-05.php
        //audioOutput->stop(); // This line run crash!!! It immediatelly call again stateChanged slot. Be care
    }
    if (error != QAudio::NoError)
    {
        switch (error)
        {
            case 0 : qDebug() << "Metronome QAudio::NoError State =" << state; break;
            case 1 : qDebug() << "Metronome QAudio::OpenError State =" << state; break;
            case 2 : qDebug() << "Metronome QAudio::IOError State =" << state; break;
            case 3 :
                  qDebug() << "Metronome QAudio::UnderrunError State =" << state;
                break;
            case 4 : qDebug() << "Metronome QAudio::FatalError State =" << state; break;
        }
    }
}
