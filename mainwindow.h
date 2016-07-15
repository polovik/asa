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
    void draw(const QVector<double> &values);
    void processOscilloscopeData (SamplesList samples);
    void startAudioCapture(bool start);

private:
    Ui::MainWindow *ui;
    ToneGenerator *m_gen;
    AudioInputThread *m_capture;

    QVector<double> m_dataX;
    QVector<double> m_dataY;
    double m_triggerLevel;
    int m_samplingRate;
    bool audioCaptureReady;
};

#endif // MAINWINDOW_H
