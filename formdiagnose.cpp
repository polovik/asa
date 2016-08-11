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
#include "tiff/imagetiff.h"

FormDiagnose::FormDiagnose(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FormDiagnose)
{
    ui->setupUi(this);
    m_dialogCamera = NULL;

    m_camerasList = QCameraInfo::availableCameras();
    foreach (const QCameraInfo &cameraInfo, m_camerasList) {
        qDebug() << "Found camera:" << cameraInfo.deviceName() << cameraInfo.description()
                 << cameraInfo.orientation() << cameraInfo.position();
        ui->boxCameras->addItem(cameraInfo.description(), QVariant(cameraInfo.deviceName()));
    }
    if (m_camerasList.count() > 0) {
        switchCamera(0);
        ui->buttonCamera->setEnabled(true);
    } else {
        ui->buttonCamera->setEnabled(false);
    }
    connect(ui->boxCameras, SIGNAL(currentIndexChanged(int)), this, SLOT(switchCamera(int)));
    connect(ui->buttonCamera, SIGNAL(pressed()), this, SLOT(showCamera()));
    connect(ui->buttonOpenBoard, SIGNAL(pressed()), this, SLOT(selectBoard()));
    connect(ui->buttonSave, SIGNAL(pressed()), this, SLOT(saveMeasures()));
    connect(ui->buttonLockMeasure, SIGNAL(pressed()), this, SLOT(captureSignature()));
    freezeForm(false);
    ui->buttonLockMeasure->setEnabled(false);

    ui->boardView->setAlignment(Qt::AlignCenter);
    ui->boardView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->boardView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->boardView->setFrameShape(QFrame::NoFrame);
    ui->boardView->setLineWidth(0);
    ui->boardView->setMidLineWidth(0);
    ui->boardView->setBackgroundBrush(QBrush(Qt::black));

    connect(ui->boardView, SIGNAL(testpointAdded(int,QPoint)),  this, SLOT(testpointAdd(int,QPoint)));
    connect(ui->boardView, SIGNAL(testpointSelected(int)),      this, SLOT(testpointSelect(int)));
    connect(ui->boardView, SIGNAL(testpointMoved(int,QPoint)),  this, SLOT(testpointMove(int,QPoint)));
    connect(ui->boardView, SIGNAL(testpointRemoved(int)),       this, SLOT(testpointRemove(int)));
    connect(ui->boardView, SIGNAL(testpointIdChanged(int,int)), this, SLOT(testpointChangeId(int,int)));
}

FormDiagnose::~FormDiagnose()
{
    delete ui;
}

void FormDiagnose::switchCamera(int index)
{
    QVariant deviceName = ui->boxCameras->itemData(index);
    QString cameraName = deviceName.toString();
    qDebug() << "Select camera:" << cameraName;
}

