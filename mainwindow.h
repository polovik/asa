#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "audioinputdevice.h"

namespace Ui {
class MainWindow;
}

class ToneGenerator;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void startToneGenerator(bool start);
    void captureDeviceInitiated (int samplingRate);
    void draw(OscCapturedChannels channel, const QVector<double> &values);
    void processOscilloscopeData (OscCapturedChannels channel, SamplesList samples);
    void startAudioCapture(bool start);
    void switchOutputAudioDevice(int index);
    void switchOutputWaveForm();
    void switchOutputFrequency();
    void switchInputAudioDevice(int index);
    void updateTriggerLevel(double voltage);
    void changeTriggerSettings();
    void changeCapturedChannels();

private:
    Ui::MainWindow *ui;
    ToneGenerator *m_gen;
    AudioInputThread *m_capture;

    QVector<double> m_dataX;
    QVector<double> m_dataY;
    double m_triggerLevel;
    int m_samplingRate;
    int m_frameLength;
    bool audioCaptureReady;
    static const int OSCILLOSCOPE_PLOT_FREQUENCY_HZ = 5;
    OscTriggerMode m_triggerMode;
    OscTriggerSlope m_triggerSlope;
    SamplesList m_samplesInputBufferLeft;
    SamplesList m_samplesInputBufferRight;
    OscCapturedChannels m_capturedChannels;
};

#endif // MAINWINDOW_H
