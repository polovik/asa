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

    //  Tone generation
    m_gen = new ToneGenerator;
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
    m_gen->start();

    // Audio capture
    m_samplingRate = -1;
    m_frameLength = -1;
    m_capture = new AudioInputThread;
    connect(m_capture, SIGNAL (initiated (int)),
             SLOT (captureDeviceInitiated (int)), Qt::QueuedConnection); // wait while main window initiated
    connect(m_capture, SIGNAL(dataForOscilloscope(OscCapturedChannels,SamplesList)),
            this, SLOT(processOscilloscopeData(OscCapturedChannels,SamplesList)));
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
    m_capture->start ();

    changeTriggerSettings();
    connect(ui->boxTriggerAuto, SIGNAL(clicked()), this, SLOT(changeTriggerSettings()));
    connect(ui->boxTriggerNormal, SIGNAL(clicked()), this, SLOT(changeTriggerSettings()));
    connect(ui->boxTriggerSingle, SIGNAL(clicked()), this, SLOT(changeTriggerSettings()));
    connect(ui->boxSlopeRising, SIGNAL(clicked()), this, SLOT(changeTriggerSettings()));
    connect(ui->boxSlopeFalling, SIGNAL(clicked()), this, SLOT(changeTriggerSettings()));

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

    m_triggerLevel = 0.;
    ui->oscilloscope->setTriggerLevel(m_triggerLevel);
