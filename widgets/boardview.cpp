#include <QtDebug>
#include <QMouseEvent>
#include <QGraphicsItem>
#include <QVariant>
#include <QMenu>
#include <QAction>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QTimer>
#include <QFontMetrics>
#include <QRandomGenerator>
#include "boardview.h"

BoardView::BoardView(QWidget *parent) :
    QGraphicsView(parent)
{
    m_entireViewIsDragging = false;
    m_testpointDragging = false;
    m_testpointActionsEnabled = false;
    m_lastMousePos = QPoint(0, 0);
    m_boardPhoto = nullptr;
    m_currentTestpoint = nullptr;
    
    setTransformationAnchor(QGraphicsView::NoAnchor);
    setResizeAnchor(QGraphicsView::NoAnchor);
    setInteractive(false);  //disable propagation events to scene and items
    
    QGraphicsScene *scene = new QGraphicsScene;
    scene->setSceneRect(-200, -200, 5000, 5000);
    setScene(scene);
    
    m_animationTimer = new QTimer(this);
    m_animationTimer->setInterval(500);
    m_animationTimer->setTimerType(Qt::CoarseTimer);
    connect(m_animationTimer, SIGNAL(timeout()), this, SLOT(timeslotAnimate()));
}

BoardView::~BoardView()
{

}

QMap<int, int> BoardView::showBoard(QPixmap pixmap, TestpointsList testpoints)
{
    QMap<int, int> ids;
    m_boardPhoto = nullptr;
    stopAnimation();
    m_currentTestpoint = nullptr;
    scene()->clear();
    m_boardPhoto = scene()->addPixmap(pixmap);
    m_boardPhoto->setData(33, QVariant("BOARD_PHOTO"));
    fitInView(m_boardPhoto, Qt::KeepAspectRatio);
    ensureVisible(m_boardPhoto, 0, 0);
//    m_scene->setFocus();
//    ui->boardView->setFocus();
    for (int id : testpoints.keys()) {
        QPoint pos = testpoints.value(id);
        int uid = insertTestpoint(pos);
        ids.insert(uid, id);
    }
    return ids;
}

void BoardView::getBoardPhoto(QImage &boardPhoto, QImage &boardPhotoWithMarkers)
{
    if (m_boardPhoto == nullptr) {
        qCritical() << "There is no board photo for getting";
        Q_ASSERT(false);
        return;
    }
    QPixmap origPhoto = m_boardPhoto->pixmap();
    boardPhoto = origPhoto.toImage();
    
    stopAnimation();
    fitInView(m_boardPhoto, Qt::KeepAspectRatio);
    ensureVisible(m_boardPhoto, 0, 0);
    for (QGraphicsItem *item : scene()->items()) {
        QGraphicsEllipseItem *pin = qgraphicsitem_cast<QGraphicsEllipseItem *>(item);
        if (pin == nullptr) {
            continue;
        }
        updateTestpointView(pin);
    }
    boardPhotoWithMarkers = boardPhoto;
    boardPhotoWithMarkers.fill(Qt::black);
    QPainter painter(&boardPhotoWithMarkers);
    render(&painter);
}

void BoardView::testpointChangeText(int uid, QString text)
{
    QGraphicsEllipseItem *pin = getPinByUid(uid);
    if (pin == nullptr) {
        qWarning() << "Testpoint" << uid << "isn't present";
        return;
    }
    QGraphicsTextItem *itemId = nullptr;
    for (QGraphicsItem *item : pin->childItems()) {
        itemId = qgraphicsitem_cast<QGraphicsTextItem *>(item);
        if (itemId != nullptr) {
            break;
        }
    }
    if (itemId == nullptr) {
        qWarning() << "Testpoint view" << uid << "doesn't contain the ID item";
        return;
    }
    itemId->setPlainText(text);
    updateTestpointView(pin);
}

void BoardView::enableTestpointActions(bool enable)
{
    m_testpointActionsEnabled = enable;
    if (m_testpointActionsEnabled) {
        stopAnimation();
        m_currentTestpoint = nullptr;
    }
}

