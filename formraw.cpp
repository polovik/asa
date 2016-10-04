#include <QDebug>
#include "formraw.h"
#include "ui_formraw.h"
#include "devices/tonegenerator.h"
#include "devices/audioinputdevice.h"
#include "widgets/oscilloscopeengine.h"
#include "common_types.h"
#include "devices/audioinputdevice.h"

extern bool g_verboseOutput;

FormRaw::FormRaw(ToneGenerator *gen, AudioInputThread *capture, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FormRaw)
{
    ui->setupUi(this);
    
    //  Tone generation
    m_gen = gen;
    connect(m_gen, SIGNAL(deviceReady(bool)), this, SLOT(switchOutputFrequency()));
    connect(m_gen, SIGNAL(deviceReady(bool)), this, SLOT(switchOutputWaveForm()));
    ui->boxWaveForm->addItem(QIcon(":/icons/oscillator_sine.png"), "Sine", QVariant(ToneWaveForm::WAVE_SINE));
    ui->boxWaveForm->addItem(QIcon(":/icons/oscillator_square.png"), "Square", QVariant(ToneWaveForm::WAVE_SQUARE));
    ui->boxWaveForm->addItem(QIcon(":/icons/oscillator_saw.png"), "Sawtooth", QVariant(ToneWaveForm::WAVE_SAWTOOTH));
    ui->boxWaveForm->addItem(QIcon(":/icons/oscillator_triangle.png"), "Triangle", QVariant(ToneWaveForm::WAVE_TRIANGLE));
    
    // Audio capture
    m_capture = capture;
    connect(m_capture, SIGNAL(initiated(int)),
            SLOT(captureDeviceInitiated(int)), Qt::QueuedConnection);   // wait while main window initiated
            
    // Oscilloscope
    m_oscEngine = new OscilloscopeEngine(ui->oscilloscope);
    connect(m_capture, SIGNAL(dataForOscilloscope(SamplesList, SamplesList)),
            m_oscEngine, SLOT(processOscilloscopeData(SamplesList, SamplesList)));

    changeTriggerSettings();
    connect(ui->boxTriggerAuto, SIGNAL(clicked()), this, SLOT(changeTriggerSettings()));
    connect(ui->boxTriggerNormal, SIGNAL(clicked()), this, SLOT(changeTriggerSettings()));
    connect(ui->boxTriggerSingle, SIGNAL(clicked()), this, SLOT(changeTriggerSettings()));
    connect(ui->boxSlopeRising, SIGNAL(clicked()), this, SLOT(changeTriggerSettings()));
    connect(ui->boxSlopeFalling, SIGNAL(clicked()), this, SLOT(changeTriggerSettings()));
    connect(ui->boxTriggerChannel, SIGNAL(currentIndexChanged(int)), this, SLOT(changeTriggerSettings()));
    
    connect(ui->buttonGenerate, SIGNAL(toggled(bool)), this, SLOT(startToneGenerator(bool)));
    connect(ui->boxFrequency, SIGNAL(valueChanged(int)), this, SLOT(switchOutputFrequency()));
    connect(ui->boxWaveForm, SIGNAL(currentIndexChanged(int)), this, SLOT(switchOutputWaveForm()));
    
    connect(ui->buttonCapture, SIGNAL(toggled(bool)), this, SLOT(startAudioCapture(bool)));
    connect(ui->boxChannelLeft, SIGNAL(toggled(bool)), this, SLOT(changeCapturedChannels()));
    connect(ui->boxChannelRight, SIGNAL(toggled(bool)), this, SLOT(changeCapturedChannels()));
}

FormRaw::~FormRaw()
{
    delete ui;
}

void FormRaw::enterForm()
{
    QString inputName = m_capture->getDeviceName();
    QString outputName = m_gen->getDeviceName();
    ui->labelAudioInputDevice->setText(inputName);
    ui->labelAudioOutputDevice->setText(outputName);
    qreal max = m_gen->getMaxVoltageAmplitude();
    m_oscEngine->setMaximumAmplitude(max);
}

void FormRaw::leaveForm()
{
    qDebug() << "Leave form \"Raw\"";
    startToneGenerator(false);
    startAudioCapture(false);
    ui->buttonGenerate->setChecked(false);
    ui->buttonCapture->setChecked(false);
    m_oscEngine->stop();
}

void FormRaw::startToneGenerator(bool start)
{
    if (start) {
        switchOutputWaveForm();
        switchOutputFrequency();
        m_gen->setActiveChannels(CHANNEL_BOTH);
        m_gen->setCurVoltageAmplitude(m_gen->getMaxVoltageAmplitude());
    }
    m_gen->runGenerator(start);
}

void FormRaw::captureDeviceInitiated(int samplingRate)
{
    if (!ui->buttonCapture->isChecked()) {
        return;
    }
    // Audio device ready to capture - display this
    Q_ASSERT(m_capture);
    
    m_oscEngine->prepareToStart(samplingRate);
    changeCapturedChannels(); // Start receive data from input device
}

void FormRaw::startAudioCapture(bool start)
{
    m_capture->startCapturing(start);
    if (start == false) {
        m_oscEngine->stop();
    }
}

void FormRaw::switchOutputWaveForm()
{
    int index = ui->boxWaveForm->currentIndex();
    QVariant data = ui->boxWaveForm->itemData(index);
    ToneWaveForm form((ToneWaveForm::Id)data.toInt());
    if (ui->buttonGenerate->isChecked()) {
        m_gen->switchWaveForm(form);
    }
}

void FormRaw::switchOutputFrequency()
{
    int freq = ui->boxFrequency->value();
    if (ui->buttonGenerate->isChecked()) {
        m_gen->changeFrequency(freq);
    }
}

void FormRaw::changeTriggerSettings()
{
    OscTriggerMode mode = TRIG_AUTO;
    if (ui->boxTriggerAuto->isChecked()) {
        mode = TRIG_AUTO;
    } else if (ui->boxTriggerNormal->isChecked()) {
        mode = TRIG_NORMAL;
    } else {
        mode = TRIG_SINGLE;
    }
    m_oscEngine->setTriggerMode(mode);

    OscTriggerSlope slope;
    if (ui->boxSlopeRising->isChecked())
        slope = TRIG_RISING;
    else
        slope = TRIG_FALLING;
    m_oscEngine->setTriggerSlope(slope);
    
    AudioChannels channel;
    if (ui->boxTriggerChannel->currentIndex() == 0) {
        channel = CHANNEL_LEFT;
    } else if (ui->boxTriggerChannel->currentIndex() == 1) {
        channel = CHANNEL_RIGHT;
    } else {
        qWarning() << "Incorrect channel selected for tracking:" << ui->boxTriggerChannel->currentIndex();
        Q_ASSERT(false);
    }
    m_oscEngine->setTriggerChannel(channel);
}

void FormRaw::changeCapturedChannels()
{
    int channels = CHANNEL_NONE;
    if (ui->boxChannelLeft->isChecked()) {
        channels |= CHANNEL_LEFT;
    }
    if (ui->boxChannelRight->isChecked()) {
        channels |= CHANNEL_RIGHT;
    }
    m_oscEngine->setDisplayedChannels((AudioChannels)channels);
    m_capture->setCapturedChannels((AudioChannels)channels);
}
