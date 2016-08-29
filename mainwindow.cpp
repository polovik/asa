#include <QDebug>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "devices/tonegenerator.h"
#include "devices/audioinputdevice.h"

Q_DECLARE_METATYPE(SamplesList)
Q_DECLARE_METATYPE(AudioChannels)

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    qRegisterMetaType <SamplesList> ();
    qRegisterMetaType <AudioChannels> ();
    
    ui->setupUi(this);
    connect(ui->buttonCalibration, SIGNAL(pressed()), this, SLOT(showForm()));
    connect(ui->buttonRaw, SIGNAL(pressed()), this, SLOT(showForm()));
    connect(ui->buttonAnalyze, SIGNAL(pressed()), this, SLOT(showForm()));
    connect(ui->buttonDiagnose, SIGNAL(pressed()), this, SLOT(showForm()));
    
    m_gen = new ToneGenerator;
    m_capture = new AudioInputThread;
    
    m_formCalibration = new FormCalibration(m_gen, m_capture);
    m_formRaw = new FormRaw(m_gen, m_capture);
    m_formAnalyze = new FormAnalyze(m_gen, m_capture);
    m_formDiagnose = new FormDiagnose(m_gen, m_capture);
    ui->mainArea->addWidget(m_formCalibration);
    ui->mainArea->addWidget(m_formRaw);
    ui->mainArea->addWidget(m_formAnalyze);
    ui->mainArea->addWidget(m_formDiagnose);
    
    m_currentForm = m_formCalibration;
    ui->mainArea->setCurrentWidget(m_currentForm);
    ui->buttonCalibration->setChecked(true);
    
    //  Start audio device only after enumeration completion
    m_gen->start();
    m_capture->start();
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
        m_formCalibration->leaveForm();
    } else if (m_currentForm == m_formRaw) {
        qDebug() << "Close form \"Raw\"";
        m_formRaw->leaveForm();
    } else if (m_currentForm == m_formAnalyze) {
        qDebug() << "Close form \"Analyze\"";
        m_formAnalyze->leaveForm();
    } else if (m_currentForm == m_formDiagnose) {
        qDebug() << "Close form \"Diagnose\"";
        m_formDiagnose->leaveForm();
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
        m_formRaw->enterForm();
    } else if (m_currentForm == m_formAnalyze) {
        qDebug() << "Open form \"Analyze\"";
        m_formAnalyze->enterForm();
    } else if (m_currentForm == m_formDiagnose) {
        qDebug() << "Open form \"Diagnose\"";
        m_formDiagnose->enterForm();
    } else {
        qWarning() << "Unknown Form:" << button;
        Q_ASSERT(false);
        return;
    }
    ui->mainArea->setCurrentWidget(m_currentForm);
}
