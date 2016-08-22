#include "formanalyze.h"
#include "ui_formanalyze.h"

FormAnalyze::FormAnalyze(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FormAnalyze)
{
    ui->setupUi(this);
}

FormAnalyze::~FormAnalyze()
{
    delete ui;
}
