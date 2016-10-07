#include <QDebug>
#include <QAudioDeviceInfo>
#include <QAudioOutput>
#include <QMediaPlayer>
#include <QBuffer>
#include <QEventLoop>
#include <QtMath>
#include <QProcess>
#if defined(_WIN32)
#include <DSound.h>
#endif
#include "tonegenerator.h"
#include "settings.h"

AudioOutputDevice::AudioOutputDevice(QObject *parent) : QIODevice(parent)
{
    m_frequency = 0;
    m_sampleIndex = 0;
    m_activeChannels = CHANNEL_NONE;
    m_volume = 0.0;
}

AudioOutputDevice::~AudioOutputDevice()
{

}

void AudioOutputDevice::configure(const QAudioFormat &format, qint32 frequency,
                                  ToneWaveForm form, AudioChannels activeChannels,
                                  qreal volume)
{
    QMutexLocker lockerSettings(&m_settingsMutex);
    m_audioFormat = format;
    m_frequency = frequency;
    m_waveForm = form;
    m_activeChannels = activeChannels;
    m_volume = volume;
    m_sampleIndex = 0;
    // flush old audio samples from the QIODevice's buffer
    reset();
    qDebug() << "Configure tone generator: frequency" << m_frequency
             << "waveform" << form << "active channels" << m_activeChannels
             << "current volume" << m_volume
             << m_audioFormat.sampleRate() << m_audioFormat.byteOrder()
             << " " << m_audioFormat.channelCount() << " " << m_audioFormat.codec()
             << " " << m_audioFormat.sampleSize() << " " << m_audioFormat.sampleType();
}

