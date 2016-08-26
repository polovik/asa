#include <QDebug>
#include "formraw.h"
#include "ui_formraw.h"
#include "devices/tonegenerator.h"
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
    m_dataForSingleCaptureAcqured = true;
    m_triggerChannel = CHANNEL_NONE;
    m_samplingRate = -1;
    m_frameLength = -1;
    m_capture = capture;
    connect(m_capture, SIGNAL(initiated(int)),
            SLOT(captureDeviceInitiated(int)), Qt::QueuedConnection);   // wait while main window initiated
    connect(m_capture, SIGNAL(dataForOscilloscope(SamplesList, SamplesList)),
            this, SLOT(processOscilloscopeData(SamplesList, SamplesList)));
            
    changeTriggerSettings();
    connect(ui->boxTriggerAuto, SIGNAL(clicked()), this, SLOT(changeTriggerSettings()));
    connect(ui->boxTriggerNormal, SIGNAL(clicked()), this, SLOT(changeTriggerSettings()));
    connect(ui->boxTriggerSingle, SIGNAL(clicked()), this, SLOT(changeTriggerSettings()));
    connect(ui->boxSlopeRising, SIGNAL(clicked()), this, SLOT(changeTriggerSettings()));
    connect(ui->boxSlopeFalling, SIGNAL(clicked()), this, SLOT(changeTriggerSettings()));
    connect(ui->boxTriggerChannel, SIGNAL(currentIndexChanged(int)), this, SLOT(changeTriggerSettings()));
    
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
}

void FormRaw::leaveForm()
{
    qDebug() << "Leave form \"Raw\"";
    startToneGenerator(false);
    startAudioCapture(false);
    ui->buttonGenerate->setChecked(false);
    ui->buttonCapture->setChecked(false);
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
    audioCaptureReady = true;
    m_samplingRate = samplingRate;
    Q_ASSERT(m_capture);
    
    m_frameLength = m_samplingRate / OSCILLOSCOPE_PLOT_FREQUENCY_HZ;
    double bound = 1000. * (m_frameLength / 2.) / m_samplingRate;
    ui->oscilloscope->setXaxisRange(-bound, bound);
    qDebug() << "Capture device is ready with sampling rate =" << samplingRate << "Frame len =" << m_frameLength;
    
    if (m_triggerMode == TRIG_SINGLE) {
        m_dataForSingleCaptureAcqured = false;
    } else {
        m_dataForSingleCaptureAcqured = true;
    }
    m_samplesInputBufferLeft.clear();
    m_samplesInputBufferRight.clear();
    changeCapturedChannels(); // Start receive data from input device
}

void FormRaw::draw(AudioChannels channel, const QVector<double> &values)
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

void FormRaw::displayOscilloscopeChannelData(int dislayFrom, int displayedLength, int removeFrom)
{
    if (m_capturedChannels != CHANNEL_BOTH) {
        SamplesList *buffer = NULL;
        if (m_capturedChannels == CHANNEL_LEFT) {
            buffer = &m_samplesInputBufferLeft;
        } else if (m_capturedChannels == CHANNEL_RIGHT) {
            buffer = &m_samplesInputBufferRight;
        }
        QVector<double> values;
        values = QVector<double>::fromList(buffer->mid(dislayFrom, displayedLength));
        draw(m_capturedChannels, values);
        *buffer = buffer->mid(removeFrom);
    } else {
        QVector<double> values;
        values = QVector<double>::fromList(m_samplesInputBufferLeft.mid(dislayFrom, displayedLength));
        draw(CHANNEL_LEFT, values);
        values = QVector<double>::fromList(m_samplesInputBufferRight.mid(dislayFrom, displayedLength));
        draw(CHANNEL_RIGHT, values);
        m_samplesInputBufferLeft = m_samplesInputBufferLeft.mid(removeFrom);
        m_samplesInputBufferRight = m_samplesInputBufferRight.mid(removeFrom);
    }
}

