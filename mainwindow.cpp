#include <QDebug>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ToneGenerator.h"
#include "audioinputdevice.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_gen = new ToneGenerator;

    m_capture = new AudioInputThread;
    connect(m_capture, SIGNAL (initiated (int)),
             SLOT (captureDeviceInitiated (int)), Qt::QueuedConnection); // wait while main window initiated
    m_capture->start ();

    // create plot (from quadratic plot example):
    QVector<double> x(1024), y(1024);
    for (int i = 0; i < 100; ++i) {
        x[i] = i * 0.016;
        y[i] = 400 / 1024. * 50.;
    }
    for (int i = 100; i < 150; ++i) {
        x[i] = i * 0.016;
        y[i] = 100 / 1024. * 50.;
    }
    for (int i = 150; i < 151; ++i) {
        x[i] = i * 0.016;
        y[i] = 300 / 1024. * 50.;
    }
    for (int i = 151; i < 1024; ++i) {
        x[i] = i * 0.016;
        y[i] = (i) / 1024. * 50.;
    }
    m_dataX = x;
    m_dataY = y;

    m_triggerLevel = 1.;
//    ui->sliderPrecedingInterval->setValue(150);
//    setPrecedingInterval(150);

    ui->oscilloscope->setYaxisRange(-3.0, 3.0);
    ui->oscilloscope->setXaxisRange(0, 1000 * 8000. / 44100);
    draw(m_dataY);

    connect(ui->buttonCupture, SIGNAL(toggled(bool)), this, SLOT(startAudioCapture(bool)));
    connect(ui->buttonGenerate, SIGNAL(toggled(bool)), this, SLOT(startToneGenerator(bool)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::startToneGenerator(bool start)
{
    m_gen->start();
}

void MainWindow::captureDeviceInitiated (int samplingRate)
{
    qDebug() << "Capture device is ready with sampling rate =" << samplingRate;
    // Audio device ready to capture - display this
    audioCaptureReady = true;
    this->m_samplingRate = samplingRate;
    Q_ASSERT(m_capture);

    m_capture->changeFrameSize (AudioInputThread::OSCILLOSCOPE, 8000);
    connect (m_capture, SIGNAL (dataForOscilloscope (SamplesList)), this, SLOT (processOscilloscopeData (SamplesList)));
}

void MainWindow::draw(const QVector<double> &values)
{
    QVector<double> keys;
    keys.resize(values.count());
    for (int i = 0; i < values.count(); i++) {
        keys[i] = 1000. * i / this->m_samplingRate;
    }

//    m_dataX.clear();
//    m_dataY.clear();
//    m_dataX = keys;
//    m_dataY = values;

//    for (int i = 0; i < m_dataX.count(); i++) {
//        keys[i] = m_dataX[i] - m_precedingInterval;
//    }
    ui->oscilloscope->draw(keys, values);

/*    if (ui->boxSweepSingle->isChecked()) {
        m_isRunning = false;
        changeStartButtonView();
        emit stopped();
    } else {
        m_timerPacketReceiving.start();
        changeStartButtonView();
    }*/
}

void MainWindow::processOscilloscopeData (SamplesList samples)
{
    qDebug() << "Got" << samples.length() << "samples";
    QVector<double> values;
    values = values.fromList(samples);
    draw(values);
}

void MainWindow::startAudioCapture(bool start)
{
    m_capture->startCapturing(start);
}
