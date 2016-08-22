#ifndef FORMANALYZE_H
#define FORMANALYZE_H

#include <QWidget>

namespace Ui {
class FormAnalyze;
}

class FormAnalyze : public QWidget
{
    Q_OBJECT

public:
    explicit FormAnalyze(QWidget *parent = 0);
    ~FormAnalyze();

public slots:
    void leaveForm();

private slots:
    void setFrequency(int frequency);
    void setVoltage(double voltage);
    void setVoltage(int vol10);
    void switchOutputWaveForm();
    void runAnalyze(bool start);
    void saveSignature();

private:
    Ui::FormAnalyze *ui;
};

#endif // FORMANALYZE_H
