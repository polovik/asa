#ifndef FORMDIAGNOSE_H
#define FORMDIAGNOSE_H

#include <QWidget>

namespace Ui {
class FormDiagnose;
}

class QCamera;

class FormDiagnose : public QWidget
{
    Q_OBJECT

public:
    explicit FormDiagnose(QWidget *parent = 0);
    ~FormDiagnose();

private slots:
    void showCamera();
    void closeCamera();
    void savePhoto(int id, const QImage &preview);

private:
    Ui::FormDiagnose *ui;
    QCamera *m_camera;
    QDialog *m_dialogCamera;
};

#endif // FORMDIAGNOSE_H
