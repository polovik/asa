#ifndef FORMCALIBRATION_H
#define FORMCALIBRATION_H

#include <QWidget>
#include "common_types.h"

namespace Ui
{
class FormCalibration;
}

class ToneGenerator;
class AudioInputThread;
class OscilloscopeEngine;

class FormCalibration : public QWidget
{
    Q_OBJECT
    
public:
    explicit FormCalibration(ToneGenerator *gen, AudioInputThread *capture, QWidget *parent = 0);
    ~FormCalibration();
    
public slots:
    void enterForm();
    void leaveForm();
    
private slots:
    void switchOutputAudioDevice(int index);
    void switchInputAudioDevice(int index);
    void switchInputAudioDevicePort(int index);
    void playTestTone();
    void captureDeviceInitiated(int samplingRate);
    void processOscilloscopeData(SamplesList leftChannelData, SamplesList rightChannelData);
    void runCalibration(bool start);
    void showHint();
    void setGeneratorMagnitudePeak(double voltage);
    void setGeneratorMagnitudeRMS(double voltage);
    void changeCaptureHardwareVolume(int volume);
    void changeCaptureSoftwareVolume(int volume);
    void changeInputOffset(int percents);

private:
    void setGeneratorMagnitude(qreal peak);
    void lockWidgets(bool locked);

    Ui::FormCalibration *ui;
    ToneGenerator *m_gen;
    AudioInputThread *m_capture;
    OscilloscopeEngine *m_oscEngine;
    int m_baseCaptureVolume;
};

#endif // FORMCALIBRATION_H
