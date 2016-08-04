#include <QtDebug>
#include <QMouseEvent>
#include <QGraphicsItem>
#include <QVariant>
#include <QMenu>
#include <QAction>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include "boardview.h"

BoardView::BoardView(QWidget* parent = 0) :
    QGraphicsView(parent)
{
    m_lastMousePos = QPoint (0, 0);
    m_boardPhoto = NULL;
    setTransformationAnchor(QGraphicsView::NoAnchor);
    setResizeAnchor(QGraphicsView::NoAnchor);
    setInteractive (false);	//disable propagation events to scene and items

    QGraphicsScene *scene = new QGraphicsScene;
    scene->setSceneRect(-200, -200, 5000, 5000);
    setScene(scene);
}

BoardView::~BoardView()
{

}

void BoardView::showBoard(QPixmap pixmap)
{
    m_boardPhoto = NULL;
    scene()->clear();
    m_boardPhoto = scene()->addPixmap(pixmap);
    m_boardPhoto->setData(33, QVariant("BOARD_PHOTO"));
    fitInView((QGraphicsItem *)m_boardPhoto, Qt::KeepAspectRatio);
    ensureVisible((QGraphicsItem *)m_boardPhoto, 0, 0);
//    m_scene->setFocus();
//    ui->boardView->setFocus();
}

void BoardView::mousePressEvent(QMouseEvent* event)
{
    qDebug() << "press" << event->button() << "at" << event->pos();
    if(event->button() == Qt::LeftButton) {
        m_lastMousePos = event->pos();
        QList<QGraphicsItem *> listItems = items(event->pos());
        bool dragEntireView = true;
        foreach (QGraphicsItem *item, listItems) {
            if (!item->data(33).toString().isEmpty()) {
                qDebug() << "There is the board photo:" << item->data(33).toString();
                continue;
            }
        }
        if (dragEntireView) {
            qDebug() << "Start view dragging";
            m_entireViewIsDragging = true;
        } else {
            qDebug() << "Clicked at items";
        }
    }
    else
        QGraphicsView::mousePressEvent(event);
    event->accept();
}

void BoardView::mouseMoveEvent(QMouseEvent* event)
{
    QPoint curPos = event->pos();
    qDebug() << "move" << event->buttons() << "at" << m_lastMousePos;
//    if (mMovePan)
//    {
//        if(event->buttons() & Qt::LeftButton)
//        {
//            centerOn (mapToScene (curPos));
//            viewport()->setCursor(Qt::ClosedHandCursor);
//        }
//    }
    if (m_entireViewIsDragging)
    {
        if(event->buttons() & Qt::LeftButton)
        {
            viewport()->setCursor(Qt::ClosedHandCursor);
            qDebug() << "move from" << m_lastMousePos << "to" << curPos;
            QPointF oldScenePos = mapToScene(m_lastMousePos);
            QPointF newScenePos = mapToScene(curPos);
            translate(newScenePos.x() - oldScenePos.x(), newScenePos.y() - oldScenePos.y());
        }
    }
    m_lastMousePos = curPos;
    event->accept();
}

void BoardView::mouseReleaseEvent(QMouseEvent* event)
{
//    qDebug() << "release" << "at" << event->pos();
    m_entireViewIsDragging = false;
    viewport()->setCursor(Qt::ArrowCursor);
//	QGraphicsView::mouseReleaseEvent(event);
    event->accept();
}

void BoardView::wheelEvent(QWheelEvent* event)
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

    foreach (QGraphicsItem *item, scene()->items()) {
        QGraphicsEllipseItem *pin = qgraphicsitem_cast<QGraphicsEllipseItem *>(item);
        if (pin == NULL) {
            continue;
        }
        updateTestpointView(pin);
    }
    event->accept();
}

void BoardView::contextMenuEvent(QContextMenuEvent *event)
{
    qDebug() << "show context menu";
    QMenu menu(this);
    QAction actionAddTestpoint(tr("Add testpoint"), &menu);
    QAction actionRemoveTestpoint(tr("Remove testpoint"), &menu);
    QAction actionFitBoardToView(tr("Fit to window"), &menu);
    actionAddTestpoint.setEnabled(false);
    actionRemoveTestpoint.setEnabled(false);
    actionFitBoardToView.setEnabled(false);
    menu.addAction(&actionAddTestpoint);
    menu.addAction(&actionRemoveTestpoint);
    menu.addSeparator();
    menu.addAction(&actionFitBoardToView);

    QGraphicsEllipseItem *testpoint = NULL;
    if (m_boardPhoto != NULL) {
        QList<QGraphicsItem *> listItems = items(event->pos());
        bool cursorOverBoard = true;
        if (listItems.isEmpty()) {
            cursorOverBoard = false;
        }
        foreach (QGraphicsItem *item, listItems) {
            if (item != m_boardPhoto) {
                cursorOverBoard = false;
            }
            QGraphicsEllipseItem *ellipse = qgraphicsitem_cast<QGraphicsEllipseItem *>(item);
            if (ellipse != NULL) {
                testpoint = ellipse;
            }
        }
        if (cursorOverBoard) {
            actionAddTestpoint.setEnabled(true);
        }
        if (testpoint != NULL) {
            actionRemoveTestpoint.setEnabled(true);
        }
        actionFitBoardToView.setEnabled(true);
    }
    QAction *action = menu.exec(mapToGlobal(event->pos()));
    if (action == &actionFitBoardToView) {
        qDebug() << "fit to view";
        fitInView((QGraphicsItem *)m_boardPhoto, Qt::KeepAspectRatio);
        ensureVisible((QGraphicsItem *)m_boardPhoto, 0, 0);
        foreach (QGraphicsItem *item, scene()->items()) {
            QGraphicsEllipseItem *pin = qgraphicsitem_cast<QGraphicsEllipseItem *>(item);
            if (pin == NULL) {
                continue;
            }
            updateTestpointView(pin);
        }
    } else if (action == &actionAddTestpoint) {
        qDebug() << "add testpoint at" << event->pos();
        QGraphicsEllipseItem *item = new QGraphicsEllipseItem();
        updateTestpointView(item);
        scene()->addItem(item);
        item->setPos(mapToScene(event->pos()));
    } else if (action == &actionRemoveTestpoint) {
        qDebug() << "remove testpoint at" << event->pos();
        scene()->removeItem(testpoint);
    }
    event->accept();
}

void BoardView::updateTestpointView(QGraphicsEllipseItem *pin)
{
    int radius = height() * 0.1;
    QPointF pt1 = mapToScene(0, 0);
    QPointF pt2 = mapToScene(radius, radius);
    int r = pt2.x() - pt1.x();
    QRectF circleRect(-r, -r, r * 2, r * 2);
    QPen pen(QBrush(Qt::red), r * 0.3);
    pin->setPen(pen);
    pin->setRect(circleRect);
}
