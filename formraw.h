#ifndef FORMRAW_H
#define FORMRAW_H

#include <QWidget>

namespace Ui
{
class FormRaw;
}

class ToneGenerator;
class AudioInputThread;
class OscilloscopeEngine;

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
    void captureDeviceInitiated(int samplingRate);
    void startAudioCapture(bool start);
    void switchOutputWaveForm();
    void switchOutputFrequency();
    void changeTriggerSettings();
    void changeCapturedChannels();
    
private:
    Ui::FormRaw *ui;
    ToneGenerator *m_gen;
    AudioInputThread *m_capture;
    OscilloscopeEngine *m_oscEngine;
};

#endif // FORMRAW_H
