#ifndef FORMDIAGNOSE_H
#define FORMDIAGNOSE_H

#include <QWidget>
#include <QMap>
#if QT_VERSION >= QT_VERSION_CHECK(5, 3, 0)
#include <QCameraInfo>
#endif
#include "common_types.h"

namespace Ui
{
class FormDiagnose;
}

class ToneGenerator;
class AudioInputThread;

class FormDiagnose : public QWidget
{
    Q_OBJECT
    
public:
    explicit FormDiagnose(ToneGenerator *gen, AudioInputThread *capture, QWidget *parent = 0);
    ~FormDiagnose();
    
public slots:
    void enterForm();
    void leaveForm();
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
    void changeModeButton(bool pressed);
    void showStoredSignature(bool show);
    void captureSignature();
    void setFrequency(int frequency);
    void setVoltage(double voltage);
    void setVoltage(int vol10);
    void switchOutputWaveForm();
    void runAnalyze(bool start);
    void captureDeviceInitiated(int samplingRate);
    void captureDeviceStopped();
    void processOscilloscopeData(SamplesList leftChannelData, SamplesList rightChannelData);

    void testpointAdd(int uid, QPoint pos);
    void testpointSelect(int uid);
    void testpointMove(int uid, QPoint pos);
    void testpointRemove(int uid);
    void testpointUnselect();
    
private:
    typedef enum {
        MODE_UNDEFINED          = 0,
        MODE_EDIT_TESTPOINTS    = 1,
        MODE_ANALYZE_SIGNATURE  = 2,
    } UiMode;

    void switchMode(UiMode mode);
    int getCurrentTestpointUid() const;
    bool isTestpointSelected() const;
    
    Ui::FormDiagnose *ui;
#if QT_VERSION >= QT_VERSION_CHECK(5, 3, 0)
    QList<QCameraInfo> m_camerasList;
#else
    QList<QByteArray> m_camerasList;
#endif
    QDialog *m_dialogCamera;
    QString m_boardPhotoPath;
    bool m_needSave;
    QMap<int, TestpointMeasure> m_testpoints; // key is UID of QGraphicItem
    ToneGenerator *m_gen;
    AudioInputThread *m_capture;
    FormDiagnose::UiMode m_uiMode;
};

#endif // FORMDIAGNOSE_H
