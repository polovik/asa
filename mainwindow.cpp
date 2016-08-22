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
    connect(ui->buttonAnalyze, SIGNAL(pressed()), this, SLOT(showForm()));
    connect(ui->buttonDiagnose, SIGNAL(pressed()), this, SLOT(showForm()));

    m_formCalibration = new FormCalibration();
    m_formRaw = new FormRaw();
    m_formAnalyze = new FormAnalyze();
    m_formDiagnose = new FormDiagnose();
    ui->mainArea->addWidget(m_formCalibration);
    ui->mainArea->addWidget(m_formRaw);
    ui->mainArea->addWidget(m_formAnalyze);
    ui->mainArea->addWidget(m_formDiagnose);

    m_currentForm = m_formCalibration;
    ui->mainArea->setCurrentWidget(m_currentForm);
    ui->buttonCalibration->setChecked(true);
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
        newForm = m_formCalibration;
    } else if (button == ui->buttonRaw) {
        newForm = m_formRaw;
    } else if (button == ui->buttonAnalyze) {
        newForm = m_formAnalyze;
    } else if (button == ui->buttonDiagnose) {
        newForm = m_formDiagnose;
    } else {
        qWarning() << "Unknown Form:" << button;
        Q_ASSERT(false);
        return;
    }
    if (newForm == m_currentForm) {
        return;
    }
    if (m_currentForm == m_formCalibration) {
        qDebug() << "Close form \"Calibration\"";
    } else if (m_currentForm == m_formRaw) {
        qDebug() << "Close form \"Raw\"";
    } else if (m_currentForm == m_formAnalyze) {
        qDebug() << "Close form \"Analyze\"";
        m_formAnalyze->leaveForm();
    } else if (m_currentForm == m_formDiagnose) {
        qDebug() << "Close form \"Diagnose\"";
    } else {
        qWarning() << "Unknown Form:" << button;
        Q_ASSERT(false);
        return;
    }
    m_currentForm = newForm;
    if (m_currentForm == m_formCalibration) {
        qDebug() << "Open form \"Calibration\"";
    } else if (m_currentForm == m_formRaw) {
        qDebug() << "Open form \"Raw\"";
    } else if (m_currentForm == m_formAnalyze) {
        qDebug() << "Open form \"Analyze\"";
    } else if (m_currentForm == m_formDiagnose) {
        qDebug() << "Open form \"Diagnose\"";
    } else {
        qWarning() << "Unknown Form:" << button;
        Q_ASSERT(false);
        return;
    }
    ui->mainArea->setCurrentWidget(m_currentForm);
}
