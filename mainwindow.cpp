#include <QDebug>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ToneGenerator.h"
#include "audiocapture.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_gen = new ToneGenerator;
    m_capture = new AudioCapture;

    // create plot (from quadratic plot example):
    QVector<double> x(1024), y(1024);
    for (int i = 0; i < 100; ++i) {
        x[i] = i * 0.016;
        y[i] = 400 / 1024. * 50.;
    }
    for (int i = 100; i < 150; ++i) {
        x[i] = i * 0.016;
        y[i] = 100 / 1024. * 50.;
    }
    for (int i = 150; i < 151; ++i) {
        x[i] = i * 0.016;
        y[i] = 300 / 1024. * 50.;
    }
    for (int i = 151; i < 1024; ++i) {
        x[i] = i * 0.016;
        y[i] = (i) / 1024. * 50.;
    }
    m_dataX = x;
    m_dataY = y;

    m_triggerLevel = 1.;
//    ui->sliderPrecedingInterval->setValue(150);
//    setPrecedingInterval(150);

    ui->oscilloscope->setYaxisRange(0.0, 4095 * 0.01);
    draw(m_dataY);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    m_gen->start();
    m_capture->start();
}

void MainWindow::draw(const QVector<double> &values)
{
    QVector<double> keys;
    keys.resize(values.count());
    for (int i = 0; i < values.count(); i++) {
        keys[i] = i * 0.016;
    }

//    m_dataX.clear();
//    m_dataY.clear();
//    m_dataX = keys;
//    m_dataY = values;

//    for (int i = 0; i < m_dataX.count(); i++) {
//        keys[i] = m_dataX[i] - m_precedingInterval;
//    }
    ui->oscilloscope->draw(keys, m_dataY);

/*    if (ui->boxSweepSingle->isChecked()) {
        m_isRunning = false;
        changeStartButtonView();
        emit stopped();
    } else {
        m_timerPacketReceiving.start();
        changeStartButtonView();
    }*/
}
