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

class FormCalibration : public QWidget
{
    Q_OBJECT
    
public:
    explicit FormCalibration(ToneGenerator *gen, AudioInputThread *capture, QWidget *parent = 0);
    ~FormCalibration();
    
public slots:
    void leaveForm();
    
private slots:
    void switchOutputAudioDevice(int index);
    void switchInputAudioDevice(int index);
    void playTestTone();
    void captureDeviceInitiated(int samplingRate);
    void processOscilloscopeData(SamplesList leftChannelData, SamplesList rightChannelData);
    void checkInputLevel(bool start);
    void adjustGenerator(bool start);
    void setGeneratorMagnitude(double voltage);
    void showHint();
    
private:
    Ui::FormCalibration *ui;
    ToneGenerator *m_gen;
    AudioInputThread *m_capture;
};

#endif // FORMCALIBRATION_H
