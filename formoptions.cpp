#include <QDebug>
#include <QDir>
#include <QTextCodec>
#include <QMessageBox>
#include "common_types.h"
#include "settings.h"
#include "formoptions.h"
#include "ui_formoptions.h"

FormOptions::FormOptions(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FormOptions)
{
    ui->setupUi(this);

    ui->boxLanguage->addItem(QIcon(":/icons/flag_united_states_of_america_usa.ico"), tr("English"), QVariant(QString("en_US")));
    ui->boxLanguage->addItem(QIcon(":/icons/flag_russian_federation.ico"), tr("Russian"), QVariant(QString("ru_RU")));

    ui->boxWaveForm->addItem(QIcon(":/icons/oscillator_sine.png"), "Sine", QVariant(ToneWaveForm::WAVE_SINE));
    ui->boxWaveForm->addItem(QIcon(":/icons/oscillator_square.png"), "Square", QVariant(ToneWaveForm::WAVE_SQUARE));
    ui->boxWaveForm->addItem(QIcon(":/icons/oscillator_saw.png"), "Sawtooth", QVariant(ToneWaveForm::WAVE_SAWTOOTH));
    ui->boxWaveForm->addItem(QIcon(":/icons/oscillator_triangle.png"), "Triangle", QVariant(ToneWaveForm::WAVE_TRIANGLE));
}

FormOptions::~FormOptions()
{
    delete ui;
}

void FormOptions::enterForm()
{
    Settings *settings = Settings::getSettings();
    QString locale = settings->value("Global/Locale", "en_US").toString();
    if (locale == "en_US") {
        ui->boxLanguage->setCurrentIndex(0);
    } else if (locale == "ru_RU") {
        ui->boxLanguage->setCurrentIndex(1);
    } else {
        qWarning() << "Invalid application's locale:" << locale;
    }
    connect(ui->boxLanguage, SIGNAL(currentIndexChanged(int)), this, SLOT(switchApplicationLanguage(int)));

    const QString defaultWaveName = settings->value("DiagnoseDefaults/SignalForm",
                                       ToneWaveForm::getName(ToneWaveForm::WAVE_SINE)).toString();
    const ToneWaveForm waveForm(defaultWaveName);
    if (waveForm.id() == ToneWaveForm::WAVE_SINE) {
        ui->boxWaveForm->setCurrentIndex(0);
    } else if (waveForm.id() == ToneWaveForm::WAVE_SQUARE) {
        ui->boxWaveForm->setCurrentIndex(1);
    } else if (waveForm.id() == ToneWaveForm::WAVE_SAWTOOTH) {
        ui->boxWaveForm->setCurrentIndex(2);
    } else if (waveForm.id() == ToneWaveForm::WAVE_TRIANGLE) {
        ui->boxWaveForm->setCurrentIndex(3);
    } else {
        qWarning() << "Invalid format of signal form:" << defaultWaveName << ". Select \"sine\"";
        ui->boxWaveForm->setCurrentIndex(0);
    }
    connect(ui->boxWaveForm, SIGNAL(currentIndexChanged(int)), this, SLOT(switchOutputWaveForm()));

    const qreal defaultVoltage = settings->value("DiagnoseDefaults/SignalAmplitude", 0.4).toDouble();
    ui->boxVoltage->setValue(defaultVoltage);
    connect(ui->boxVoltage, SIGNAL(valueChanged(double)), this, SLOT(setVoltage(double)));

    const int defaultFrequency = settings->value("DiagnoseDefaults/SignalFrequency", 440).toInt();
    ui->boxFrequency->setValue(defaultFrequency);
    connect(ui->boxFrequency, SIGNAL(valueChanged(int)), this, SLOT(setFrequency(int)));
}

void FormOptions::leaveForm()
{
    ui->boxLanguage->disconnect();
    ui->boxWaveForm->disconnect();
    ui->boxVoltage->disconnect();
    ui->boxFrequency->disconnect();

    setFrequency(ui->boxFrequency->value());
    setVoltage(ui->boxVoltage->value());
    ui->boxVoltage->disconnect();
    switchOutputWaveForm();
}

void FormOptions::switchApplicationLanguage(int index)
{
    Settings *settings = Settings::getSettings();
    QString locale = "";
    if (index >= 0) {
        QVariant varData = ui->boxLanguage->itemData(index);
        locale = varData.toString();
    } else {
        qWarning() << "Invalid application language index:" << index;
        return;
    }
    qDebug() << "Application language was switched to" << locale;
    settings->setValue("Global/Locale", locale);
    QMessageBox::information(this, tr("Language switching"), tr("Language will be changed after the application restart"));
}

void FormOptions::setFrequency(int frequency)
{
    Settings *settings = Settings::getSettings();
    settings->setValue("DiagnoseDefaults/SignalFrequency", frequency);
}

void FormOptions::setVoltage(double voltage)
{
    // for avoid recursion, stop processing signal "valueChanged" until UI boxes are updated
    disconnect(ui->boxVoltage, SIGNAL(valueChanged(double)), this, SLOT(setVoltage(double)));

    int v = qRound(voltage * 10.);
    double curV = v / 10.;
    int prevV = qRound(ui->boxVoltage->value() * 10.);
    if (prevV != v) {
        ui->boxVoltage->setValue(curV);
    }
    Settings *settings = Settings::getSettings();
    settings->setValue("DiagnoseDefaults/SignalAmplitude", curV);

    connect(ui->boxVoltage, SIGNAL(valueChanged(double)), this, SLOT(setVoltage(double)));
}

void FormOptions::switchOutputWaveForm()
{
    int index = ui->boxWaveForm->currentIndex();
    QVariant waveType = ui->boxWaveForm->itemData(index);
    ToneWaveForm form(static_cast<ToneWaveForm::Id>(waveType.toInt()));

    Settings *settings = Settings::getSettings();
    settings->setValue("DiagnoseDefaults/SignalForm", form.getName());
}
