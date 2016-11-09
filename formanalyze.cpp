#include <QDebug>
#include "formanalyze.h"
#include "ui_formanalyze.h"
#include "common_types.h"
#include "devices/tonegenerator.h"
#include "devices/audioinputdevice.h"
#include "settings.h"
#include "tiff/imagetiff.h"

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
    
    ui->boxWaveForm->addItem(QIcon(":/icons/oscillator_sine.png"), "Sine", QVariant(ToneWaveForm::WAVE_SINE));
    ui->boxWaveForm->addItem(QIcon(":/icons/oscillator_square.png"), "Square", QVariant(ToneWaveForm::WAVE_SQUARE));
    ui->boxWaveForm->addItem(QIcon(":/icons/oscillator_saw.png"), "Sawtooth", QVariant(ToneWaveForm::WAVE_SAWTOOTH));
    ui->boxWaveForm->addItem(QIcon(":/icons/oscillator_triangle.png"), "Triangle", QVariant(ToneWaveForm::WAVE_TRIANGLE));
    connect(ui->boxWaveForm, SIGNAL(currentIndexChanged(int)), this, SLOT(switchOutputWaveForm()));
    
    connect(ui->buttonRun, SIGNAL(clicked(bool)), this, SLOT(runAnalyze(bool)));
    connect(ui->buttonOpenSignature, SIGNAL(clicked()), this, SLOT(openSignature()));
    connect(ui->buttonLockSignature, SIGNAL(clicked(bool)), this, SLOT(lockSignature(bool)));
    connect(ui->buttonSave, SIGNAL(clicked()), this, SLOT(saveSignature()));
    
    connect(m_capture, SIGNAL(initiated(int)),
            SLOT(captureDeviceInitiated(int)), Qt::QueuedConnection);   // wait while main window initiated
    connect(m_capture, SIGNAL(dataForOscilloscope(SamplesList, SamplesList)),
            this, SLOT(processOscilloscopeData(SamplesList, SamplesList)));
}

FormAnalyze::~FormAnalyze()
{
    delete ui;
}

void FormAnalyze::enterForm()
{
    qreal max = m_gen->getMaxVoltageAmplitude();
    int maxV = qRound(max * 10.);
    double maxVV = maxV / 10.;
    ui->sliderVoltage->setMaximum(maxV);
    ui->boxVoltage->setMaximum(maxVV);
    Settings *settings = Settings::getSettings();
    qreal curV = settings->value("Analyzer/SignalAmplitude", 0.4).toDouble();
    if (curV > maxVV) {
        curV = maxVV;
    }
    setVoltage(curV);
    int freq = settings->value("Analyzer/SignalFrequency", 440).toInt();
    setFrequency(freq);
    QString waveForm = settings->value("Analyzer/SignalForm",
                                       ToneWaveForm::getName(ToneWaveForm::WAVE_SINE)).toString();
    if (waveForm == ToneWaveForm::getName(ToneWaveForm::WAVE_SINE)) {
        ui->boxWaveForm->setCurrentIndex(0);
    } else if (waveForm == ToneWaveForm::getName(ToneWaveForm::WAVE_SQUARE)) {
        ui->boxWaveForm->setCurrentIndex(1);
    } else if (waveForm == ToneWaveForm::getName(ToneWaveForm::WAVE_SAWTOOTH)) {
        ui->boxWaveForm->setCurrentIndex(2);
    } else if (waveForm == ToneWaveForm::getName(ToneWaveForm::WAVE_TRIANGLE)) {
        ui->boxWaveForm->setCurrentIndex(3);
    } else {
        qWarning() << "Invalid format of signal form:" << waveForm << ". Select \"sine\"";
        ui->boxWaveForm->setCurrentIndex(0);
    }
}

void FormAnalyze::leaveForm()
{
    qDebug() << "Leave form \"Analyze\"";
    ui->buttonRun->setChecked(false);
    runAnalyze(false);
}

