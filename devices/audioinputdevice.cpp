#include <QDebug>
#include <QtMath>
#include <QTime>
#include <QEventLoop>
#include <QtCore/qendian.h>
#include <QProcess>
#if defined(_WIN32)
#include <DSound.h>
#endif
#include "audioinputdevice.h"
#include "settings.h"

AudioInputDevice::AudioInputDevice(QObject *parent) :
    QIODevice(parent)
{
    m_channels = CHANNEL_NONE;
    m_scaleFactor = 100.;
}

AudioInputDevice::~AudioInputDevice()
{
}

void AudioInputDevice::setChannels(AudioChannels channels)
{
    qDebug() << "Select input channels: left -"
             << ((channels & CHANNEL_LEFT) ? "on," : "off,")
             << "right -" << ((channels & CHANNEL_RIGHT) ? "on" : "off");
    m_channels = channels;
}

void AudioInputDevice::setVoltageScaleFactor(qreal maxVoltage)
{
    m_scaleFactor = maxVoltage;
    qDebug() << "Set voltage scale factor to" << m_scaleFactor << "V";
}

qint64 AudioInputDevice::readData(char *data, qint64 maxSize)
{
    qDebug() << "AudioInputDevice::readData" << data;
    return maxSize;
}

qreal charToReal(const char *ch, bool littleEndian = true)
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
    res = qreal(val);
    // 2^16=65536, signed range = [-32768; 32767]
    res /= 32768;
    return res;
}

qint64 AudioInputDevice::writeData(const char *data, qint64 maxSize)
{
    Q_ASSERT(maxSize % 4 == 0);
    SamplesList samplesLeft;
    SamplesList samplesRight;
    for (int i = 0; i < maxSize; i += 4) { // choose only left channel
        // convert from char* to real
        if (m_channels & CHANNEL_LEFT) {
            qreal sample = charToReal(&data[i], true);
            sample = sample * m_scaleFactor;
            samplesLeft.append(sample);
        }
        if (m_channels & CHANNEL_RIGHT) {
            qreal sample = charToReal(&data[i + 2], true);
            sample = sample * m_scaleFactor;
            samplesRight.append(sample);
        }
    }
//    qDebug() << samplesLeft.size() << samplesRight.size();
    // TODO attach timestamp
//    emit samplesReceived (CHANNEL_LEFT, samplesLeft); // emit this signal always before readyRead
//    emit samplesReceived (CHANNEL_RIGHT, samplesRight); // emit this signal always before readyRead
    emit samplesReceived(samplesLeft, samplesRight);
    emit readyRead(); // if after emit this signal device can write more data, stop processing more and this function called again
    return maxSize;
}

//============================AudioInputThread================================//
AudioInputThread::AudioInputThread()
{
    m_inputBuffer = NULL;
    m_audioInput = NULL;
    m_capturedChannels = CHANNEL_NONE;
    m_captureEnabled = false;
    m_maxInputVoltage = 0.01;
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

void AudioInputThread::run()
{
    m_inputBuffer = new AudioInputDevice();
    m_inputBuffer->open(QIODevice::ReadWrite | QIODevice::Truncate);
    m_inputBuffer->setVoltageScaleFactor(m_maxInputVoltage);
//    connect(m_inputBuffer, SIGNAL(samplesReceived(AudioChannels,SamplesList)), SLOT (updateBuffers(AudioChannels,SamplesList)), Qt::QueuedConnection);
//    connect(m_inputBuffer, SIGNAL(samplesReceived(AudioChannels,SamplesList)), SIGNAL(dataForOscilloscope(AudioChannels,SamplesList)), Qt::QueuedConnection);
    connect(m_inputBuffer, SIGNAL(samplesReceived(SamplesList, SamplesList)), SIGNAL(dataForOscilloscope(SamplesList, SamplesList)), Qt::QueuedConnection);
    
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
                setCapturedChannels(CHANNEL_NONE);
            }
            continue;
        }
        if (captureStarted == false) {
            qDebug() << "Start audio capture";
            setCapturedChannels(CHANNEL_NONE);
            m_audioInput = new QAudioInput(m_curAudioDeviceInfo, m_audioFormat);
            connect(m_audioInput, SIGNAL(stateChanged(QAudio::State)), SLOT(stateChanged(QAudio::State)));
            m_audioInput->start(m_inputBuffer);
            captureStarted = true;
            emit initiated(m_sampleRate);
        }
