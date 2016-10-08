#include <QDebug>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "devices/tonegenerator.h"
#include "devices/audioinputdevice.h"
#include "widgets/FancyTabBar/fancytabbar.h"

Q_DECLARE_METATYPE(SamplesList)
Q_DECLARE_METATYPE(AudioChannels)

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    qRegisterMetaType <SamplesList> ();
    qRegisterMetaType <AudioChannels> ();
    
    ui->setupUi(this);

    FancyTabBar* mainTabs = new FancyTabBar(FancyTabBar::TabBarPosition::Left);
    mainTabs->insertTab(0, QIcon(":/icons/potentiometer.ico"), tr("Calibration"));
    mainTabs->insertTab(1, QIcon(":/icons/oscilloscope.ico"), tr("Raw"));
    mainTabs->insertTab(2, QIcon(":/icons/Lissajous_curve_1by2.svg.png"), tr("Analyze"));
    mainTabs->insertTab(3, QIcon(":/icons/diagram_v2_14.ico"), tr("Diagnose"));
    mainTabs->insertTab(4, QIcon(":/icons/application_x_desktop.ico"), tr("Options"));
    mainTabs->insertTab(5, QIcon(":/icons/get_info.ico"), tr("About"));
    mainTabs->setTabEnabled(0, true);
    mainTabs->setTabEnabled(1, true);
    mainTabs->setTabEnabled(2, true);
    mainTabs->setTabEnabled(3, true);
    mainTabs->setTabEnabled(4, true);
    mainTabs->setTabEnabled(5, true);
    mainTabs->setCurrentIndex(0);
    ui->horizontalLayout->insertWidget(0, mainTabs);
    connect(mainTabs, SIGNAL(currentChanged(int)), this, SLOT(showForm(int)));

    m_gen = new ToneGenerator;
    m_capture = new AudioInputThread;
    
    m_formCalibration = new FormCalibration(m_gen, m_capture);
    m_formRaw = new FormRaw(m_gen, m_capture);
    m_formAnalyze = new FormAnalyze(m_gen, m_capture);
    m_formDiagnose = new FormDiagnose(m_gen, m_capture);
    m_formOptions = new FormOptions;
    m_formAbout = new FormAbout;
    ui->mainArea->addWidget(m_formCalibration);
    ui->mainArea->addWidget(m_formRaw);
    ui->mainArea->addWidget(m_formAnalyze);
    ui->mainArea->addWidget(m_formDiagnose);
    ui->mainArea->addWidget(m_formOptions);
    ui->mainArea->addWidget(m_formAbout);

    m_currentForm = m_formCalibration;
    ui->mainArea->setCurrentWidget(m_currentForm);
    
    //  Start audio device only after enumeration completion
    m_gen->start();
    m_capture->start();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showForm(int formId)
{
    QWidget *newForm = NULL;
    switch (formId) {
    case 0:
        newForm = m_formCalibration;
        break;
    case 1:
        newForm = m_formRaw;
        break;
    case 2:
        newForm = m_formAnalyze;
        break;
    case 3:
        newForm = m_formDiagnose;
        break;
    case 4:
        newForm = m_formOptions;
        break;
    case 5:
        newForm = m_formAbout;
        break;
    default:
        qWarning() << "Unknown Form:" << formId;
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
    } else if (m_currentForm == m_formOptions) {
        qDebug() << "Close form \"Options\"";
    } else if (m_currentForm == m_formAbout) {
        qDebug() << "Close form \"About\"";
    } else {
        qWarning() << "Unknown Form:" << formId;
        Q_ASSERT(false);
        return;
    }
    m_currentForm = newForm;
    if (m_currentForm == m_formCalibration) {
        qDebug() << "Open form \"Calibration\"";
        m_formCalibration->enterForm();
    } else if (m_currentForm == m_formRaw) {
        qDebug() << "Open form \"Raw\"";
        m_formRaw->enterForm();
    } else if (m_currentForm == m_formAnalyze) {
        qDebug() << "Open form \"Analyze\"";
        m_formAnalyze->enterForm();
    } else if (m_currentForm == m_formDiagnose) {
        qDebug() << "Open form \"Diagnose\"";
        m_formDiagnose->enterForm();
    } else if (m_currentForm == m_formOptions) {
        qDebug() << "Open form \"Options\"";
    } else if (m_currentForm == m_formAbout) {
        qDebug() << "Open form \"About\"";
    } else {
        qWarning() << "Unknown Form:" << formId;
        Q_ASSERT(false);
        return;
    }
    ui->mainArea->setCurrentWidget(m_currentForm);
}
