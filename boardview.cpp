#include <QtDebug>
#include <QMouseEvent>
#include <QGraphicsItem>
#include <QVariant>
#include "boardview.h"

BoardView::BoardView(QWidget* parent = 0) :
    QGraphicsView(parent)
{
    m_lastMousePos = QPoint (0, 0);
    setTransformationAnchor(QGraphicsView::NoAnchor);
    setResizeAnchor(QGraphicsView::NoAnchor);
    setInteractive (false);	//disable propagation events to scene and items
}

BoardView::~BoardView()
{

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

    event->accept();
}
