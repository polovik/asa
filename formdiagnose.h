#ifndef FORMDIAGNOSE_H
#define FORMDIAGNOSE_H

#include <QWidget>
#include <QCameraInfo>

namespace Ui {
class FormDiagnose;
}

class QGraphicsScene;

class FormDiagnose : public QWidget
{
    Q_OBJECT

public:
    explicit FormDiagnose(QWidget *parent = 0);
    ~FormDiagnose();

private slots:
    void switchCamera(int index);
    void showCamera();
    void closeCamera();
    void savePhoto(int id, const QImage &preview);
    void selectBoard();
    void loadBoardData(QString boardPhotoPath);

private:
    Ui::FormDiagnose *ui;
    QList<QCameraInfo> m_camerasList;
    QDialog *m_dialogCamera;
    QString m_boardPhotoPath;
    QGraphicsScene *m_scene;
};

#endif // FORMDIAGNOSE_H
