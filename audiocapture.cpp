#include <QThread>
#include <QAudioInput>
#include <QEventLoop>
#include <QDebug>
#include <QBuffer>
#include <QTimer>
#include "audiocapture.h"

AudioCapture::AudioCapture(QObject *parent) : QThread(parent)
{
//    foreach (const QAudioDeviceInfo &info, QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
//        if (info.isNull()) {
//            continue;
//        }
//        QList<int> sampleRates = info.supportedSampleRates();
//        if (sampleRates.empty()) {
//            continue;
//        }
//        qDebug() << "Device input name: " << info.deviceName() << sampleRates
//                 << info.supportedCodecs() << info.supportedSampleTypes()
//                 << info.supportedByteOrders() << info.supportedChannelCounts()
//                 << info.supportedSampleSizes();
//    }
    m_inputBuffer = new QBuffer();
}

AudioCapture::~AudioCapture()
{

}

qreal charToReal (const char* ch, bool littleEndian = true)
{
    uchar val1 = uchar(ch[0]);
    uchar val2 = uchar(ch[1]);
    qreal res = 0;
    if (littleEndian)
    {
        short val = val1 | (val2 << 8);
        res = qreal (val);
    }
    // 2^16=65536, signed range = [-32768; 32767]
    res /= 32768;
    return res;
}

void AudioCapture::stopCapture()
{
    m_audioInput->stop();
    QByteArray data = m_inputBuffer->data();
    char* rawData = data.data();
    QList<qreal> samples;
    for (int i = 0; i < data.length(); i += 4) // choose only left channel
    {
        // convert from char* to real
        samples.append(charToReal(&rawData[i]));
    }
//    qDebug() << data.toHex();
    m_inputBuffer->close();
}

void AudioCapture::run()
{
    QAudioFormat format;
    // set up the format you want, eg.
    qint32 sampleRate = 44100;
    format.setSampleRate(sampleRate);
    format.setChannelCount(2);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);
    qDebug() << __FUNCTION__ << format.byteOrder() << format.channelCount() << format.codec()
             << format.sampleRate() << format.sampleSize() << format.sampleType();

    QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();
    qDebug() << info.deviceName();
    foreach (QAudioDeviceInfo deviceInfo, QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
    {
        if (deviceInfo.deviceName() == "alsa_input.usb-046d_0825_36D88820-02-U0x46d0x825.analog-mono") {
            info = deviceInfo;
        }
    }
    qDebug() << info.deviceName();
    if (!info.isFormatSupported(format))
    {
        qDebug() << "Metronome::Metronome default format not supported try to use nearest";
        qDebug() << info.deviceName();
        Q_ASSERT(false);
        return;
    }
    m_audioInput = new QAudioInput(info, format);
    connect(m_audioInput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(stateChanged(QAudio::State)));
    m_inputBuffer->open(QIODevice::WriteOnly);
    m_audioInput->start(m_inputBuffer);

    QTimer::singleShot(1000, this, SLOT(stopCapture()));

    QEventLoop loop;
    loop.exec();
    forever {
    }
}

void AudioCapture::stateChanged(QAudio::State newState)
{
    Q_ASSERT (m_audioInput);
    qDebug() << "QAudio stateChanged" << newState;
    if (m_audioInput->error() != QAudio::NoError)
    {
        qDebug() << "QAudio error" << m_audioInput->error();
        Q_ASSERT (false);
    }
}
