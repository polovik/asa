#include <QDebug>
#include <QTime>
#include <QEventLoop>
#include <QtCore/qendian.h>
#include "audioinputdevice.h"

Q_DECLARE_METATYPE (SamplesList)

AudioInputDevice::AudioInputDevice(QObject *parent) :
    QIODevice(parent)
{
}

AudioInputDevice::~AudioInputDevice()
{
}

qint64 AudioInputDevice::readData (char* data, qint64 maxSize)
{
    qDebug() << "AudioInputDevice::readData" << data;
    return maxSize;
}

qreal charToReal (const char* ch, bool littleEndian = true)
{
    uchar val1 = uchar(ch[0]);
    uchar val2 = uchar(ch[1]);
    qreal res = 0;
    short val = 0;
    if (littleEndian) {
        val = val1 | (val2 << 8);
    } else {
        val = (val1 << 8) | val2;
    }
    res = qreal (val);
    // 2^16=65536, signed range = [-32768; 32767]
    res /= 32768;
    return res;
}

qint64 AudioInputDevice::writeData (const char* data, qint64 maxSize)
{
    Q_ASSERT (maxSize % 4 == 0);
    SamplesList samples;
    for (int i = 0; i < maxSize; i += 4) // choose only left channel
    {
        // convert from char* to real
        samples.append (charToReal(&data[i], true));
    }
    emit samplesReceived (samples); // emit this signal always before readyRead
    emit readyRead(); // if after emit this signal device can write more data, stop processing more and this function called again
    return maxSize;
}

//============================AudioInputThread================================//
AudioInputThread::AudioInputThread () :
        compressorFrameSize (0), volumeIndicatorFrameSize (0), tunerFrameSize (0), oscilloscopeFrameSize(0)
{
    qRegisterMetaType <SamplesList> ();
    m_captureEnabled = false;
    // set up the format you want, eg.
    m_sampleRate = 44100;
    m_audioFormat.setSampleRate(m_sampleRate);
    m_audioFormat.setChannelCount(2);
    m_audioFormat.setSampleSize(16);
    m_audioFormat.setCodec("audio/pcm");
    m_audioFormat.setByteOrder(QAudioFormat::LittleEndian);
    m_audioFormat.setSampleType(QAudioFormat::SignedInt);
    qDebug() << "AudioInputThread::AudioInputThread " << m_audioFormat.byteOrder() << m_audioFormat.channelCount() << m_audioFormat.codec()
             << m_audioFormat.sampleRate() << m_audioFormat.sampleSize() << m_audioFormat.sampleType();
}

void AudioInputThread::run ()
{
    m_inputBuffer = new AudioInputDevice();
    m_inputBuffer->open(QIODevice::ReadWrite | QIODevice::Truncate);
    connect(m_inputBuffer, SIGNAL (samplesReceived (SamplesList)), SLOT (updateBuffers (SamplesList)), Qt::QueuedConnection);

    bool captureStarted = false;
    QEventLoop loop;
    forever {
        msleep(1);
        loop.processEvents();
        if (!m_captureEnabled) {
            if (captureStarted == true) {
                qDebug() << "Stop audio capture";
                m_audioInput->disconnect();
                m_audioInput->stop();
                m_audioInput->deleteLater();
                m_audioInput = NULL;
                captureStarted = false;
            }
            continue;
        }
        if (captureStarted == false) {
            qDebug() << "Start audio capture";
            m_audioInput = new QAudioInput (m_curAudioDeviceInfo, m_audioFormat);
            connect(m_audioInput, SIGNAL(stateChanged(QAudio::State)), SLOT(stateChanged(QAudio::State)));
            m_audioInput->start(m_inputBuffer);
            captureStarted = true;
            emit initiated (m_sampleRate);
        }
//        qDebug() << "AudioInputThread::run" << m_audioOutput->bufferSize() << m_audioOutput->bytesFree() << m_audioOutput->periodSize();
//        qDebug() << "AudioInputThread::run" << m_outputBuffer->pos();
    }
}

void AudioInputThread::stateChanged(QAudio::State newState)
 {
    Q_ASSERT (m_audioInput);
    qDebug() << "QAudio stateChanged" << newState;
    if (m_audioInput->error() != QAudio::NoError)
    {
        qDebug() << "QAudio error" << m_audioInput->error();
        Q_ASSERT (false);
    }
}