qint64 AudioOutputDevice::readData(char *data, qint64 maxSize)
{
    QMutexLocker lockerSettings(&m_settingsMutex);
//    qDebug() << "AudioOutputDevice::readData" << maxSize << m_sampleIndex;

    qint64 amplitude = (qint32)qPow(2, m_audioFormat.sampleSize()) / 2 - 1;
    amplitude = 1.0 * amplitude * m_volume;
//    int channelCount = m_audioFormat.channelCount();
    int sampleLength = m_audioFormat.sampleSize() / 8;
    double periodLength = 1. * m_audioFormat.sampleRate() / m_frequency;
    // fill less data if maxSize isn't divisible by (channelCount * sampleLength)
    qint64 samplesCount = maxSize / m_audioFormat.bytesPerFrame();
    maxSize = samplesCount * m_audioFormat.bytesPerFrame();
    for (qint64 i = 0; i < samplesCount; ++i) {
        qint64 pos = i * m_audioFormat.bytesPerFrame();
        double v;
        if (m_waveForm.id() == ToneWaveForm::WAVE_SINE) {
            double angle = 360. * m_sampleIndex / periodLength;
            v = qSin(qDegreesToRadians(angle));
        } else if (m_waveForm.id() == ToneWaveForm::WAVE_SQUARE) {
            int relativePos = m_sampleIndex % int(periodLength);
            if (relativePos < periodLength / 2)
                v = 1.0;
            else
                v = -1.0;
        } else if (m_waveForm.id() == ToneWaveForm::WAVE_SAWTOOTH) {
            int relativePos = m_sampleIndex % int(periodLength);
            v = -1. + 2.0 * relativePos / periodLength;
        } else if (m_waveForm.id() == ToneWaveForm::WAVE_TRIANGLE) {
            int relativePos = m_sampleIndex % int(periodLength);
            if (relativePos < periodLength / 2)
                v = -1. + 2.0 * (relativePos * 2) / periodLength;
            else
                v =  3. - 2.0 * (relativePos * 2) / periodLength;
        } else {
            v = 1.0 * qrand() / RAND_MAX;
        }
        qint64 val = v * amplitude;
        if (m_audioFormat.byteOrder() == QAudioFormat::LittleEndian) {
//            data[pos + 0] = data[pos + 2] = (val & 0x00FF);
//            data[pos + 1] = data[pos + 3] = (val >> 8) & 0x00FF;
            for (int byte = 0; byte < m_audioFormat.bytesPerFrame(); byte++) {
                if (byte % sampleLength == 0) {
                    val = 0.0;
                    if ((byte == 0) && (m_activeChannels & CHANNEL_LEFT)) {
                        val = v * amplitude;
                    }
                    if ((byte != 0) && (m_activeChannels & CHANNEL_RIGHT)) {
                        val = v * amplitude;
                    }
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
    m_outputBuffer = NULL;
    m_activeChannels = CHANNEL_NONE;
    m_relativeAmplitude = -1;
    m_maxVoltageAmplitude = -10.;
    
    // Set up the format, eg.
    m_toneFrequency = 100;
    m_audioFormat.setSampleRate(44100);
    m_audioFormat.setChannelCount(2);
    m_audioFormat.setSampleSize(16);
    m_audioFormat.setCodec("audio/pcm");
    m_audioFormat.setByteOrder(QAudioFormat::LittleEndian);
    m_audioFormat.setSampleType(QAudioFormat::SignedInt);
    qDebug() << "ToneGenerator::ToneGenerator " << m_audioFormat.sampleRate() << m_audioFormat.byteOrder()
             << " " << m_audioFormat.channelCount() << " " << m_audioFormat.codec()
             << " " << m_audioFormat.sampleSize() << " " << m_audioFormat.sampleType();
}

ToneGenerator::~ToneGenerator()
{

}

#if defined(_WIN32)
static BOOL CALLBACK enumerateCallbackDS(LPGUID lpGUID, LPCTSTR lpszDesc, LPCTSTR lpszDrvName, LPVOID lpContext)
{
    Q_UNUSED(lpGUID);
    Q_UNUSED(lpszDrvName);

    QStringList *devicesList = (QStringList *)lpContext;
    QString description = QString::fromWCharArray(lpszDesc);
    devicesList->append(description);
    return TRUE;
}
#endif

QList<QPair<QString, QString>> ToneGenerator::enumerateDevices()
{
    extern bool g_verboseOutput;
    QList<QPair<QString, QString>> devices;
    if (m_generationEnabled) {
        qWarning() << "Devices can't be enumerated' - some of them is already in use";
        return devices;
    }
    Settings *settings = Settings::getSettings();
    QString prevDeviceName = settings->value("Generator/AudioOutputDevice", "").toString();
    QAudioDeviceInfo defaultDevice = QAudioDeviceInfo::defaultOutputDevice();
    if (prevDeviceName.isEmpty()) {
        prevDeviceName = defaultDevice.deviceName();
    }
    bool highlighted = false;
    m_audioDeviceInfos.clear();
#if defined(_WIN32)
    QStringList devicesFullNames;
    if (FAILED(DirectSoundEnumerate((LPDSENUMCALLBACK)enumerateCallbackDS, (VOID *)&devicesFullNames))) {
        qWarning() << "The list of devices full names couldn't be retrieved";
    }
    qDebug() << "Devices full names:" << devicesFullNames;
#else
    QProcess pacmd;
    pacmd.start("pacmd", QStringList() << "list-sinks", QIODevice::ReadOnly);
    QString pacmdOutput = "";
    if (!pacmd.waitForFinished()) {
        qWarning() << "Command \"pacmd list-sinks\" is invalid";
        Q_ASSERT(false);
    } else {
        if ((pacmd.exitStatus() != QProcess::NormalExit) || (pacmd.exitCode() != 0)) {
            qWarning() << "Command \"pacmd list-sinks\" was interrupted";
            Q_ASSERT(false);
        } else {
            pacmdOutput = pacmd.readAll();
        }
    }
#endif
    foreach(const QAudioDeviceInfo &info, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)) {
        if (info.isNull()) {
            continue;
        }
        QString name = info.deviceName();
        QString description = name;
#if defined(_WIN32)
        foreach (QString fullName, devicesFullNames) {
            if (fullName.startsWith(name)) {
                description = fullName;
                qDebug() << name << "--" << description;
                break;
            }
        }
#else
        if (!name.contains("alsa", Qt::CaseInsensitive)) {
            if (g_verboseOutput) {
                qDebug() << "Skip non-ALSA device:" << name;
            }
            continue;
        }
        /*
          index: 2
                name: <alsa_output.pci-0000_00_03.0.hdmi-stereo-extra1>
                properties:
                    device.description = "Built-in Audio Digital Stereo (HDMI)"
        */
        QString pattern = QRegExp::escape(name) + QString(".*device.description\\s*=\\s*\\\"([^\\\"]*)\\\"");
        QRegExp pulseAudioRegExp(pattern, Qt::CaseSensitive, QRegExp::RegExp2);
        pulseAudioRegExp.setMinimal(true);
        int pos = pulseAudioRegExp.indexIn(pacmdOutput);
        if (pos > -1) {
            description = pulseAudioRegExp.cap(1);
            qDebug() << name << "--" << description;
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
        if (name == prevDeviceName) {
            highlighted = true;
            description.prepend("* ");
        }
        devices.append(qMakePair<QString, QString>(name, description));
        m_audioDeviceInfos.append(info);
    }
    if (!highlighted) {
        qWarning() << "Previous selected output device" << prevDeviceName << "is missed. Select default:" << defaultDevice.deviceName();
        for (int i = 0; i < devices.count(); i++) {
            QPair<QString, QString> &deviceInfo = devices[i];
            if (deviceInfo.first == defaultDevice.deviceName()) {
                deviceInfo.second = "* " + deviceInfo.second;
                break;
            }
        }
    }
    
    qDebug() << "Detected audio output devices:" << devices;
    return devices;
}

QString ToneGenerator::getDeviceName()
{
    return m_curAudioDeviceInfo.deviceName();
}

void ToneGenerator::runGenerator(bool start)
{
    qDebug() << "run tone generator:" << start;
    m_generationEnabled = start;
}

void ToneGenerator::changeFrequency(int freq)
{
    m_toneFrequency = freq;
    if (m_outputBuffer == NULL) {
        qWarning() << "Skip frequency applying for ToneGenerator because it hasn't been started yet";
        return;
    }
    m_outputBuffer->configure(m_audioFormat, m_toneFrequency, m_waveForm, m_activeChannels, m_relativeAmplitude);
}

void ToneGenerator::switchOutputDevice(QString name)
{
    if (m_generationEnabled) {
        qWarning() << "Output audio device can't be changed - it is already in use";
        return;
    }
    qDebug() << "Switch audio output device to" << name;
    foreach(const QAudioDeviceInfo &info, m_audioDeviceInfos) {
        if (info.deviceName() == name) {
            m_curAudioDeviceInfo = info;
            break;
        }
    }
    Settings *settings = Settings::getSettings();
    settings->setValue("Generator/AudioOutputDevice", name);
}

void ToneGenerator::switchWaveForm(ToneWaveForm form)
{
    m_waveForm = form;
    if (m_outputBuffer == NULL) {
        qWarning() << "Skip waveform applying for ToneGenerator because it hasn't been started yet";
        return;
    }
    m_outputBuffer->configure(m_audioFormat, m_toneFrequency, m_waveForm, m_activeChannels, m_relativeAmplitude);
}

void ToneGenerator::setActiveChannels(AudioChannels channels)
{
    m_activeChannels = channels;
    if (m_outputBuffer == NULL) {
        qWarning() << "Skip active channels applying for ToneGenerator because it hasn't been started yet";
        return;
    }
    m_outputBuffer->configure(m_audioFormat, m_toneFrequency, m_waveForm, m_activeChannels, m_relativeAmplitude);
}

qreal ToneGenerator::getCurVoltageAmplitude()
{
    qreal absAmplitude = m_relativeAmplitude * getMaxVoltageAmplitude();
    return absAmplitude;
}

void ToneGenerator::setCurVoltageAmplitude(qreal voltage)
{
    m_relativeAmplitude = voltage / getMaxVoltageAmplitude();
    if (m_outputBuffer == NULL) {
        qWarning() << "Skip cur voltage amplitude applying for ToneGenerator because it hasn't been started yet";
        return;
    }
    m_outputBuffer->configure(m_audioFormat, m_toneFrequency, m_waveForm, m_activeChannels, m_relativeAmplitude);
}

qreal ToneGenerator::getMaxVoltageAmplitude()
{
    if (m_maxVoltageAmplitude < 0) {
        Settings *settings = Settings::getSettings();
        m_maxVoltageAmplitude = settings->value("Generator/MaxVoltageAmplitude", 10.).toDouble();
    }
    return m_maxVoltageAmplitude;
}

void ToneGenerator::setMaxVoltageAmplitude(qreal voltage)
{
    qDebug() << "Set generator max voltage amplitude to" << voltage << "Volts";
    qreal absAmplitude = m_relativeAmplitude * m_maxVoltageAmplitude;
    m_maxVoltageAmplitude = voltage;
    m_relativeAmplitude = absAmplitude / m_maxVoltageAmplitude;
    Settings *settings = Settings::getSettings();
    settings->setValue("Generator/MaxVoltageAmplitude", m_maxVoltageAmplitude);
    if (m_outputBuffer == NULL) {
        qWarning() << "Skip cur voltage amplitude updating for ToneGenerator because it hasn't been started yet";
        return;
    }
    m_outputBuffer->configure(m_audioFormat, m_toneFrequency, m_waveForm, m_activeChannels, m_relativeAmplitude);
}

void ToneGenerator::run()
{
    m_outputBuffer = new AudioOutputDevice();
    m_outputBuffer->open(QIODevice::ReadOnly);
    emit deviceReady(true);
    
    bool generationStarted = false;
    QEventLoop loop;
    forever {
        msleep(1);
        loop.processEvents();
        if (!m_generationEnabled) {
            if (generationStarted == true) {
                qDebug() << "Stop tone generation";
                m_audioOutput->reset();
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
            m_audioOutput->reset();
            connect(m_audioOutput, SIGNAL(stateChanged(QAudio::State)), SLOT(stateChanged(QAudio::State)));
            m_outputBuffer->configure(m_audioFormat, m_toneFrequency, m_waveForm, m_activeChannels, m_relativeAmplitude);
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
    if (state == QAudio::IdleState) {
        qDebug() << "Metronome::stateChanged IdleState Stop output";
        // http://www.bitchx.com/log/qt-f/qt-f-20-Apr-2010/qt-f-20-Apr-2010-05.php
        //audioOutput->stop(); // This line run crash!!! It immediatelly call again stateChanged slot. Be care
    }
    if (error != QAudio::NoError) {
        switch (error) {
        case 0 :
            qDebug() << "Metronome QAudio::NoError State =" << state;
            break;
        case 1 :
            qDebug() << "Metronome QAudio::OpenError State =" << state;
            break;
        case 2 :
            qDebug() << "Metronome QAudio::IOError State =" << state;
            break;
        case 3 :
            qDebug() << "Metronome QAudio::UnderrunError State =" << state;
            break;
        case 4 :
            qDebug() << "Metronome QAudio::FatalError State =" << state;
            break;
        }
    }
}
