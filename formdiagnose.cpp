#include "formdiagnose.h"
#include "ui_formdiagnose.h"
#include <QCamera>
#if QT_VERSION >= QT_VERSION_CHECK(5, 3, 0)
#include <QCameraInfo>
#endif
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

extern bool g_verboseOutput;

FormDiagnose::FormDiagnose(ToneGenerator *gen, AudioInputThread *capture, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FormDiagnose)
{
    ui->setupUi(this);
    m_gen = gen;
    m_capture = capture;
    m_dialogCamera = nullptr;
    
#if QT_VERSION >= QT_VERSION_CHECK(5, 3, 0)
    m_camerasList = QCameraInfo::availableCameras();
    for (const QCameraInfo &cameraInfo : m_camerasList) {
        if (g_verboseOutput) {
            qDebug() << "Found camera:" << cameraInfo.deviceName() << cameraInfo.description()
                     << cameraInfo.orientation() << cameraInfo.position();
        }
        ui->boxCameras->addItem(cameraInfo.description(), QVariant(cameraInfo.deviceName()));
    }
#else
    m_camerasList = QCamera::availableDevices();
    foreach(const QByteArray &cameraDevice, m_camerasList) {
        QString description = QCamera::deviceDescription(cameraDevice);
        if (g_verboseOutput) {
            qDebug() << "Found camera:" << cameraDevice << description;
        }
        ui->boxCameras->addItem(description, QVariant(cameraDevice));
    }
#endif
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
    connect(ui->buttonRun, SIGNAL(clicked(bool)), this, SLOT(runAnalyze(bool)));
    connect(ui->buttonLockMeasure, SIGNAL(pressed()), this, SLOT(captureSignature()));

    ui->boxWaveForm->addItem(QIcon(":/icons/oscillator_sine.png"), "Sine", QVariant(ToneWaveForm::WAVE_SINE));
    ui->boxWaveForm->addItem(QIcon(":/icons/oscillator_square.png"), "Square", QVariant(ToneWaveForm::WAVE_SQUARE));
    ui->boxWaveForm->addItem(QIcon(":/icons/oscillator_saw.png"), "Sawtooth", QVariant(ToneWaveForm::WAVE_SAWTOOTH));
    ui->boxWaveForm->addItem(QIcon(":/icons/oscillator_triangle.png"), "Triangle", QVariant(ToneWaveForm::WAVE_TRIANGLE));
    connect(ui->boxWaveForm, SIGNAL(currentIndexChanged(int)), this, SLOT(switchOutputWaveForm()));
    connect(ui->boxFrequency, SIGNAL(valueChanged(int)), this, SLOT(setFrequency(int)));
    connect(ui->sliderFrequency, SIGNAL(valueChanged(int)), this, SLOT(setFrequency(int)));
    connect(ui->boxVoltage, SIGNAL(valueChanged(double)), this, SLOT(setVoltage(double)));
    connect(ui->sliderVoltage, SIGNAL(valueChanged(int)), this, SLOT(setVoltage(int)));

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

    m_boardPhotoPath = "";
    m_needSave = false;
    ui->buttonSave->setEnabled(false);
    switchMode(MODE_EDIT_TESTPOINTS);
}

FormDiagnose::~FormDiagnose()
{
    delete ui;
}

void FormDiagnose::enterForm()
{
    switchMode(MODE_EDIT_TESTPOINTS);
//    qreal max = m_gen->getMaxVoltageAmplitude();
//    ui->viewSignature->setMaximumAmplitude(max);
}

