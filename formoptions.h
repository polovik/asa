#ifndef FORMOPTIONS_H
#define FORMOPTIONS_H

#include <QWidget>

namespace Ui {
class FormOptions;
}

class FormOptions : public QWidget
{
    Q_OBJECT

public:
    explicit FormOptions(QWidget *parent = nullptr);
    ~FormOptions();

public slots:
    void enterForm();
    void leaveForm();

private slots:
    void switchApplicationLanguage(int index);
    void setFrequency(int frequency);
    void setVoltage(double voltage);
    void switchOutputWaveForm();

private:
    Ui::FormOptions *ui;
};

#endif // FORMOPTIONS_H
