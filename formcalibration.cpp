#include <QDebug>
#include "formcalibration.h"
#include "ui_formcalibration.h"
#include "devices/tonegenerator.h"
#include "devices/audioinputdevice.h"
#include "widgets/oscilloscopeengine.h"

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
    foreach(QString name, outputDeviceNames) {
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
    QList<QPair<QString, QString>> inputDeviceNames = m_capture->enumerateDevices();
    QString inputDeviceName;
    ui->boxAudioInputDevice->clear();
    index = 0;
    for (int i = 0; i < inputDeviceNames.count(); i++) {
        QString name = inputDeviceNames.at(i).first;
        QString description = inputDeviceNames.at(i).second;
        if (description.startsWith("* ")) {
            inputDeviceName = name;
            ui->boxAudioInputDevice->addItem(description, QVariant(inputDeviceName));
            ui->boxAudioInputDevice->setCurrentIndex(index);
            switchInputAudioDevice(index);
        } else {
            ui->boxAudioInputDevice->addItem(description, QVariant(name));
        }
        index++;
    }
    connect(ui->boxAudioInputDevice, SIGNAL(currentIndexChanged(int)), this, SLOT(switchInputAudioDevice(int)));
    
    connect(ui->buttonLeftOutputChannel, SIGNAL(clicked()), this, SLOT(playTestTone()));
    connect(ui->buttonRightOutputChannel, SIGNAL(clicked()), this, SLOT(playTestTone()));
    connect(ui->buttonCalibrate, SIGNAL(clicked(bool)), this, SLOT(runCalibration(bool)));
    connect(m_capture, SIGNAL(initiated(int)),
            SLOT(captureDeviceInitiated(int)), Qt::QueuedConnection);   // wait while main window initiated
    connect(m_capture, SIGNAL(dataForOscilloscope(SamplesList, SamplesList)),
            this, SLOT(processOscilloscopeData(SamplesList, SamplesList)));

    m_oscEngine = new OscilloscopeEngine(ui->oscilloscope);
    m_oscEngine->setTriggerMode(TRIG_AUTO);
    m_oscEngine->setDisplayedChannels(CHANNEL_BOTH);

    qreal magnitude = m_gen->getMaxVoltageAmplitude();
    double rms = magnitude / qSqrt(2);
    ui->boxGeneratorPeak->setValue(magnitude);
    ui->boxGeneratorRMS->setValue(rms);
    qDebug() << "Load max generator voltage: Vpk" << magnitude << "Vrms" << rms;
    connect(ui->boxGeneratorPeak, SIGNAL(valueChanged(double)), this, SLOT(setGeneratorMagnitudePeak(double)));
    connect(ui->boxGeneratorRMS, SIGNAL(valueChanged(double)), this, SLOT(setGeneratorMagnitudeRMS(double)));
    setGeneratorMagnitude(magnitude);

    connect(ui->buttonHintCalibrate, SIGNAL(clicked()), this, SLOT(showHint()));
    connect(ui->buttonHintPlayTone, SIGNAL(clicked()), this, SLOT(showHint()));
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
    ui->buttonCalibrate->setChecked(false);
    m_oscEngine->stop();
}

void FormCalibration::switchOutputAudioDevice(int index)
{
    for (int i = 0; i < ui->boxAudioOutputDevice->count(); i++) {
        QString text = ui->boxAudioOutputDevice->itemText(i);
        if (text.startsWith("* ")) {
            text.remove(0, 2);
            ui->boxAudioOutputDevice->setItemText(i, text);
        }
    }
    QVariant cleanName = ui->boxAudioOutputDevice->itemData(index);
    QString name = cleanName.toString();
    ui->boxAudioOutputDevice->setItemText(index, "* " + name);
    m_gen->switchOutputDevice(name);
}

void FormCalibration::switchInputAudioDevice(int index)
{
    for (int i = 0; i < ui->boxAudioInputDevice->count(); i++) {
        QString text = ui->boxAudioInputDevice->itemText(i);
        if (text.startsWith("* ")) {
            text.remove(0, 2);
            ui->boxAudioInputDevice->setItemText(i, text);
        }
    }
    QString curText = ui->boxAudioInputDevice->itemText(index);
    ui->boxAudioInputDevice->setItemText(index, "* " + curText);

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
    m_gen->switchWaveForm(ToneWaveForm::WAVE_SINE);
    m_gen->setActiveChannels((AudioChannels)channels);
    m_gen->setCurVoltageAmplitude(m_gen->getMaxVoltageAmplitude());
    m_gen->runGenerator(true);
}

