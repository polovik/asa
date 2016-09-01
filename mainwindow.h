#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <formcalibration.h>
#include <formraw.h>
#include <formanalyze.h>
#include <formdiagnose.h>
#include <formoptions.h>
#include <formabout.h>

namespace Ui
{
class MainWindow;
}

class ToneGenerator;
class AudioInputThread;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private slots:
    void showForm();
    
private:
    Ui::MainWindow *ui;
    ToneGenerator *m_gen;
    AudioInputThread *m_capture;
    
    FormCalibration *m_formCalibration;
    FormRaw *m_formRaw;
    FormAnalyze *m_formAnalyze;
    FormDiagnose *m_formDiagnose;
    FormOptions *m_formOptions;
    FormAbout *m_formAbout;

    QWidget *m_currentForm;
};

#endif // MAINWINDOW_H
