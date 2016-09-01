#include "formabout.h"
#include "ui_formabout.h"

FormAbout::FormAbout(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FormAbout)
{
    ui->setupUi(this);

    QString version(APP_VERSION);
    ui->labelVersion->setText(version);
}

FormAbout::~FormAbout()
{
    delete ui;
}
