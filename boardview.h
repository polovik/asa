#ifndef BOARDVIEW_H
#define BOARDVIEW_H

#include <QGraphicsView>

class BoardView : public QGraphicsView
{
public:
    BoardView(QWidget* parent);
    ~BoardView();
    void showBoard(QPixmap pixmap);

protected:
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent * event);
    void mouseReleaseEvent(QMouseEvent* event);
    void wheelEvent (QWheelEvent* event);
    void contextMenuEvent(QContextMenuEvent* event);

private:
    void updateTestpointView(QGraphicsEllipseItem *pin);

    bool m_entireViewIsDragging;
    QPoint m_lastMousePos;
    QGraphicsPixmapItem *m_boardPhoto;
};

#endif // BOARDVIEW_H
