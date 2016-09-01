#include <QDebug>
#include "formcalibration.h"
#include "ui_formcalibration.h"
#include "devices/tonegenerator.h"
#include "devices/audioinputdevice.h"

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
    QStringList inputDeviceNames = m_capture->enumerateDevices();
    QString inputDeviceName;
    ui->boxAudioInputDevice->clear();
    index = 0;
    foreach(QString name, inputDeviceNames) {
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
    connect(ui->buttonAdjustGenerator, SIGNAL(clicked(bool)), this, SLOT(adjustGenerator(bool)));
//    connect(ui->buttonGenerate, SIGNAL(toggled(bool)), ui->boxAudioOutputDevice, SLOT(setDisabled(bool)));
//    connect(ui->buttonCupture, SIGNAL(toggled(bool)), ui->boxAudioInputDevice, SLOT(setDisabled(bool)));
    connect(m_capture, SIGNAL(initiated(int)),
            SLOT(captureDeviceInitiated(int)), Qt::QueuedConnection);   // wait while main window initiated
    connect(m_capture, SIGNAL(dataForOscilloscope(SamplesList, SamplesList)),
            this, SLOT(processOscilloscopeData(SamplesList, SamplesList)));
            
    qreal magnitude = m_gen->getMaxVoltageAmplitude();
    connect(ui->boxGeneratorPeak, SIGNAL(valueChanged(double)), this, SLOT(setGeneratorMagnitude(double)));
    connect(ui->boxGeneratorRMS, SIGNAL(valueChanged(double)), this, SLOT(setGeneratorMagnitude(double)));
    setGeneratorMagnitude(magnitude);

    connect(ui->buttonHintAdjustGenerator, SIGNAL(clicked()), this, SLOT(showHint()));
    connect(ui->buttonHintCheckInputLevel, SIGNAL(clicked()), this, SLOT(showHint()));
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
    ui->buttonCheckInputLevel->setChecked(false);
    ui->buttonAdjustGenerator->setChecked(false);
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
    QVariant cleanName = ui->boxAudioInputDevice->itemData(index);
    QString name = cleanName.toString();
    ui->boxAudioInputDevice->setItemText(index, "* " + name);
    m_capture->switchInputDevice(name);
}

void FormCalibration::playTestTone()
{
    if (!ui->buttonLeftOutputChannel->isChecked() && !ui->buttonRightOutputChannel->isChecked()) {
        m_gen->runGenerator(false);
        return;
    }
    ui->buttonAdjustGenerator->setChecked(false);
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
    QVector<double> voltage;
    voltage = QVector<double>::fromList(leftChannelData);
    QVector<double> current;
    current = QVector<double>::fromList(rightChannelData);
    ui->viewSignature->draw(voltage, current);
}

void FormCalibration::checkInputLevel(bool start)
{
    qDebug() << "Run checking voltage on Audio input:" << start;
    if (start) {
    
    }
    m_capture->startCapturing(start);
}

void FormCalibration::adjustGenerator(bool start)
{
    if (start) {
        m_gen->changeFrequency(50);
        m_gen->switchWaveForm(ToneWaveForm::WAVE_SINE);
        m_gen->setActiveChannels(CHANNEL_BOTH);
        m_gen->setCurVoltageAmplitude(m_gen->getMaxVoltageAmplitude());
        ui->buttonLeftOutputChannel->setChecked(false);
        ui->buttonRightOutputChannel->setChecked(false);
    }
    m_gen->runGenerator(start);
}

void FormCalibration::setGeneratorMagnitude(double voltage)
{
    // for avoid recursion, stop processing signal "valueChanged" until UI boxes are updated
    disconnect(ui->boxGeneratorPeak, SIGNAL(valueChanged(double)), this, SLOT(setGeneratorMagnitude(double)));
    disconnect(ui->boxGeneratorRMS, SIGNAL(valueChanged(double)), this, SLOT(setGeneratorMagnitude(double)));

    double peak = -1.;
    double rms = -1.;
    if (sender() == ui->boxGeneratorPeak) {
        peak = voltage;
        rms = peak / qSqrt(2);
        ui->boxGeneratorRMS->setValue(rms);
    } else if (sender() == ui->boxGeneratorRMS) {
        rms = voltage;
        peak = rms * qSqrt(2);
        ui->boxGeneratorPeak->setValue(peak);
    } else {
        peak = voltage;
        rms = peak / qSqrt(2);
        ui->boxGeneratorPeak->setValue(peak);
        ui->boxGeneratorRMS->setValue(rms);
    }
    qDebug() << "Set max generator voltage: Vpk" << peak << "Vrms" << rms;
    m_gen->setMaxVoltageAmplitude(peak);
    m_gen->setCurVoltageAmplitude(peak);
    ui->viewSignature->setMaximumAmplitude(peak);

    connect(ui->boxGeneratorPeak, SIGNAL(valueChanged(double)), this, SLOT(setGeneratorMagnitude(double)));
    connect(ui->boxGeneratorRMS, SIGNAL(valueChanged(double)), this, SLOT(setGeneratorMagnitude(double)));
}

void FormCalibration::showHint()
{
    QString title;
    QString text;
    if (sender() == ui->buttonHintAdjustGenerator) {
        title = tr("Adjust max generated voltage");
        text = tr("This button helps to adjust tone generator's amplitude.\n"
               "Button pressing leads to play tone on left and right channel simultaneously.\n"
               "Tone is Sine 50Hz with maximal amplitude.\n"
               "Measured voltage should be written in corresponding field:\n"
               "- Amplitude (Vpk) - when real oscilloscope is used\n"
               "- RMS - when ordinary voltmeter for ~U is used");
    } else if (sender() == ui->buttonHintCheckInputLevel) {
        title = tr("Check input channel");
        text = tr("This area helps to adjust maximal voltage on audio input pins.\n"
               "Button pressing leads to play tone on left and right channel simultaneously.\n"
               "Tone is Sine 50Hz with maximal amplitude.\n"
               "Potentiometers on the board have to be adjusted for making the same voltage "
               "on both audio input pins.");
    } else if (sender() == ui->buttonHintPlayTone) {
        title = tr("Play tone");
        text = tr("This buttons help to distinguish audio output pins.\n"
               "Button pressing leads to play tone on specific channel.\n"
               "Tone is Sine 50Hz with maximal amplitude.");
    }
    QMessageBox::information(this, title, text);
}
