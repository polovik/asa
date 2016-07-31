#include "formdiagnose.h"
#include "ui_formdiagnose.h"
#include <QCamera>
#include <QCameraInfo>
#include <QDebug>
#include <QCameraViewfinder>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSpacerItem>
#include <QCameraImageCapture>
#include <QImageEncoderSettings>
#include <QFileDialog>

FormDiagnose::FormDiagnose(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FormDiagnose)
{
    ui->setupUi(this);
    m_camera = NULL;
    m_dialogCamera = NULL;

    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    foreach (const QCameraInfo &cameraInfo, cameras) {
        qDebug() << "Found camera:" << cameraInfo.deviceName() << cameraInfo.description()
                 << cameraInfo.orientation() << cameraInfo.position();
        ui->boxCameras->addItem(cameraInfo.description());
//        if (cameraInfo.deviceName() == "mycamera")
//            camera = new QCamera(cameraInfo);
    }

    connect(ui->buttonCamera, SIGNAL(pressed()), this, SLOT(showCamera()));

}

FormDiagnose::~FormDiagnose()
{
    delete ui;
}

void FormDiagnose::showCamera()
{
    if (m_dialogCamera != NULL) {
        qCritical() << "Trying to open dialog camera twice";
        Q_ASSERT(false);
        return;
    }
    m_dialogCamera = new QDialog(this);
    m_dialogCamera->setModal(true);
    QVBoxLayout *vboxLayout = new QVBoxLayout();
    m_dialogCamera->setLayout(vboxLayout);
    QCameraViewfinder *cameraVideo = new QCameraViewfinder();
    vboxLayout->addWidget(cameraVideo, 2);
    QHBoxLayout *hboxLayout = new QHBoxLayout();
    QPushButton *buttonTakePhoto = new QPushButton("Take a photo");
    QSpacerItem *spacer = new QSpacerItem(40, 20);
    QPushButton *buttonCancel = new QPushButton("Cancel");
    connect(buttonCancel, SIGNAL(pressed()), this, SLOT(closeCamera()));
    hboxLayout->addWidget(buttonTakePhoto);
    hboxLayout->addSpacerItem(spacer);
    hboxLayout->addWidget(buttonCancel);
    vboxLayout->addLayout(hboxLayout, 0);
    m_dialogCamera->show();
    qDebug() << "Dialog for grab a photo is opened";

    m_camera = new QCamera(m_dialogCamera);
    m_camera->setViewfinder(cameraVideo);

    QCameraImageCapture *imageCapture = new QCameraImageCapture(m_camera);
    imageCapture->setCaptureDestination(QCameraImageCapture::CaptureToBuffer);
    QImageEncoderSettings imageSettings;
    imageSettings.setCodec("image/jpeg");
    imageCapture->setEncodingSettings(imageSettings);
    connect(buttonTakePhoto, SIGNAL(pressed()), imageCapture, SLOT(capture()));
    connect(imageCapture, SIGNAL(imageCaptured(int,QImage)), this, SLOT(savePhoto(int,QImage)));

    m_camera->setCaptureMode(QCamera::CaptureStillImage);
    m_camera->start();
}

void FormDiagnose::closeCamera()
{
    if (m_dialogCamera == NULL) {
        qCritical() << "Dialog camera is missed";
        Q_ASSERT(false);
        return;
    }
    m_dialogCamera->close();
    m_dialogCamera->deleteLater();
    m_dialogCamera = NULL;
}

void FormDiagnose::savePhoto(int id, const QImage &preview)
{
    Q_UNUSED(id);
    QFileDialog dialog(m_dialogCamera);
    dialog.setWindowTitle(tr("Save Photo"));
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setNameFilter(tr("Images (*.jpg)"));

    if (dialog.exec() != QDialog::Accepted) {
        qDebug() << "Photo" << preview.size() << "is discarded";
        return;
    }
    QStringList files = dialog.selectedFiles();
    if (files.count() != 1) {
        qWarning() << "Incorrect selected dir for store photo:" << files;
        Q_ASSERT(false);
        return;
    }
    QString fileName = files.first();
    if (!fileName.endsWith(".jpg", Qt::CaseInsensitive)) {
        fileName.append(".jpg");
    }
    bool saved = preview.save(fileName, "JPG");
    if (saved) {
        qDebug() << "Photo" << preview.size() << "is stored to" << fileName;
    } else {
        qWarning() << "Photo" << preview.size() << "can't be stored to" << fileName;
    }
}
