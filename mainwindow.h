#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class ToneGenerator;
class AudioCapture;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();
    void draw(const QVector<double> &values);

private:
    Ui::MainWindow *ui;
    ToneGenerator *m_gen;
    AudioCapture *m_capture;

    QVector<double> m_dataX;
    QVector<double> m_dataY;
    double m_triggerLevel;
};

#endif // MAINWINDOW_H