void FormCalibration::captureDeviceInitiated(int samplingRate)
{
    if (!ui->buttonCalibrate->isChecked()) {
        return;
    }
    // Audio device ready to capture - display this
    ui->viewLeftChannelLevel->setSamplingRate(samplingRate);
    ui->viewRightChannelLevel->setSamplingRate(samplingRate);
    m_oscEngine->prepareToStart(samplingRate);
    Q_ASSERT(m_capture);
    m_capture->setCapturedChannels(CHANNEL_BOTH);
}

void FormCalibration::processOscilloscopeData(SamplesList leftChannelData, SamplesList rightChannelData)
{
    if (!ui->buttonCalibrate->isChecked()) {
        return;
    }
    ui->viewLeftChannelLevel->processSamples(leftChannelData);
    ui->viewRightChannelLevel->processSamples(rightChannelData);
    m_oscEngine->processOscilloscopeData(leftChannelData, rightChannelData);
}

void FormCalibration::runCalibration(bool start)
{
    qDebug() << "Run checking voltage on Audio input:" << start;
    if (start) {
        m_gen->changeFrequency(50);
        m_gen->switchWaveForm(ToneWaveForm::WAVE_SINE);
        m_gen->setActiveChannels(CHANNEL_BOTH);
        m_gen->setCurVoltageAmplitude(m_gen->getMaxVoltageAmplitude());
        ui->buttonLeftOutputChannel->setChecked(false);
        ui->buttonRightOutputChannel->setChecked(false);
    }
    m_capture->startCapturing(start);
    m_gen->runGenerator(start);
    if (start == false) {
        m_oscEngine->stop();
    }
}

void FormCalibration::setGeneratorMagnitudePeak(double voltage)
{
    // for avoid recursion, stop processing signal "valueChanged" until UI boxes are updated
    disconnect(ui->boxGeneratorRMS, SIGNAL(valueChanged(double)), this, SLOT(setGeneratorMagnitudeRMS(double)));

    double peak = voltage;
    double rms = peak / qSqrt(2);
    qDebug() << "Set max generator voltage: Vpk" << peak << "Vrms" << rms;
    ui->boxGeneratorRMS->setValue(rms);
    setGeneratorMagnitude(peak);

    connect(ui->boxGeneratorRMS, SIGNAL(valueChanged(double)), this, SLOT(setGeneratorMagnitudeRMS(double)));
}

void FormCalibration::setGeneratorMagnitudeRMS(double voltage)
{
    // for avoid recursion, stop processing signal "valueChanged" until UI boxes are updated
    disconnect(ui->boxGeneratorPeak, SIGNAL(valueChanged(double)), this, SLOT(setGeneratorMagnitudePeak(double)));

    double rms = voltage;
    double peak = rms * qSqrt(2);
    qDebug() << "Set max generator voltage: Vpk" << peak << "Vrms" << rms;
    ui->boxGeneratorPeak->setValue(peak);
    setGeneratorMagnitude(peak);

    connect(ui->boxGeneratorPeak, SIGNAL(valueChanged(double)), this, SLOT(setGeneratorMagnitudePeak(double)));
}

void FormCalibration::setGeneratorMagnitude(qreal peak)
{
    m_gen->setMaxVoltageAmplitude(peak);
    m_gen->setCurVoltageAmplitude(peak);
    m_capture->setSensivity(peak);
    m_oscEngine->setMaximumAmplitude(peak);
    ui->viewLeftChannelLevel->setMaximumAmplitude(peak);
    ui->viewRightChannelLevel->setMaximumAmplitude(peak);
}

void FormCalibration::showHint()
{
    QString title;
    QString text;
    if (sender() == ui->buttonHintCalibrate) {
        title = tr("Perform calibration");
        text = tr("This area helps to adjust maximal voltage on audio input pins.\n"
                  "Button pressing leads to play tone on left and right channel simultaneously.\n"
                  "Tone is Sine 50Hz with maximal amplitude.\n"
                  "Potentiometers on the board have to be adjusted for making the same voltage "
                  "on both audio input pins.\n"
                  "For correct signature displaying, it needs to specify range of axes."
                  "Measured voltage should be written in corresponding field:\n"
                  "- Amplitude (Vpk) - when real oscilloscope is used\n"
                  "- RMS - when ordinary voltmeter for ~U is used");
    } else if (sender() == ui->buttonHintPlayTone) {
        title = tr("Play tone");
        text = tr("This buttons help to distinguish audio output pins.\n"
               "Button pressing leads to play tone on specific channel.\n"
               "Tone is Sine 50Hz with maximal amplitude.");
    }
    QMessageBox::information(this, title, text);
}