void BoardView::mousePressEvent(QMouseEvent *event)
{
//    qDebug() << "press" << event->button() << "at" << event->pos();
    if (event->button() == Qt::LeftButton) {
        m_lastMousePos = event->pos();
        QList<QGraphicsItem *> listItems = items(event->pos());
        bool dragEntireView = true;
        QGraphicsEllipseItem *testpoint = nullptr;
        if (listItems.isEmpty()) {
            // clicked outside of any item (outside boardPhoto too)
            dragEntireView = false;
            stopAnimation();
            m_currentTestpoint = nullptr;
            emit testpointUnselected();
        }
        for (QGraphicsItem *item : listItems) {
            if (item != m_boardPhoto) {
                dragEntireView = false;
            } else {
//                qDebug() << "There is the board photo:" << item->data(33).toString();
            }
            QGraphicsEllipseItem *pin = qgraphicsitem_cast<QGraphicsEllipseItem *>(item);
            if (pin != nullptr) {
                testpoint = pin;
                continue;
            }
        }
        if (dragEntireView) {
//            qDebug() << "Start view dragging";
            m_entireViewIsDragging = true;
        }
        if (testpoint != nullptr) {
            bool ok = false;
            int id = testpoint->data(DATA_TESTPOINT_UID).toInt(&ok);
            qDebug() << "Testpoint is" << id << "selected";
            if (ok) {
                stopAnimation();
                m_currentTestpoint = testpoint;
                if (m_testpointActionsEnabled) {
                    m_testpointDragging = true;
                } else {
                    startAnimation();
                    emit testpointSelected(id);
                }
            } else {
                qCritical() << "Testpoint have invalid ID:" << testpoint->data(DATA_TESTPOINT_UID);
                Q_ASSERT(false);
            }
        }
    } else
        QGraphicsView::mousePressEvent(event);
    event->accept();
}

void BoardView::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        QPoint curPos = event->pos();
        QPointF oldScenePos = mapToScene(m_lastMousePos);
        QPointF newScenePos = mapToScene(curPos);
        if (m_entireViewIsDragging) {
            viewport()->setCursor(Qt::ClosedHandCursor);
            translate(newScenePos.x() - oldScenePos.x(), newScenePos.y() - oldScenePos.y());
            m_lastMousePos = curPos;
        }
        if (m_testpointDragging) {
            viewport()->setCursor(Qt::ClosedHandCursor);
            if (m_boardPhoto->contains(newScenePos)) {
                m_currentTestpoint->setPos(newScenePos);
                m_lastMousePos = curPos;
            }
        }
    }
    event->accept();
}

void BoardView::mouseReleaseEvent(QMouseEvent *event)
{
//    qDebug() << "release" << "at" << event->pos();
    if (m_entireViewIsDragging) {
        if (viewport()->cursor() != Qt::ClosedHandCursor) {
            stopAnimation();
            m_currentTestpoint = nullptr;
            emit testpointUnselected();
        }
        m_entireViewIsDragging = false;
    }
    if (m_testpointDragging) {
        bool ok = false;
        int id = m_currentTestpoint->data(DATA_TESTPOINT_UID).toInt(&ok);
        if (!ok) {
            qCritical() << "Testpoint have invalid ID:" << m_currentTestpoint->data(DATA_TESTPOINT_UID);
            Q_ASSERT(false);
        }
        QPointF scenePos = mapToScene(m_lastMousePos);
        emit testpointMoved(id, scenePos.toPoint());
        m_testpointDragging = false;
    }
    viewport()->setCursor(Qt::ArrowCursor);
//  QGraphicsView::mouseReleaseEvent(event);
    event->accept();
}

void BoardView::wheelEvent(QWheelEvent *event)
{
//    qDebug() << "wheel angle delta" << event->angleDelta() << "at" << event->pos();
    QPoint cursorPos = event->pos();
    QPointF oldScenePos = mapToScene(cursorPos);
    
    QPoint numDegrees = event->angleDelta() / 8;
    if (numDegrees.isNull()) {
        qWarning() << "Wheel event is invalid - angle delta is null";
        event->accept();
        return;
    }
    QPoint numSteps = numDegrees / 15;
    if ((numSteps.x() > 0) || (numSteps.y() > 0)) {
        scale(1.3, 1.3);
    } else {
        scale(0.7, 0.7);
    }
    
    QPointF newScenePos = mapToScene(cursorPos);
    translate(newScenePos.x() - oldScenePos.x(), newScenePos.y() - oldScenePos.y());
    
    for (QGraphicsItem *item : scene()->items()) {
        QGraphicsEllipseItem *pin = qgraphicsitem_cast<QGraphicsEllipseItem *>(item);
        if (pin == nullptr) {
            continue;
        }
        updateTestpointView(pin);
    }
    event->accept();
}