void FormDiagnose::leaveForm()
{
    if (m_needSave) {
        QMessageBox::information(this, tr("Unsaved changes"),
            tr("You have unsaved changes in diagnostics tab"));
    }
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
    if (m_needSave) {
        QMessageBox::information(this, tr("Unsaved changes"),
            tr("You have unsaved changes. If you open new board, all unsaved changes will be lost"));
    }
    if (m_dialogCamera != nullptr) {
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
    
#if QT_VERSION >= QT_VERSION_CHECK(5, 3, 0)
    QCameraInfo info = m_camerasList.at(ui->boxCameras->currentIndex());
    QCamera *camera = new QCamera(info, m_dialogCamera);
#else
    QByteArray deviceName = m_camerasList.at(ui->boxCameras->currentIndex());
    QCamera *camera = new QCamera(deviceName, m_dialogCamera);
#endif
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
    if (m_dialogCamera == nullptr) {
        qCritical() << "Dialog camera is missed";
        Q_ASSERT(false);
        return;
    }
    qDebug() << "Camera dialog is closing";
    m_dialogCamera->close();
    m_dialogCamera->deleteLater();
    m_dialogCamera = nullptr;
}

void FormDiagnose::savePhoto(int id, const QImage &preview)
{
    Q_UNUSED(id)
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
    if (m_needSave) {
        QMessageBox::information(this, tr("Unsaved changes"),
            tr("You have unsaved changes. If you open new board, all unsaved changes will be lost"));
    }
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
    for (const TestpointMeasure &meas : loadedTestpoints) {
        testpoints.insert(meas.id, meas.pos);
    }
    testpointSelect(-1);
    QMap<int, int> ids = ui->boardView->showBoard(pix, testpoints);

    for (int uid : ids.keys()) {
        int id = ids.value(uid);
        for (const TestpointMeasure &meas : loadedTestpoints) {
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
    for (int uid : m_testpoints.keys()) {
        const TestpointMeasure &meas = m_testpoints.value(uid);
        QString label = QString::number(meas.id);
        ui->boardView->testpointChangeText(uid, label);
    }
    ui->buttonRun->setEnabled(true);
    m_needSave = false;
    ui->buttonSave->setEnabled(false);
}

void FormDiagnose::runAnalyze(bool start)
{
    if (start) {
        switchMode(MODE_ANALYZE_SIGNATURE);
        if (m_testpoints.isEmpty()) {
            QMessageBox::information(this, tr("No testpoints"), tr("Add at least one testpoint for start signature analyze"));
        }
    } else {
        switchMode(MODE_EDIT_TESTPOINTS);
    }
}

void FormDiagnose::captureSignature()
{
    bool found = false;
    for (int uid : m_testpoints.keys()) {
        TestpointMeasure &pt = m_testpoints[uid];
        if (pt.isCurrent) {
            SignalParameters params;
            ui->viewSignature->getView(params, pt.signature, pt.data);
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
    for (int key : m_testpoints.keys()) {
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

    m_needSave = true;
    ui->buttonSave->setEnabled(true);
}

void FormDiagnose::testpointSelect(int uid)
{
    for (int id : m_testpoints.keys()) {
        TestpointMeasure &pt = m_testpoints[id];
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

    m_needSave = true;
    ui->buttonSave->setEnabled(true);
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
    for (int id : m_testpoints.keys()) {
        TestpointMeasure &curPt = m_testpoints[id];
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
        ui->boardView->testpointChangeText(id, label);
    }

    m_needSave = true;
    ui->buttonSave->setEnabled(true);
}

void FormDiagnose::saveMeasures()
{
    if (!m_needSave) {
        qWarning() << "There are no changes. Do not store diagnostic data";
        return;
    }

    QFileDialog dialog(this);
    dialog.setWindowTitle(tr("Save Diagnostic Results"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setNameFilter(tr("Images (*.tif *.tiff)"));
    if (dialog.exec() != QDialog::Accepted) {
        qDebug() << "Diagnostic results saving is discarded";
        return;
    }
    QStringList files = dialog.selectedFiles();
    if (files.count() != 1) {
        qWarning() << "Incorrect selected dir for store diagnostic results:" << files;
        Q_ASSERT(false);
        return;
    }
    QString filePath = files.first();
    if (!filePath.endsWith(".tif", Qt::CaseInsensitive)
        && !filePath.endsWith(".tiff", Qt::CaseInsensitive)) {
        filePath.append(".tiff");
    }

    ImageTiff tiff;
    QImage boardPhoto;
    QImage boardPhotoWithMarkers;
    ui->boardView->getBoardPhoto(boardPhoto, boardPhotoWithMarkers);
    // TODO display measure environment on the testpoint's signature
    QMap<int, TestpointMeasure> sortedTestpoints;
    for (const TestpointMeasure &pt : m_testpoints.values()) {
        sortedTestpoints.insert(pt.id, pt);
    }
    QList<TestpointMeasure> savedTestpoints;
    savedTestpoints = sortedTestpoints.values();
    bool saved = tiff.writeImageSeries(filePath, boardPhoto, boardPhotoWithMarkers, savedTestpoints);
    
    if (saved) {
        m_needSave = false;
        ui->buttonSave->setEnabled(false);
    } else {
        QMessageBox::warning(this, tr("Save Diagnostic Results"),
            tr("Couldn't save diagnostic results. Please, check log-file"));
    }
}

void FormDiagnose::switchMode(FormDiagnose::UiMode mode)
{
    qDebug() << "switch UI mode to:" << mode;
    switch (mode) {
    case MODE_EDIT_TESTPOINTS:
        ui->boxWaveForm->setEnabled(false);
        ui->boxVoltage->setEnabled(false);
        ui->boxFrequency->setEnabled(false);
        ui->sliderVoltage->setEnabled(false);
        ui->sliderFrequency->setEnabled(false);
        ui->buttonLockMeasure->setEnabled(false);
        ui->buttonOpenBoard->setEnabled(true);
        ui->buttonCamera->setEnabled(true);
        ui->boxCameras->setEnabled(true);
        ui->buttonRun->setText(tr("Start testpoints diagnose"));
        if (m_boardPhotoPath.isEmpty()) {
            ui->buttonRun->setEnabled(false);
        }
        break;
    case MODE_ANALYZE_SIGNATURE:
        ui->buttonOpenBoard->setEnabled(false);
        ui->buttonCamera->setEnabled(false);
        ui->boxCameras->setEnabled(false);
        ui->buttonRun->setText(tr("Stop diagnose"));
        break;
    default:
        qWarning() << "try to switch to unexpected UI mode:" << mode;
        break;
    }
    m_uiMode = mode;
}