void FormAnalyze::setFrequency(int frequency)
{
    // for avoid recursion, stop processing signal "valueChanged" until UI boxes are updated
    disconnect(ui->boxFrequency, SIGNAL(valueChanged(int)), this, SLOT(setFrequency(int)));
    disconnect(ui->sliderFrequency, SIGNAL(valueChanged(int)), this, SLOT(setFrequency(int)));

    if (ui->boxFrequency->value() != frequency) {
        ui->boxFrequency->setValue(frequency);
    }
    if (ui->sliderFrequency->value() != frequency) {
        ui->sliderFrequency->setValue(frequency);
    }
    if (ui->buttonRun->isChecked()) {
        m_gen->changeFrequency(frequency);
    }
    Settings *settings = Settings::getSettings();
    settings->setValue("Analyzer/SignalFrequency", frequency);

    connect(ui->boxFrequency, SIGNAL(valueChanged(int)), this, SLOT(setFrequency(int)));
    connect(ui->sliderFrequency, SIGNAL(valueChanged(int)), this, SLOT(setFrequency(int)));
}

void FormAnalyze::setVoltage(double voltage)
{
    // for avoid recursion, stop processing signal "valueChanged" until UI boxes are updated
    disconnect(ui->boxVoltage, SIGNAL(valueChanged(double)), this, SLOT(setVoltage(double)));
    disconnect(ui->sliderVoltage, SIGNAL(valueChanged(int)), this, SLOT(setVoltage(int)));

    int v = qRound(voltage * 10.);
    double curV = v / 10.;
    int prevV = qRound(ui->boxVoltage->value() * 10.);
    if (prevV != v) {
        ui->boxVoltage->setValue(curV);
    }
    if (ui->sliderVoltage->value() != v) {
        ui->sliderVoltage->setValue(v);
    }
    if (ui->buttonRun->isChecked()) {
        m_gen->setCurVoltageAmplitude(curV);
    }
    Settings *settings = Settings::getSettings();
    settings->setValue("Analyzer/SignalAmplitude", curV);

    ui->viewSignature->setMaximumAmplitude(curV);

    connect(ui->boxVoltage, SIGNAL(valueChanged(double)), this, SLOT(setVoltage(double)));
    connect(ui->sliderVoltage, SIGNAL(valueChanged(int)), this, SLOT(setVoltage(int)));
}

void FormAnalyze::setVoltage(int vol10)
{
    setVoltage(vol10 / 10.);
}

void FormAnalyze::switchOutputWaveForm()
{
    int index = ui->boxWaveForm->currentIndex();
    QVariant data = ui->boxWaveForm->itemData(index);
    ToneWaveForm form((ToneWaveForm::Id)data.toInt());
    if (ui->buttonRun->isChecked()) {
        m_gen->switchWaveForm(form);
    }
    Settings *settings = Settings::getSettings();
    settings->setValue("Analyzer/SignalForm", form.getName());
}

void FormAnalyze::runAnalyze(bool start)
{
    qDebug() << "Run analyze:" << start;
    if (start) {
        switchOutputWaveForm();
        int frequency = ui->boxFrequency->value();
        m_gen->changeFrequency(frequency);
        m_gen->setActiveChannels(CHANNEL_BOTH);
        m_gen->setCurVoltageAmplitude(ui->boxVoltage->value());
    }
    m_gen->runGenerator(start);
    m_capture->startCapturing(start);
}

