#ifndef FORMDIAGNOSE_H
#define FORMDIAGNOSE_H

#include <QWidget>
#include <QCameraInfo>
#include "common_types.h"

namespace Ui {
class FormDiagnose;
}

class FormDiagnose : public QWidget
{
    Q_OBJECT

public:
    explicit FormDiagnose(QWidget *parent = 0);
    ~FormDiagnose();

public slots:
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
    void captureSignature();

    void testpointAdd(int id, QPoint pos);
    void testpointSelect(int id);
    void testpointMove(int id, QPoint pos);
    void testpointRemove(int id);
    void testpointChangeId(int oldId, int newId);

private:
    void freezeForm(bool changesNotStored);

    Ui::FormDiagnose *ui;
    QList<QCameraInfo> m_camerasList;
    QDialog *m_dialogCamera;
    QString m_boardPhotoPath;
    bool m_needSave;
    QList<TestpointMeasure> m_testpoints;
};

#endif // FORMDIAGNOSE_H
