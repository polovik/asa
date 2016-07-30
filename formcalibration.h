#ifndef FORMCALIBRATION_H
#define FORMCALIBRATION_H

#include <QWidget>

namespace Ui {
class FormCalibration;
}

class FormCalibration : public QWidget
{
    Q_OBJECT

public:
    explicit FormCalibration(QWidget *parent = 0);
    ~FormCalibration();

private:
    Ui::FormCalibration *ui;
};

#endif // FORMCALIBRATION_H
