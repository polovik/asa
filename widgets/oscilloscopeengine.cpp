#include <QDebug>
#include "oscilloscopeengine.h"
#include "oscilloscopeview.h"

extern bool g_verboseOutput;

OscilloscopeEngine::OscilloscopeEngine(OscilloscopeView *view, QObject *parent) :
    QObject(parent)
{
    m_view = view;
    m_samplingRate = -1;
    m_frameLength = -1;
    m_triggerChannel = CHANNEL_NONE;
    m_dataForSingleCaptureAcqured = true;
    stop();

    m_triggerLevel = 0.;
    m_view->setTriggerLevel(m_triggerLevel);
    m_view->setYaxisRange(-1.0, 1.0);
    m_view->setXaxisRange(0, 1000 * 8000. / 44100);
    connect(m_view, SIGNAL(triggerLevelChanged(double)), this, SLOT(updateTriggerLevel(double)));
}

OscilloscopeEngine::~OscilloscopeEngine()
{

}

void OscilloscopeEngine::prepareToStart(int samplingRate)
{
    m_samplingRate = samplingRate;
    m_frameLength = m_samplingRate / OSCILLOSCOPE_PLOT_FREQUENCY_HZ;
    double bound = 1000. * (m_frameLength / 2.) / m_samplingRate;
    m_view->setXaxisRange(-bound, bound);
    qDebug() << "Capture device is ready with sampling rate =" << samplingRate << "Frame len =" << m_frameLength;

    if (m_triggerMode == TRIG_SINGLE) {
        m_dataForSingleCaptureAcqured = false;
    } else {
        m_dataForSingleCaptureAcqured = true;
    }
    m_samplesInputBufferLeft.clear();
    m_samplesInputBufferRight.clear();
    m_isRunning = true;
}

void OscilloscopeEngine::stop()
{
    m_isRunning = false;
}

void OscilloscopeEngine::setDisplayedChannels(AudioChannels channels)
{
    m_capturedChannels = channels;
    m_view->showGraph(GRAPH_CHANNEL_LEFT, m_capturedChannels & CHANNEL_LEFT);
    m_view->showGraph(GRAPH_CHANNEL_RIGHT, m_capturedChannels & CHANNEL_RIGHT);
}

void OscilloscopeEngine::setTriggerChannel(AudioChannels channel)
{
    if ((channel != CHANNEL_LEFT) && (channel != CHANNEL_RIGHT)) {
        qWarning() << "Trigger must be only one. Got:" << channel;
        Q_ASSERT(false);
        return;
    }

    if (channel != m_triggerChannel) {
        qDebug() << "Oscilloscope trigger channel has been changed from" << m_triggerChannel << "to" << channel;
        m_triggerChannel = channel;
    }
}

void OscilloscopeEngine::setTriggerMode(OscTriggerMode mode)
{
    if (mode == TRIG_AUTO) {
        m_view->showTriggerLine(false);
    } else if (mode == TRIG_NORMAL) {
        m_view->showTriggerLine(true);
    } else if (mode == TRIG_SINGLE) {
        m_view->showTriggerLine(true);
    } else {
        qWarning() << "Invalid trigger's mode:" << mode;
        Q_ASSERT(false);
        return;
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
}

void OscilloscopeEngine::setTriggerSlope(OscTriggerSlope slope)
{
    if (slope != m_triggerSlope) {
        m_samplesInputBufferLeft.clear();
        m_samplesInputBufferRight.clear();
        qDebug() << "Oscilloscope trigger slope has been changed from" << m_triggerSlope << "to" << slope;
        m_triggerSlope = slope;
    }
}

void OscilloscopeEngine::setMaximumAmplitude(qreal voltage)
{
    m_view->setYaxisRange(-voltage * 1.1, voltage * 1.1);
}

void OscilloscopeEngine::draw(AudioChannels channel, const QVector<double> &values)
{
    QVector<double> keys;
    keys.resize(values.count());
    int offset = values.count() / 2;
    for (int i = 0; i < values.count(); i++) {
        keys[i] = 1000. * (i - offset) / m_samplingRate;
    }
    if (channel == CHANNEL_LEFT) {
        m_view->draw(GRAPH_CHANNEL_LEFT, keys, values);
    } else if (channel == CHANNEL_RIGHT) {
        m_view->draw(GRAPH_CHANNEL_RIGHT, keys, values);
    } else {
        qWarning() << "Couldn't draw incorrect channel" << channel;
        Q_ASSERT(false);
        return;
    }
}

void OscilloscopeEngine::displayOscilloscopeChannelData(int dislayFrom, int displayedLength, int removeFrom)
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

void OscilloscopeEngine::processOscilloscopeData(SamplesList leftChannelData, SamplesList rightChannelData)
{
    if (!m_isRunning) {
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

void OscilloscopeEngine::updateTriggerLevel(double voltage)
{
    if (g_verboseOutput) {
        qDebug() << "Trigger level is changed to" << voltage << "V";
    }
    m_triggerLevel = voltage;
}
