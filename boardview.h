#ifndef BOARDVIEW_H
#define BOARDVIEW_H

#include <QGraphicsView>

class BoardView : public QGraphicsView
{
public:
    BoardView(QWidget* parent);
    ~BoardView();

protected:
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent * event);
    void mouseReleaseEvent(QMouseEvent* event);
    void wheelEvent (QWheelEvent* event);

private:
    bool m_entireViewIsDragging;
    QPoint m_lastMousePos;
};

#endif // BOARDVIEW_H