void FormRaw::processOscilloscopeData(SamplesList leftChannelData, SamplesList rightChannelData)
{
    if (!ui->buttonCapture->isChecked()) {
        return;
    }
//    qDebug() << "Got" << samples.length() << "samples";
    if (m_capturedChannels == CHANNEL_NONE) {
        return;
    }
    SamplesList *buffer = NULL;
    SamplesList *samples = NULL;
    // I.   Select primary channel
    if (m_capturedChannels != CHANNEL_BOTH) {
        // For single channel capture, process only one buffer.
        if (m_capturedChannels == CHANNEL_LEFT) {
            buffer = &m_samplesInputBufferLeft;
            samples = &leftChannelData;
        } else if (m_capturedChannels == CHANNEL_RIGHT) {
            buffer = &m_samplesInputBufferRight;
            samples = &rightChannelData;
        }
    } else {
        // For both channel capture, only one channel is selected for Normal or Single triggering
        if (m_triggerChannel == CHANNEL_LEFT) {
            buffer = &m_samplesInputBufferLeft;
        } else if (m_triggerChannel == CHANNEL_RIGHT) {
            buffer = &m_samplesInputBufferRight;
        } else if (m_triggerMode != TRIG_AUTO) {
            qWarning() << "For non-auto trigger mode an one channel have to be choosen for triggering";
            Q_ASSERT(false);
            return;
        }
    }
    // II.  For both channel capture we have to check buffers state - samples must be captured simultaneously
    if (m_capturedChannels == CHANNEL_BOTH) {
        // This is not a bug when
        if (m_samplesInputBufferLeft.size() != m_samplesInputBufferRight.size()) {
            qWarning() << "Audio input buffers are not synced. Reset them."
                       << "Left:" << m_samplesInputBufferLeft.size()
                       << "Right:" << m_samplesInputBufferRight.size();
            m_samplesInputBufferLeft.clear();
            m_samplesInputBufferRight.clear();
//            Q_ASSERT(false);
        }
        if (leftChannelData.size() != rightChannelData.size()) {
            qWarning() << "Fresh audio samples are not synced. Reset them."
                       << "Left:" << leftChannelData.size()
                       << "Right:" << rightChannelData.size();
            leftChannelData.clear();
            rightChannelData.clear();
//            Q_ASSERT(false);
        }
    }
    // III. Trigger mode: Automatic - display data immediatelly if there are enough data in buffer
    if (m_triggerMode == TRIG_AUTO) {
        if (m_capturedChannels != CHANNEL_BOTH) {
            buffer->append(*samples);
            if (buffer->size() < m_frameLength) {
                return;
            }
        } else {
            m_samplesInputBufferLeft.append(leftChannelData);
            m_samplesInputBufferRight.append(rightChannelData);
            if (m_samplesInputBufferLeft.size() < m_frameLength) {
                return;
            }
        }
        displayOscilloscopeChannelData(0, m_frameLength, m_frameLength);
        return;
    }
    // IV.  Trigger mode: Single - if data acqured, the graph have to hold data
    if ((m_triggerMode == TRIG_SINGLE) && m_dataForSingleCaptureAcqured) {
        return;
    }
    // V.   Trigger mode: Normal or Single (when trigger hasn't fired yet)
    if ((m_triggerMode == TRIG_NORMAL) or (m_triggerMode == TRIG_SINGLE)) {
        // Fill buffers with samples
        if (m_capturedChannels != CHANNEL_BOTH) {
            buffer->append(*samples);
            if (buffer->size() < m_frameLength * 2) {
                return;
            }
        } else {
            m_samplesInputBufferLeft.append(leftChannelData);
            m_samplesInputBufferRight.append(rightChannelData);
            if (m_samplesInputBufferLeft.size() < m_frameLength * 2) {
                return;
            }
        }
        // Find moments when signal intersects trigger line by required slope
        // Start searching with offset in half of frame length, because this half has been already plotted
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
                    displayOscilloscopeChannelData(offset - m_frameLength / 2, m_frameLength, m_frameLength / 2);
                    m_dataForSingleCaptureAcqured = true;
                    return;
                }
            }
            // or select last event
            if (eventsOffsets.size() > 0) {
                int offset = eventsOffsets.last();
                displayOscilloscopeChannelData(offset - m_frameLength / 2, m_frameLength, m_frameLength / 2);
                m_dataForSingleCaptureAcqured = true;
                return;
            }
        }
        // buffer is overflowed
        if (buffer->size() > m_frameLength * 2) {
            if (m_capturedChannels != CHANNEL_BOTH) {
                *buffer = buffer->mid(m_frameLength);
            } else {
                m_samplesInputBufferLeft = m_samplesInputBufferLeft.mid(m_frameLength);
                m_samplesInputBufferRight = m_samplesInputBufferRight.mid(m_frameLength);
            }
        }
    }
}

void FormRaw::startAudioCapture(bool start)
{
    m_capture->startCapturing(start);
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

void FormRaw::updateTriggerLevel(double voltage)
{
    if (g_verboseOutput) {
        qDebug() << "Trigger level is changed to" << voltage << "V";
    }
    m_triggerLevel = voltage;
}

void FormRaw::changeTriggerSettings()
{
    OscTriggerMode mode = m_triggerMode;
    if (ui->boxTriggerAuto->isChecked()) {
        mode = TRIG_AUTO;
        ui->oscilloscope->showTriggerLine(false);
    } else if (ui->boxTriggerNormal->isChecked()) {
        mode = TRIG_NORMAL;
        ui->oscilloscope->showTriggerLine(true);
    } else {
        mode = TRIG_SINGLE;
        ui->oscilloscope->showTriggerLine(true);
    }
    if (mode != m_triggerMode) {
        qDebug() << "Oscilloscope trigger mode has been changed from" << m_triggerMode << "to" << mode;
        m_triggerMode = mode;
        m_samplesInputBufferLeft.clear();
        m_samplesInputBufferRight.clear();
        if (m_triggerMode == TRIG_SINGLE) {
            m_dataForSingleCaptureAcqured = false;
        } else {
            m_dataForSingleCaptureAcqured = true;
        }
    }
    
    OscTriggerSlope slope = m_triggerSlope;
    if (ui->boxSlopeRising->isChecked())
        slope = TRIG_RISING;
    else
        slope = TRIG_FALLING;
    if (slope != m_triggerSlope) {
        m_samplesInputBufferLeft.clear();
        m_samplesInputBufferRight.clear();
        qDebug() << "Oscilloscope trigger slope has been changed from" << m_triggerSlope << "to" << slope;
        m_triggerSlope = slope;
    }
    
    AudioChannels channel;
    if (ui->boxTriggerChannel->currentIndex() == 0) {
        channel = CHANNEL_LEFT;
    } else if (ui->boxTriggerChannel->currentIndex() == 1) {
        channel = CHANNEL_RIGHT;
    } else {
        qWarning() << "Incorrect channel selected for tracking:" << ui->boxTriggerChannel->currentIndex();
        Q_ASSERT(false);
    }
    if (channel != m_triggerChannel) {
        qDebug() << "Oscilloscope trigger channel has been changed from" << m_triggerChannel << "to" << channel;
        m_triggerChannel = channel;
    }
}

void FormRaw::changeCapturedChannels()
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
    m_capturedChannels = (AudioChannels)channels;
    m_capture->setCapturedChannels(m_capturedChannels);
}
