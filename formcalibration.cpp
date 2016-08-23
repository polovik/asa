#include <QDebug>
#include "formcalibration.h"
#include "ui_formcalibration.h"
#include "ToneGenerator.h"
#include "audioinputdevice.h"

FormCalibration::FormCalibration(ToneGenerator *gen, AudioInputThread *capture, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FormCalibration)
{
    ui->setupUi(this);
    m_gen = gen;
    m_capture = capture;

    //  Tone generation
    QStringList outputDeviceNames = m_gen->enumerateDevices();
    QString outputDeviceName;
    ui->boxAudioOutputDevice->clear();
    int index = 0;
    foreach (QString name, outputDeviceNames) {
        if (name.startsWith("* ")) {
            outputDeviceName = name;
            outputDeviceName.remove(0, 2);
            ui->boxAudioOutputDevice->addItem(name, QVariant(outputDeviceName));
            ui->boxAudioOutputDevice->setCurrentIndex(index);
            switchOutputAudioDevice(index);
        } else {
            ui->boxAudioOutputDevice->addItem(name, QVariant(name));
        }
        index++;
    }
    connect(ui->boxAudioOutputDevice, SIGNAL(currentIndexChanged(int)), this, SLOT(switchOutputAudioDevice(int)));

    // Audio capture
    QStringList inputDeviceNames = m_capture->enumerateDevices();
    QString inputDeviceName;
    ui->boxAudioInputDevice->clear();
    index = 0;
    foreach (QString name, inputDeviceNames) {
        if (name.startsWith("* ")) {
            inputDeviceName = name;
            inputDeviceName.remove(0, 2);
            ui->boxAudioInputDevice->addItem(name, QVariant(inputDeviceName));
            ui->boxAudioInputDevice->setCurrentIndex(index);
            switchInputAudioDevice(index);
        } else {
            ui->boxAudioInputDevice->addItem(name, QVariant(name));
        }
        index++;
    }
    connect(ui->boxAudioInputDevice, SIGNAL(currentIndexChanged(int)), this, SLOT(switchInputAudioDevice(int)));

    connect(ui->buttonLeftOutputChannel, SIGNAL(clicked()), this, SLOT(playTestTone()));
    connect(ui->buttonRightOutputChannel, SIGNAL(clicked()), this, SLOT(playTestTone()));
    connect(ui->buttonCheckInputLevel, SIGNAL(clicked(bool)), this, SLOT(checkInputLevel(bool)));
//    connect(ui->buttonGenerate, SIGNAL(toggled(bool)), ui->boxAudioOutputDevice, SLOT(setDisabled(bool)));
//    connect(ui->buttonCupture, SIGNAL(toggled(bool)), ui->boxAudioInputDevice, SLOT(setDisabled(bool)));
    connect(m_capture, SIGNAL (initiated (int)),
             SLOT (captureDeviceInitiated (int)), Qt::QueuedConnection); // wait while main window initiated
    connect(m_capture, SIGNAL(dataForOscilloscope(SamplesList,SamplesList)),
            this, SLOT(processOscilloscopeData(SamplesList,SamplesList)));
}

FormCalibration::~FormCalibration()
{
    delete ui;
}

void FormCalibration::leaveForm()
{
    qDebug() << "Leave form \"Calibration\"";
    m_gen->runGenerator(false);
    m_capture->startCapturing(false);
    ui->buttonLeftOutputChannel->setChecked(false);
    ui->buttonRightOutputChannel->setChecked(false);
    ui->buttonCheckInputLevel->setChecked(false);
}

void FormCalibration::switchOutputAudioDevice(int index)
{
    QVariant cleanName = ui->boxAudioOutputDevice->itemData(index);
    QString name = cleanName.toString();
    m_gen->switchOutputDevice(name);
}

void FormCalibration::switchInputAudioDevice(int index)
{
    QVariant cleanName = ui->boxAudioInputDevice->itemData(index);
    QString name = cleanName.toString();
    m_capture->switchInputDevice(name);
}

void FormCalibration::playTestTone()
{
    if (!ui->buttonLeftOutputChannel->isChecked() && !ui->buttonRightOutputChannel->isChecked()) {
        m_gen->runGenerator(false);
        return;
    }
    int channels = CHANNEL_NONE;
    if (ui->buttonLeftOutputChannel->isChecked()) {
        channels = CHANNEL_LEFT;
    }
    if (ui->buttonRightOutputChannel->isChecked()) {
        channels |= CHANNEL_RIGHT;
    }
    m_gen->changeFrequency(50);
    m_gen->switchWaveForm(WAVE_SINE);
    m_gen->setActiveChannels((AudioChannels)channels);
    m_gen->runGenerator(true);
}

void FormCalibration::captureDeviceInitiated(int samplingRate)
{
    if (!ui->buttonCheckInputLevel->isChecked()) {
        return;
    }
    // Audio device ready to capture - display this
    ui->viewLeftChannelLevel->setSamplingRate(samplingRate);
    ui->viewRightChannelLevel->setSamplingRate(samplingRate);
    Q_ASSERT(m_capture);
    m_capture->setCapturedChannels(CHANNEL_BOTH);
}

void FormCalibration::processOscilloscopeData(SamplesList leftChannelData, SamplesList rightChannelData)
{
    if (!ui->buttonCheckInputLevel->isChecked()) {
        return;
    }
    ui->viewLeftChannelLevel->processSamples(leftChannelData);
    ui->viewRightChannelLevel->processSamples(rightChannelData);
}

void FormCalibration::checkInputLevel(bool start)
{
    qDebug() << "Run checking voltage on Audio input:" << start;
    if (start) {

    }
    m_capture->startCapturing(start);
}