int BoardView::insertTestpoint(QPointF posOnScene)
{
    int uid = generateUID();
    qDebug() << "add testpoint" << uid << "at" << posOnScene;
    QGraphicsEllipseItem *item = new QGraphicsEllipseItem();
    item->setData(DATA_TESTPOINT_UID, QVariant(uid));
    QGraphicsTextItem *itemId = new QGraphicsTextItem(item);
    itemId->setDefaultTextColor(Qt::green);
    scene()->addItem(item);
    item->setPos(posOnScene);
    return uid;
}

QGraphicsEllipseItem *BoardView::getPinByUid(int uid)
{
    for (QGraphicsItem *item : scene()->items()) {
        QGraphicsEllipseItem *testpoint = qgraphicsitem_cast<QGraphicsEllipseItem *>(item);
        if (testpoint == nullptr) {
            continue;
        }
        bool ok = false;
        int id = testpoint->data(DATA_TESTPOINT_UID).toInt(&ok);
        if (!ok) {
            qCritical() << "Testpoint have invalid ID:" << testpoint->data(DATA_TESTPOINT_UID);
            Q_ASSERT(false);
            continue;
        }
        if (id == uid) {
            qDebug() << "Testpoint" << uid << "has been found at" << testpoint->pos();
            return testpoint;
        }
    }
    qCritical() << "Testpoint" << uid << "has not been found";
    Q_ASSERT(false);
    return nullptr;
}

void BoardView::contextMenuEvent(QContextMenuEvent *event)
{
//    qDebug() << "show context menu";
    event->accept();
    QMenu menu(this);
    QAction actionAddTestpoint(QIcon(":/icons/plus.ico"), tr("Add testpoint"), &menu);
    QAction actionRemoveTestpoint(QIcon(":/icons/minus.ico"), tr("Remove testpoint"), &menu);
    QAction actionFitBoardToView(QIcon(":/icons/fit_to_width.ico"), tr("Fit to window"), &menu);
    actionAddTestpoint.setEnabled(false);
    actionRemoveTestpoint.setEnabled(false);
    actionFitBoardToView.setEnabled(false);
    menu.addAction(&actionAddTestpoint);
    menu.addAction(&actionRemoveTestpoint);
    menu.addSeparator();
    menu.addAction(&actionFitBoardToView);
    
    QGraphicsEllipseItem *testpoint = nullptr;
    if (m_boardPhoto != nullptr) {
        QList<QGraphicsItem *> listItems = items(event->pos());
        bool cursorOverBoard = true;
        if (listItems.isEmpty()) {
            cursorOverBoard = false;
        }
        for (QGraphicsItem *item : listItems) {
            if (item != m_boardPhoto) {
                cursorOverBoard = false;
            }
            QGraphicsEllipseItem *ellipse = qgraphicsitem_cast<QGraphicsEllipseItem *>(item);
            if (ellipse != nullptr) {
                testpoint = ellipse;
            }
        }
        if (m_testpointActionsEnabled && cursorOverBoard) {
            actionAddTestpoint.setEnabled(true);
        }
        if (m_testpointActionsEnabled && (testpoint != nullptr)) {
            actionRemoveTestpoint.setEnabled(true);
        }
        actionFitBoardToView.setEnabled(true);
    }
    QAction *action = menu.exec(mapToGlobal(event->pos()));
    if (action == &actionFitBoardToView) {
//        qDebug() << "fit to view";
        fitInView(m_boardPhoto, Qt::KeepAspectRatio);
        ensureVisible(m_boardPhoto, 0, 0);
        for (QGraphicsItem *item : scene()->items()) {
            QGraphicsEllipseItem *pin = qgraphicsitem_cast<QGraphicsEllipseItem *>(item);
            if (pin == nullptr) {
                continue;
            }
            updateTestpointView(pin);
        }
    } else if (action == &actionAddTestpoint) {
        QPointF scenePos = mapToScene(event->pos());
        int uid = insertTestpoint(scenePos);
        emit testpointAdded(uid, scenePos.toPoint());
    } else if (action == &actionRemoveTestpoint) {
        bool ok = false;
        int id = testpoint->data(DATA_TESTPOINT_UID).toInt(&ok);
        qDebug() << "remove testpoint" << id << "at" << event->pos();
        if (m_currentTestpoint == testpoint) {
            stopAnimation();
            m_currentTestpoint = nullptr;
        }
        scene()->removeItem(testpoint);
        delete testpoint;
        if (ok) {
            emit testpointRemoved(id);
        } else {
            qCritical() << "Testpoint have invalid ID:" << testpoint->data(DATA_TESTPOINT_UID);
            Q_ASSERT(false);
            return;
        }
    }
}

