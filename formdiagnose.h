#ifndef FORMDIAGNOSE_H
#define FORMDIAGNOSE_H

#include <QWidget>
#include <QCameraInfo>

namespace Ui {
class FormDiagnose;
}

typedef struct {
    int id;
    QPoint pos;
    int signalType;
    int signalFrequency;
    int signalVoltage;
    QImage signature;
} Tespoint;

class FormDiagnose : public QWidget
{
    Q_OBJECT

public:
    explicit FormDiagnose(QWidget *parent = 0);
    ~FormDiagnose();
    void saveMeasures();

signals:
//    void preventClosing(bool changesNotStored);

private slots:
    void switchCamera(int index);
    void showCamera();
    void closeCamera();
    void savePhoto(int id, const QImage &preview);
    void selectBoard();
    void loadBoardData(QString boardPhotoPath);

private:
    void freezeForm(bool changesNotStored);

    Ui::FormDiagnose *ui;
    QList<QCameraInfo> m_camerasList;
    QDialog *m_dialogCamera;
    QString m_boardPhotoPath;
    bool m_needSave;
};

#endif // FORMDIAGNOSE_H
