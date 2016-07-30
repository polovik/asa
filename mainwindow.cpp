#include <QDebug>
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->buttonCalibration, SIGNAL(pressed()), this, SLOT(showForm()));
    connect(ui->buttonRaw, SIGNAL(pressed()), this, SLOT(showForm()));
//    connect(ui->buttonOptions, SIGNAL(pressed()), this, SLOT(showForm()));

    m_formCalibration = new FormCalibration();
    m_formRaw = new FormRaw();
    ui->mainArea->addWidget(m_formCalibration);
    ui->mainArea->addWidget(m_formRaw);

    m_currentForm = m_formCalibration;
    ui->mainArea->setCurrentWidget(m_currentForm);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showForm()
{
    QPushButton *button = qobject_cast<QPushButton *>(sender());
    QWidget *newForm = NULL;
    if (button == ui->buttonCalibration) {
        qDebug() << "Open form \"Calibration\"";
        newForm = m_formCalibration;
    } else if (button == ui->buttonRaw) {
        qDebug() << "Open form \"Raw\"";
        newForm = m_formRaw;
    } else {
        qWarning() << "Unknown Form:" << button;
        Q_ASSERT(false);
        return;
    }
    if (newForm == m_currentForm) {
        return;
    }
    m_currentForm = newForm;
    ui->mainArea->setCurrentWidget(m_currentForm);
}
