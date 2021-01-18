#include <QDebug>
#include <QtMath>
#include <QTime>
#include <QEventLoop>
#include <QtCore/qendian.h>
#include <QProcess>
#if defined(_WIN32)
#include <DSound.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <initguid.h>
// #include <Functiondiscoverykeys_devpkey.h>
#include <propkeydef.h>
#endif
#include "audioinputdevice.h"
#include "settings.h"

extern bool g_verboseOutput;

AudioInputDevice::AudioInputDevice(QObject *parent) :
    QIODevice(parent)
{
    m_channels = CHANNEL_NONE;
    m_scaleFactor = 100.;
    m_leftChannelOffset = 1.;
    m_rightChannelOffset = 1.;
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

void AudioInputDevice::setOffsets(qreal leftChannelOffset, qreal rightChannelOffset)
{
    m_leftChannelOffset = leftChannelOffset;
    m_rightChannelOffset = rightChannelOffset;
    qDebug() << "Set offsets for audio input data: left -" << leftChannelOffset
             << "right -" << rightChannelOffset;
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
            sample = sample + m_leftChannelOffset;
            sample = sample * m_scaleFactor;
            samplesLeft.append(sample);
        }
        if (m_channels & CHANNEL_RIGHT) {
            qreal sample = charToReal(&data[i + 2], true);
            sample = sample + m_rightChannelOffset;
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
    m_inputBuffer = nullptr;
    m_audioInput = nullptr;
    m_capturedChannels = CHANNEL_NONE;
    m_captureEnabled = false;
    m_maxInputVoltage = 0.01;
    Settings *settings = Settings::getSettings();
    m_amplifyFactor = settings->value("Capture/VoltageAmplifyFactor", 1.0).toDouble();
    m_leftChannelOffset = settings->value("Capture/LeftChannelOffset", 0.0).toDouble();
    m_rightChannelOffset = settings->value("Capture/RightChannelOffset", 0.0).toDouble();
    m_systemVolume = settings->value("Capture/SystemVolume", 0).toInt();

    // set up the format you want, eg.
    m_sampleRate = 44100;
    m_audioFormat.setSampleRate(m_sampleRate);
    m_audioFormat.setChannelCount(2);
    m_audioFormat.setSampleSize(16);
    m_audioFormat.setCodec("audio/pcm");
    m_audioFormat.setByteOrder(QAudioFormat::LittleEndian);
    m_audioFormat.setSampleType(QAudioFormat::SignedInt);
    if (g_verboseOutput) {
        qDebug() << "AudioInputThread::AudioInputThread " << m_audioFormat.byteOrder() << m_audioFormat.channelCount() << m_audioFormat.codec()
                 << m_audioFormat.sampleRate() << m_audioFormat.sampleSize() << m_audioFormat.sampleType();
    }
}

void AudioInputThread::run()
{
    m_inputBuffer = new AudioInputDevice();
    m_inputBuffer->open(QIODevice::ReadWrite | QIODevice::Truncate);
//    connect(m_inputBuffer, SIGNAL(samplesReceived(AudioChannels,SamplesList)), SLOT (updateBuffers(AudioChannels,SamplesList)), Qt::QueuedConnection);
//    connect(m_inputBuffer, SIGNAL(samplesReceived(AudioChannels,SamplesList)), SIGNAL(dataForOscilloscope(AudioChannels,SamplesList)), Qt::QueuedConnection);
    m_inputBuffer->setOffsets(m_leftChannelOffset, m_rightChannelOffset);
    connect(m_inputBuffer, SIGNAL(samplesReceived(SamplesList, SamplesList)), SIGNAL(dataForOscilloscope(SamplesList, SamplesList)), Qt::QueuedConnection);
    emit prepared();

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
                m_audioInput = nullptr;
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
    if (newState == QAudio::ActiveState) {
        setVolume(m_systemVolume);
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
    for (const QAudioDeviceInfo &info : QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
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
        for (const QString &fullName : devicesFullNames) {
            if (fullName.startsWith(name)) {
                description = fullName;
                if (g_verboseOutput) {
                    qDebug() << name << "--" << description;
                }
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
            if (g_verboseOutput) {
                qDebug() << name << "--" << description;
            }
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
        bool found = false;
        for (int i = 0; i < devices.count(); i++) {
            QPair<QString, QString> &deviceInfo = devices[i];
            if (deviceInfo.first == defaultDevice.deviceName()) {
                deviceInfo.second = "* " + deviceInfo.second;
                found = true;
                break;
            }
        }
        if (!found) {
            qWarning() << "Couldn't found default input device, use first available input device";
            if (!devices.isEmpty()) {
                QPair<QString, QString> &deviceInfo = devices.first();
                deviceInfo.second = "* " + deviceInfo.second;
            }
        }
    }
    
    if (g_verboseOutput) {
        qDebug() << "Detected audio input devices:" << devices;
    }
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
        if (g_verboseOutput) {
            qDebug() << "Audio device" << pulseAudioDeviceName << "is linked with alsa card -" << alsaCardIndex;
        }
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
        if (g_verboseOutput) {
            qDebug() << "Audio device" << getDeviceName() << "doesn't have ports (PulseAudio)";
        }
        return ports;
    }
    if ((nextCardPos > -1) && (nextCardPos < portsPos)) {
        if (g_verboseOutput) {
            qDebug() << "Audio device" << getDeviceName() << "doesn't have ports (PulseAudio)";
        }
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
    if (g_verboseOutput) {
        qDebug() << "Audio device" << getDeviceName() << "has ports (PulseAudio):" << paPorts;
    }

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
                if (g_verboseOutput) {
                    qDebug() << "Audio device" << getDeviceName() << "doesn't have control field \"PCM Capture Source\"";
                }
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
    if (g_verboseOutput) {
        qDebug() << "Audio device" << getDeviceName() << "has ports (Alsa):" << alsaPorts;
    }

    for (const QString &alsaPort : alsaPorts) {
        for (const QString &paPort, paPorts) {
            if (paPort.startsWith(alsaPort, Qt::CaseInsensitive)) {
                m_portsMap.insert(alsaPort, QString("analog-input-%1").arg(paPort));
                break;
            }
        }
    }
    if (g_verboseOutput) {
        qDebug() << "Audio device" << getDeviceName() << "has ports (Total):" << m_portsMap;
    }

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
    if (g_verboseOutput) {
        qDebug() << "Audio device" << getDeviceName() << "has ports for choosing:" << ports;
    }

    return ports;
#endif
}

void AudioInputThread::setMaxInputVoltage(qreal maxInputVoltage)
{
    Q_ASSERT(m_inputBuffer);
    if (m_inputBuffer == nullptr) {
        qCritical() << "Couldn't set capture max input voltage. m_inputBuffer is nullptr";
        return;
    }
    m_maxInputVoltage = maxInputVoltage;
    m_inputBuffer->setVoltageScaleFactor(m_maxInputVoltage * m_amplifyFactor);
}

void AudioInputThread::setAmplifyFactor(qreal amplifyFactor)
{
    Q_ASSERT(m_inputBuffer);
    if (m_inputBuffer == nullptr) {
        qCritical() << "Couldn't set capture amplify factor. m_inputBuffer is nullptr";
        return;
    }
    m_amplifyFactor = amplifyFactor;
    m_inputBuffer->setVoltageScaleFactor(m_maxInputVoltage * m_amplifyFactor);
    Settings *settings = Settings::getSettings();
    settings->setValue("Capture/VoltageAmplifyFactor", m_amplifyFactor);
}

qreal AudioInputThread::getAmplifyFactor()
{
    return m_amplifyFactor;
}

void AudioInputThread::setChannelOffset(AudioChannels channel, qreal offset)
{
    Q_ASSERT(m_inputBuffer);
    if (m_inputBuffer == nullptr) {
        qCritical() << "Couldn't set capture channel offset. m_inputBuffer is nullptr";
        return;
    }
    QString settingsItem;
    if (channel == CHANNEL_LEFT) {
        m_leftChannelOffset = offset;
        settingsItem = "Capture/LeftChannelOffset";
    } else if (channel == CHANNEL_RIGHT) {
        m_rightChannelOffset = offset;
        settingsItem = "Capture/RightChannelOffset";
    } else {
        qWarning() << "Couldn't set offset" << offset << "for incorrect input channel:" << channel;
        Q_ASSERT(false);
        return;
    }
    m_inputBuffer->setOffsets(m_leftChannelOffset, m_rightChannelOffset);
    Settings *settings = Settings::getSettings();
    settings->setValue(settingsItem, offset);
}

qreal AudioInputThread::getChannelOffset(AudioChannels channel)
{
    if (channel == CHANNEL_LEFT) {
        return m_leftChannelOffset;
    } else if (channel == CHANNEL_RIGHT) {
        return m_rightChannelOffset;
    } else {
        qWarning() << "Couldn't get offset for incorrect input channel:" << channel;
        Q_ASSERT(false);
        return 0.0;
    }
}

#if defined(_WIN32)
DEFINE_PROPERTYKEY(PKEY_Device_FriendlyName, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 14);

IAudioEndpointVolume *getAudioEndpointVolume(QString deviceName)
{
    IMMDeviceEnumerator *deviceEnumerator = nullptr;
    IMMDeviceCollection *pCollection = nullptr;
    HRESULT hr;
    IAudioEndpointVolume *endpointVolume = nullptr;
    bool foundEndpoint = false;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);
    if (FAILED(hr)) {
        qWarning() << "Couldn't create instance of MMDeviceEnumerator";
        goto clear_res;
    }
    hr = deviceEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE | DEVICE_STATE_DISABLED, &pCollection);
    if (FAILED(hr)) {
        qWarning() << "Couldn't enumerate Capture AudioEndpoints";
        goto clear_res;
    }
    UINT count;
    hr = pCollection->GetCount(&count);
    if (FAILED(hr)) {
        qWarning() << "Couldn't get count of Capture AudioEndpoints";
        goto clear_res;
    }
    for (ULONG i = 0; i < count; i++) {
        IMMDevice *pEndpoint = nullptr;
        IPropertyStore *pProps = nullptr;
        endpointVolume = nullptr;
        LPWSTR pwszID = nullptr;
        PROPVARIANT varName;
        QString pointName;
        // Initialize container for property value.
        PropVariantInit(&varName);
        // Get pointer to endpoint number i.
        hr = pCollection->Item(i, &pEndpoint);
        if (FAILED(hr)) {
            qWarning() << "Couldn't get AudioEndpoint at" << i;
            goto endpoint_clear;
        }
        // Get the endpoint ID string.
        hr = pEndpoint->GetId(&pwszID);
        if (FAILED(hr)) {
            qWarning() << "Couldn't get AudioEndpoint's ID string at" << i;
            goto endpoint_clear;
        }
        hr = pEndpoint->OpenPropertyStore(STGM_READ, &pProps);
        if (FAILED(hr)) {
            qWarning() << "Couldn't read properties for AudioEndpoint at" << i;
            goto endpoint_clear;
        }
        hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
        if (FAILED(hr)) {
            qWarning() << "Couldn't get friendly name of AudioEndpoint at" << i;
            goto endpoint_clear;
        }
        pointName = QString::fromWCharArray(varName.pwszVal);
        if (pointName.startsWith(deviceName)) {
            hr = pEndpoint->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, nullptr, (LPVOID *)&endpointVolume);
            if (FAILED(hr)) {
                qWarning() << "Couldn't get access to AudioEndpoint at" << i << "name:" << pointName;
                goto endpoint_clear;
            }
            foundEndpoint = true;
        }

endpoint_clear:
        if (pwszID != nullptr) {
            CoTaskMemFree(pwszID);
        }
        PropVariantClear(&varName);
        if (pProps != nullptr) {
            pProps->Release();
        }
        if (pEndpoint != nullptr) {
            pEndpoint->Release();
        }
        if (foundEndpoint) {
            break;
        } else {
            if (endpointVolume != nullptr) {
                endpointVolume->Release();
            }
        }
    }
clear_res:
    if (deviceEnumerator != nullptr) {
        deviceEnumerator->Release();
    }
    if (pCollection != nullptr) {
        pCollection->Release();
    }
    return endpointVolume;
}

#endif

void AudioInputThread::getVolume(int &baseVolume, int &curVolume, int &maxVolume)
{
    baseVolume = -1;
    curVolume = -1;
    maxVolume = -1;
#if defined(_WIN32)
    IAudioEndpointVolume *endpointVolume = getAudioEndpointVolume(getDeviceName());
    if (endpointVolume != nullptr) {
        float currentVolume = 0;
        HRESULT hr = endpointVolume->GetMasterVolumeLevelScalar(&currentVolume);
        endpointVolume->Release();
        if (FAILED(hr)) {
            qWarning() << "Couldn't get master volume level of AudioEndpoint"
                       << getDeviceName() << QString("hresult: 0x%1").arg(hr, 8, 16, QLatin1Char('0'));
            Q_ASSERT(false);
            return;
        } else {
            qDebug() << "Endpoint" << getDeviceName() << "- current volume as a scalar is:" << currentVolume;
            baseVolume = 1;
            curVolume = qRound(currentVolume * 100);
            maxVolume = 100;
        }
    } else {
        qWarning() << "Couldn't find AudioEndpoint for device" << getDeviceName();
        Q_ASSERT(false);
        return;
    }
#else
    QProcess pacmd;
    pacmd.start("pacmd", QStringList() << "list-sources", QIODevice::ReadOnly);
    QString pacmdOutput = "";
    if (!pacmd.waitForFinished()) {
        qWarning() << "Command \"pacmd list-sources\" is invalid";
        Q_ASSERT(false);
        return;
    } else {
        if ((pacmd.exitStatus() != QProcess::NormalExit) || (pacmd.exitCode() != 0)) {
            qWarning() << "Command \"pacmd list-sources\" was interrupted";
            Q_ASSERT(false);
            return;
        } else {
            pacmdOutput = pacmd.readAll();
        }
    }
    /*
      index: 3
            name: <alsa_input.pci-0000_00_1b.0.analog-stereo>
            volume: front-left: 65222 / 100% / -0.13 dB,   front-right: 65222 / 100% / -0.13 dB
                    balance 0.00
            base volume: 42869 /  65% / -11.06 dB
            volume steps: 65537
      or
    * index: 1
            name: <alsa_input.usb-046d_0825_36D88820-02.analog-mono>
            volume: mono: 42703 /  65% / -11.16 dB
                    balance 0.00
            base volume: 20724 /  32% / -30.00 dB
            volume steps: 65537
    */
    QString pattern = QRegExp::escape(getDeviceName()) + QString(".*base\\svolume:\\s*(\\d+)\\s");
    QRegExp rx(pattern, Qt::CaseSensitive, QRegExp::RegExp2);
    rx.setMinimal(true);
    int pos = rx.indexIn(pacmdOutput);
    if (pos > -1) {
        QString volume = rx.cap(1);
        bool ok = false;
        baseVolume = volume.toInt(&ok);
        if (!ok) {
            qWarning() << "Couldn't get integer value from base volume" << volume << "for audio device" << getDeviceName();
            Q_ASSERT(false);
            return;
        } else {
            if (g_verboseOutput) {
                qDebug() << "Audio device" << getDeviceName() << "has base volume -" << baseVolume;
            }
        }
    } else {
        qWarning() << "Couldn't find base volume for audio device" << getDeviceName();
        Q_ASSERT(false);
        return;
    }
    pattern = QRegExp::escape(getDeviceName()) + QString(".*\\tvolume:\\s*(front-left|mono):\\s*(\\d+)\\s");
    rx.setPattern(pattern);
    pos = rx.indexIn(pacmdOutput);
    if (pos > -1) {
        QString volume = rx.cap(2);
        bool ok = false;
        curVolume = volume.toInt(&ok);
        if (!ok) {
            qWarning() << "Couldn't get integer value from current volume" << volume << "for audio device" << getDeviceName();
            Q_ASSERT(false);
            return;
        } else {
            if (g_verboseOutput) {
                qDebug() << "Audio device" << getDeviceName() << "has current volume -" << curVolume;
            }
        }
    } else {
        qWarning() << "Couldn't find current volume for audio device" << getDeviceName();
        Q_ASSERT(false);
        return;
    }
    pattern = QRegExp::escape(getDeviceName()) + QString(".*\\svolume\\ssteps:\\s*(\\d+)\\b");
    rx.setPattern(pattern);
    pos = rx.indexIn(pacmdOutput);
    if (pos > -1) {
        QString volume = rx.cap(1);
        bool ok = false;
        maxVolume = volume.toInt(&ok);
        if (!ok) {
            qWarning() << "Couldn't get integer value from max volume" << volume << "for audio device" << getDeviceName();
            Q_ASSERT(false);
            return;
        } else {
            if (g_verboseOutput) {
                qDebug() << "Audio device" << getDeviceName() << "has max volume -" << maxVolume;
            }
        }
        maxVolume = maxVolume * 2;
    } else {
        qWarning() << "Couldn't find max volume for audio device" << getDeviceName();
        Q_ASSERT(false);
        return;
    }
#endif
    if (baseVolume <= 0) {
        m_systemVolume = 0;
    } else {
        Settings *settings = Settings::getSettings();
        m_systemVolume = settings->value("Capture/SystemVolume", baseVolume).toInt();
        curVolume = m_systemVolume;
    }
}

void AudioInputThread::setVolume(int volume)
{
    qDebug() << "Set volume for audio input device to" << volume;

#if defined(_WIN32)
    IAudioEndpointVolume *endpointVolume = getAudioEndpointVolume(getDeviceName());
    if (endpointVolume != nullptr) {
        float newVolume = volume / 100.f;
        HRESULT hr = endpointVolume->SetMasterVolumeLevelScalar((float)newVolume, nullptr);
        endpointVolume->Release();
        if (FAILED(hr)) {
            qWarning() << "Couldn't get master volume level of AudioEndpoint"
                       << getDeviceName() << QString("hresult: 0x%1").arg(hr, 8, 16, QLatin1Char('0'));
            Q_ASSERT(false);
            return;
        } else {
            if (g_verboseOutput) {
                qDebug() << "Audio source device's volume has been changed to" << volume;
            }
        }
    } else {
        qWarning() << "Couldn't find AudioEndpoint for device" << getDeviceName();
        Q_ASSERT(false);
        return;
    }
#else
    QProcess pacmd;
    pacmd.start("pacmd", QStringList() << "set-source-volume" << getDeviceName() << QString::number(volume), QIODevice::ReadOnly);
    if (!pacmd.waitForFinished(5000)) {
        qWarning() << "Command \"pacmd set-source-volume\" is invalid";
        Q_ASSERT(false);
        return;
    } else {
        if ((pacmd.exitStatus() != QProcess::NormalExit) || (pacmd.exitCode() != 0)) {
            qWarning() << "Command \"pacmd set-source-volume\" was interrupted";
            Q_ASSERT(false);
            return;
        } else {
            if (g_verboseOutput) {
                qDebug() << "Audio source device's volume has been changed to" << volume;
            }
        }
    }
#endif

    Settings *settings = Settings::getSettings();
    settings->setValue("Capture/SystemVolume", volume);
    m_systemVolume = volume;
}

void AudioInputThread::switchInputDevice(QString name)
{
    if (m_captureEnabled) {
        qWarning() << "Input audio device can't be changed - it is already in use";
        return;
    }
    qDebug() << "Switch audio input device to" << name;
    for (const QAudioDeviceInfo &info : m_audioDeviceInfos) {
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
