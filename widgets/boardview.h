#ifndef BOARDVIEW_H
#define BOARDVIEW_H

#include <QObject>
#include <QSet>
#include <QGraphicsView>

typedef enum {
    DATA_TESTPOINT_UID = 133
} ItemDataType;

typedef QMap<int, QPoint> TestpointsList;

class BoardView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit BoardView(QWidget *parent);
    ~BoardView();
    QMap<int, int> showBoard(QPixmap pixmap, TestpointsList testpoints);
    void getBoardPhoto(QImage &boardPhoto, QImage &boardPhotoWithMarkers);
    void testpointChangeText(int uid, QString text);
    void enableTestpointActions(bool enable);
    
signals:
    void testpointAdded(int uid, QPoint pos);
    void testpointSelected(int uid);
    void testpointMoved(int uid, QPoint pos);
    void testpointRemoved(int uid);
    void testpointUnselected();
    
protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);
    
private slots:
    void timeslotAnimate();
    
private:
    int insertTestpoint(QPointF posOnScene);
    QGraphicsEllipseItem *getPinByUid(int uid);
    void updateTestpointView(QGraphicsEllipseItem *pin);
    void startAnimation();
    void stopAnimation();
    int fitLabelFontSize(QFont &currentFont, const QRect &rectToBeFit,
                         const QString &text, int startFromSize);
    int generateUID() const;
                         
    bool m_entireViewIsDragging;
    bool m_testpointDragging;
    bool m_testpointActionsEnabled;
    QPoint m_lastMousePos;
    QGraphicsPixmapItem *m_boardPhoto;
    QGraphicsEllipseItem *m_currentTestpoint;
    QTimer *m_animationTimer;

    // TODO add and remove uids to/from m_uids
    QSet<int> m_uids; // [100000...999999]
};

#endif // BOARDVIEW_H
