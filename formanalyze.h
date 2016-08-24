#ifndef FORMANALYZE_H
#define FORMANALYZE_H

#include <QWidget>
#include "common_types.h"

namespace Ui {
class FormAnalyze;
}

class ToneGenerator;
class AudioInputThread;

class FormAnalyze : public QWidget
{
    Q_OBJECT

public:
    explicit FormAnalyze(ToneGenerator *gen, AudioInputThread *capture, QWidget *parent = 0);
    ~FormAnalyze();

public slots:
    void enterForm();
    void leaveForm();

private slots:
    void setFrequency(int frequency);
    void setVoltage(double voltage);
    void setVoltage(int vol10);
    void switchOutputWaveForm();
    void runAnalyze(bool start);
    void saveSignature();
    void captureDeviceInitiated (int samplingRate);
    void processOscilloscopeData(SamplesList leftChannelData, SamplesList rightChannelData);
    void lockSignature(bool lock);

private:
    Ui::FormAnalyze *ui;
    ToneGenerator *m_gen;
    AudioInputThread *m_capture;
};

#endif // FORMANALYZE_H
