#include <QDebug>
#include "formanalyze.h"
#include "ui_formanalyze.h"
#include "common_types.h"
#include "ToneGenerator.h"

FormAnalyze::FormAnalyze(ToneGenerator *gen, AudioInputThread *capture, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FormAnalyze)
{
    ui->setupUi(this);
    m_gen = gen;
    m_capture = capture;

    connect(ui->boxFrequency, SIGNAL(valueChanged(int)), this, SLOT(setFrequency(int)));
    connect(ui->sliderFrequency, SIGNAL(valueChanged(int)), this, SLOT(setFrequency(int)));
    connect(ui->boxVoltage, SIGNAL(valueChanged(double)), this, SLOT(setVoltage(double)));
    connect(ui->sliderVoltage, SIGNAL(valueChanged(int)), this, SLOT(setVoltage(int)));
    setFrequency(440);
    setVoltage(0.5);

    ui->boxWaveForm->addItem(QIcon(":/icons/oscillator_sine.png"), "Sine", QVariant(WAVE_SINE));
    ui->boxWaveForm->addItem(QIcon(":/icons/oscillator_square.png"), "Square", QVariant(WAVE_SQUARE));
    ui->boxWaveForm->addItem(QIcon(":/icons/oscillator_saw.png"), "Sawtooth", QVariant(WAVE_SAWTOOTH));
    ui->boxWaveForm->addItem(QIcon(":/icons/oscillator_triangle.png"), "Triangle", QVariant(WAVE_TRIANGLE));
    connect(ui->boxWaveForm, SIGNAL(currentIndexChanged(int)), this, SLOT(switchOutputWaveForm()));

    connect(ui->buttonRun, SIGNAL(clicked(bool)), this, SLOT(runAnalyze(bool)));
    connect(ui->buttonSave, SIGNAL(clicked()), this, SLOT(saveSignature()));

    connect(m_capture, SIGNAL (initiated (int)),
             SLOT (captureDeviceInitiated (int)), Qt::QueuedConnection); // wait while main window initiated
    connect(m_capture, SIGNAL(dataForOscilloscope(SamplesList,SamplesList)),
            this, SLOT(processOscilloscopeData(SamplesList,SamplesList)));
}

FormAnalyze::~FormAnalyze()
{
    delete ui;
}

void FormAnalyze::leaveForm()
{
    qDebug() << "Leave form \"Analyze\"";
    ui->buttonRun->setChecked(false);
    runAnalyze(false);
}

void FormAnalyze::setFrequency(int frequency)
{
    if (ui->boxFrequency->value() != frequency) {
        ui->boxFrequency->setValue(frequency);
    }
    if (ui->sliderFrequency->value() != frequency) {
        ui->sliderFrequency->setValue(frequency);
    }
    if (ui->buttonRun->isChecked()) {
        m_gen->changeFrequency(frequency);
    }
}

void FormAnalyze::setVoltage(double voltage)
{
    int v = qRound(voltage * 10.);
    double curV = v / 10.;
    int prevV = qRound(ui->boxVoltage->value() * 10.);
    if (prevV != v) {
        ui->boxVoltage->setValue(curV);
    }
    if (ui->sliderVoltage->value() != v) {
        ui->sliderVoltage->setValue(v);
    }
}

void FormAnalyze::setVoltage(int vol10)
{
    setVoltage(vol10 / 10.);
}

void FormAnalyze::switchOutputWaveForm()
{
    int index = ui->boxWaveForm->currentIndex();
    QVariant data = ui->boxWaveForm->itemData(index);
    ToneWaveForm form = (ToneWaveForm)data.toInt();
    if (ui->buttonRun->isChecked()) {
        m_gen->switchWaveForm(form);
    }
}

void FormAnalyze::runAnalyze(bool start)
{
    qDebug() << "Run analyze:" << start;
    if (start) {
        switchOutputWaveForm();
        int frequency = ui->boxFrequency->value();
        m_gen->changeFrequency(frequency);
    }
    m_gen->runGenerator(start);
    m_capture->startCapturing(start);
}

void FormAnalyze::saveSignature()
{
    ui->viewSignature->saveView();
}

void FormAnalyze::captureDeviceInitiated(int samplingRate)
{
    Q_UNUSED(samplingRate);
    if (!ui->buttonRun->isChecked()) {
        return;
    }
    // Audio device ready to capture - display this
    Q_ASSERT(m_capture);
    m_capture->setCapturedChannels(CHANNEL_BOTH);
}

void FormAnalyze::processOscilloscopeData(SamplesList leftChannelData, SamplesList rightChannelData)
{
    if (!ui->buttonRun->isChecked()) {
        return;
    }
    QVector<double> voltage;
    voltage = QVector<double>::fromList(leftChannelData);
    QVector<double> current;
    current = QVector<double>::fromList(rightChannelData);
    ui->viewSignature->draw(voltage, current);
}
