#include <QDebug>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ToneGenerator.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_gen = new ToneGenerator;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    m_gen->start();
}
