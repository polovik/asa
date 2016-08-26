#ifndef SQUAREWIDGETHOLDER_H
#define SQUAREWIDGETHOLDER_H

#include <QWidget>

class SquareWidgetHolder : public QWidget
{
    Q_OBJECT
public:
    explicit SquareWidgetHolder(QWidget *parent = 0);
    ~SquareWidgetHolder();
    
public slots:

signals:

protected:
    void resizeEvent(QResizeEvent *event);
};

#endif // SQUAREWIDGETHOLDER_H
