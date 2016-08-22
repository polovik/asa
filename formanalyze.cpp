#include <QDebug>
#include "formanalyze.h"
#include "ui_formanalyze.h"
#include "common_types.h"

FormAnalyze::FormAnalyze(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FormAnalyze)
{
    ui->setupUi(this);

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
}

FormAnalyze::~FormAnalyze()
{
    delete ui;
}

void FormAnalyze::leaveForm()
{
    qDebug() << "Leave form";
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
    qDebug() << "Selected waveform" << form;
    //    m_gen->switchWaveForm(form);
}

void FormAnalyze::runAnalyze(bool start)
{
    qDebug() << "Run analyze:" << start;
}

void FormAnalyze::saveSignature()
{
    ui->viewSignature->saveView();
}
