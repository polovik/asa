#ifndef BOARDVIEW_H
#define BOARDVIEW_H

#include <QObject>
#include <QGraphicsView>

typedef enum {
    DATA_TESTPOINT_ID = 133
} ItemDataType;

class BoardView : public QGraphicsView
{
    Q_OBJECT
public:
    BoardView(QWidget* parent);
    ~BoardView();
    void showBoard(QPixmap pixmap);

signals:
    void testpointAdded(int id);
    void testpointSelected(int id);
    void testpointRemoved(int id);

protected:
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent * event);
    void mouseReleaseEvent(QMouseEvent* event);
    void wheelEvent (QWheelEvent* event);
    void contextMenuEvent(QContextMenuEvent* event);

private slots:
    void timeslotAnimate();

private:
    void updateTestpointView(QGraphicsEllipseItem *pin);
    void startAnimation();
    void stopAnimation();
    int fitLabelFontSize(QFont& currentFont, const QRect &rectToBeFit,
                         const QString& text, int startFromSize);

    bool m_entireViewIsDragging;
    bool m_testpointDragging;
    QPoint m_lastMousePos;
    QGraphicsPixmapItem *m_boardPhoto;
    QGraphicsEllipseItem *m_currentTestpoint;
    QTimer *m_animationTimer;
};

#endif // BOARDVIEW_H
