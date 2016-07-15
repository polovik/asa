#include <QDebug>
#include <QAudioDeviceInfo>
#include <QAudioOutput>
#include <QMediaPlayer>
#include <QBuffer>
#include <QEventLoop>
#include <QtMath>
#include "ToneGenerator.h"

AudioOutputDevice::AudioOutputDevice(QObject *parent) : QIODevice(parent)
{
    m_samplingRate = 0;
    m_frequency = 0;
    m_sampleIndex = 0;
}

AudioOutputDevice::~AudioOutputDevice()
{

}

void AudioOutputDevice::configure(qint32 samplingRate, qint32 frequency)
{
    m_samplingRate = samplingRate;
    m_frequency = frequency;
    m_sampleIndex = 0;
}

qint64 AudioOutputDevice::readData(char *data, qint64 maxSize)
{
    qDebug() << "AudioOutputDevice::readData" << maxSize << m_sampleIndex;

    qint32 amplitude = 65536 / 2 - 1;
    int channelCount = 2;
    int sampleLength = 2;
    double periodLength = 1. * m_samplingRate / m_frequency;
    // fill less data if maxSize isn't divisible by (channelCount * sampleLength)
    qint64 samplesCount = maxSize / channelCount / sampleLength;
    maxSize = samplesCount * channelCount * sampleLength;
//    m_buffer.clear();
//    m_buffer.resize(maxSize);
    for (qint64 i = 0; i < samplesCount; ++i) {
        qint64 pos = i * channelCount * sampleLength;
        double angle = 360. * m_sampleIndex / periodLength;
        double v = qSin(qDegreesToRadians(angle));
        qint32 val = v * amplitude;
        data[pos + 0] = data[pos + 2] = (val & 0x00FF);
        data[pos + 1] = data[pos + 3] = (val >> 8) & 0x00FF;
        ++m_sampleIndex;
    }
//    memcpy(data, m_buffer.data(), maxSize);

    return maxSize;
}

qint64 AudioOutputDevice::writeData(const char *data, qint64 maxSize)
{
    Q_UNUSED(data);
    qDebug() << "AudioOutputDevice::writeData" << maxSize;
    return maxSize;
}

ToneGenerator::ToneGenerator(QObject *parent) : QThread(parent)
{
    m_generationEnabled = false;
    m_audioOutput = NULL;
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

void ToneGenerator::runGenerator(bool start)
{
    qDebug() << "run tone generator:" << start;
    m_generationEnabled = start;
}

void ToneGenerator::run()
{
    m_outputBuffer = new AudioOutputDevice();
    /*  Prepare audio output device */
    QAudioFormat format;
    // Set up the format, eg.
    m_sampleRate = 44100;
    m_toneFrequency = 440;
    format.setSampleRate(m_sampleRate);
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
    m_outputBuffer->open(QIODevice::ReadOnly);

    bool generationStarted = false;
    QEventLoop loop;
    forever {
        msleep(1);
        loop.processEvents();
        if (!m_generationEnabled) {
            if (generationStarted == true) {
                qDebug() << "Stop tone generation";
                m_audioOutput->disconnect();
                m_audioOutput->stop();
                m_audioOutput->deleteLater();
                m_audioOutput = NULL;
                generationStarted = false;
            }
            continue;
        }
        if (generationStarted == false) {
            qDebug() << "Start tone generation";
            m_audioOutput = new QAudioOutput(info, format);
            connect(m_audioOutput, SIGNAL(stateChanged(QAudio::State)), SLOT(stateChanged(QAudio::State)));
            m_outputBuffer->configure(m_sampleRate, m_toneFrequency);
            m_audioOutput->start(m_outputBuffer);
            generationStarted = true;
        }
//        qDebug() << "ToneGenerator::run" << m_audioOutput->bufferSize() << m_audioOutput->bytesFree() << m_audioOutput->periodSize();
//        qDebug() << "ToneGenerator::run" << m_outputBuffer->pos();
    }
}

void ToneGenerator::stateChanged(QAudio::State state)
{
    if (m_audioOutput == NULL) {
        qWarning() << "Audio output is NULL";
//        Q_ASSERT(m_audioOutput);
        return;
    }
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