void FormAnalyze::saveSignature()
{
    QImage image;
    QList<QPointF> graphData;
    ui->viewSignature->getView(image, graphData);
    
    QFileDialog dialog(this);
    dialog.setWindowTitle(tr("Save Signature"));
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setNameFilter(tr("Images (*.tif *.tiff)"));
    
    if (dialog.exec() != QDialog::Accepted) {
        qDebug() << "Signature saving is discarded";
        return;
    }
    QStringList files = dialog.selectedFiles();
    if (files.count() != 1) {
        qWarning() << "Incorrect selected dir for store signature:" << files;
        Q_ASSERT(false);
        return;
    }
    QString filePath = files.first();
    if (!filePath.endsWith(".tif", Qt::CaseInsensitive)
        && !filePath.endsWith(".tiff", Qt::CaseInsensitive)) {
        filePath.append(".tiff");
    }
    
    ImageTiff tiff;
    TestpointMeasure point;
    point.id = 0;
    point.pos = QPoint(0, 0);
    int index = ui->boxWaveForm->currentIndex();
    QVariant data = ui->boxWaveForm->itemData(index);
    point.signalType.setId((ToneWaveForm::Id)data.toInt());
    point.signalFrequency = ui->boxFrequency->value();
    point.signalVoltage = ui->boxVoltage->value();
    point.isCurrent = false;
    point.signature = image;
    point.data = graphData;
    QList<TestpointMeasure> testpoints;
    testpoints.append(point);
    if (tiff.writeImageSeries(filePath, QImage(), QImage(), testpoints)) {
        qDebug() << "signature is stored to" << filePath;
    } else {
        qWarning() << "signature couldn't be stored to" << filePath;
        QMessageBox::critical(this, "Save Signature", "Signature couldn't be stored to " + filePath);
    }
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
    voltage = QVector<double>::fromList(rightChannelData);
    QVector<double> current;
    current = QVector<double>::fromList(leftChannelData);
    ui->viewSignature->draw(voltage, current);
}

void FormAnalyze::openSignature()
{
    // Select file
    QFileDialog dialog(this);
    dialog.setWindowTitle(tr("Open signature"));
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("Images (*.tif *.tiff)"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    QStringList files = dialog.selectedFiles();
    if (files.count() != 1) {
        qWarning() << "Incorrect selected signature file:" << files;
        Q_ASSERT(false);
        return;
    }
    QString filePath = files.first();
    
    // Read signature
    ImageTiff tiff;
    QImage boardPhoto;
    QList<TestpointMeasure> testpoints;
    tiff.readImageSeries(filePath, boardPhoto, testpoints);
    if (testpoints.count() != 1) {
        qWarning() << "File" << filePath << "doesn't contain signature";
        QMessageBox::warning(this, "Open Signature", "File" + filePath + "doesn't contain signature");
        return;
    }
    
    // Display signature
    TestpointMeasure measure = testpoints.first();
    ui->viewSignature->loadPreviousSignature(measure.data);
    ui->buttonLockSignature->setChecked(true);
    
    // Restore signature's test environment
    setFrequency(measure.signalFrequency);
    if (measure.signalVoltage > ui->boxVoltage->maximum()) {
        qWarning() << "Voltage is higher than available maximum:"
                   << measure.signalVoltage, ui->boxVoltage->maximum();
        measure.signalVoltage = ui->boxVoltage->maximum();
    }
    setVoltage(measure.signalVoltage);
    if (measure.signalType.id() == ToneWaveForm::WAVE_SINE) {
        ui->boxWaveForm->setCurrentIndex(0);
    } else if (measure.signalType.id() == ToneWaveForm::WAVE_SQUARE) {
        ui->boxWaveForm->setCurrentIndex(1);
    } else if (measure.signalType.id() == ToneWaveForm::WAVE_SAWTOOTH) {
        ui->boxWaveForm->setCurrentIndex(2);
    } else if (measure.signalType.id() == ToneWaveForm::WAVE_TRIANGLE) {
        ui->boxWaveForm->setCurrentIndex(3);
    } else {
        qWarning() << "Invalid format of signal form:" << measure.signalType << ". Select \"sine\"";
        ui->boxWaveForm->setCurrentIndex(0);
    }
}

void FormAnalyze::lockSignature(bool lock)
{
    qDebug() << "Lock signature view:" << lock;
    if (lock) {
        QImage image;
        QList<QPointF> graphData;
        ui->viewSignature->getView(image, graphData);
        ui->viewSignature->loadPreviousSignature(graphData);
    } else {
        QList<QPointF> graphData;
        ui->viewSignature->loadPreviousSignature(graphData);
    }
}
