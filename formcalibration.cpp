#include "formcalibration.h"
#include "ui_formcalibration.h"

FormCalibration::FormCalibration(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FormCalibration)
{
    ui->setupUi(this);
}

FormCalibration::~FormCalibration()
{
    delete ui;
}