int BoardView::fitLabelFontSize(QFont &currentFont, const QRect &rectToBeFit,
                                const QString &text, int startFromSize)
{
    // NOTE cache font size for labels with similar rect and numbers count
    int topBound = startFromSize;
    int bottomBound = 1;
    int curHeight = qBound(bottomBound, bottomBound + qRound((topBound - bottomBound) / 2.), topBound);
    while (true) {
        currentFont.setPointSize(curHeight);
        QFontMetrics metrics(currentFont);
        QRect newRect = metrics.boundingRect(rectToBeFit,
            Qt::TextWordWrap | Qt::AlignVCenter | Qt::AlignHCenter, text);
        if ((newRect.width() <= rectToBeFit.width())
            && (newRect.height() <= rectToBeFit.height())) {
            bottomBound = curHeight;
        } else {
            topBound = curHeight;
        }
        if ((topBound - bottomBound) == 1) {
            curHeight = bottomBound;
            break;
        }
        curHeight = qBound(bottomBound, bottomBound + qRound((topBound - bottomBound) / 2.), topBound);
    }
//    qDebug() << "label" << text << "with rect" << rectToBeFit << "font:" << curHeight;
    return curHeight;
}

void BoardView::updateTestpointView(QGraphicsEllipseItem *pin)
{
    int radius = 15;
    QPointF pt1 = mapToScene(0, 0);
    QPointF pt2 = mapToScene(radius, radius);
    int r = qRound(pt2.x() - pt1.x());
    QRectF circleRect(-r, -r, r * 2, r * 2);
    QPen pen(QBrush(Qt::red), r * 0.3);
    pin->setPen(pen);
    pin->setRect(circleRect);
    
    QGraphicsTextItem *itemId = nullptr;
    for (QGraphicsItem *item : pin->childItems()) {
        itemId = qgraphicsitem_cast<QGraphicsTextItem *>(item);
        if (itemId != nullptr) {
            break;
        }
    }
    if (itemId == nullptr) {
        qWarning() << "Testpoint view doesn't contain the ID item";
        return;
    }
    // TODO use monospace font QFontDatabase::families QFontDialog::MonospacedFonts
    QFont idFont("Arial");
    int fontSize = fitLabelFontSize(idFont, circleRect.toRect(), itemId->toPlainText(), 200);
    idFont.setPixelSize(fontSize);
    itemId->setFont(idFont);
    QRectF boundRect = itemId->boundingRect();
    itemId->setPos(-boundRect.width() / 2., -boundRect.height() / 2.);
}

void BoardView::startAnimation()
{
    if (m_currentTestpoint == nullptr) {
        qWarning() << "Try to start animation for missed testpoint";
        Q_ASSERT(false);
        return;
    }
    QPen pen = m_currentTestpoint->pen();
    pen.setColor(Qt::darkGray);
    m_currentTestpoint->setPen(pen);
    m_animationTimer->start();
}

void BoardView::stopAnimation()
{
    m_animationTimer->stop();
    if (m_currentTestpoint == nullptr) {
        return;
    }
    QPen pen = m_currentTestpoint->pen();
    pen.setColor(Qt::red);
    m_currentTestpoint->setPen(pen);
}

void BoardView::timeslotAnimate()
{
    if (m_currentTestpoint == nullptr) {
        qWarning() << "Current testpoint is missed. Therefore stop animation";
        stopAnimation();
        Q_ASSERT(false);
        return;
    }
    QPen pen = m_currentTestpoint->pen();
    if (pen.color() == Qt::red) {
        pen.setColor(Qt::darkGray);
    } else {
        pen.setColor(Qt::red);
    }
    m_currentTestpoint->setPen(pen);
}

int BoardView::generateUID()
{
    int uid = 0;
    while (true) {
        int i = QRandomGenerator::system()->bounded(100000, 1000000);
        if (m_uids.contains(i)) {
            qWarning() << "UID not unique:" << uid;
            continue;
        }
        uid = i;
        break;
    }
    if ((uid < 100000) || (uid > 999999) || m_uids.contains(uid)) {
        qCritical() << "Invalid UID generation:" << uid << m_uids.contains(uid);
        Q_ASSERT(false);
    }
    return uid;
}
