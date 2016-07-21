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
    m_frequency = 0;
    m_sampleIndex = 0;
}

AudioOutputDevice::~AudioOutputDevice()
{

}

void AudioOutputDevice::configure(const QAudioFormat &format, qint32 frequency)
{
    m_audioFormat = format;
    m_frequency = frequency;
    m_sampleIndex = 0;
}

qint64 AudioOutputDevice::readData(char *data, qint64 maxSize)
{
//    qDebug() << "AudioOutputDevice::readData" << maxSize << m_sampleIndex;

    qint64 amplitude = (qint32)qPow(2, m_audioFormat.sampleSize()) / 2 - 1;
//    int channelCount = m_audioFormat.channelCount();
    int sampleLength = m_audioFormat.sampleSize() / 8;
    double periodLength = 1. * m_audioFormat.sampleRate() / m_frequency;
    // fill less data if maxSize isn't divisible by (channelCount * sampleLength)
    qint64 samplesCount = maxSize / m_audioFormat.bytesPerFrame();
    maxSize = samplesCount * m_audioFormat.bytesPerFrame();
    for (qint64 i = 0; i < samplesCount; ++i) {
        qint64 pos = i * m_audioFormat.bytesPerFrame();
        double angle = 360. * m_sampleIndex / periodLength;
        double v = qSin(qDegreesToRadians(angle));
        qint64 val = v * amplitude;
        if (m_audioFormat.byteOrder() == QAudioFormat::LittleEndian) {
//            data[pos + 0] = data[pos + 2] = (val & 0x00FF);
//            data[pos + 1] = data[pos + 3] = (val >> 8) & 0x00FF;
            for (int byte = 0; byte < m_audioFormat.bytesPerFrame(); byte++) {
                if (byte % sampleLength == 0) {
                    val = v * amplitude;
                }
                data[pos + byte] = val & 0xFF;
                val = val >> 8;
            }
        } else {
//            data[pos + 0] = data[pos + 2] = (val >> 8) & 0x00FF;
//            data[pos + 1] = data[pos + 3] = (val & 0x00FF);
            for (int byte = m_audioFormat.bytesPerFrame(); byte > 0; byte--) {
                if (byte % sampleLength == 0) {
                    val = v * amplitude;
                }
                data[pos + byte - 1] = val & 0xFF;
                val = val >> 8;
            }
        }
        ++m_sampleIndex;
    }

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

    // Set up the format, eg.
    m_sampleRate = 44100;
    m_toneFrequency = 440;
    m_audioFormat.setSampleRate(m_sampleRate);
    m_audioFormat.setChannelCount(2);
    m_audioFormat.setSampleSize(16);
    m_audioFormat.setCodec("audio/pcm");
    m_audioFormat.setByteOrder(QAudioFormat::LittleEndian);
    m_audioFormat.setSampleType(QAudioFormat::SignedInt);
    qDebug() << "ToneGenerator::ToneGenerator " << m_audioFormat.byteOrder() << m_audioFormat.channelCount()
            << m_audioFormat.codec() << m_audioFormat.sampleRate() << m_audioFormat.sampleSize()
            << m_audioFormat.sampleType();
}

ToneGenerator::~ToneGenerator()
{

}

QStringList ToneGenerator::enumerateDevices()
{
    extern bool g_verboseOutput;
    QStringList devices;
    if (m_generationEnabled) {
        qWarning() << "Devices can't be enumerated' - some of them is already in use";
        return devices;
    }
    m_audioDeviceInfos.clear();
    QAudioDeviceInfo defaultDevice = QAudioDeviceInfo::defaultOutputDevice();
    foreach (const QAudioDeviceInfo &info, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)) {
        if (info.isNull()) {
            continue;
        }
        QString name = info.deviceName();
#if !defined(_WIN32)
        if (!name.contains("alsa", Qt::CaseInsensitive)) {
            if (g_verboseOutput) {
                qDebug() << "Skip non-ALSA device:" << name;
            }
            continue;
        }
#endif
        if (g_verboseOutput) {
            qDebug() << "Device output name:" << name << info.supportedSampleRates()
                     << info.supportedCodecs() << info.supportedSampleTypes()
                     << info.supportedByteOrders() << info.supportedChannelCounts()
                     << info.supportedSampleSizes();
        }
        if (!info.isFormatSupported(m_audioFormat)) {
            continue;
        }
//        QList<int> sampleRates = info.supportedSampleRates();
//        if (sampleRates.empty()) {
//            continue;
//        }
//        if ("default" == deviceInfo.deviceName())
//            info = deviceInfo;
        if (name == defaultDevice.deviceName()) {
            name.prepend("* ");
        }
        devices.append(name);
        m_audioDeviceInfos.append(info);
    }

    qDebug() << "Detected audio output devices:" << devices;
    return devices;
}

void ToneGenerator::runGenerator(bool start)
{
    qDebug() << "run tone generator:" << start;
    m_generationEnabled = start;
}

void ToneGenerator::changeFrequency(int freq)
{
    m_toneFrequency = freq;
    m_outputBuffer->configure(m_audioFormat, m_toneFrequency);
}

void ToneGenerator::switchOutputDevice(QString name)
{
    if (m_generationEnabled) {
        qWarning() << "Output audio device can't be changed - it is already in use";
        return;
    }
    qDebug() << "Switch audio output device to" << name;
    foreach (const QAudioDeviceInfo &info, m_audioDeviceInfos) {
        if (info.deviceName() == name) {
            m_curAudioDeviceInfo = info;
            break;
        }
    }
}

void ToneGenerator::run()
{
    m_outputBuffer = new AudioOutputDevice();
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
            m_audioOutput = new QAudioOutput(m_curAudioDeviceInfo, m_audioFormat);
            connect(m_audioOutput, SIGNAL(stateChanged(QAudio::State)), SLOT(stateChanged(QAudio::State)));
            m_outputBuffer->configure(m_audioFormat, m_toneFrequency);
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
