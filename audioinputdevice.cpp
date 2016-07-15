#include <QDebug>
#include <QTime>
#include <QtCore/qendian.h>
#include "audioinputdevice.h"

Q_DECLARE_METATYPE (SamplesList)

AudioInputDevice::AudioInputDevice(QObject *parent) :
    QIODevice(parent)
{
    samplesReaded = 0;
    setFormat();
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
}

AudioInputDevice::~AudioInputDevice()
{
    stopRecording();
}

void AudioInputDevice::stopRecording()
{
    audio->stop();
    audio->disconnect();
    delete audio;
}

void AudioInputDevice::setFormat()
{
    QAudioFormat format;
    // set up the format you want, eg.
    format.setSampleRate(44100); //8000
    format.setChannelCount(2);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);
    qDebug() << __FUNCTION__ << format.byteOrder() << format.channelCount() << format.codec()
             << format.sampleRate() << format.sampleSize() << format.sampleType();

    QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();
    qDebug() << info.deviceName();
    foreach (QAudioDeviceInfo deviceInfo, QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
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

    if (!info.isFormatSupported(format))
    {
        qDebug() << info.deviceName();
        qWarning() << "default format not supported try to use nearest";
        format = info.nearestFormat(format);
        qDebug() << format.byteOrder() << format.channelCount() << format.codec()
                << format.sampleRate() << format.sampleSize() << format.sampleType();

        foreach (QAudioDeviceInfo deviceInfo, QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
            qDebug() << "Device name: " << deviceInfo.deviceName();
        foreach (QString codec, info.supportedCodecs())
            qDebug() << "Supported codec: " << codec;
        foreach (int freq, info.supportedSampleRates())
            qDebug() << "Supported freq: " << freq;
        foreach (int sampleSize, info.supportedSampleSizes())
            qDebug() << "Supported sample size: " << sampleSize;
        foreach (int channels, info.supportedChannelCounts())
            qDebug() << "Supported channels: " << channels;
        foreach (int byteOrder, info.supportedByteOrders())
            qDebug() << "Supported byte orders: " << byteOrder;
        foreach (int sampleTypes, info.supportedSampleTypes())
            qDebug() << "Supported sample types: " << sampleTypes;
    }

    samplingRate = format.sampleRate();
    audio = new QAudioInput (info, format);
    connect (audio, SIGNAL(stateChanged(QAudio::State)), SLOT(stateChanged(QAudio::State)));
}

void AudioInputDevice::startReading (bool start)
{
    Q_ASSERT (audio);
    if (audio)
    {
        if (start)
        {
            timer.start ();
            audio->start (this);
            qDebug() << "AudioInputDevice::startReading" << audio->periodSize() << audio->bufferSize();
        }
        else
        {
            qDebug() << "AudioInputDevice::stopReading";
            audio->stop ();
        }
    }
}

void AudioInputDevice::stateChanged(QAudio::State newState)
 {
    Q_ASSERT (audio);
    qDebug() << "QAudio stateChanged" << newState;
    if (audio->error() != QAudio::NoError)
    {
        qDebug() << "QAudio error" << audio->error();
        Q_ASSERT (false);
    }
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
    m_audioFormat.setSampleRate(44100); //8000
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
    // Prepare audio device
    audioDevice = new AudioInputDevice;
    if (!audioDevice)
    {
        Q_ASSERT (false);
    }
    Q_ASSERT (audioDevice->open (QIODevice::ReadWrite | QIODevice::Truncate));
    connect (audioDevice, SIGNAL (samplesReceived (SamplesList)), SLOT (updateBuffers (SamplesList)), Qt::QueuedConnection);
    int samplingRate = audioDevice->samplingRate;
    emit initiated (samplingRate);

    exec ();
}

/*  Start or stop capture   */
void AudioInputThread::startCapturing (bool start)
{
    Q_ASSERT (audioDevice);
    if (audioDevice)
        QMetaObject::invokeMethod (audioDevice, "startReading", Q_ARG (bool, start));
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
        qDebug() << "Device input name:" << info.deviceName() << info.supportedSampleRates()
                 << info.supportedCodecs() << info.supportedSampleTypes()
                 << info.supportedByteOrders() << info.supportedChannelCounts()
                 << info.supportedSampleSizes();
        QString name = info.deviceName();
        if (info.deviceName() == "alsa_input.usb-046d_0825_36D88820-02-U0x46d0x825.analog-mono") {
            name.prepend("- ");
        }
        if (info.deviceName() == defaultDevice.deviceName()) {
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