//    ui->sliderPrecedingInterval->setValue(150);
//    setPrecedingInterval(150);

    ui->oscilloscope->setYaxisRange(-1.0, 1.0);
    ui->oscilloscope->setXaxisRange(0, 1000 * 8000. / 44100);
    draw(CHANNEL_LEFT, m_dataY);
    connect(ui->oscilloscope, SIGNAL(triggerLevelChanged(double)), this, SLOT(updateTriggerLevel(double)));

    connect(ui->buttonGenerate, SIGNAL(toggled(bool)), this, SLOT(startToneGenerator(bool)));
    connect(ui->boxFrequency, SIGNAL(valueChanged(int)), m_gen, SLOT(changeFrequency(int)));
    connect(ui->buttonGenerate, SIGNAL(toggled(bool)), ui->boxAudioOutputDevice, SLOT(setDisabled(bool)));
    connect(ui->boxAudioOutputDevice, SIGNAL(currentIndexChanged(int)), this, SLOT(switchOutputAudioDevice(int)));

    connect(ui->buttonCupture, SIGNAL(toggled(bool)), this, SLOT(startAudioCapture(bool)));
    connect(ui->buttonCupture, SIGNAL(toggled(bool)), ui->boxAudioInputDevice, SLOT(setDisabled(bool)));
    connect(ui->boxAudioInputDevice, SIGNAL(currentIndexChanged(int)), this, SLOT(switchInputAudioDevice(int)));
    connect(ui->boxChannelLeft, SIGNAL(toggled(bool)), this, SLOT(changeCapturedChannels()));
    connect(ui->boxChannelRight, SIGNAL(toggled(bool)), this, SLOT(changeCapturedChannels()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::startToneGenerator(bool start)
{
    m_gen->runGenerator(start);
}

void MainWindow::captureDeviceInitiated (int samplingRate)
{
    // Audio device ready to capture - display this
    audioCaptureReady = true;
    m_samplingRate = samplingRate;
    Q_ASSERT(m_capture);

    m_frameLength = m_samplingRate / OSCILLOSCOPE_PLOT_FREQUENCY_HZ;
    double bound = 1000. * (m_frameLength / 2.) / m_samplingRate;
    ui->oscilloscope->setXaxisRange(-bound, bound);
    qDebug() << "Capture device is ready with sampling rate =" << samplingRate << "Frame len =" << m_frameLength;
//    m_capture->changeFrameSize (AudioInputThread::OSCILLOSCOPE, m_frameLength / 2);
    changeCapturedChannels();
}

void MainWindow::draw(OscCapturedChannels channel, const QVector<double> &values)
{
    QVector<double> keys;
    keys.resize(values.count());
    int offset = values.count() / 2;
    for (int i = 0; i < values.count(); i++) {
        keys[i] = 1000. * (i - offset) / m_samplingRate;
    }
    if (channel == CHANNEL_LEFT) {
        ui->oscilloscope->draw(GRAPH_CHANNEL_LEFT, keys, values);
    } else if (channel == CHANNEL_RIGHT) {
        ui->oscilloscope->draw(GRAPH_CHANNEL_RIGHT, keys, values);
    } else {
        qWarning() << "Couldn't draw incorrect channel" << channel;
        Q_ASSERT(false);
        return;
    }
}

void MainWindow::processOscilloscopeData(OscCapturedChannels channel, SamplesList samples)
{
//    qDebug() << "Got" << samples.length() << "samples";
    SamplesList *buffer = NULL;
    if (channel == CHANNEL_LEFT) {
        buffer = &m_samplesInputBufferLeft;
    } else if (channel == CHANNEL_RIGHT) {
        buffer = &m_samplesInputBufferRight;
    } else {
        qWarning() << "Got" << samples.length() << "samples from incorrect channel" << channel;
        Q_ASSERT(false);
        return;
    }
    if (m_triggerMode == TRIG_AUTO) {
        buffer->append(samples);
        if (buffer->size() < m_frameLength) {
            return;
        }
        QVector<double> values;
        values = values.fromList(buffer->mid(0, m_frameLength));
        draw(channel, values);
        *buffer = buffer->mid(m_frameLength);
        return;
    }
    if (m_triggerMode == TRIG_NORMAL) {
        buffer->append(samples);
        if (buffer->size() < m_frameLength * 2) {
            return;
        }
        QList<int> eventsOffsets;
        for (int offset = m_frameLength / 2 - 1;  offset < buffer->size(); ++offset) {
            qreal cur = buffer->at(offset);
            qreal prev = buffer->at(offset - 1);
            if (m_triggerSlope == TRIG_RISING) {
                if ((prev <= m_triggerLevel) && (cur >= m_triggerLevel)) {
                    eventsOffsets.append(offset);
                }
            } else {
                if ((prev >= m_triggerLevel) && (cur <= m_triggerLevel)) {
                    eventsOffsets.append(offset);
                }
            }
        }
        if (eventsOffsets.size() > 0) {
            // take first event in next frame
            for (int i = 0; i < eventsOffsets.size(); ++i) {
                int offset = eventsOffsets[i];
                if (offset >= m_frameLength * 3 / 2) {
                    // there is not enough data for display full frame
                    eventsOffsets.clear();
                    break;
                }
                if (offset >= m_frameLength / 2) {
                    QVector<double> values;
                    values = values.fromList(buffer->mid(offset - m_frameLength / 2, m_frameLength));
                    draw(channel, values);
                    *buffer = buffer->mid(m_frameLength / 2);
                    return;
                }
            }
            // or select last event
            if (eventsOffsets.size() > 0) {
                int offset = eventsOffsets.last();
                QVector<double> values;
                values = values.fromList(buffer->mid(offset - m_frameLength / 2, m_frameLength));
                draw(channel, values);
                *buffer = buffer->mid(m_frameLength / 2);
                return;
            }
        }
        // buffer is truncated
        if (buffer->size() > m_frameLength * 2) {
            *buffer = buffer->mid(m_frameLength);
        }
    }
}

void MainWindow::startAudioCapture(bool start)
{
    m_capture->startCapturing(start);
}

void MainWindow::switchOutputAudioDevice(int index)
{
    QVariant cleanName = ui->boxAudioOutputDevice->itemData(index);
    QString name = cleanName.toString();
    m_gen->switchOutputDevice(name);
}

void MainWindow::switchInputAudioDevice(int index)
{
    QVariant cleanName = ui->boxAudioInputDevice->itemData(index);
    QString name = cleanName.toString();
    m_capture->switchInputDevice(name);
}

void MainWindow::updateTriggerLevel(double voltage)
{
    qDebug() << "Trigger level is changed to" << voltage << "V";
    m_triggerLevel = voltage;
}

void MainWindow::changeTriggerSettings()
{
    if (ui->boxTriggerAuto->isChecked()) {
        m_triggerMode = TRIG_AUTO;
        ui->oscilloscope->showTriggerLine(false);
    } else if (ui->boxTriggerNormal->isChecked()) {
        m_triggerMode = TRIG_NORMAL;
        ui->oscilloscope->showTriggerLine(true);
    } else {
        m_triggerMode = TRIG_SINGLE;
        ui->oscilloscope->showTriggerLine(true);
    }

    if (ui->boxSlopeRising->isChecked())
        m_triggerSlope = TRIG_RISING;
    else
        m_triggerSlope = TRIG_FALLING;
}

void MainWindow::changeCapturedChannels()
{
    int channels = CHANNEL_NONE;
    channels = CHANNEL_NONE;
    if (ui->boxChannelLeft->isChecked()) {
        channels |= CHANNEL_LEFT;
    }
    ui->oscilloscope->showGraph(GRAPH_CHANNEL_LEFT, ui->boxChannelLeft->isChecked());
    if (ui->boxChannelRight->isChecked()) {
        channels |= CHANNEL_RIGHT;
    }
    ui->oscilloscope->showGraph(GRAPH_CHANNEL_RIGHT, ui->boxChannelRight->isChecked());
    m_capturedChannels = (OscCapturedChannels)channels;
    m_capture->setCapturedChannels(m_capturedChannels);
}