/*  Start or stop capture   */
void AudioInputThread::startCapturing (bool start)
{
    qDebug() << "run audio capture process:" << start;
    m_captureEnabled = start;
}

/*
    If 'size' > 0 then capturing samples will be stored in appropriate buffer,
    otherwise they will be skip
*/
void AudioInputThread::changeFrameSize (ThreadPurpose purpose, int size)
{
    switch (purpose)
    {
        case COMPRESSOR :
        {
            compressorFrameSize = size;
            bufferCompressor.clear ();
            break;
        }
        case VOLUME_INDICATOR :
        {
            volumeIndicatorFrameSize = size;
            bufferIndicator.clear ();
            break;
        }
        case TUNER :
        {
            tunerFrameSize = size;
            bufferTuner.clear ();
            break;
        }
        case OSCILLOSCOPE :
        {
            oscilloscopeFrameSize = size;
            bufferOscilloscope.clear ();
            break;
        }
        default :
        {
            Q_ASSERT (false);
        }
    }
}

QStringList AudioInputThread::enumerateDevices()
{
    QStringList devices;
    if (m_captureEnabled) {
        qWarning() << "Devices can't be enumerated' - some of them is already in use";
        return devices;
    }
    m_audioDeviceInfos.clear();
    QAudioDeviceInfo defaultDevice = QAudioDeviceInfo::defaultInputDevice();
    foreach (const QAudioDeviceInfo &info, QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
        if (info.isNull()) {
            continue;
        }
        if (!info.isFormatSupported(m_audioFormat)) {
            continue;
        }
        QString name = info.deviceName();
        qDebug() << "Device input name:" << name << info.supportedSampleRates()
                 << info.supportedCodecs() << info.supportedSampleTypes()
                 << info.supportedByteOrders() << info.supportedChannelCounts()
                 << info.supportedSampleSizes();
        if (name == "alsa_input.usb-046d_0825_36D88820-02-U0x46d0x825.analog-mono") {
//            name.prepend("- ");
        }
        if (name == defaultDevice.deviceName()) {
            name.prepend("* ");
        }
        devices.append(name);
        m_audioDeviceInfos.append(info);
    }

    qDebug() << "Detected audio input devices:" << devices;
    return devices;
}

/*
    After appropriated buffer filled then samples emit.
    If certain frame size equal to 0 - do not store samples in this buffer
*/
void AudioInputThread::updateBuffers (SamplesList samples)
{
    //qDebug() << "Got" << samples.length() << "samples";
    if (compressorFrameSize > 0)
    {
        bufferCompressor.append (samples);
        while (bufferCompressor.size() >= compressorFrameSize)
        {
            SamplesList data = bufferCompressor.mid (0, compressorFrameSize);
            bufferCompressor = bufferCompressor.mid (compressorFrameSize);
            emit dataForCompressor (data);
        }
    }
    if (volumeIndicatorFrameSize > 0)
    {
        bufferIndicator.append (samples);
        while (bufferIndicator.size() >= volumeIndicatorFrameSize)
        {
            SamplesList data = bufferIndicator.mid (0, volumeIndicatorFrameSize);
            bufferIndicator = bufferIndicator.mid (volumeIndicatorFrameSize);
            emit dataForVolumeIndicator (data);
        }
    }
    if (tunerFrameSize > 0)
    {
        bufferTuner.append (samples);
        while (bufferTuner.size() >= tunerFrameSize)
        {
            SamplesList data = bufferTuner.mid (0, tunerFrameSize);
            bufferTuner = bufferTuner.mid (tunerFrameSize);
            emit dataForTuner (data);
        }
    }
    if (oscilloscopeFrameSize > 0)
    {
        bufferOscilloscope.append (samples);
        while (bufferOscilloscope.size() >= oscilloscopeFrameSize)
        {
            SamplesList data = bufferOscilloscope.mid (0, oscilloscopeFrameSize);
            bufferOscilloscope = bufferOscilloscope.mid (oscilloscopeFrameSize);
            emit dataForOscilloscope (data);
        }
    }
}

void AudioInputThread::switchInputDevice(QString name)
{
    if (m_captureEnabled) {
        qWarning() << "Input audio device can't be changed - it is already in use";
        return;
    }
    qDebug() << "Switch audio input device to" << name;
    foreach (const QAudioDeviceInfo &info, m_audioDeviceInfos) {
        if (info.deviceName() == name) {
            m_curAudioDeviceInfo = info;
            break;
        }
    }
}