//        qDebug() << "AudioInputThread::run" << m_audioOutput->bufferSize() << m_audioOutput->bytesFree() << m_audioOutput->periodSize();
//        qDebug() << "AudioInputThread::run" << m_outputBuffer->pos();
    }
}

void AudioInputThread::stateChanged(QAudio::State newState)
{
    Q_ASSERT(m_audioInput);
    qDebug() << "QAudio stateChanged" << newState;
    if (m_audioInput->error() != QAudio::NoError) {
        qDebug() << "QAudio error" << m_audioInput->error();
        Q_ASSERT(false);
    }
}


/*  Start or stop capture   */
void AudioInputThread::startCapturing(bool start)
{
    qDebug() << "run audio capture process:" << start;
    m_captureEnabled = start;
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

QList<QPair<QString, QString>> AudioInputThread::enumerateDevices()
{
    extern bool g_verboseOutput;
    QList<QPair<QString, QString>> devices;
    if (m_captureEnabled) {
        qWarning() << "Devices can't be enumerated' - some of them is already in use";
        return devices;
    }
    Settings *settings = Settings::getSettings();
    QString prevDeviceName = settings->value("Capture/AudioInputDevice", "").toString();
    QAudioDeviceInfo defaultDevice = QAudioDeviceInfo::defaultInputDevice();
    if (prevDeviceName.isEmpty()) {
        prevDeviceName = defaultDevice.deviceName();
    }
    bool highlighted = false;
    m_audioDeviceInfos.clear();
#if defined(_WIN32)
    QStringList devicesFullNames;
    if (FAILED(DirectSoundCaptureEnumerate((LPDSENUMCALLBACK)enumerateCallbackDS, (VOID *)&devicesFullNames))) {
        qWarning() << "The list of devices full names couldn't be retrieved";
    }
    qDebug() << "Devices full names:" << devicesFullNames;
#else
    QProcess pacmd;
    pacmd.start("pacmd", QStringList() << "list-sources", QIODevice::ReadOnly);
    QString pacmdOutput = "";
    if (!pacmd.waitForFinished()) {
        qWarning() << "Command \"pacmd list-sources\" is invalid";
        Q_ASSERT(false);
    } else {
        if ((pacmd.exitStatus() != QProcess::NormalExit) || (pacmd.exitCode() != 0)) {
            qWarning() << "Command \"pacmd list-sources\" was interrupted";
            Q_ASSERT(false);
        } else {
            pacmdOutput = pacmd.readAll();
        }
    }
#endif
    foreach(const QAudioDeviceInfo &info, QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
        if (info.isNull()) {
            continue;
        }
        QString name = info.deviceName();
        QString description = name;
        if (g_verboseOutput) {
            qDebug() << "Device input name:" << name << info.supportedSampleRates()
                     << info.supportedCodecs() << info.supportedSampleTypes()
                     << info.supportedByteOrders() << info.supportedChannelCounts()
                     << info.supportedSampleSizes();
        }
        if (!info.isFormatSupported(m_audioFormat)) {
            continue;
        }
#if defined(_WIN32)
        foreach (QString fullName, devicesFullNames) {
            if (fullName.startsWith(name)) {
                description = fullName;
                qDebug() << name << "--" << description;
                break;
            }
        }
#else
        if (!name.contains("alsa_input", Qt::CaseInsensitive)) {
            if (g_verboseOutput) {
                qDebug() << "Skip non-ALSA device:" << name;
            }
            continue;
        }
        /*
          index: 3
                name: <alsa_input.pci-0000_00_1b.0.analog-stereo>
                properties:
                    device.description = "Built-in Audio Analog Stereo"
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
        if (name == prevDeviceName) {
            highlighted = true;
            description.prepend("* ");
        }
        devices.append(qMakePair<QString, QString>(name, description));
        m_audioDeviceInfos.append(info);
    }
    if (!highlighted) {
        qWarning() << "Previous selected input device" << prevDeviceName << "is missed. Select default:" << defaultDevice.deviceName();
        for (int i = 0; i < devices.count(); i++) {
            QPair<QString, QString> &deviceInfo = devices[i];
            if (deviceInfo.first == defaultDevice.deviceName()) {
                deviceInfo.second = "* " + deviceInfo.second;
                break;
            }
        }
    }
    
    qDebug() << "Detected audio input devices:" << devices;
    return devices;
}

void AudioInputThread::setCapturedChannels(AudioChannels channels)
{
    m_capturedChannels = channels;
    m_inputBuffer->setChannels(m_capturedChannels);
}

QString AudioInputThread::getDeviceName()
{
    return m_curAudioDeviceInfo.deviceName();
}

#if !defined(_WIN32)
QString getAlsaPort(QString pulseAudioDeviceName)
{
    QString alsaCardIndex;
    QProcess pacmd;
    pacmd.start("pacmd", QStringList() << "list-sources", QIODevice::ReadOnly);
    QString pacmdOutput = "";
    if (!pacmd.waitForFinished()) {
        qWarning() << "Command \"pacmd list-sources\" is invalid";
        Q_ASSERT(false);
        return alsaCardIndex;
    } else {
        if ((pacmd.exitStatus() != QProcess::NormalExit) || (pacmd.exitCode() != 0)) {
            qWarning() << "Command \"pacmd list-sources\" was interrupted";
            Q_ASSERT(false);
            return alsaCardIndex;
        } else {
            pacmdOutput = pacmd.readAll();
        }
    }
    /*
      index: 3
            name: <alsa_input.pci-0000_00_1b.0.analog-stereo>
            properties:
                alsa.card = "3"
                alsa.card_name = "USB Sound Device"
    */
    QString pattern = QRegExp::escape(pulseAudioDeviceName) + QString(".*alsa\\.card\\s*=\\s*\\\"([^\\\"]*)\\\"");
    QRegExp pulseAudioRegExp(pattern, Qt::CaseSensitive, QRegExp::RegExp2);
    pulseAudioRegExp.setMinimal(true);
    int pos = pulseAudioRegExp.indexIn(pacmdOutput);
    if (pos > -1) {
        alsaCardIndex = pulseAudioRegExp.cap(1);
        qDebug() << "Audio device" << pulseAudioDeviceName << "is linked with alsa card -" << alsaCardIndex;
    } else {
        qWarning() << "Couldn't find linked alsa card for audio device" << pulseAudioDeviceName;
        Q_ASSERT(false);
        return alsaCardIndex;
    }

    return alsaCardIndex;
}
#endif

QStringList AudioInputThread::getPortsList()
{
    QStringList ports;
    m_portsMap.clear();
#if defined(_WIN32)
    return ports;
#else
    if (getDeviceName().isEmpty()) {
        qWarning() << "Name of audio device is empty.";
        return ports;
    }
    QString alsaCardIndex = getAlsaPort(getDeviceName());
    if (alsaCardIndex.isEmpty()) {
        qWarning() << "Alsa ports couldn't be determinied"
                   << "because of undefined Alsa card index for device" << getDeviceName();
        return ports;
    }
    QProcess pacmd;
    pacmd.start("pacmd", QStringList() << "list-sources", QIODevice::ReadOnly);
    QString pacmdOutput = "";
    if (!pacmd.waitForFinished()) {
        qWarning() << "Command \"pacmd list-sources\" is invalid";
        Q_ASSERT(false);
    } else {
        if ((pacmd.exitStatus() != QProcess::NormalExit) || (pacmd.exitCode() != 0)) {
            qWarning() << "Command \"pacmd list-sources\" was interrupted";
            Q_ASSERT(false);
        } else {
            pacmdOutput = pacmd.readAll();
        }
    }
    /*
      index: 3
            name: <alsa_input.pci-0000_00_1b.0.analog-stereo>
            properties:
            ports:
                analog-input-microphone: Microphone (priority 8700, latency offset 0 usec, available: unknown)
                    properties:
                        device.icon_name = "audio-input-microphone"
                analog-input-linein: Line In (priority 8100, latency offset 0 usec, available: unknown)
                    properties:

            active port: <analog-input-linein>
    */
    QStringList paPorts;
    int linePos = pacmdOutput.indexOf("\tname: <" + getDeviceName() + ">");
    QString paPortsOutput = pacmdOutput.mid(linePos);
    int nextCardPos = paPortsOutput.indexOf("\tindex: ");
    int portsPos = paPortsOutput.indexOf("\tports:\n");
    if (portsPos == -1) {
        qDebug() << "Audio device" << getDeviceName() << "doesn't have ports (PulseAudio)";
        return ports;
    }
    if ((nextCardPos > -1) && (nextCardPos < portsPos)) {
        qDebug() << "Audio device" << getDeviceName() << "doesn't have ports (PulseAudio)";
        return ports;
    }
    paPortsOutput = paPortsOutput.mid(portsPos);
    int activePortPos = paPortsOutput.indexOf("\tactive port: ");
    paPortsOutput = paPortsOutput.left(activePortPos);
    QString pattern = QString("\\sanalog-input-([^:]+):");
    QRegExp portsRe(pattern, Qt::CaseSensitive, QRegExp::RegExp);
    portsRe.setMinimal(true);
    linePos = 0;
    while (linePos >= 0) {
        linePos = portsRe.indexIn(paPortsOutput, linePos);
        if (linePos > 0) {
            paPorts.append(portsRe.cap(1));
            linePos += portsRe.matchedLength();
        }
    }
    qDebug() << "Audio device" << getDeviceName() << "has ports (PulseAudio):" << paPorts;

    QProcess amixerCmd;
    amixerCmd.start("amixer", QStringList() << "-c" << alsaCardIndex << "sget" << "\"PCM Capture Source\"", QIODevice::ReadOnly);
    QString amixerOutput = "";
    if (!amixerCmd.waitForFinished(5000)) {
        qWarning() << "Command \"amixer sget PCM \"Capture Source\"\" is invalid";
        Q_ASSERT(false);
    } else {
        if ((amixerCmd.exitStatus() != QProcess::NormalExit) || (amixerCmd.exitCode() != 0)) {
            amixerOutput = amixerCmd.readAllStandardError();
            if (amixerOutput.contains("Unable to find simple control")) {
                qDebug() << "Audio device" << getDeviceName() << "doesn't have control field \"PCM Capture Source\"";
            } else {
                qWarning() << "Command \"amixer sget PCM \"Capture Source\"\" was interrupted";
                qDebug() << amixerOutput;
                Q_ASSERT(false);
            }
            return ports;
        } else {
            amixerOutput = amixerCmd.readAll();
        }
    }
    /*
      Simple mixer control 'PCM Capture Source',0
        Capabilities: enum
        Items: 'Mic' 'Line' 'IEC958 In' 'Mixer'
        Item0: 'Mic'
    */
    QStringList alsaPorts;
    linePos = amixerOutput.indexOf("Items:");
    amixerOutput = amixerOutput.mid(linePos);
    amixerOutput = amixerOutput.left(amixerOutput.indexOf("\n"));
    pattern = QString("\\s\\'([^\\']+)\\'");
    QRegExp itemsRe(pattern, Qt::CaseSensitive, QRegExp::RegExp);
    itemsRe.setMinimal(true);
    linePos = 0;
    while (linePos >= 0) {
        linePos = itemsRe.indexIn(amixerOutput, linePos);
        if (linePos > 0) {
            alsaPorts.append(itemsRe.cap(1));
            linePos += itemsRe.matchedLength();
        }
    }
    qDebug() << "Audio device" << getDeviceName() << "has ports (Alsa):" << alsaPorts;

    foreach (QString alsaPort, alsaPorts) {
        foreach (QString paPort, paPorts) {
            if (paPort.startsWith(alsaPort, Qt::CaseInsensitive)) {
                m_portsMap.insert(alsaPort, QString("analog-input-%1").arg(paPort));
                break;
            }
        }
    }
    qDebug() << "Audio device" << getDeviceName() << "has ports (Total):" << m_portsMap;

    Settings *settings = Settings::getSettings();
    QString prevPort = settings->value("Capture/AudioInputDevicePort", "").toString();
    ports = m_portsMap.keys();
    bool highlighted = false;
    for (int i = 0; i < ports.count(); i++) {
        QString &alsaPort = ports[i];
        if (highlighted) {
            break;
        }
        if (prevPort.isEmpty()) {
            alsaPort.prepend("* ");
            highlighted = true;
            continue;
        }
        if (alsaPort == prevPort) {
            alsaPort.prepend("* ");
            highlighted = true;
            continue;
        }
        if (i == (ports.count() - 1)) {
            alsaPort.prepend("* ");
        }
    }
    qDebug() << "Audio device" << getDeviceName() << "has ports for choosing:" << ports;

    return ports;
#endif
}

void AudioInputThread::setSensivity(qreal maxInputVoltage)
{
    m_maxInputVoltage = maxInputVoltage;
    if (m_inputBuffer != NULL) {
        m_inputBuffer->setVoltageScaleFactor(m_maxInputVoltage);
    }
}

void AudioInputThread::switchInputDevice(QString name)
{
    if (m_captureEnabled) {
        qWarning() << "Input audio device can't be changed - it is already in use";
        return;
    }
    qDebug() << "Switch audio input device to" << name;
    foreach(const QAudioDeviceInfo &info, m_audioDeviceInfos) {
        if (info.deviceName() == name) {
            m_curAudioDeviceInfo = info;
            break;
        }
    }
    Settings *settings = Settings::getSettings();
    settings->setValue("Capture/AudioInputDevice", name);
}

void AudioInputThread::switchPort(QString alsaPort)
{
    if (m_captureEnabled) {
        qWarning() << "Input audio device's port can't be changed - it is already in use";
        return;
    }
    qDebug() << "Switch audio input device's port to" << alsaPort;
    QString pulseAudioPort = m_portsMap.value(alsaPort, "");
    if (pulseAudioPort.isEmpty()) {
        qWarning() << "PulseAudio port isn't defined for device" << getDeviceName()
                   << "Alsa port:" << alsaPort << "Map:" << m_portsMap;
        return;
    }

#if !defined(_WIN32)
    QProcess pacmd;
    pacmd.start("pacmd", QStringList() << "set-source-port" << getDeviceName() << pulseAudioPort, QIODevice::ReadOnly);
//    QString pacmdOutput = "";
    if (!pacmd.waitForFinished(5000)) {
        qWarning() << "Command \"pacmd set-source-port\" is invalid";
        Q_ASSERT(false);
        return;
    } else {
        if ((pacmd.exitStatus() != QProcess::NormalExit) || (pacmd.exitCode() != 0)) {
            qWarning() << "Command \"pacmd set-source-port\" was interrupted";
            Q_ASSERT(false);
            return;
        } else {
            qDebug() << "Audio source device's PulseAudio port has been changed to" << pulseAudioPort;
        }
    }

    QString alsaCardIndex = getAlsaPort(getDeviceName());
    if (alsaCardIndex.isEmpty()) {
        qWarning() << "Alsa port couldn't be switched to" << alsaPort
                   << "because of undefined Alsa card index for device" << getDeviceName();
        return;
    }
    QProcess amixerCmd;
    amixerCmd.start("amixer", QStringList() << "-c" << alsaCardIndex << "sset"
                    << "\"PCM Capture Source\"" << alsaPort, QIODevice::ReadOnly);
//    QString amixerOutput = "";
    if (!amixerCmd.waitForFinished(5000)) {
        qWarning() << "Command \"amixer sset PCM \"Capture Source\"\" is invalid";
        Q_ASSERT(false);
        return;
    } else {
        if ((amixerCmd.exitStatus() != QProcess::NormalExit) || (amixerCmd.exitCode() != 0)) {
            qWarning() << "Command \"amixer sset PCM \"Capture Source\"\" was interrupted";
            Q_ASSERT(false);
            return;
        } else {
            qDebug() << "Audio source device's Alsa port has been changed to" << alsaPort;
        }
    }

    pacmd.start("pacmd", QStringList() << "set-source-mute" << getDeviceName() << "0", QIODevice::ReadOnly);
//    QString pacmdOutput = "";
    if (!pacmd.waitForFinished(5000)) {
        qWarning() << "Command \"pacmd set-source-mute\" is invalid";
        Q_ASSERT(false);
        return;
    } else {
        if ((pacmd.exitStatus() != QProcess::NormalExit) || (pacmd.exitCode() != 0)) {
            qWarning() << "Command \"pacmd set-source-mute\" was interrupted";
            Q_ASSERT(false);
            return;
        } else {
            qDebug() << "Audio source device" << getDeviceName() << "has been unmuted";
        }
    }
#endif

    Settings *settings = Settings::getSettings();
    settings->setValue("Capture/AudioInputDevicePort", alsaPort);
}
