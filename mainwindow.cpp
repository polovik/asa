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
        qDebug() << "Open form \"Calibration\"";
        newForm = m_formCalibration;
    } else if (button == ui->buttonRaw) {
        qDebug() << "Open form \"Raw\"";
        newForm = m_formRaw;
    } else if (button == ui->buttonAnalyze) {
        qDebug() << "Open form \"Analyze\"";
        newForm = m_formAnalyze;
    } else if (button == ui->buttonDiagnose) {
        qDebug() << "Open form \"Diagnose\"";
        newForm = m_formDiagnose;
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