void FormDiagnose::showCamera()
{
    if (m_dialogCamera != NULL) {
        qCritical() << "Trying to open dialog camera twice";
        Q_ASSERT(false);
        return;
    }
    if ((m_camerasList.count() < 0) || (ui->boxCameras->currentIndex() >= m_camerasList.count())) {
        qWarning() << "Invalid camera is selected:" << m_camerasList.count() << ui->boxCameras->currentIndex();
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
    connect(buttonCancel, SIGNAL(pressed()), m_dialogCamera, SLOT(reject()));
    hboxLayout->addWidget(buttonTakePhoto);
    hboxLayout->addSpacerItem(spacer);
    hboxLayout->addWidget(buttonCancel);
    vboxLayout->addLayout(hboxLayout, 0);
    qDebug() << "Dialog for grab a photo is opened";

    QCameraInfo info = m_camerasList.at(ui->boxCameras->currentIndex());
    QCamera *camera = new QCamera(info, m_dialogCamera);
    camera->setViewfinder(cameraVideo);

    QCameraImageCapture *imageCapture = new QCameraImageCapture(camera);
    imageCapture->setCaptureDestination(QCameraImageCapture::CaptureToBuffer);
    QImageEncoderSettings imageSettings;
    imageSettings.setCodec("image/jpeg");
    imageCapture->setEncodingSettings(imageSettings);
    connect(buttonTakePhoto, SIGNAL(pressed()), imageCapture, SLOT(capture()));
    connect(m_dialogCamera, SIGNAL(rejected()), this, SLOT(closeCamera()));
    connect(imageCapture, SIGNAL(imageCaptured(int,QImage)), this, SLOT(savePhoto(int,QImage)));

    camera->setCaptureMode(QCamera::CaptureStillImage);
    camera->start();

    m_dialogCamera->showMaximized();
}

void FormDiagnose::closeCamera()
{
    if (m_dialogCamera == NULL) {
        qCritical() << "Dialog camera is missed";
        Q_ASSERT(false);
        return;
    }
    qDebug() << "Camera dialog is closing";
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
    dialog.setNameFilter(tr("Images (*.tif *.tiff)"));

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
    if (!fileName.endsWith(".tif", Qt::CaseInsensitive)
        || !fileName.endsWith(".tiff", Qt::CaseInsensitive)) {
        fileName.append(".tiff");
    }
    bool saved = preview.save(fileName, "TIFF");
    if (saved) {
        qDebug() << "Photo" << preview.size() << "is stored to" << fileName;
        loadBoardData(fileName);
    } else {
        qWarning() << "Photo" << preview.size() << "can't be stored to" << fileName;
    }
}

void FormDiagnose::selectBoard()
{
    QFileDialog dialog(this);
    dialog.setWindowTitle(tr("Open board by photo"));
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("Images (*.tif *.tiff)"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    QStringList files = dialog.selectedFiles();
    if (files.count() != 1) {
        qWarning() << "Incorrect selected board file:" << files;
        Q_ASSERT(false);
        return;
    }
    QString fileName = files.first();
    loadBoardData(fileName);
}

void FormDiagnose::loadBoardData(QString boardPhotoPath)
{
    qDebug() << "Load board data by photo:" << boardPhotoPath;
    m_boardPhotoPath = boardPhotoPath;
    m_testpoints.clear();
    QVector<double> keys, values;
    ui->viewSignature->draw(keys, values);
//    QImage img(m_boardPhotoPath);
//    ImageTiff tiff;
//    tiff.write(m_boardPhotoPath + ".tiff", img);
//    QImage img2 = img.scaled(500,500);
//    QList<QImage> images;
//    images.append(img);
//    images.append(img2);
//    tiff.writeImageSeries(m_boardPhotoPath + ".tiff", images);

    // TODO extract from TIFF
    QPixmap pix(m_boardPhotoPath);
    TestpointsList testpoints;
    testpointSelect(-1);

    ui->boardView->showBoard(pix, testpoints);
}

void FormDiagnose::captureSignature()
{
    bool found = false;
    for (int i = 0; i < m_testpoints.count(); i++) {
        TestpointMeasure &pt = m_testpoints[i];
        if (pt.isCurrent) {
            ui->viewSignature->getView(pt.signature, pt.data);
            found = true;
            break;
        }
    }
    if (!found) {
        qWarning() << "There is no selected testpoint";
        Q_ASSERT(false);
    }
}

void FormDiagnose::testpointAdd(int id, QPoint pos)
{
    TestpointMeasure point;
    point.id = id;
    point.pos = pos;
    point.signalType = 0;
    point.signalFrequency = 0;
    point.signalVoltage = 0;
    point.isCurrent = false;
    point.data.clear();
    foreach (const TestpointMeasure &pt, m_testpoints) {
        if (pt.id == id) {
            qWarning() << "Testpoint" << id << "is already presented in array";
            Q_ASSERT(false);
            return;
        }
    }
    m_testpoints.append(point);
    freezeForm(true);
}

void FormDiagnose::testpointSelect(int id)
{
    for (int i = 0; i < m_testpoints.count(); i++) {
        TestpointMeasure &pt = m_testpoints[i];
        pt.isCurrent = false;
    }
    if (id == -1) {
        QList<QPointF> graphData;
        ui->viewSignature->loadPreviousSignature(graphData);
        ui->buttonLockMeasure->setEnabled(false);
        return;
    }
    ui->buttonLockMeasure->setEnabled(true);
    bool found = false;
    for (int i = 0; i < m_testpoints.count(); i++) {
        TestpointMeasure &pt = m_testpoints[i];
        if (pt.id == id) {
            pt.isCurrent = true;
            ui->viewSignature->loadPreviousSignature(pt.data);
            found = true;
            break;
        }
    }
    if (!found) {
        qWarning() << "Testpoint" << id << "is missed in array";
        Q_ASSERT(false);
    }
}

void FormDiagnose::testpointMove(int id, QPoint pos)
{
    bool found = false;
    for (int i = 0; i < m_testpoints.count(); i++) {
        TestpointMeasure &pt = m_testpoints[i];
        if (pt.id == id) {
            pt.pos = pos;
            found = true;
            break;
        }
    }
    if (!found) {
        qWarning() << "Testpoint" << id << "is missed in array";
        Q_ASSERT(false);
        return;
    }
    freezeForm(true);
}

void FormDiagnose::testpointRemove(int id)
{
    bool found = false;
    for (int i = 0; i < m_testpoints.count(); i++) {
        TestpointMeasure pt = m_testpoints.at(i);
        if (pt.id == id) {
            m_testpoints.removeAt(i);
            found = true;
            break;
        }
    }
    if (!found) {
        qWarning() << "Testpoint" << id << "is missed in array";
        Q_ASSERT(false);
        return;
    }
    freezeForm(true);
}

void FormDiagnose::testpointChangeId(int oldId, int newId)
{
    bool found = false;
    for (int i = 0; i < m_testpoints.count(); i++) {
        TestpointMeasure &pt = m_testpoints[i];
        if (pt.id == oldId) {
            pt.id = newId;
            found = true;
            break;
        }
    }
    if (!found) {
        qWarning() << "Testpoint" << oldId << "is missed in array";
        Q_ASSERT(false);
        return;
    }
    freezeForm(true);
}

void FormDiagnose::saveMeasures()
{
    if (!m_needSave) {
        qDebug() << "There are no changes. Do not store diagnostic data";
        return;
    }
    // ToDo add save filedialog
    ImageTiff tiff;
    QImage boardPhoto;
    QImage boardPhotoWithMarkers;
    ui->boardView->getBoardPhoto(boardPhoto, boardPhotoWithMarkers);
    tiff.writeImageSeries(m_boardPhotoPath + ".tiff", boardPhoto, boardPhotoWithMarkers, m_testpoints);

    freezeForm(false);
}

void FormDiagnose::freezeForm(bool changesNotStored)
{
    if (changesNotStored) {
        ui->buttonCamera->setEnabled(false);
        ui->buttonOpenBoard->setEnabled(false);
        ui->buttonSave->setEnabled(true);
        ui->buttonDiscard->setEnabled(true);
    } else {
        ui->buttonCamera->setEnabled(true);
        ui->buttonOpenBoard->setEnabled(true);
        ui->buttonSave->setEnabled(false);
        ui->buttonDiscard->setEnabled(false);
    }
    m_needSave = changesNotStored;
}
