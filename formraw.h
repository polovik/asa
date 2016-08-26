#ifndef FORMRAW_H
#define FORMRAW_H

#include <QWidget>
#include "common_types.h"
#include "devices/audioinputdevice.h"

namespace Ui {
class FormRaw;
}

class ToneGenerator;

class FormRaw : public QWidget
{
    Q_OBJECT

public:
    explicit FormRaw(ToneGenerator *gen, AudioInputThread *capture, QWidget *parent = 0);
    ~FormRaw();

public slots:
    void enterForm();
    void leaveForm();

private slots:
    void startToneGenerator(bool start);
    void captureDeviceInitiated (int samplingRate);
    void draw(AudioChannels channel, const QVector<double> &values);
    void processOscilloscopeData (SamplesList leftChannelData, SamplesList rightChannelData);
    void startAudioCapture(bool start);
    void switchOutputWaveForm();
    void switchOutputFrequency();
    void updateTriggerLevel(double voltage);
    void changeTriggerSettings();
    void changeCapturedChannels();

private:
    void displayOscilloscopeChannelData(int dislayFrom, int displayedLength, int removeFrom);

    Ui::FormRaw *ui;
    ToneGenerator *m_gen;
    AudioInputThread *m_capture;

    QVector<double> m_dataX;
    QVector<double> m_dataY;
    double m_triggerLevel;
    int m_samplingRate;
    int m_frameLength;
    bool audioCaptureReady;
    bool m_dataForSingleCaptureAcqured;
    static const int OSCILLOSCOPE_PLOT_FREQUENCY_HZ = 5;
    OscTriggerMode m_triggerMode;
    OscTriggerSlope m_triggerSlope;
    SamplesList m_samplesInputBufferLeft;
    SamplesList m_samplesInputBufferRight;
    AudioChannels m_capturedChannels;
    AudioChannels m_triggerChannel;
};

#endif // FORMRAW_H
