#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <formcalibration.h>
#include <formraw.h>
#include <formdiagnose.h>

namespace Ui {
class MainWindow;
}

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
    FormCalibration *m_formCalibration;
    FormRaw *m_formRaw;
    FormDiagnose *m_formDiagnose;

    QWidget *m_currentForm;
};

#endif // MAINWINDOW_H
