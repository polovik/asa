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
#include "devices/tonegenerator.h"
#include "devices/audioinputdevice.h"

FormDiagnose::FormDiagnose(ToneGenerator *gen, AudioInputThread *capture, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FormDiagnose)
{
    ui->setupUi(this);
    m_gen = gen;
    m_capture = capture;
    m_dialogCamera = NULL;
    
    m_camerasList = QCameraInfo::availableCameras();
    foreach(const QCameraInfo &cameraInfo, m_camerasList) {
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
    
    connect(ui->boardView, SIGNAL(testpointAdded(int, QPoint)),  this, SLOT(testpointAdd(int, QPoint)));
    connect(ui->boardView, SIGNAL(testpointSelected(int)),       this, SLOT(testpointSelect(int)));
    connect(ui->boardView, SIGNAL(testpointMoved(int, QPoint)),  this, SLOT(testpointMove(int, QPoint)));
    connect(ui->boardView, SIGNAL(testpointRemoved(int)),        this, SLOT(testpointRemove(int)));
}

FormDiagnose::~FormDiagnose()
{
    delete ui;
}

void FormDiagnose::enterForm()
{
    qreal max = m_gen->getMaxVoltageAmplitude();
    ui->viewSignature->setMaximumAmplitude(max);
}

void FormDiagnose::leaveForm()
{
    qDebug() << "Leave form \"Diagnose\"";
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
    connect(imageCapture, SIGNAL(imageCaptured(int, QImage)), this, SLOT(savePhoto(int, QImage)));
    
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
    QString filePath = files.first();
    if (!filePath.endsWith(".tif", Qt::CaseInsensitive)
        && !filePath.endsWith(".tiff", Qt::CaseInsensitive)) {
        filePath.append(".tiff");
    }
    
    ImageTiff tiff;
    QList<TestpointMeasure> testpoints;
    if (tiff.writeImageSeries(filePath, preview, QImage(), testpoints)) {
        qDebug() << "photo" << preview.size() << "is stored to" << filePath;
        loadBoardData(filePath);
    } else {
        qWarning() << "photo" << preview.size() << "couldn't be stored to" << filePath;
        QMessageBox::critical(this, "Save Signature", "Signature couldn't be stored to " + filePath);
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
    
    ImageTiff tiff;
    QImage boardPhoto;
    QList<TestpointMeasure> loadedTestpoints;
    tiff.readImageSeries(m_boardPhotoPath, boardPhoto, loadedTestpoints);
    QPixmap pix = QPixmap::fromImage(boardPhoto);
    
    TestpointsList testpoints;
    foreach(const TestpointMeasure &meas, loadedTestpoints) {
        testpoints.insert(meas.id, meas.pos);
    }
    testpointSelect(-1);
    QMap<int, int> ids = ui->boardView->showBoard(pix, testpoints);
    foreach (int uid, ids.keys()) {
        int id = ids.value(uid);
        foreach(const TestpointMeasure &meas, loadedTestpoints) {
            if (meas.id == id) {
                m_testpoints.insert(uid, meas);
                break;
            }
        }
    }
    if (loadedTestpoints.count() != m_testpoints.count()) {
        qCritical() << "Inconsistent data of loaded testpoints buffers:"
                    << loadedTestpoints.count() << m_testpoints.count();
        Q_ASSERT(false);
        m_testpoints.clear();
    }
    foreach (int uid, m_testpoints.keys()) {
        const TestpointMeasure &meas = m_testpoints.value(uid);
        QString label = QString::number(meas.id);
        ui->boardView->testpointChangeText(uid, label);
    }
}

void FormDiagnose::captureSignature()
{
    bool found = false;
    foreach (int uid, m_testpoints.keys()) {
        TestpointMeasure &pt = m_testpoints[uid];
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

void FormDiagnose::testpointAdd(int uid, QPoint pos)
{
    if (m_testpoints.contains(uid)) {
        qWarning() << "Testpoint" << uid << "is already presented in array for adding";
        Q_ASSERT(false);
        return;
    }

    QList<int> ids;
    ids.append(-1);
    foreach (int key, m_testpoints.keys()) {
        const TestpointMeasure &meas = m_testpoints.value(key);
        ids.append(meas.id);
    }
    qSort(ids);
    int testpointId = ids.last() + 1;

    TestpointMeasure point;
    point.id = testpointId;
    point.pos = pos;
    point.signalType.setId(ToneWaveForm::WAVE_SINE);
    point.signalFrequency = 0;
    point.signalVoltage = 0;
    point.isCurrent = false;
    point.signature = QImage(5, 5, QImage::Format_RGB32);
    point.signature.fill(Qt::white);
    point.data.clear();
    m_testpoints.insert(uid, point);
    QString label = QString::number(testpointId);
    ui->boardView->testpointChangeText(uid, label);
    freezeForm(true);
}

void FormDiagnose::testpointSelect(int uid)
{
    foreach (int uid, m_testpoints.keys()) {
        TestpointMeasure &pt = m_testpoints[uid];
        pt.isCurrent = false;
    }

    if (uid == -1) {
        QList<QPointF> graphData;
        ui->viewSignature->loadPreviousSignature(graphData);
        ui->buttonLockMeasure->setEnabled(false);
        return;
    }

    if (!m_testpoints.contains(uid)) {
        qWarning() << "Testpoint" << uid << "is missed in array for selecting";
        Q_ASSERT(false);
        return;
    }
    ui->buttonLockMeasure->setEnabled(true);
    TestpointMeasure &selectedPoint = m_testpoints[uid];
    selectedPoint.isCurrent = true;
    ui->viewSignature->loadPreviousSignature(selectedPoint.data);
}

void FormDiagnose::testpointMove(int uid, QPoint pos)
{
    if (!m_testpoints.contains(uid)) {
        qWarning() << "Testpoint" << uid << "is missed in array for moving";
        Q_ASSERT(false);
        return;
    }
    TestpointMeasure &movedPoint = m_testpoints[uid];
    movedPoint.pos = pos;
    freezeForm(true);
}

void FormDiagnose::testpointRemove(int uid)
{
    if (!m_testpoints.contains(uid)) {
        qWarning() << "Testpoint" << uid << "is missed in array for removing";
        Q_ASSERT(false);
        return;
    }
    TestpointMeasure pt = m_testpoints.value(uid);
    int removedId = pt.id;
    m_testpoints.remove(uid);

    // Reenumerate rest of testpoints - they have to had sequential number (for simplify TIFF store procedure)
    foreach (int uid, m_testpoints.keys()) {
        TestpointMeasure &curPt = m_testpoints[uid];
        int curId = curPt.id;
        if (curId < removedId) {
            continue;
        }
        if (curId == removedId) {
            qCritical() << "Testpoint" << curId << "have the same ID as removed testpoint";
            Q_ASSERT(false);
            return;
        }
        curId = curId - 1;
        curPt.id = curId;
        QString label = QString::number(curId);
        ui->boardView->testpointChangeText(uid, label);
    }

    freezeForm(true);
}

void FormDiagnose::saveMeasures()
{
    if (!m_needSave) {
        qDebug() << "There are no changes. Do not store diagnostic data";
        return;
    }
    // TODO add save filedialog
    ImageTiff tiff;
    QImage boardPhoto;
    QImage boardPhotoWithMarkers;
    ui->boardView->getBoardPhoto(boardPhoto, boardPhotoWithMarkers);
    // TODO save in ascending order
    // TODO display measure environment on the testpoint's signature
    QList<TestpointMeasure> savedTestpoints;
    savedTestpoints = m_testpoints.values();
    tiff.writeImageSeries(m_boardPhotoPath + ".tiff", boardPhoto, boardPhotoWithMarkers, savedTestpoints);
    
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
